#ifndef __MB_H__
#define __MB_H__

#include "PacketSwitch.h"



SC_MODULE(MB)
{
    public:
    sc_in_clk     clk;
    sc_in<bool>   rst;

    // Interface Ports
    Connections::In<PacketSwitch::Packet>   packet_in;
    Connections::Out<PacketSwitch::Packet>  packet_out;

    PacketSwitch::ID_type id;

    SC_HAS_PROCESS(MB);
    MB(sc_module_name name_) : sc_module(name_) {
        SC_THREAD (run);
        sensitive << clk.pos();
        NVHLS_NEG_RESET_SIGNAL_IS(rst);
    }


    void run() {
        packet_in.Reset();
        packet_out.Reset();

        PacketSwitch::Packet packet_reg;
        vector<vector<PacketSwitch::AccumType>> weight_reg(N, vector<PacketSwitch::AccumType>(N)); 
        vector<vector<PacketSwitch::AccumType>> act_reg(N, vector<PacketSwitch::AccumType>(N)); 

        // vector<vector<T>> mat(rows, vector<T>(cols)); 

        #pragma hls_pipeline_init_interval 1
        while(1) {
            if (packet_in.PopNB(packet_reg)) {
                if(packet_reg.src == (2*POD_SZ) && packet_reg.d_type == 0) { // weights
                    for(int i = 0; i < N; i++) {
                        for (int j = 0; j < N; j++) {
                            weight_reg[i][j] = packet_reg.data[i][j];
                        }
                    }
                    printf("received weight at MB from SRAM\n");
                    packet_reg.src = id;
                    packet_reg.dst = id + POD_SZ;
                    packet_reg.d_type = 0;
                    packet_out.Push(packet_reg);                  
                }

                else if(packet_reg.src == (2*POD_SZ) && packet_reg.d_type == 1) { // activation
                    for(int i = 0; i < N; i++) {
                        for (int j = 0; j < N; j++) {
                            act_reg[i][j] = packet_reg.data[i][j];
                        }
                    }
                    printf("received activation at MB from SRAM\n");
                    packet_reg.src = id;
                    packet_reg.dst = id + POD_SZ;
                    packet_reg.d_type = 1;
                    packet_out.Push(packet_reg);                    
                }
            }

            wait();
        }
    }
};
#endif

