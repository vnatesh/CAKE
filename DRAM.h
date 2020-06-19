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

  double io_time_send;
  double io_time_recv;
  int packet_counter_send;
  int packet_counter_recv;


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

    struct timeval start, end;
    gettimeofday (&start, NULL);

    for (int n1 = 0; n1 < (N / N_dr); n1++) {
      for (int m_prime = 0; m_prime < (M / M_dr); m_prime++) {

        int m1;

        if((n1 % 2) == 0) {
          m1 = m_prime;
        } else {
          m1 = (M / M_dr) - m_prime - 1;
        }

        for (int k_prime = 0; k_prime < (K / K_dr); k_prime++) {

          int k1;

          if ((m_prime % 2) == 0) {
            k1 = k_prime;
          } else {
            k1 = (K / K_dr) - k_prime - 1;
          }

          if(DEBUG) cout <<  n1 << " " << m1 << " " << k1 << "\n";

          // Slice weights to be w[m*Wy:(m+1)Wy, k*Wz:(k+1)Wz]
          vector<vector<PacketSwitch::AccumType>> weight_blk_dr(M_dr, vector<PacketSwitch::AccumType>(K_dr)); 
          for (int i = 0; i < M_dr; i++) {
            for (int j = 0; j < K_dr; j++) {
              weight_blk_dr[i][j] = weights[m1 * M_dr + i][k1 * K_dr + j];
            }
          }

          // wait(10);
          wait();

          // Slice data to be d[k*Dz: (k+1)Dz, n*Dx: (n+1)*Dx]
          vector<vector<PacketSwitch::AccumType>> activation_blk_dr(K_dr, vector<PacketSwitch::AccumType>(N_dr)); 
          for (int i = 0; i < K_dr; i++) {
            for (int j = 0; j < N_dr; j++) {
              activation_blk_dr[i][j] = activations[k1 * K_dr + i][n1 * N_dr + j];
            }
          }

          // wait(10);
          wait();

          // Send weights to SRAM. For each block, loop over the weight tiles, assign src/dst addrs
          if(DEBUG) cout << "Sending weights from DRAM to SRAM \n";
          for (int m = 0; m < M_dr/tile_sz; m++) { // row
            for (int k = 0; k < K_dr/tile_sz; k++) { // col

              for (int i = 0; i < tile_sz; i++) {
                for (int j = 0; j < tile_sz; j++) {
                  p_out1.data[i][j] = weight_blk_dr[m * tile_sz + i][k * tile_sz + j];
                }
              }
              // wait(10);
              wait();
              p_out1.d_type = 0;
              packet_out.Push(p_out1);   
              // wait(20);
              wait();
              packet_counter_send++;
            }
          }

          if(DEBUG) cout << "Sending activations from DRAM to SRAM \n";
          for (int n = 0; n < N_dr/tile_sz; n++) {
            for (int k = 0; k < K_dr/tile_sz; k++) {
           
              for (int i = 0; i < tile_sz; i++) {
                for (int j = 0; j < tile_sz; j++) {
                  p_out2.data[i][j] = activation_blk_dr[k * tile_sz + i][n * tile_sz + j];
                }
              }

              // wait(10);
              wait();
              p_out2.d_type = 1;
              packet_out.Push(p_out2);
              // wait(20);
              wait();
              packet_counter_send++;
            }
          }
        }
      }
    }

    gettimeofday (&end, NULL);
    io_time_send += ((((end.tv_sec - start.tv_sec) * 1000000L)
        + (end.tv_usec - start.tv_usec)) / 1000000.0);
  }


  void receive_results() {

    PacketSwitch::Packet   p_in;
    packet_in.Reset();
    wait(20.0, SC_NS);


    int M_cnt = 0;
    int N_cnt = 0;
    int M_dr_cnt[NUM_PODS] = {0};
    int N_dr_cnt[NUM_PODS] = {0};
    int pod_cnt = 0;
    int pod_id;
    bool snake = false;

    // int tiles_recv[NUM_PODS][M_dr / M_sr][N_dr / tile_sz] = {0};

    struct timeval start, end;

    // Store the completed sum in the final result [m*Wy:(m+1)*Wy, n*Dx: (n+1)Dx]
    gettimeofday (&start, NULL);
    while(1) {

      if(packet_in.PopNB(p_in)) {

        packet_counter_recv++;
        pod_id = p_in.srcPod;

        for (int i = 0; i < tile_sz; i++) {
          for (int j = 0; j < tile_sz; j++) {
            // result[(m*Wy) + (row_tile*tile_sz) + i][(n * Dx) + (col_tile * tile_sz) + j] = p_in.data[i][j];
            result[(M_cnt * M_dr) + (M_dr_cnt[pod_id] * NUM_PODS) + (pod_id * tile_sz) + i][(N_cnt * N_dr) + (N_dr_cnt[pod_id] * tile_sz) + j] = p_in.data[i][j];
          }
        }

        N_dr_cnt[pod_id]++;

        if(N_dr_cnt[pod_id] == N_dr / tile_sz) {
          N_dr_cnt[pod_id] = 0;
          M_dr_cnt[pod_id]++;
        }

        if(M_dr_cnt[pod_id] == M_dr / M_sr) {
          N_dr_cnt[pod_id] = 0;
          M_dr_cnt[pod_id] = 0;
          pod_cnt++;
        }

        if(pod_cnt == NUM_PODS) {
          // reset pod counters
          for(int i = 0; i < NUM_PODS; i++) {
            M_dr_cnt[i] = 0;
          }
          for(int i = 0; i < NUM_PODS; i++) {
            N_dr_cnt[i] = 0;
          }

          pod_cnt = 0;
          M_cnt = snake ? M_cnt-1 : M_cnt+1; 
        }

        if((!snake && (M_cnt == (M / M_dr))) || (snake && M_cnt == -1)) {
          // reset pod counters
          for(int i = 0; i < NUM_PODS; i++) {
            M_dr_cnt[i] = 0;
          }
          for(int i = 0; i < NUM_PODS; i++) {
            N_dr_cnt[i] = 0;
          }

          pod_cnt = 0;
          M_cnt = snake ? 0 : M_cnt-1; 
          N_cnt++;
          snake = !snake;
        }


        if(N_cnt == N / N_dr) {
          gettimeofday (&end, NULL);
          io_time_recv += ((((end.tv_sec - start.tv_sec) * 1000000L)
                    + (end.tv_usec - start.tv_usec)) / 1000000.0);

          cout << "\n\nMAESTRO MMM RESULT\n\n";
          PrintMat(result); 
          sc_stop(); // STOP the stimulation here
        }
      }

      // wait(5);
      wait();
    }
  }

};


#endif


