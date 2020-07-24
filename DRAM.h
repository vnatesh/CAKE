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

    sc_time start, end;
    start = sc_time_stamp();

    for (int n1 = 0; n1 < (N / N_sr); n1++) {

      for (int m_prime = 0; m_prime < (M / M_sr); m_prime++) {

        int m1;

        if((n1 % 2) == 0) {
          m1 = m_prime;
        } else {
          m1 = (M / M_sr) - m_prime - 1;
        }

        for (int k_prime = 0; k_prime < (K / K_sr); k_prime++) {

          int k1;

          if ((m_prime % 2) == 0) {
            k1 = k_prime;
          } else {
            k1 = (K / K_sr) - k_prime - 1;
          }

          if(DEBUG) cout <<  n1 << " " << m1 << " " << k1 << "\n";

          // Send weights to SRAM. For each block, loop over the weight tiles, assign src/dst addrs
          if(DEBUG) cout << "Sending weights from DRAM to SRAM \n";
          for (int m = 0; m < M_sr; m++) { // row
            for (int k = 0; k < K_sr; k++) { // col

              for (int i = 0; i < tile_sz; i++) {
                for (int j = 0; j < tile_sz; j++) {
                  p_out1.data[i][j] = weights[(m1 * M_sr * tile_sz) + (m * tile_sz) + i][(k1 * K_sr * tile_sz) + (k * tile_sz) + j];
                }
              }

              p_out1.X = (m1 * M_sr) + m;
              p_out1.Y = -1;
              p_out1.Z = (k1 * K_sr) + k;

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
          for (int n = 0; n < N_sr; n++) {
            for (int k = 0; k < K_sr; k++) {
           
              for (int i = 0; i < tile_sz; i++) {
                for (int j = 0; j < tile_sz; j++) {
                  p_out2.data[i][j] = activations[(k1 * K_sr * tile_sz) + (k * tile_sz) + i][(n1 * N_sr * tile_sz) + (n * tile_sz) + j];
                }
              }

              p_out2.X = -1;
              p_out2.Y = (n1 * N_sr) + n;
              p_out2.Z = (k1 * K_sr) + k;

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

    end = sc_time_stamp();
    io_time_send += (end - start).to_default_time_units();
  }


  void receive_results() {

    PacketSwitch::Packet   p_in;
    packet_in.Reset();
    wait(20.0, SC_NS);

    int p_cnt = 0;
    int m;
    int n;

    sc_time start, end;
    start = sc_time_stamp();

    while(1) {

      if(packet_in.PopNB(p_in)) {

        packet_counter_recv++;
        m = p_in.X;
        n = p_in.Y;

        for (int i = 0; i < tile_sz; i++) {
          for (int j = 0; j < tile_sz; j++) {
            result[m * tile_sz + i][n * tile_sz + j] = p_in.data[i][j];            
          }
        }

        p_cnt++;

        if(p_cnt == (M * N)) {
          end = sc_time_stamp();
          io_time_recv += (end - start).to_default_time_units();
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


