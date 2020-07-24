

#ifndef __LEAFSWITCH_H__
#define __LEAFSWITCH_H__
#include "arch.h"
#include "PacketSwitch.h"


SC_MODULE(LeafSwitch)
{
    sc_in_clk     clk;
    sc_in<bool>   rst;

    Connections::In<PacketSwitch::Packet>   sb_in;
    Connections::In<PacketSwitch::Packet>   sa_in;
    Connections::In<PacketSwitch::Packet>   parent_in;

    Connections::Out<PacketSwitch::Packet>    sb_out; 
    Connections::Out<PacketSwitch::Packet>    sa_out;
    Connections::Out<PacketSwitch::Packet>    parent_out;

    PacketSwitch::ID_type   id;

    SC_HAS_PROCESS(LeafSwitch);
    LeafSwitch(sc_module_name name_) : sc_module(name_) {
        SC_THREAD (run); 
        sensitive << clk.pos(); 
        NVHLS_NEG_RESET_SIGNAL_IS(rst);
    }

    void run() {

        sb_in.Reset();
        sa_in.Reset();
        parent_in.Reset();
        sb_out.Reset();
        sa_out.Reset();
        parent_out.Reset();
        wait(20.0, SC_NS); // wait for reset
        PacketSwitch::Packet p_in;

        while (1) {

          if(parent_in.PopNB(p_in)) {


            if(p_in.d_type == 1) {
              if(p_in.bcast[id + NUM_SA - 1]) {
                p_in.bcast[id + NUM_SA - 1] = 0;
                sb_out.Push(p_in);
              }
            } else if(p_in.d_type == 0) {
              if(p_in.dst == id) { // drop packet if not meant for this leaf
                sb_out.Push(p_in);
              }
            }
          }

          if(sb_in.PopNB(p_in)) {
            sa_out.Push(p_in);
          }

          if(sa_in.PopNB(p_in)) {
            parent_out.Push(p_in);
          }

          wait();
        }
    }

};

#endif

