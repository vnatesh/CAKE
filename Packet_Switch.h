#ifndef __PACKETSWITCH_H__
#define __PACKETSWITCH_H__

#include <systemc.h>
#include <nvhls_int.h>
#include <nvhls_connections.h>
#include <nvhls_vector.h>

#include <ArbitratedScratchpad.h>
#include <ArbitratedScratchpad/ArbitratedScratchpadTypes.h>

const static int N = 8;

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

    typedef typename nvhls::nv_scvector<nvhls::nv_scvector <AccumType, N>, N> VectorType;

    class Packet: public nvhls_message{
     public:
        VectorType data;
        AddrType src;
        AddrType dst;
        ID_type d_type; // weight, activation, result
        
        static const unsigned int width = ID_type::width + 2 * AddrType::width + VectorType::width; // sizeof(int) * N;

        template <unsigned int Size>
        void Marshall(Marshaller<Size>& m) {
            m& data;
            m& src;
            m& dst;
            m& d_type;
        }
    };
    

    // I/O 
    Connections::In<Packet>    in_ports[2];
    Connections::Out<Packet>   out_ports[2];

    SC_HAS_PROCESS(PacketSwitch);
    PacketSwitch(sc_module_name name_) : sc_module(name_) {
        SC_THREAD (run); 
        sensitive << clk.pos(); 
        NVHLS_NEG_RESET_SIGNAL_IS(rst);
    }


    void run() {

        for (int i = 0; i < 2; i++) {
            in_ports[i].Reset();
            out_ports[i].Reset();
        }

        wait(2); // wait for reset
        
        AddrType d;
        Packet packet_reg;

        while (1) {

            for (int i = 0; i < 2; i++) {
                if(in_ports[i].PopNB(packet_reg)) {
                    d = packet_reg.dst;
                    wait();
                    out_ports[d].Push(packet_reg);
                    wait();
                }
            }

            wait();
        }
    }
};

#endif
