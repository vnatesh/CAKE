#ifndef __POD_H__
#define __POD_H__

#include "PacketSwitch.h"


SC_MODULE(Pod) {

  PacketSwitch* p_switch;
  MB* mb[POD_SZ];
  SA* sa[POD_SZ];

  Connections::In<PacketSwitch::Packet> pod_sram_in;
  Connections::Out<PacketSwitch::Packet> pod_sram_out;

  Connections::In<PacketSwitch::Packet> packet_in;
  Connections::Out<PacketSwitch::Packet> packet_out;

  sc_in_clk     clk;
  sc_in<bool> rst;


  // 0-3 is MB ind, 4-7 is SA ind, 8 is SRAM, 9 is CB
  SC_HAS_PROCESS(Pod);
  Pod(sc_module_name name_) : sc_module(name_) {

    p_switch = new PacketSwitch(sc_gen_unique_name("p_switch"));

    for (int i = 0; i < POD_SZ; i++) {
      mb[i] = new MB(sc_gen_unique_name("mb"));
      sa[i] = new SA(sc_gen_unique_name("sa"));

    }

    SC_THREAD(run);
    sensitive << clk.pos();
    NVHLS_NEG_RESET_SIGNAL_IS(rst);
  }


  void run() {

    pod_sram_in.Reset();
    packet_out.Reset();
    wait(20.0, SC_NS);

    PacketSwitch::Packet p_in;

    while(1) {
      if(pod_sram_in.PopNB(p_in)) {
        printf("dude %d\n", p_in.src);
        packet_out.Push(p_in);
      }
      wait(1);            
    }
  }


};



#endif
