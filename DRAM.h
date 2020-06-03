#ifndef __DRAM_H__
#define __DRAM_H__

#include "PacketSwitch.h"


SC_MODULE(DRAM) {

  Connections::In<PacketSwitch::Packet> packet_in;
  Connections::Out<PacketSwitch::Packet> packet_out;

  sc_in <bool> clk;
  sc_in <bool> rst;

  vector<vector<PacketSwitch::AccumType>> weights;
  vector<vector<PacketSwitch::AccumType>> activations;

  PacketSwitch::ID_type id;

  SC_CTOR(DRAM) {
    SC_THREAD(send_blocks);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);

    SC_THREAD(receive_results);
    sensitive << clk.pos();
    NVHLS_NEG_RESET_SIGNAL_IS(rst); 
  }


  void send_blocks() {

    PacketSwitch::Packet   p_out1;
    PacketSwitch::Packet   p_out2;

    packet_out.Reset();
    // Wait for initial reset.
    wait(20.0, SC_NS);

    // send in weights then data
    int M = weights.size(); // 8 when tile_sz = 2
    int K = weights[0].size(); // 8 when tile_sz = 2
    int N = activations[0].size(); // 16 when tile_sz = 2 

    // for(int b_m = 0; b_m < M; b_m++) {
    //   for(int b_ = 0; b_m < M; b_m++) {

    // TODO : run of 3 blocks for now, change to full matrices
    for(int b = 0; b < 3; b++) {

      // Send weights to SRAM. For each block, loop over the weight tiles, assign src/dst addrs
      printf("Sending weights from DRAM to SRAM \n");
      for (int m = 0; m < M/tile_sz; m++) { // row
        for (int k = 0; k < K/tile_sz; k++) { // col

          for (int i = 0; i < tile_sz; i++) {
            for (int j = 0; j < tile_sz; j++) {
              p_out1.data[i][j] = weights[m * tile_sz + i][k * tile_sz + j];
            }
          }
          wait(50);

          // p_out1.src = 999; // sram src
          // p_out1.srcPod = 0; // sram default src pod
          // p_out1.dst = k;
          // p_out1.dstPod = m;
          p_out1.d_type = 0;
          packet_out.Push(p_out1);   
          wait(100);
        }
      }

      printf("Sending activations from DRAM to SRAM \n");
      for (int n = 0; n < N/tile_sz; n++) {
        for (int k = 0; k < K/tile_sz; k++) {
       
          for (int i = 0; i < tile_sz; i++) {
            for (int j = 0; j < tile_sz; j++) {
              p_out2.data[i][j] = activations[k * tile_sz + i][n * tile_sz + j];
            }
          }

          wait(50);

          // p_out2.src = 999;
          // p_out2.srcPod = 0;
          // p_out2.dst = k;
          // p_out2.dstPod = m;
          p_out2.d_type = 1;
          packet_out.Push(p_out2);
          wait(100);
        }
      }
    }
  }

  void receive_results() {

    PacketSwitch::Packet   p_in;
    packet_in.Reset();
    wait(20.0, SC_NS);

    while(1) {

      if(packet_in.PopNB(p_in)) {

        printf("DRAM RESULT\n");
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


