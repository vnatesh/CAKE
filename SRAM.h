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

    vector<vector<PacketSwitch::Packet>> weight_blk_sr(M_dr/tile_sz, vector<PacketSwitch::Packet>(K_dr/tile_sz)); 
    vector<vector<PacketSwitch::Packet>> activation_blk_sr(K_dr/tile_sz, vector<PacketSwitch::Packet>(N_dr/tile_sz)); 

    sc_time start, end;

    while(1) {
      if(sram_dram_in.PopNB(p_in)) {

        start = sc_time_stamp();
        if(p_in.d_type == 0) {

          weight_blk_sr[p_in.X % (M_dr/tile_sz)][p_in.Z % (K_dr/tile_sz)] = p_in;
          k_cnt++;
          
          if(k_cnt == K_dr / tile_sz) {
            k_cnt = 0;
            m_cnt++;
          }
        }

        else if(p_in.d_type == 1) {

          activation_blk_sr[p_in.Z % (K_dr/tile_sz)][p_in.Y % (N_dr/tile_sz)] = p_in;
          k_cnt++;

          if(k_cnt == K_dr / tile_sz) {
            k_cnt = 0;
            n_cnt++;
          }
        }
      }

      wait();


      // when a full DRAM block (both weight and act) have been stored in SRAM, sweep the DRAM block via snake, 
      // i.e. send each weight/act SRAM_block pair in the snaking pattern, 
      // with act being sent first, then weight. 
      // TODO : need to actually implement data and weight reuse.   
      if((m_cnt == M_dr / tile_sz) && (n_cnt ==  N_dr / tile_sz)) {

        for (int n1 = 0; n1 < (N_dr / N_sr); n1++) {

          for (int k_prime = 0; k_prime < (K_dr / K_sr); k_prime++) {

            int k1;
            if ((n1 % 2) == 0) {
              k1 = k_prime;
            } else {
              k1 = (K_dr / K_sr) - k_prime - 1;
            }

            // send activations first to load the MBs with an entire SRAM Block
            if(DEBUG) cout << "Sending SRAM activation block from SRAM to Maestro" << "\n";
            for (int n = 0; n < (N_sr / tile_sz); n++){
              for (int k = 0; k < (K_sr / tile_sz); k++) {
                   
                p_out2 = activation_blk_sr[(k1 * (K_sr/tile_sz)) + k][(n1 * (N_sr/tile_sz)) + n];
                p_out2.x = -1; // dummy value for x (pod_id)...it gets set by the packet switch during bcast
                p_out2.y = n;
                p_out2.z = k;
                p_out2.MB = k; 
                p_out2.SA = k + POD_SZ; 
                p_out2.SRAM = INT_MAX; // sram src
                p_out2.ttl = M_dr / M_sr; // each MB stores (N_sr/tile_sz) data tiles for ttl rounds
                p_out2.src = INT_MAX; // sram src
                p_out2.dst = k; // dst is MB
                // p_out2.srcPod = 0;
                p_out2.d_type = 1;
                p_out2.bcast = 1;
                packet_out.Push(p_out2);
                packet_counter++;

                wait();
              }
            }
          }

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


              // Sweep through the DRAM block, sending data to be loaded on MBs, then weights
              if(DEBUG) cout << "Sending SRAM weight block from SRAM to Maestro" << "\n";
              for (int m = 0; m < (M_sr / tile_sz); m++) {
                for (int k = 0; k < (K_sr / tile_sz); k++) {

                  p_out1 = weight_blk_sr[(m1 * (M_sr/tile_sz)) + m][(k1 * (K_sr/tile_sz)) + k];
                  p_out1.x = m; // represents POD id
                  p_out1.y = -1;
                  p_out1.z = k;
                  p_out1.MB = k; 
                  p_out1.SA = k + POD_SZ; 
                  p_out1.SRAM = INT_MAX; // sram src
                  p_out1.ttl = 0; // weight ttl set to 0 except when new region about to start
                  p_out1.src = INT_MAX; // sram src
                  p_out1.dst = k; // dst is MB
                  // p_out1.srcPod = 0; // sram default src pod
                  // p_out1.dstPod = m;
                  p_out1.d_type = 0;
                  p_out1.bcast = 0;
                  packet_out.Push(p_out1); 
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
    vector<vector<PacketSwitch::Packet>> result_blk_sr(M_dr/tile_sz, vector<PacketSwitch::Packet>(N_dr/tile_sz)); 

    // ofstream myfile;
    // myfile.open ("sram_traffic.csv");

    while(1) {

      if(packet_in.PopNB(p_in2)) {

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



