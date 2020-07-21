#ifndef __MAESTRO_H__
#define __MAESTRO_H__

#include "PacketSwitch.h"
#include "SB.h"
#include "SA.h"
#include "AB.h"

SC_MODULE(Maestro) {

  PacketSwitch* p_switch;
  SB* sb[NUM_PODS][POD_SZ];
  SA* sa[NUM_PODS][POD_SZ];
  AB* ab[NUM_PODS];


  Connections::In<PacketSwitch::Packet> maestro_sram_in;
  Connections::Out<PacketSwitch::Packet> maestro_sram_out;

  Connections::In<PacketSwitch::Packet> packet_in;
  Connections::Out<PacketSwitch::Packet> packet_out;

  sc_in_clk     clk;
  sc_in<bool> rst;


  // 0-3 is SB ind, 4-7 is SA ind, 8 is SRAM, 9 is AB
  SC_HAS_PROCESS(Maestro);
  Maestro(sc_module_name name_) : sc_module(name_) {

    p_switch = new PacketSwitch(sc_gen_unique_name("p_switch"));

    for (int j = 0; j < NUM_PODS; j++) {
      for (int i = 0; i < POD_SZ; i++) {
        sb[j][i] = new SB(sc_gen_unique_name("sb"));
        sa[j][i] = new SA(sc_gen_unique_name("sa"));
      }
      ab[j] = new AB(sc_gen_unique_name("ab"));
    }


    SC_THREAD(send_packet);
    sensitive << clk.pos();
    NVHLS_NEG_RESET_SIGNAL_IS(rst);

    SC_THREAD(receive_packet);
    sensitive << clk.pos();
    NVHLS_NEG_RESET_SIGNAL_IS(rst);

  }


  void send_packet() {

    maestro_sram_in.Reset();
    packet_out.Reset();
    wait(20.0, SC_NS);

    PacketSwitch::Packet p_in1;

    while(1) {
      if(maestro_sram_in.PopNB(p_in1)) {
        packet_out.Push(p_in1);
      }

      wait();            
    }
  }


  void receive_packet() {

    maestro_sram_out.Reset();
    packet_in.Reset();
    wait(20.0, SC_NS);

    PacketSwitch::Packet p_in2;

    while(1) {
      if(packet_in.PopNB(p_in2)) {
        maestro_sram_out.Push(p_in2);
      } 

      wait();            
    }
  }


};


#endif


