#ifndef __PACKETSWITCH_H__
#define __PACKETSWITCH_H__

#include "arch.h"


template<typename T, typename U>
vector<vector<U>> MatMul(vector<vector<T>> mat_A, vector<vector<T>> mat_B) {

  int _N = (int) mat_A.size();
  int _M = (int) mat_A[0].size();
  int _P = (int) mat_B[0].size();
  assert(_M == (int) mat_B.size());
  vector<vector<U>> mat_C(_N, vector<U>(_P, 0)); 

  for (int i = 0; i < _N; i++) {
    for (int j = 0; j < _P; j++) {
      mat_C[i][j] = 0;
      for (int k = 0; k < _M; k++) {
        mat_C[i][j] += mat_A[i][k]*mat_B[k][j];
      }
    }
  }
  return mat_C;
}


// define types for tile matmul. same types as in Packet class
typedef NVINT32  AccumType;
typedef typename nvhls::nv_scvector<nvhls::nv_scvector <AccumType, tile_sz>, tile_sz> VectorType;

VectorType TileMul(VectorType mat_A, VectorType mat_B) {
  
  VectorType mat_C; 

  for (int i = 0; i < tile_sz; i++) {
    for (int j = 0; j < tile_sz; j++) {
      mat_C[i][j] = 0;
      for (int k = 0; k < tile_sz; k++) {
        mat_C[i][j] += mat_A[i][k]*mat_B[k][j];
      }
    }
  }
  return mat_C;
}


template<typename T> void PrintMat(vector<vector<T>> mat) {
  int rows = (int) mat.size();
  int cols = (int) mat[0].size();
  for (int i = 0; i < rows; i++) {
    cout << "\t";
    for (int j = 0; j < cols; j++) {
      cout << mat[i][j] << "\t";
    }
    cout << "\n";
  }
  cout << "\n";
}


SC_MODULE(PacketSwitch)
{
    public:
    sc_in_clk     clk;
    sc_in<bool>   rst;

    typedef NVINT8   WeightType;    
    typedef NVINT32  AccumType;
    typedef NVINT32  AddrType;
    typedef NVINT8   ID_type;
    typedef NVUINT1  Bcast;

    typedef typename nvhls::nv_scvector<nvhls::nv_scvector <AccumType, tile_sz>, tile_sz> VectorType;
    typedef typename nvhls::nv_scvector<Bcast, 2*NUM_SA - 1> BcastVectorType;
    typedef typename nvhls::nv_scvector<AddrType, NUM_LEVELS> AB_ChainType;



    class Packet: public nvhls_message {
     public:
        VectorType data;
        AddrType X;
        AddrType Y;
        AddrType Z;
        AddrType x;
        AddrType y;
        AddrType z;
        AddrType SB;
        AddrType SA;
        AB_ChainType AB;
        AddrType SRAM;
        AddrType src;
        AddrType dst;
        ID_type d_type; // weight (0), activation (1), result (2)
        BcastVectorType bcast;

        static const unsigned int width = ID_type::width + 11 * AddrType::width 
                                        + VectorType::width + BcastVectorType::width
                                        + AB_ChainType::width; // sizeof(int) * N;

        template <unsigned int Size>
        void Marshall(Marshaller<Size>& m) {
            m& data;
            m& X;
            m& Y;
            m& Z;
            m& x;
            m& y;
            m& z;
            m& SB;
            m& SA;
            m& AB;
            m& SRAM;
            m& src;
            m& dst;
            m& d_type;
            m& bcast;
        }
    };
    
    ID_type id; // id for the switch and associated AB
    ID_type level; // id for level of this switch in the h-tree

    Connections::In<Packet>   left_in;
    Connections::In<Packet>   right_in;
    Connections::In<Packet>   parent_in;

    Connections::Out<Packet>    left_out; 
    Connections::Out<Packet>    right_out;
    Connections::Out<Packet>    parent_out;

    vector<vector<PacketSwitch::Packet>> acc_buf; 
    int accum_cnt = 0;
    int n_ind;
    int m_ind;

    int accum_cnt1 = 0;

    int chain[NUM_LEVELS+1];
    bool chain_set = false;

    SC_HAS_PROCESS(PacketSwitch);
    PacketSwitch(sc_module_name name_) : sc_module(name_) {

      SC_THREAD (recv_parent); 
      sensitive << clk.pos(); 
      NVHLS_NEG_RESET_SIGNAL_IS(rst);

      SC_THREAD (recv_children); 
      sensitive << clk.pos(); 
      NVHLS_NEG_RESET_SIGNAL_IS(rst);
    }


    void recv_parent() {

      left_out.Reset();
      right_out.Reset();
      parent_in.Reset();
      // ab_in.Reset();
      // ab_out.Reset();

      wait(20.0, SC_NS); // wait for reset
      
      Packet p_in;
      Packet p_out;

      while (1) {

        // TODO : make parent_in receive on a separate thread
        if(parent_in.PopNB(p_in)) {

          // if packet came from SRAM, send it down left/right children
          if(p_in.src == INT_MIN) {

            if(p_in.d_type == 1) {
              if(p_in.bcast[id]) {
                p_in.bcast[id] = 0;
                left_out.Push(p_in);
                right_out.Push(p_in);
              }
            } else if(p_in.d_type == 0) {
              if(((p_in.dst >> (NUM_LEVELS - level - 1)) & 1)) { // if bit is 1, go right
                right_out.Push(p_in);
              } else {
                left_out.Push(p_in);
              }
            }
          }
        } 

        wait();
      }
    }



    void recv_children() {

      left_in.Reset();
      right_in.Reset();
      parent_out.Reset();
   
      wait(20.0, SC_NS); // wait for reset

      Packet p_in;
      Packet p_out;

      while(1) {

        if(left_in.PopNB(p_in)) {

          if(p_in.dst == id) {

            if(level == NUM_LEVELS - 1) { // second to last level holds accumulates partials the longest

              n_ind = p_in.Y % N_sr; 
              m_ind = 0; 
    
              for (int i = 0; i < tile_sz; i++) {
                for (int j = 0; j < tile_sz; j++) {
                  acc_buf[m_ind][n_ind].data[i][j] += p_in.data[i][j];
                }
              }

              acc_buf[m_ind][n_ind].X = p_in.X;
              acc_buf[m_ind][n_ind].Y = p_in.Y;
              acc_buf[m_ind][n_ind].Z = p_in.Z;
              acc_buf[m_ind][n_ind].x = p_in.x;
              acc_buf[m_ind][n_ind].y = p_in.y;
              acc_buf[m_ind][n_ind].z = p_in.z;
              acc_buf[m_ind][n_ind].SRAM = p_in.SRAM;
              accum_cnt++;

              wait();

              if(accum_cnt == K_ob * N_sr * (K/K_sr)) { // accumulate full MM block in the K dim 

                for(int n = 0; n < N_sr; n++) { 

                  p_out = acc_buf[m_ind][n]; // this sets X,Y,Z,x,y,z headers for final result output                    
                  p_out.src = id;

                  for(int s = 0; s < NUM_LEVELS+1; s++) {
                    p_out.AB[s] = p_in.AB[s];
                  }

                  p_out.dst = p_in.AB[1]; // send to next AB (or SRAM) in chain
                  p_out.d_type = 2; // result type                    
                  // if(DEBUG) cout <<  "AB " << id << " sending tile to " << p_out.dst << "\n";
                  parent_out.Push(p_out);
                  // cout <<  "AB " << id << " sending tile to " << p_out.dst << "\n";
                  for (int i = 0; i < tile_sz; i++) {
                    for (int j = 0; j < tile_sz; j++) {
                      acc_buf[m_ind][n].data[i][j] = 0; // reset acc_buf to 0;
                    }
                  }

                  wait();   
                }

                accum_cnt = 0;
              }
            }

            else { // at levels other than 2nd to last, accumulate each partial only twice

              n_ind = p_in.Y % N_sr; 
              m_ind = p_in.X % M_ob; 

              for (int i = 0; i < tile_sz; i++) {
                for (int j = 0; j < tile_sz; j++) {
                  acc_buf[m_ind][n_ind].data[i][j] += p_in.data[i][j];
                }
              }

              acc_buf[m_ind][n_ind].X = p_in.X;
              acc_buf[m_ind][n_ind].Y = p_in.Y;
              acc_buf[m_ind][n_ind].Z = p_in.Z;
              acc_buf[m_ind][n_ind].x = p_in.x;
              acc_buf[m_ind][n_ind].y = p_in.y;
              acc_buf[m_ind][n_ind].z = p_in.z;
              acc_buf[m_ind][n_ind].SRAM = p_in.SRAM;
              accum_cnt1++;

              wait();

              if(accum_cnt1 == (2 * M_ob * N_sr)) { // At this level only 
                                                   //accumulate each of the M_ob*N_sr tiles twice
                
                // cout << "AB " << id << "level " << level << "\n";
                for(int m = 0; m < M_ob; m++) { 
                  for(int n = 0; n < N_sr; n++) { 
                   
                    p_out = acc_buf[m][n]; // this sets X,Y,Z,x,y,z headers for final result output
                    p_out.src = id;

                    for(int s = 0; s < NUM_LEVELS+1; s++) {
                      p_out.AB[s] = p_in.AB[s];
                      // cout << p_in.AB[s] << " ";
                    }
                    // cout << "\n";                      

                    for(int s = 0; s < NUM_LEVELS+1; s++) {
                      if(p_in.AB[s] < id) {
                        p_out.dst = p_in.AB[s];
                        break;
                      }
                    }

                    p_out.d_type = 2; // result type                    
                    // if(DEBUG) cout <<  "AB " << id << " sending tile to " << p_out.dst << "\n";
                    parent_out.Push(p_out);

                    for (int i = 0; i < tile_sz; i++) {
                      for (int j = 0; j < tile_sz; j++) {
                        acc_buf[m][n].data[i][j] = 0; // reset acc_buf to 0;
                      }
                    }

                    wait();   
                  }
                }

                accum_cnt1 = 0;
              } 
            }
          }

          else {
            parent_out.Push(p_in);
            wait();
          }
        }




        if(right_in.PopNB(p_in)) {

          if(p_in.dst == id) {

            if(level == NUM_LEVELS - 1) { // second to last level holds accumulates partials the longest

              n_ind = p_in.Y % N_sr; 
              m_ind = 0; 
    
              for (int i = 0; i < tile_sz; i++) {
                for (int j = 0; j < tile_sz; j++) {
                  acc_buf[m_ind][n_ind].data[i][j] += p_in.data[i][j];
                }
              }

              acc_buf[m_ind][n_ind].X = p_in.X;
              acc_buf[m_ind][n_ind].Y = p_in.Y;
              acc_buf[m_ind][n_ind].Z = p_in.Z;
              acc_buf[m_ind][n_ind].x = p_in.x;
              acc_buf[m_ind][n_ind].y = p_in.y;
              acc_buf[m_ind][n_ind].z = p_in.z;
              acc_buf[m_ind][n_ind].SRAM = p_in.SRAM;
              accum_cnt++;

              wait();

              if(accum_cnt == K_ob * N_sr * (K/K_sr)) { // accumulate full MM block in the K dim 

                for(int n = 0; n < N_sr; n++) { 

                  p_out = acc_buf[m_ind][n]; // this sets X,Y,Z,x,y,z headers for final result output                    
                  p_out.src = id;

                  for(int s = 0; s < NUM_LEVELS+1; s++) {
                    p_out.AB[s] = p_in.AB[s];
                  }

                  p_out.dst = p_in.AB[1]; // send to next AB (or SRAM) in chain
                  p_out.d_type = 2; // result type                    
                  // if(DEBUG) cout <<  "AB " << id << " sending tile to " << p_out.dst << "\n";
                  parent_out.Push(p_out);
                  // cout <<  "AB " << id << " sending tile to " << p_out.dst << "\n";
                  for (int i = 0; i < tile_sz; i++) {
                    for (int j = 0; j < tile_sz; j++) {
                      acc_buf[m_ind][n].data[i][j] = 0; // reset acc_buf to 0;
                    }
                  }

                  wait();   
                }

                accum_cnt = 0;
              }
            }

            else { // at levels other than 2nd to last, accumulate each partial only twice

              n_ind = p_in.Y % N_sr; 
              m_ind = p_in.X % M_ob; 

              for (int i = 0; i < tile_sz; i++) {
                for (int j = 0; j < tile_sz; j++) {
                  acc_buf[m_ind][n_ind].data[i][j] += p_in.data[i][j];
                }
              }

              acc_buf[m_ind][n_ind].X = p_in.X;
              acc_buf[m_ind][n_ind].Y = p_in.Y;
              acc_buf[m_ind][n_ind].Z = p_in.Z;
              acc_buf[m_ind][n_ind].x = p_in.x;
              acc_buf[m_ind][n_ind].y = p_in.y;
              acc_buf[m_ind][n_ind].z = p_in.z;
              acc_buf[m_ind][n_ind].SRAM = p_in.SRAM;
              accum_cnt1++;

              wait();

              if(accum_cnt1 == (2 * M_ob * N_sr)) { // At this level only 
                                                   //accumulate each of the M_ob*N_sr tiles twice
                
                // cout << "AB " << id << "level " << level << "\n";
                for(int m = 0; m < M_ob; m++) { 
                  for(int n = 0; n < N_sr; n++) { 
                   
                    p_out = acc_buf[m][n]; // this sets X,Y,Z,x,y,z headers for final result output
                    p_out.src = id;

                    // set AB chain in outupt packet
                    for(int s = 0; s < NUM_LEVELS+1; s++) {
                      p_out.AB[s] = p_in.AB[s];
                      // cout << p_in.AB[s] << " ";
                    }
                    // cout << "\n";                      

                    for(int s = 0; s < NUM_LEVELS+1; s++) {
                      if(p_in.AB[s] < id) {
                        p_out.dst = p_in.AB[s];
                        break;
                      }
                    }

                    p_out.d_type = 2; // result type                    
                    // if(DEBUG) cout <<  "AB " << id << " sending tile to " << p_out.dst << "\n";
                    parent_out.Push(p_out);

                    for (int i = 0; i < tile_sz; i++) {
                      for (int j = 0; j < tile_sz; j++) {
                        acc_buf[m][n].data[i][j] = 0; // reset acc_buf to 0;
                      }
                    }

                    wait();   
                  }
                }

                accum_cnt1 = 0;
              } 
            }
          }

          else {
            parent_out.Push(p_in);
            wait();
          }
        }

        wait();
      }
    }

};

#endif
