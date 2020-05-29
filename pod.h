#ifndef __POD_H__
#define __POD_H__

#include "PacketSwitch.h"
#include "MB.h"
#include "SA.h"
#include "CB.h"

SC_MODULE(Pod) {

  PacketSwitch* p_switch;
  MB* mb[NUM_PODS][POD_SZ];
  SA* sa[NUM_PODS][POD_SZ];
  CB* cb[NUM_PODS];


  Connections::In<PacketSwitch::Packet> pod_sram_in;
  // Connections::Out<PacketSwitch::Packet> pod_sram_out;

  // Connections::In<PacketSwitch::Packet> packet_in;
  Connections::Out<PacketSwitch::Packet> packet_out;


  sc_in_clk     clk;
  sc_in<bool> rst;


  // 0-3 is MB ind, 4-7 is SA ind, 8 is SRAM, 9 is CB
  SC_HAS_PROCESS(Pod);
  Pod(sc_module_name name_) : sc_module(name_) {

    p_switch = new PacketSwitch(sc_gen_unique_name("p_switch"));

    for (int j = 0; j < NUM_PODS; j++) {
      for (int i = 0; i < POD_SZ; i++) {
        mb[j][i] = new MB(sc_gen_unique_name("mb"));
        sa[j][i] = new SA(sc_gen_unique_name("sa"));
      }
      cb[j] = new CB(sc_gen_unique_name("cb"));
    }


    SC_THREAD(run);
    sensitive << clk.pos();
    NVHLS_NEG_RESET_SIGNAL_IS(rst);
  }


  void run() {

    pod_sram_in.Reset();
    packet_out.Reset();
    // pod_sram_out.Reset();
    wait(20.0, SC_NS);

    PacketSwitch::Packet p_in1;
    PacketSwitch::Packet p_in2;

    while(1) {
      if(pod_sram_in.PopNB(p_in1)) {
        // for (int i = 0; i < tile_sz; i++) {
        //   for (int j = 0; j < tile_sz; j++) {
        //     printf("%d ", p_in1.data[i][j]);
        //   }
        //   printf("\n");
        // }
        // printf("\n");

        packet_out.Push(p_in1);
      }

      wait(1);            
    }
  }

};


#endif


