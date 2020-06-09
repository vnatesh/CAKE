#ifndef __PACKETSWITCH_H__
#define __PACKETSWITCH_H__

#include <systemc.h>
#include <nvhls_int.h>
#include <nvhls_connections.h>
#include <nvhls_vector.h>

#include <ArbitratedScratchpad.h>
#include <ArbitratedScratchpad/ArbitratedScratchpadTypes.h>

#include<bits/stdc++.h> 

const static bool DEBUG = false;


/*

Metrics:
I/O (time spent receiving), idle/ waiting and compute time

Fastest to slowest
Switch,MB and SRAM (both on chip memory), CB, SA (2 *tile_sz cycles), DRAM

notes from 6/8/2020
Blocking and waiting, hard to know deterministic without running it.
HT says its deterministic but is a waste of time.
DRAM may hog the switch, so its not available.

Measure by checking everything going out of SA, know how many computations you have done.
Put some counters on the sender side, if receiver only receives 90 then you know theres a block
Now we have to figure out where the counters should go.

HT says make some assumptions to ensure that the program runs.

*/


/* examples:
    P = 2:    2*2 = 4 pods, each pod has 2x2 SAs    (16 SAs)
    P = 4:    4*4 = 16 pods, each pod has 4x4 SAs   (256 SAs)
    P = 8:    8*8 = 64  pods, each pod has 8x8 SAs  (4096 SAs)
*/

// Define P, each pod has (P,P) element pairs
const static int P = 4;
const static int alpha = 2;
const static int tile_sz = 2; // change to 8 later
const static int NUM_PODS = P*P;
const static int POD_SZ = P*P;
const static int NUM_CB = 1;
const static int NUM_SRAM = 1;

const static int Dx = alpha*P*P*tile_sz; // Size of data block in N dimension i.e alpha*Wz
const static int Dz = P*P*tile_sz; // Size of data block in the k dimension
const static int Wz = P*P*tile_sz; // Size of block in the K dimension, same as Dz
const static int Wy = Wz; // Size of block in the M dimension

// Number of processing elements is Wz* Wy

template<typename T, typename U>
vector<vector<U>> MatMul(vector<vector<T>> mat_A, vector<vector<T>> mat_B) {
  // mat_A _N*_M
  // mat_B _M*_P
  // mat_C _N*_P
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

    typedef typename nvhls::nv_scvector<nvhls::nv_scvector <AccumType, tile_sz>, tile_sz> VectorType;

    class Packet: public nvhls_message {
     public:
        VectorType data;
        AddrType srcPod;
        AddrType dstPod;
        AddrType src;
        AddrType dst;
        ID_type d_type; // weight (0), activation (1), result (2)
        
        static const unsigned int width = ID_type::width + 4 * AddrType::width + VectorType::width; // sizeof(int) * N;

        template <unsigned int Size>
        void Marshall(Marshaller<Size>& m) {
            m& data;
            m& srcPod;
            m& dstPod;
            m& src;
            m& dst;
            m& d_type;
        }
    };
    

    Connections::In<Packet>    maestro_in_port;
    Connections::Out<Packet>   maestro_out_port;

    Connections::In<Packet>    in_ports[NUM_PODS][2*POD_SZ + NUM_CB];
    Connections::Out<Packet>   out_ports[NUM_PODS][2*POD_SZ + NUM_CB];


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
            d = p_in1.dst;
            e = p_in1.dstPod;
            out_ports[e][d].Push(p_in1);
          }

          for (int j = 0; j < NUM_PODS; j++) {
            for (int i = 0; i < (2*POD_SZ+NUM_CB); i++) {

              if(in_ports[j][i].PopNB(p_in2)) {
                d = p_in2.dst;
                e = p_in2.dstPod;
                // out_ports[e][d].Push(p_in2);
                if(d == INT_MAX && e == 0) {
                  maestro_out_port.Push(p_in2); // This  sends value to maestro top module, 
                                                  // which then interconnects directly to SRAM
                } else {
                  out_ports[e][d].Push(p_in2);
                }
              }
            }
          }

          wait();
        }
    }

};

#endif

