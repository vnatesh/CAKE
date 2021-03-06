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
  // bool ready = 1;
  // bool start_send = 0;
  bool ready_result = 1;
  bool start_send_result = 0;

  bool heyy = false;


  vector<vector<PacketSwitch::Packet>> weight_buf;
  vector<vector<PacketSwitch::Packet>> activation_buf;
  vector<vector<PacketSwitch::Packet>> result_buf;


  // 0-3 is SB ind, 4-7 is SA ind, 8 is SRAM, 9 is CB
  SC_HAS_PROCESS(SRAM);
  SRAM(sc_module_name name_) : sc_module(name_) {
    
    SC_THREAD(recv_dram);
    sensitive << clk.pos();
    NVHLS_NEG_RESET_SIGNAL_IS(rst);

    SC_THREAD(recv_result_maestro);
    sensitive << clk.pos();
    NVHLS_NEG_RESET_SIGNAL_IS(rst);

    SC_THREAD(send_result_dram);
    sensitive << clk.pos();
    NVHLS_NEG_RESET_SIGNAL_IS(rst);

  }


  void recv_dram() {


    PacketSwitch::Packet p_out1;
    PacketSwitch::Packet p_out2;


    sram_dram_in.Reset();
    packet_out.Reset();

    wait(20.0, SC_NS);

    PacketSwitch::Packet p_in;

    int m_cnt = 0; // weight row
    int k_cnt = 0; // weight col / act row
    int n_cnt = 0; // act col
    // bool buffer_opt = 1;

    int act_block_cnt = 0;
    int weight_block_cnt = 0;

    int weight_cnt = 0;
    int act_cnt = 0;


    vector<vector<PacketSwitch::Packet>> weight_blk_sr(M_sr, vector<PacketSwitch::Packet>(K_sr)); 
    vector<vector<PacketSwitch::Packet>> activation_blk_sr(K_sr, vector<PacketSwitch::Packet>(N_sr)); 

    sc_time start, end;

    while(1) {

      if(sram_dram_in.PopNB(p_in)) {

        // cout << "SRAM Receive packet from DRAM " << sc_time_stamp() << "\n";  

        start = sc_time_stamp();
        if(p_in.d_type == 0) { // receive weights first

          weight_blk_sr[p_in.X % M_sr][p_in.Z % K_sr] = p_in;
          k_cnt++;
          
          if(k_cnt == K_sr) {
            k_cnt = 0;
            m_cnt++;
          }
        }

        else if(p_in.d_type == 1) {

          activation_blk_sr[p_in.Z % K_sr][p_in.Y % N_sr] = p_in;

          k_cnt++;

          if(k_cnt == K_sr) {
            k_cnt = 0;
            n_cnt++;
          }
        }
      }

      wait();

      // when a full DRAM block (both weight and act) have been stored in SRAM, sweep the DRAM block via snake, 
      // i.e. send each weight/act SRAM_block pair in the snaking pattern, 
      // with act being sent first, then weight. 
      // TODO : need to implement weight reuse.   
      if((m_cnt == M_sr) && (n_cnt ==  N_sr)) {
        
        weight_buf = weight_blk_sr;
        activation_buf = activation_blk_sr;

        for (int n1 = 0; n1 < (N_sr / N_ob); n1++) {

          for (int k_prime = 0; k_prime < (K_sr / K_ob); k_prime++) {

            int k1;
            if ((n1 % 2) == 0) {
              k1 = k_prime;
            } else {
              k1 = (K_sr / K_ob) - k_prime - 1;
            }

            // send activations first to load the SBs with a region of activations
            // if(DEBUG) cout << "Sending SRAM activation block from SRAM to Maestro" << "\n";
            // cout << "HEYYY Sending SRAM activation block from SRAM to Maestro" << "\n";
            for (int n = 0; n < (N_ob); n++){
              for (int k = 0; k < (K_ob); k++) {
                   
                p_out2 = activation_buf[(k1 * (K_ob)) + k][(n1 * (N_ob)) + n];
                p_out2.x = -1; // dummy value for x (pod_id)...it gets set by the packet switch during bcast
                p_out2.y = n;
                p_out2.z = k;
                p_out2.SB = k; 
                p_out2.SA = k + POD_SZ; 
                p_out2.SRAM = INT_MAX; // sram src
                p_out2.ttl = M_sr / M_ob; // each SB stores (N_sr/tile_sz) data tiles that are reused ttl times
                p_out2.src = INT_MAX; // sram src
                p_out2.dst = k; // dst is SB
                p_out2.d_type = 1;
                p_out2.bcast = 1;
                packet_out.Push(p_out2);
                packet_counter++;
                wait();

                act_cnt++;


                // cout << "SRAM send packet to maestro\n"; 
              }
            }
          }

          for (int m_prime = 0; m_prime < (M_sr / M_ob); m_prime++) {

            int m1;
            if((n1 % 2) == 0) {
              m1 = m_prime;
            } else {
              m1 = (M_sr / M_ob) - m_prime - 1;
            }

            // if(m_prime == 0 && n1 != 0) { // skip sending this weight strip to allow reuse in SBs
            //   continue;
            // }

            int ttl = 0;
            // cout << m_prime << " " << n1 << "\n";
            // int ttl;
            // if((m_prime ==  ((M_dr / M_sr) - 1)) && (n1 != ((N_dr / N_sr) - 1))) {

            //   if(M_dr == M_sr && n1 == 0) {
            //     ttl = (N_dr / N_sr) - 1;
            //   } else {
            //     ttl = 1;
            //   } // weight ttl set to 1 to indicate reuse when new region is beginning
            // } else { 
            //   ttl = 0; // weight ttl set to 0 except when new region about to start
            // }

            for (int k_prime = 0; k_prime < (K_sr / K_ob); k_prime++) {

              int k1;
              if ((m_prime % 2) == 0) {
                k1 = k_prime;
              } else {
                k1 = (K_sr / K_ob) - k_prime - 1;
              }

              // Sweep through the DRAM block, sending data to be loaded on SBs, then weights
              // if(DEBUG) cout << "YOOO Sending SRAM weight block from SRAM to Maestro" << "\n";
              // cout << "YOOO Sending SRAM weight block from SRAM to Maestro" << "\n";
              for (int m = 0; m < M_ob; m++) {
                for (int k = 0; k < K_ob; k++) {

                  p_out1 = weight_buf[(m1 * (M_ob)) + m][(k1 * (K_ob)) + k];
                  p_out1.x = m; // represents POD id
                  p_out1.y = -1;
                  p_out1.z = k;
                  p_out1.SB = k; 
                  p_out1.SA = k + POD_SZ; 
                  p_out1.SRAM = INT_MAX; // sram src
                  p_out1.ttl = ttl;
                  p_out1.src = INT_MAX; // sram src
                  p_out1.dst = k; // dst is SB
                  p_out1.d_type = 0;
                  p_out1.bcast = 0;
                  packet_out.Push(p_out1); 
                  packet_counter++;
                  // cout << "SRAM send packet to maestro " << sc_time_stamp() << "\n"; 
                  wait();

                  weight_cnt++;

                  
                }
              }
            }
          }
        }

        m_cnt = 0;
        n_cnt = 0;
        act_block_cnt++;
        weight_block_cnt++;

      }
    
      end = sc_time_stamp();
      io_time += (end - start).to_default_time_units();
    
      start = sc_time_stamp();
      wait();
      end = sc_time_stamp();
      idle_time += (end - start).to_default_time_units();
    }
  }






  void send_result_dram() {

    sram_dram_out.Reset();
    wait(20.0, SC_NS);

    while(1) {
      if(start_send_result) {  

        for(int m = 0; m < M_sr; m++) {
          for(int n = 0; n < N_sr; n++) {
            sram_dram_out.Push(result_buf[m][n]);
            wait();
            // cout << "SRAM send result to DRAM\n"; 
          }
        }

        ready_result = 1;
        start_send_result = 0;
      }

      wait();
    }
  }


  void recv_result_maestro() {

    packet_in.Reset();
    wait(20.0, SC_NS);

    PacketSwitch::Packet p_in;

    vector<vector<PacketSwitch::Packet>> result_blk_sr(M_sr, vector<PacketSwitch::Packet>(N_sr)); 

    // ofstream myfile;
    // myfile.open ("sram_traffic.csv");

    int p_cnt = 0;
    // int r = 0;

    while(1) {

      if(packet_in.PopNB(p_in)) {
        // collect an entire DRAM block, then send to DRAM
        // cout << "SRAM Receive result from Maestro\n"; 

        // r++;
        // cout << "final r_cnt = " << r << "\n";
        result_blk_sr[p_in.X % M_sr][p_in.Y % N_sr] = p_in;

        result_cnt++;
        p_cnt++;

        if(p_cnt == (M_sr * N_sr)) {

          while(!ready_result) wait(); // wait to send the next SRAM block until sending thread
                                // has finished sending the current SRAM block to maestro
          ready_result = 0;
          result_buf = result_blk_sr;

          start_send_result = 1;
          p_cnt = 0;
        }
      } 

      wait(); 
      // wait_cnt++; 
      // if(wait_cnt % 10 == 0) {
      // cout << sc_time_stamp().to_default_time_units() << " " << result_cnt << "\n";
      //   myfile << sc_time_stamp().to_default_time_units() << " " << result_cnt << "\n";
      // }

      // if(result_cnt == Wz*Dx) {
      //   myfile.close();
      // }
    }
  }


};


#endif



