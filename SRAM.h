#ifndef __SRAM_H__
#define __SRAM_H__

#include "PacketSwitch.h"


SC_MODULE (SRAM) {


  Connections::In<PacketSwitch::Packet> packet_in;
  Connections::Out<PacketSwitch::Packet> packet_out;
  sc_in <bool> clk;
  sc_in <bool> rst;

  vector<vector<vector<PacketSwitch::AccumType>>> weight_mat;
  vector<vector<vector<PacketSwitch::AccumType>>> act_mat;
  // vector<vector<PacketSwitch::AccumType>> output_mat[POD_SZ];


  PacketSwitch::ID_type id;

  SC_CTOR(SRAM) {
    SC_THREAD(run);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  }


  void run() {

    PacketSwitch::Packet   p_out1;
    PacketSwitch::Packet   p_out2;

    packet_out.Reset();
    // Wait for initial reset.
    wait(20.0, SC_NS);

    for(int x = 0; x < POD_SZ; x++) {

      printf("Sending weights from SRAM to MB \n");
      for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
          p_out1.data[i][j] = weight_mat[x][i][j];
        }
      }

      wait(5);

      printf("Sending activations from SRAM to MB \n");
      for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
          p_out2.data[i][j] = act_mat[x][i][j];
        }
      }

      p_out1.src = 8;
      p_out1.dst = x;
      p_out1.d_type = 0;

      p_out2.src = 8;
      p_out2.dst = x;
      p_out2.d_type = 1;

      packet_out.Push(p_out1);            
      packet_out.Push(p_out2);
      wait(10);
    }
  }
};


#endif

