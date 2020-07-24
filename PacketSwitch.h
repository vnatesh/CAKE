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
        AddrType AB;
        AddrType SRAM;
        AddrType src;
        AddrType dst;
        ID_type d_type; // weight (0), activation (1), result (2)
        Bcast bcast;

        static const unsigned int width = ID_type::width + 12 * AddrType::width + VectorType::width + Bcast::width; // sizeof(int) * N;

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
    
    ID_type AB_id; // id for the associated AB
    ID_type level; // id for level of this switch in the h-tree

    Connections::In<Packet>   left_in;
    Connections::In<Packet>   right_in;
    Connections::In<Packet>   parent_in;
    // Connections::In<Packet>   ab_in;

    Connections::Out<Packet>    left_out; 
    Connections::Out<Packet>    right_out;
    Connections::Out<Packet>    parent_out;
    // Connections::Out<Packet>    ab_out;

    SC_HAS_PROCESS(PacketSwitch);
    PacketSwitch(sc_module_name name_) : sc_module(name_) {
        SC_THREAD (run); 
        sensitive << clk.pos(); 
        NVHLS_NEG_RESET_SIGNAL_IS(rst);
    }


    void run() {

        left_in.Reset();
        right_in.Reset();
        left_out.Reset();
        right_out.Reset();
        parent_in.Reset();
        parent_out.Reset();
        // ab_in.Reset();
        // ab_out.Reset();

        wait(20.0, SC_NS); // wait for reset
        
        AddrType d;
        AddrType e;
        Packet p_in;

        while (1) {

          if(parent_in.PopNB(p_in)) {

            // if packet came from SRAM, send it down left/right children
            if(p_in.src == INT_MAX) {

              if(p_in.d_type == 1) {
                // p_in.bcast = 0;
                left_out.Push(p_in);
                right_out.Push(p_in);
              } else if(((p_in.dst >> (NUM_LEVELS - level - 1)) & 1)) {
                left_out.Push(p_in);
              } else {
                right_out.Push(p_in);
              }
            }
          } 

          if(left_in.PopNB(p_in)) {

            if(p_in.dst == INT_MAX) {
              parent_out.Push(p_in);
            }

            else if(p_in.dst == AB_id) {
              // accumulate partials here
              // once you've accumed enough tiles, read AB header
              // of result tile and send up to next AB in chain
            }
          }

          if(right_in.PopNB(p_in)) {

            if(p_in.dst == INT_MAX) {
              parent_out.Push(p_in);
            }

            else if(p_in.dst == AB_id) {

            }
          }


        
          // send input from SRAM to maestro
          // if(maestro_in_port.PopNB(p_in1)) {
          //   // broadcast...only applies to data packets
          //   if(p_in1.bcast) {
          //     for (int m1 = 0; m1 < Wy / tile_sz; m1++) {
          //       d = p_in1.dst;
          //       // p_in1.dstPod = m1;
          //       p_in1.x = m1;
          //       p_in1.bcast = 0;
          //       out_ports[m1][d].Push(p_in1);
          //     }
          //   } else {
          //     d = p_in1.dst;
          //     // e = p_in1.dstPod;
          //     e = p_in1.x;
          //     out_ports[e][d].Push(p_in1);
          //   }
          //   wait(3);
          // }

          // // handle inputs from maestro modules
          // for (int j = 0; j < NUM_PODS; j++) {
          //   for (int i = 0; i < (2*POD_SZ+NUM_CB); i++) {

          //     if(in_ports[j][i].PopNB(p_in2)) {

          //       d = p_in2.dst;
          //       // e = p_in2.dstPod;
          //       e = p_in2.x;

          //       if(d == INT_MAX) { // if dst is SRAM addr              
          //         maestro_out_port.Push(p_in2); // This  sends value to maestro top module, 
          //                                         // which then interconnects directly to SRAM
          //       } else {
          //         out_ports[e][d].Push(p_in2);

          //         // track how often a packet is sent to the particular systolic array
          //         // if(d >= POD_SZ && d < 2*POD_SZ) {
          //         // if(e == P && d == POD_SZ) {
          //         //   cout << "switch->SA " << sc_time_stamp().to_default_time_units() << "\n";
          //         // }
          //       }
          //     }
          //   }
          // }

          wait();
        }
    }

};

#endif

