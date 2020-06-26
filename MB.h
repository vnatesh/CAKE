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
    PacketSwitch::Packet weight_reg; // single weight tile storage for reuse when new region starts
    vector<vector<PacketSwitch::Packet>> act_buffer(K_dr/K_sr, 
          vector<PacketSwitch::Packet>(N_sr/tile_sz)); // storage for a region of data (K_dr/K_sr length  
                                                              //run since same data is used on each run) 

    int ind;

    #pragma hls_pipeline_init_interval 1
    while(1) {
      if (packet_in.PopNB(packet_reg)) {
        // check that src is SRAM
        if(packet_reg.src == INT_MAX && packet_reg.d_type == 1) { // activation
          
          ind = (packet_reg.Z % (K_dr/tile_sz)) / (K_sr/tile_sz);
          act_buffer[ind][packet_reg.y] = packet_reg; //packet_reg.y varies from 0 to N_sr/tile_sz - 1
          if(DEBUG) cout <<  "received activation at MB " << packet_reg.x << " Pod " << id << " from SRAM\n";
        }

        else if(packet_reg.src == INT_MAX && packet_reg.d_type == 0) { // weights
          weight_reg = packet_reg;
          if(DEBUG) cout <<  "received weight at MB " << packet_reg.x << " Pod " << id << " from SRAM\n";
          // send weight packet to SA
          packet_reg.src = packet_reg.dst;
          packet_reg.dst = packet_reg.SA;
          packet_reg.CB = 2*POD_SZ; 
          packet_reg.d_type = 0;
          packet_out.Push(packet_reg);                  
          wait();

          ind = (weight_reg.Z % (K_dr/tile_sz)) / (K_sr/tile_sz);
          // send data packets to SA, decrement ttl 
          for(int i = 0; i < N_sr/tile_sz; i++) {
            packet_reg = act_buffer[ind][i];
            act_buffer[ind][i].ttl--;
            packet_reg.src = weight_reg.dst;
            packet_reg.dst = weight_reg.SA;
            packet_reg.CB = 2*POD_SZ; 
            packet_reg.d_type = 1;                    
            packet_out.Push(packet_reg);                    
            wait();
          }
        }
      }

      wait();
    }
  }
};
#endif


