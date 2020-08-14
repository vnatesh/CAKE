#ifndef __SB_H__
#define __SB_H__

#include "PacketSwitch.h"



SC_MODULE(SB)
{
  public:
  sc_in_clk     clk;
  sc_in<bool>   rst;

  // Interface Ports
  Connections::In<PacketSwitch::Packet>   packet_in;
  Connections::Out<PacketSwitch::Packet>  packet_out;


  SC_HAS_PROCESS(SB);
  SB(sc_module_name name_) : sc_module(name_) {
    SC_THREAD (run);
    sensitive << clk.pos();
    NVHLS_NEG_RESET_SIGNAL_IS(rst);
  }


  void run() {
    packet_in.Reset();
    packet_out.Reset();
    wait(20.0, SC_NS);

    PacketSwitch::Packet packet_reg;
    int chain[NUM_LEVELS+1];
    // int w = 0;
    // int a = 0;

    #pragma hls_pipeline_init_interval 1
    while(1) {
      if (packet_in.PopNB(packet_reg)) {

        if(packet_reg.src == INT_MIN && packet_reg.d_type == 0) { // weights received first
          if(DEBUG) cout <<  "received weight at SB " << packet_reg.dst << " Pod " << packet_reg.dst / (sx*sy) << " from SRAM\n";

          // buffer the AB chain for this leaf. It will get inserted into the data packet 
          for(int s = 0; s < NUM_LEVELS+1; s++) {
            chain[s] = packet_reg.AB[s];
          }

          // send weight packet to SA
          packet_reg.src = packet_reg.dst;
          packet_out.Push(packet_reg);                  
          // w++;
          // if(packet_reg.x == 0 && id == 0) cout << "w_cnt = " << w << "\n";
        }

        // check that src is SRAM
        else if(packet_reg.src == INT_MIN && packet_reg.d_type == 1) { // activation
          if(DEBUG) cout <<  "received activation at SB " << packet_reg.dst << " Pod " << packet_reg.dst / (sx*sy) << " from SRAM\n";

          // The SB sets the AB chain of the data packet header
          for(int s = 0; s < NUM_LEVELS+1; s++) {
            packet_reg.AB[s] = chain[s];
          }
       
          packet_reg.src = packet_reg.dst;
          packet_out.Push(packet_reg);                    
          // a++;
          // if(packet_reg.x == 0 && id == 0) cout << "a_cnt = " << a << "\n";
        }
      }

      wait();
    }
  }
};
#endif










