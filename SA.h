#ifndef __SA_H__
#define __SA_H__

#include "PacketSwitch.h"


SC_MODULE(SA)
{
    public:
    sc_in_clk     clk;
    sc_in<bool>   rst;

    // Interface Ports
    Connections::In<PacketSwitch::Packet>   packet_in;
    Connections::Out<PacketSwitch::Packet>  packet_out;

    PacketSwitch::ID_type id;

    SC_HAS_PROCESS(SA);
    SA(sc_module_name name_) : sc_module(name_) {
        SC_THREAD (run);
        sensitive << clk.pos();
        NVHLS_NEG_RESET_SIGNAL_IS(rst);
    }


    void run() {

        packet_in.Reset();
        packet_out.Reset();
        wait(20.0, SC_NS);

        bool is_weight_in = 0 ;
        bool is_act_in = 0;
        int out_cnt = 0;


        PacketSwitch::Packet packet_reg;
        vector<vector<PacketSwitch::AccumType>> weight_reg(tile_sz, vector<PacketSwitch::AccumType>(tile_sz)); 
        vector<vector<PacketSwitch::AccumType>> act_reg(tile_sz, vector<PacketSwitch::AccumType>(tile_sz)); 
        vector<vector<PacketSwitch::AccumType>> result_reg(tile_sz, vector<PacketSwitch::AccumType>(tile_sz)); 

        // vector<vector<T>> mat(rows, vector<T>(cols)); 

        #pragma hls_pipeline_init_interval 1
        while(1) {
            if (packet_in.PopNB(packet_reg)) {
                if(packet_reg.src == (id - POD_SZ)   &&   packet_reg.d_type == 0) { // weights
                    if(is_weight_in == 0) {
                        for(int i = 0; i < tile_sz; i++) {
                            for (int j = 0; j < tile_sz; j++) {
                                weight_reg[i][j] = packet_reg.data[i][j];
                            }
                        }
                        if(DEBUG) cout << "SA " << id << " Pod " << packet_reg.dstPod << " receive weight from MB " << packet_reg.src << "\n";
                        is_weight_in = 1;
                    }
                }

                else if(packet_reg.src == (id - POD_SZ)   &&   packet_reg.d_type == 1) { // activation
                    if(is_act_in == 0) {

                        for(int i = 0; i < tile_sz; i++) {
                            for (int j = 0; j < tile_sz; j++) {
                                act_reg[i][j] = packet_reg.data[i][j];
                            }
                        }

                        if(DEBUG) cout << "SA " << id << " Pod " << packet_reg.dstPod << " receive activation from MB " << packet_reg.src << "\n";
                        is_act_in = 1;
                    }                    
                }
            }

            if(is_weight_in && is_act_in) { // do matmul and send result
          
                result_reg = MatMul<PacketSwitch::AccumType, PacketSwitch::AccumType>(weight_reg, act_reg); 
                for(int i = 0; i < tile_sz; i++) {
                    for (int j = 0; j < tile_sz; j++) {                          
                        packet_reg.data[i][j] = result_reg[i][j];
                    }
                }

                packet_reg.src = id;
                packet_reg.srcPod = packet_reg.dstPod; // send to CB in the same pod
                packet_reg.dst = 2*POD_SZ; // destination is CB
                packet_reg.d_type = 2; // result type                    

                if(DEBUG) cout << "SA " << id << " Pod " << packet_reg.srcPod << " sending result to CB\n";
                packet_out.Push(packet_reg);                
                is_act_in = 0;
                out_cnt++;
            }

            // check if SA has sent alpha tiles. If so, the next block is ready so allow for reloading of weights
            if(out_cnt == (alpha * POD_SZ)) {
                is_weight_in = 0;
                out_cnt = 0;
            }

            wait();
        }
    }
};
#endif


