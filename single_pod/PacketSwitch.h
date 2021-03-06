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

    // typedef NVINT8   InputType;
    // typedef NVINT8   ActType;
    typedef NVINT8   WeightType;    
    typedef NVINT32  AccumType;
    typedef NVINT32  AddrType;
    typedef NVINT8   ID_type;
    typedef NVUINT1  Bcast;

    typedef typename nvhls::nv_scvector<nvhls::nv_scvector <AccumType, tile_sz>, tile_sz> VectorType;

    class Packet: public nvhls_message {
     public:
        VectorType data;
        // AddrType srcPod;
        // AddrType dstPod;
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
        AddrType ttl;
        AddrType src;
        AddrType dst;
        ID_type d_type; // weight (0), activation (1), result (2)
        Bcast bcast;

        static const unsigned int width = ID_type::width + 13 * AddrType::width + VectorType::width + Bcast::width; // sizeof(int) * N;

        template <unsigned int Size>
        void Marshall(Marshaller<Size>& m) {
            m& data;
            // m& srcPod;
            // m& dstPod;
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
            m& ttl;
            m& src;
            m& dst;
            m& d_type;
            m& bcast;
        }
    };
    

    Connections::In<Packet>    maestro_in_port;
    Connections::Out<Packet>   maestro_out_port;

    Connections::In<Packet>    in_ports[NUM_PODS][2*POD_SZ + NUM_AB];
    Connections::Out<Packet>   out_ports[NUM_PODS][2*POD_SZ + NUM_AB];


    SC_HAS_PROCESS(PacketSwitch);
    PacketSwitch(sc_module_name name_) : sc_module(name_) {
        SC_THREAD (run); 
        sensitive << clk.pos(); 
        NVHLS_NEG_RESET_SIGNAL_IS(rst);
    }



    void run() {

        maestro_in_port.Reset();
        maestro_out_port.Reset();

        for (int j = 0; j < NUM_PODS; j++) {
          for (int i = 0; i < 2*POD_SZ; i++) {
            in_ports[j][i].Reset();
            out_ports[j][i].Reset();
          }
          out_ports[j][2*POD_SZ].Reset();
        }

        wait(20.0, SC_NS); // wait for reset
        
        AddrType d;
        AddrType e;
        Packet p_in1;
        Packet p_in2;


        while (1) {
          // send input from SRAM to maestro
          if(maestro_in_port.PopNB(p_in1)) {
            // broadcast...only applies to data packets
            if(p_in1.bcast) {
              for (int m1 = 0; m1 < Wy; m1++) {
                d = p_in1.dst;
                // p_in1.dstPod = m1;
                p_in1.x = m1;
                p_in1.bcast = 0;
                out_ports[m1][d].Push(p_in1);
              }
            } else {
              d = p_in1.dst;
              // e = p_in1.dstPod;
              e = p_in1.x;
              out_ports[e][d].Push(p_in1);
            }
            wait(3);
          }

          // handle inputs from maestro modules
          for (int j = 0; j < NUM_PODS; j++) {
            for (int i = 0; i < (2*POD_SZ+NUM_AB); i++) {

              if(in_ports[j][i].PopNB(p_in2)) {

                d = p_in2.dst;
                // e = p_in2.dstPod;
                e = p_in2.x;

                if(d == INT_MAX) { // if dst is SRAM addr              
                  maestro_out_port.Push(p_in2); // This  sends value to maestro top module, 
                                                  // which then interconnects directly to SRAM
                } else {
                  out_ports[e][d].Push(p_in2);

                  // track how often a packet is sent to the particular systolic array
                  // if(d >= POD_SZ && d < 2*POD_SZ) {
                  // if(e == P && d == POD_SZ) {
                  //   cout << "switch->SA " << sc_time_stamp().to_default_time_units() << "\n";
                  // }
                }
              }
            }
          }

          wait();
        }
    }

};

#endif

