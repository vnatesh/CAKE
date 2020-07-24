#ifndef __MAESTRO_H__
#define __MAESTRO_H__

#include "PacketSwitch.h"
#include "LeafSwitch.h"
#include "SB.h"
#include "SA.h"
#include "AB.h"

SC_MODULE(Maestro) {

  PacketSwitch* p_switch[Sx*Sy - 1];
  AB* ab[Sx*Sy - 1];

  LeafSwitch* leaf_switch[Sx*Sy];
  SB* sb[Sx*Sy];
  SA* sa[Sx*Sy];
  
  Connections::In<PacketSwitch::Packet> maestro_sram_in;
  Connections::Out<PacketSwitch::Packet> maestro_sram_out;

  Connections::In<PacketSwitch::Packet> packet_in;
  Connections::Out<PacketSwitch::Packet> packet_out;

  sc_in_clk   clk;
  sc_in<bool> rst;

  // 0-3 is MB ind, 4-7 is SA ind, 8 is SRAM, 9 is CB
  SC_HAS_PROCESS(Maestro);
  Maestro(sc_module_name name_) : sc_module(name_) {

    for(int i = 0; i < Sx*Sy - 1; i++) {
      p_switch[i] = new PacketSwitch(sc_gen_unique_name("p_switch"));
      // ab[i] = new AB(sc_gen_unique_name("ab"));
    }

    for (int i = 0; i < Sx*Sy; i++) {
      leaf_switch[i] = new LeafSwitch(sc_gen_unique_name("leaf_switch"));
      sb[i] = new SB(sc_gen_unique_name("sb"));
      sa[i] = new SA(sc_gen_unique_name("sa"));      
    }

    SC_THREAD(send_packet);
    sensitive << clk.pos();
    NVHLS_NEG_RESET_SIGNAL_IS(rst);

    SC_THREAD(receive_packet);
    sensitive << clk.pos();
    NVHLS_NEG_RESET_SIGNAL_IS(rst);

  }

  // recv from SRAM, send to maestro
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

  // receive from maesrtro, send to SRAM
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


