#ifndef __PACKETSWITCH_H__
#define __PACKETSWITCH_H__

#include <systemc.h>
#include <nvhls_int.h>
#include <nvhls_connections.h>
#include <nvhls_vector.h>

#include <ArbitratedScratchpad.h>
#include <ArbitratedScratchpad/ArbitratedScratchpadTypes.h>

const static int alpha = 2;
const static int tile_sz = 2; // change to 8 later
const static int NUM_PODS = 2*2;
const static int POD_SZ = 2*2;
const static int NUM_CB = 1;
const static int NUM_SRAM = 1;


// Define P, each pod has (P,P) element pairs
const static int P = 2;
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
    cout << endl;
  }
  cout << endl;
}


SC_MODULE(PacketSwitch)
{
    public:
    sc_in_clk     clk;
    sc_in<bool>   rst;

    typedef NVINT8   InputType;
    typedef NVINT8   ActType;
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
    

    Connections::In<Packet>    pod_in_port;
    // Connections::Out<Packet>    pod_out_port;

    // Connections::In<Packet>    in_ports[NUM_PODS][2*POD_SZ + NUM_CB];
    Connections::In<Packet>    in_ports[NUM_PODS][2*POD_SZ];
    Connections::Out<Packet>   out_ports[NUM_PODS][2*POD_SZ + NUM_CB];


    SC_HAS_PROCESS(PacketSwitch);
    PacketSwitch(sc_module_name name_) : sc_module(name_) {
        SC_THREAD (run); 
        sensitive << clk.pos(); 
        NVHLS_NEG_RESET_SIGNAL_IS(rst);
    }



    void run() {

        pod_in_port.Reset();
        // pod_out_port.Reset();

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
        Packet packet_reg;

        while (1) {
          // send input from SRAM to pod
          if(pod_in_port.PopNB(packet_reg)) {
            d = packet_reg.dst;
            e = packet_reg.dstPod;
            out_ports[e][d].Push(packet_reg);
          }


          for (int j = 0; j < NUM_PODS; j++) {
            for (int i = 0; i < 2*POD_SZ; i++) {

              if(in_ports[j][i].PopNB(packet_reg)) {
                d = packet_reg.dst;
                e = packet_reg.dstPod;
                out_ports[e][d].Push(packet_reg);
                // if(d == 999) {
                //   pod_out_port.Push(packet_reg); // This  sends value to Pod module, 
                //                                   // which then intercpnnects directly to SRAM
                //                                   // see sram_out_port binding.
                // } else {
                //   out_ports[e][d].Push(packet_reg);
                // }
              }
            }
          }

          wait();
        }
    }

};

#endif

