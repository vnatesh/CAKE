#ifndef __SRAM_H__
#define __SRAM_H__

#include "PacketSwitch.h"


SC_MODULE(SRAM) {


  Connections::In<PacketSwitch::Packet> sram_dram_in;
  Connections::Out<PacketSwitch::Packet> sram_dram_out;

  Connections::In<PacketSwitch::Packet> packet_in;
  Connections::Out<PacketSwitch::Packet> packet_out;

  sc_in_clk     clk;
  sc_in<bool> rst;

  double io_time;
  double idle_time;
  // sc_time io_time;
  // sc_time idle_time;
  int packet_counter;

  int result_cnt = 0;
  int wait_cnt = 0;



  // 0-3 is MB ind, 4-7 is SA ind, 8 is SRAM, 9 is CB
  SC_HAS_PROCESS(SRAM);
  SRAM(sc_module_name name_) : sc_module(name_) {
    SC_THREAD(send_block);
    sensitive << clk.pos();
    NVHLS_NEG_RESET_SIGNAL_IS(rst);

    SC_THREAD(receive_result);
    sensitive << clk.pos();
    NVHLS_NEG_RESET_SIGNAL_IS(rst);

  }


  void send_block() {

    sram_dram_in.Reset();
    packet_out.Reset();
    wait(20.0, SC_NS);

    PacketSwitch::Packet p_in;

    PacketSwitch::Packet p_out1;
    PacketSwitch::Packet p_out2;

    int m_cnt = 0; // weight row
    int k_cnt = 0; // weight col / act row
    int n_cnt = 0; // act col


    vector<vector<PacketSwitch::AccumType>> weight_blk_sr(M_dr, vector<PacketSwitch::AccumType>(K_dr)); 
    vector<vector<PacketSwitch::AccumType>> activation_blk_sr(K_dr, vector<PacketSwitch::AccumType>(N_dr)); 

    sc_time start, end;


    while(1) {
      if(sram_dram_in.PopNB(p_in)) {

        start = sc_time_stamp();
        if(p_in.d_type == 0) {

          for (int i = 0; i < tile_sz; i++) {
            for (int j = 0; j < tile_sz; j++) {
              weight_blk_sr[m_cnt * tile_sz + i][k_cnt * tile_sz + j] = p_in.data[i][j];
            }
          }

          k_cnt++;
          if(k_cnt == K_dr / tile_sz) {
            k_cnt = 0;
            m_cnt++;
          }
        }

        else if(p_in.d_type == 1) {

          for (int i = 0; i < tile_sz; i++) {
            for (int j = 0; j < tile_sz; j++) {
              activation_blk_sr[k_cnt * tile_sz + i][n_cnt * tile_sz + j] = p_in.data[i][j];
            }
          }

          // wait(5);
          k_cnt++;
          if(k_cnt == K_dr / tile_sz) {
            k_cnt = 0;
            n_cnt++;
          }
        }
      }

      wait();


      // when a full weight and act DRAM block are in SRAM, sweep the DRAM block via snake, 
      // i.e. send each weight/act SRAM_block pair in the snaking pattern, 
      // with weight being sent first, then act. 
      // TODO : need to actually implement data and weight reuse. Currently  
      if((m_cnt == M_dr / tile_sz) && (n_cnt ==  N_dr / tile_sz)) {
        
        for (int n1 = 0; n1 < (N_dr / N_sr); n1++) {
          for (int m_prime = 0; m_prime < (M_dr / M_sr); m_prime++) {

            int m1;

            if((n1 % 2) == 0) {
              m1 = m_prime;
            } else {
              m1 = (M_dr / M_sr) - m_prime - 1;
            }

            for (int k_prime = 0; k_prime < (K_dr / K_sr); k_prime++) {

              int k1;

              if ((m_prime % 2) == 0) {
                k1 = k_prime;
              } else {
                k1 = (K_dr / K_sr) - k_prime - 1;
              }

              if(DEBUG) cout << "Sending weight block from SRAM to Maestro" << "\n";
              for (int m = 0; m < (M_sr / tile_sz); m++) {
                for (int k = 0; k < (K_sr / tile_sz); k++) {

                  for (int i = 0; i < tile_sz; i++) {
                    for (int j = 0; j < tile_sz; j++) {
                      p_out1.data[i][j] = weight_blk_sr[(m1 * M_sr) + (m * tile_sz + i)][(k1 * K_sr) + (k * tile_sz + j)];
                    }
                  }

                  p_out1.src = INT_MAX; // sram src
                  p_out1.srcPod = 0; // sram default src pod
                  p_out1.dst = k;
                  p_out1.dstPod = m;
                  p_out1.d_type = 0;
                  p_out1.bcast = 0;
                  packet_out.Push(p_out1); 
                  packet_counter++;

                  wait();
                }
              }


              if(DEBUG) cout << "Sending activation block from SRAM to Maestro" << "\n";
              for (int n = 0; n < (N_sr / tile_sz); n++){
                for (int k = 0; k < (K_sr / tile_sz); k++) {
               
                  for (int i = 0; i < tile_sz; i++) {
                    for (int j = 0; j < tile_sz; j++) {
                      p_out2.data[i][j] = activation_blk_sr[(k1 * K_sr) + (k * tile_sz + i)][(n1 * N_sr) + (n * tile_sz + j)];
                    }
                  }

                  p_out2.src = INT_MAX;
                  p_out2.srcPod = 0;
                  p_out2.dst = k;
                  // p_out2.dstPod = m;
                  p_out2.d_type = 1;
                  p_out2.bcast = 1;
                  packet_out.Push(p_out2);
                  packet_counter++;

                  wait();
                }
              }
            }
          }
        }
            
        m_cnt = 0;
        n_cnt = 0;
      }
         
      end = sc_time_stamp();
      io_time += (end - start).to_default_time_units();
    
      start = sc_time_stamp();
      wait();
      end = sc_time_stamp();
      idle_time += (end - start).to_default_time_units();
    }
  }


  void receive_result() {

    sram_dram_out.Reset();
    packet_in.Reset();
    wait(20.0, SC_NS);

    PacketSwitch::Packet p_in2;
    
    // ofstream myfile;
    // myfile.open ("sram_traffic.csv");


    while(1) {

      if(packet_in.PopNB(p_in2)) {

        for (int i = 0; i < tile_sz; i++) {
          for (int j = 0; j < tile_sz; j++) {
            cout << p_in2.data[i][j] << " ";
          }
          cout << "\n";
        }
        cout << "\n";
      
        sram_dram_out.Push(p_in2);
        result_cnt++;
      } 

      wait(); 
      // wait_cnt++; // use wait cnt as a way to synchronize tracking of the current time and result_cnt
      // if(wait_cnt % 10 == 0) {
      //   cout << sc_time_stamp().to_default_time_units() << " " << result_cnt << "\n";
      //   myfile << sc_time_stamp().to_default_time_units() << " " << result_cnt << "\n";
      // }

      // if(result_cnt == Wz*Dx) {
      //   myfile.close();
      // }
    }
  }


};


#endif



