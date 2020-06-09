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

        wait(20.0, SC_NS);

        PacketSwitch::Packet packet_reg;
        vector<vector<PacketSwitch::AccumType>> weight_reg(tile_sz, vector<PacketSwitch::AccumType>(tile_sz,0)); 
        vector<vector<PacketSwitch::AccumType>> act_reg(tile_sz, vector<PacketSwitch::AccumType>(tile_sz, 0)); 

        // vector<vector<T>> mat(rows, vector<T>(cols)); 

        #pragma hls_pipeline_init_interval 1
        while(1) {
            if (packet_in.PopNB(packet_reg)) {
                if(packet_reg.src == INT_MAX && packet_reg.d_type == 0) { // weights
                    for(int i = 0; i < tile_sz; i++) {
                        for (int j = 0; j < tile_sz; j++) {
                            weight_reg[i][j] = packet_reg.data[i][j];
                        }
                    }

                    if(DEBUG) cout <<  "received weight at MB " << packet_reg.dstPod << " Pod " << id << " from SRAM\n";
                    packet_reg.src = id;
                    packet_reg.srcPod = packet_reg.dstPod; // send to SA in the same pod
                    packet_reg.dst = id + POD_SZ;
                    packet_reg.d_type = 0;
                    packet_out.Push(packet_reg);                  
                }

                else if(packet_reg.src == INT_MAX && packet_reg.d_type == 1) { // activation
                    for(int i = 0; i < tile_sz; i++) {
                        for (int j = 0; j < tile_sz; j++) {
                            act_reg[i][j] = packet_reg.data[i][j];
                        }
                    }
                    
                    if(DEBUG) cout <<  "received activation at MB " << packet_reg.dstPod << " Pod " << id << " from SRAM\n";
                    packet_reg.src = id;
                    packet_reg.srcPod = packet_reg.dstPod;
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


