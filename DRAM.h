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
  vector<vector<PacketSwitch::AccumType>> result;


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


    int M = weights.size(); // Should be a multiple of 4*tile_sz
    int K = weights[0].size(); // Should be a multiple of 4* tile_sz
    int N = activations[0].size(); // Should be a multiple of 8*tile_sz


    // Send weights to SA, loop over each weight tile and send it to corresponding MB
    printf("Sending weights from DRAM to SRAM \n");
    for (int n1 = 0; n1 < (N / Dx); n1++) {
      for (int m_prime = 0; m_prime < (M / Wy); m_prime++) {

        int m1;

        if((n1 % 2) == 0) {
          m1 = m_prime;
        } else {
          m1 = (M / (Wy)) - m_prime - 1;
        }

        for (int k_prime = 0; k_prime < (K / Wz); k_prime++) {

          int k1;

          if ((m_prime % 2) == 0) {
            k1 = k_prime;
          } else {
            k1 = (K / (Wz)) - k_prime - 1;
          }

          cout <<  n1 << " " << m1 << " " << k1 << "\n";

          // Slice weights to be w[m*Wy:(m+1)Wy, k*Wz:(k+1)Wz]
          
          vector<vector<PacketSwitch::AccumType>> weight(Wy, vector<PacketSwitch::AccumType>(Wz)); 
          for (int i = 0; i < Wy; i++) {
            for (int j = 0; j < Wz; j++) {
              weight[i][j] = weights[m1 * Wy + i][k1 * Wz + j];
            }
          }

          wait(50);

          // Slice data to be d[k*Dz: (k+1)Dz, n*Dx: (n+1)*Dx]
          vector<vector<PacketSwitch::AccumType>> activation(Dz, vector<PacketSwitch::AccumType>(Dx)); 
          for (int i = 0; i < Dz; i++) {
            for (int j = 0; j < Dx; j++) {
              activation[i][j] = activations[k1 * Dz + i][n1 * Dx + j];
            }
          }

          wait(50);

          // Send weights to SRAM. For each block, loop over the weight tiles, assign src/dst addrs
          printf("Sending weights from DRAM to SRAM \n");
          for (int m = 0; m < Wy/tile_sz; m++) { // row
            for (int k = 0; k < Wz/tile_sz; k++) { // col

              for (int i = 0; i < tile_sz; i++) {
                for (int j = 0; j < tile_sz; j++) {
                  p_out1.data[i][j] = weight[m * tile_sz + i][k * tile_sz + j];
                }
              }
              wait(50);
              p_out1.d_type = 0;
              packet_out.Push(p_out1);   
              wait(100);
            }
          }

          printf("Sending activations from DRAM to SRAM \n");
          for (int n = 0; n < Dx/tile_sz; n++) {
            for (int k = 0; k < Wz/tile_sz; k++) {
           
              for (int i = 0; i < tile_sz; i++) {
                for (int j = 0; j < tile_sz; j++) {
                  p_out2.data[i][j] = activation[k * tile_sz + i][n * tile_sz + j];
                }
              }

              wait(50);
              p_out2.d_type = 1;
              packet_out.Push(p_out2);
              wait(100);
            }
          }
        



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


  // void receive_results() {

  //   PacketSwitch::Packet   p_in;
  //   packet_in.Reset();
  //   wait(20.0, SC_NS);

  //   int row_tile = 0;
  //   int col_tile = 0;

  //   // Store the completed sum in the final result [m*Wy:(m+1)*Dy, n*Dx: (n+1)Dx]
  //   while(1) {

  //     if(packet_in.PopNB(p_in)) {

  //       for (int i = 0; i < tile_sz; i++) {
  //         for (int j = 0; j < tile_sz; j++) {
  //           result[(m*Wy) + (row_tile*tile_sz) + i][(n * Dx) + (col_tile * tile_sz) + j] = p_in.data[i][j];
  //         }
  //       }

  //       col_tile++;
  //       if(col_tile == (Dx / tile_sz)) {
  //         col_tile = 0;
  //         row_tile++;
  //       }

  //       if(row_tile == (Dy / tile_sz)) {
  //         PrintMat(result); 
  //       }
  //     }

  //     wait(5);
  //   }
  // }



};


#endif


