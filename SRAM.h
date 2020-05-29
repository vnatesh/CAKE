#ifndef __SRAM_H__
#define __SRAM_H__

#include "PacketSwitch.h"


SC_MODULE (SRAM) {

  Connections::In<PacketSwitch::Packet> packet_in;
  Connections::Out<PacketSwitch::Packet> packet_out;
  sc_in <bool> clk;
  sc_in <bool> rst;

  vector<vector<PacketSwitch::AccumType>> weights;
  vector<vector<PacketSwitch::AccumType>> activations;

  PacketSwitch::ID_type id;

  SC_CTOR(SRAM) {
    SC_THREAD(run);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  }


  void run() {

    PacketSwitch::Packet   p_in;
    PacketSwitch::Packet   p_out1;
    PacketSwitch::Packet   p_out2;

    packet_out.Reset();
    packet_in.Reset();
    // Wait for initial reset.
    wait(20.0, SC_NS);

    // send in weights then data
    int M = weights.size(); // 8 when tile_sz = 2
    int K = weights[0].size(); // 8 when tile_sz = 2
    int N = activations[0].size(); // 16 when tile_sz = 2 

    // Send weights to SA, loop over each weight tile and send it to corresponding MB
    printf("Sending weights from SRAM to MB \n");
    for (int m = 0; m < M/tile_sz; m++) { // row
      for (int k = 0; k < K/tile_sz; k++) { // col

        for (int i = 0; i < tile_sz; i++) {
          for (int j = 0; j < tile_sz; j++) {
            p_out1.data[i][j] = weights[m * tile_sz + i][k * tile_sz + j];
          }
        }

        wait(5);

        p_out1.src = 999; // sram src
        p_out1.srcPod = 0; // sram default src pod
        p_out1.dst = k;
        p_out1.dstPod = m;
        p_out1.d_type = 0;
        packet_out.Push(p_out1);     
        wait(10);
      }
    }

    // TODO : this is a triple loop due to the broadcast. Change packet header to include BCAST instruction
    printf("Sending activations from SRAM to MB \n");
    for (int n = 0; n < N/tile_sz; n++){
      cout << "Sending vector " << n << "\n";
      for (int k = 0; k < K/tile_sz; k++) {
        for (int m = 0; m < M/ tile_sz; m++) {
     
          for (int i = 0; i < tile_sz; i++) {
            for (int j = 0; j < tile_sz; j++) {
              p_out2.data[i][j] = activations[k * tile_sz + i][n * tile_sz + j];
            }
          }

          wait(5);

          p_out2.src = 999;
          p_out2.srcPod = 0;
          p_out2.dst = k;
          p_out2.dstPod = m;
          p_out2.d_type = 1;
          packet_out.Push(p_out2);
          wait(10);
        }
      }
    }


    while(1) {

      if(packet_in.PopNB(p_in)) {

        for (int i = 0; i < tile_sz; i++) {
          for (int j = 0; j < tile_sz; j++) {
            printf("%d ", p_in.data[i][j]);
          }
          printf("\n");
        }
        printf("\n");
      }

      wait(5);
    }
  }
};


#endif


