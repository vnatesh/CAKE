#ifndef __SA_H__
#define __SA_H__

#include <systemc.h>
#include <nvhls_int.h>
#include <nvhls_connections.h>

#include "PacketSwitch.h"


SC_MODULE(SA)
{
    public:
    sc_in_clk     clk;
    sc_in<bool>   rst;

    // Interface Ports
    Connections::In<PacketSwitch::Packet>   packet_in;
    Connections::Out<PacketSwitch::Packet>  packet_out;

    SC_HAS_PROCESS(SA);
    SA(sc_module_name name_) : sc_module(name_) {
        SC_THREAD (run);
        sensitive << clk.pos();
        NVHLS_NEG_RESET_SIGNAL_IS(rst);
    }


    void run() {
        packet_in.Reset();
        packet_out.Reset();

        bool is_weight_in = 0 ;
        bool is_act_in = 0;


        PacketSwitch::Packet packet_reg;
        vector<vector<PacketSwitch::AccumType>> weight_reg(N, vector<PacketSwitch::AccumType>(N)); 
        vector<vector<PacketSwitch::AccumType>> act_reg(N, vector<PacketSwitch::AccumType>(N)); 
        vector<vector<PacketSwitch::AccumType>> result_reg(N, vector<PacketSwitch::AccumType>(N)); 

        // vector<vector<T>> mat(rows, vector<T>(cols)); 

        #pragma hls_pipeline_init_interval 1
        while(1) {
            if (packet_in.PopNB(packet_reg)) {
                if(packet_reg.src == 0 && packet_reg.d_type == 0) { // weights
                    if(is_weight_in == 0) {
                        for(int i = 0; i < N; i++) {
                            for (int j = 0; j < N; j++) {
                                weight_reg[i][j] = packet_reg.data[i][j];
                            }
                        }
                        printf("received weight at SA from MB\n");
                        is_weight_in = 1;
                    }
                }

                else if(packet_reg.src == 0 && packet_reg.d_type == 1) { // activation
                    if(is_act_in == 0) {

                        for(int i = 0; i < N; i++) {
                            for (int j = 0; j < N; j++) {
                                act_reg[i][j] = packet_reg.data[i][j];
                            }
                        }
                        printf("received activation at SA from MB\n");
                        is_act_in = 1;
                    }                    
                }
            }

            if(is_weight_in && is_act_in) { // do matmul and send result
          
                result_reg = MatMul<PacketSwitch::AccumType, PacketSwitch::AccumType>(weight_reg, act_reg); 
                for(int i = 0; i < N; i++) {
                    for (int j = 0; j < N; j++) {                          
                        packet_reg.data[i][j] = result_reg[i][j];
                    }
                }
                packet_reg.src = 1;
                packet_reg.dst = 0;
                packet_reg.d_type = 2; // activation type                    
                printf("sending result to SRAM\n");
                packet_out.Push(packet_reg);
                is_weight_in = 0;
                is_act_in = 0;
                // is_done = 1;
            }

            wait();
        }
    }
};
#endif

