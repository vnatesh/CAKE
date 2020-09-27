#ifndef __SRAM_H__
#define __SRAM_H__

#include "PacketSwitch.h"
#include "log.h"


vector<vector<int> > bcast_dst() {

  vector<vector<int> > bcast(K_sr, vector<int>(2*NUM_SA - 1, 0));
  int d;

  for(int kob_ind = 0; kob_ind < K_sr/K_ob; kob_ind++) {
    for(int k = 0; k < K_ob; k++) {
      for(int k_ind = kob_ind; k_ind < NUM_PODS_USED; k_ind += K_sr/K_ob) {
        for(int j = 0; j < M_ob; j++) {

          d = k_ind*POD_SZ + j*K_ob + k + (NUM_SA - 1);
        
          while(1) {
            bcast[kob_ind * K_ob + k][d] = 1;
            if(d == 0) {
              break;
            }
            d = (d-1) / 2;
          }
        }
      }
    }
  }

  return bcast;
}


int LCA(int a, int b) {
  while(1) {
    a = (a-1) / 2;
    b = (b-1) / 2;

    if(a == b) {
      return a;
    }
  }
}


vector<vector<vector<vector<vector<int> > > > > AB_chains() {

  vector<vector<vector<vector<vector<int> > > > > ab_chains(M_sr/M_ob, 
    vector<vector<vector<vector<int> > > >(K_sr/K_ob, 
      vector<vector<vector<int> > >(M_ob, 
        vector<vector<int> >(K_ob, 
          vector <int>(NUM_LEVELS+1, INT_MIN)))));  

  int next_leaf;
  int curr_leaf;

  // create m strips of leaf ids representing tiles
  // being placed k-first within pods and accross pods
  vector<vector<int> > strips(M_sr, vector <int>(K_sr, 0));
  for (int m1 = 0; m1 < (M_sr / M_ob); m1++) {
    for (int k1 = 0; k1 < (K_sr / K_ob); k1++) {
      for (int m = 0; m < M_ob; m++) {
        for (int k = 0; k < K_ob; k++) {
          strips[m1*M_ob + m][k1*K_ob + k] = m1*(K_sr / K_ob)*POD_SZ + k1*POD_SZ + m*K_ob  + k + (NUM_SA - 1);
        }
      }
    }
  }
  
  // create AB chain for each tile in every OB 
  // by comparing each tile with the same M_sr 
  // index to one another
  for (int m1 = 0; m1 < (M_sr / M_ob); m1++) {
    for (int k1 = 0; k1 < (K_sr / K_ob); k1++) {
      for (int m = 0; m < M_ob; m++) {
        for (int k = 0; k < K_ob; k++) {

          set<int, greater<int> > chain;  
          curr_leaf = m1*(K_sr / K_ob)*POD_SZ + k1*POD_SZ + m*K_ob + k + (NUM_SA - 1); 
      
          for(int i = 0; i < K_sr; i++) {
            next_leaf = strips[m1 * M_ob + m][i];
            if(next_leaf != curr_leaf) {
              chain.insert(LCA(curr_leaf, next_leaf)); 
            }
          }
          chain.insert(INT_MIN); // SRAM is last address in chain

          for(unsigned int n = 0; n < chain.size(); n++) {
            set<int>::iterator iter = chain.begin();
            advance(iter, n);
            ab_chains[m1][k1][m][k][n] = *iter;
          }

        }
      }
    }
  }

  return ab_chains;
}




SC_MODULE(SRAM) {

  Connections::In<PacketSwitch::Packet> sram_dram_in;
  Connections::Out<PacketSwitch::Packet> sram_dram_out;

  Connections::Out<PacketSwitch::Packet> sram_partial_out;


  Connections::In<PacketSwitch::Packet> packet_in;
  Connections::Out<PacketSwitch::Packet> packet_out;

  sc_in_clk     clk;
  sc_in<bool> rst;

  double io_time;
  double idle_time;
  int packet_counter;
  int result_cnt = 0;
  int wait_cnt = 0;

  sc_mutex  buffer_inp;
  sc_event  ready_send_inp;
  sc_event  send_init_inp;

  sc_mutex  buffer_res;
  sc_mutex  ready_send_res;
  sc_event  send_init_res;

  vector<vector<PacketSwitch::Packet>> weight_buf;
  vector<vector<PacketSwitch::Packet>> activation_buf;
  vector<vector<PacketSwitch::Packet>> result_buf;


  SC_HAS_PROCESS(SRAM);
  SRAM(sc_module_name name_) : sc_module(name_) {
    
    SC_THREAD(recv_dram);
    sensitive << clk.pos();
    NVHLS_NEG_RESET_SIGNAL_IS(rst);

    SC_THREAD(send_to_maestro);
    sensitive << clk.pos();
    NVHLS_NEG_RESET_SIGNAL_IS(rst);

    SC_THREAD(recv_result_maestro);
    sensitive << clk.pos();
    NVHLS_NEG_RESET_SIGNAL_IS(rst);

    SC_THREAD(send_result);
    sensitive << clk.pos();
    NVHLS_NEG_RESET_SIGNAL_IS(rst);

  }


  void recv_dram() {

    PacketSwitch::Packet p_in;
    sram_dram_in.Reset();
    wait(20.0, SC_NS);

    int m_cnt = 0; // weight row
    int k_cnt = 0; // weight col / act row
    int n_cnt = 0; // act col
    bool send_thread_start_inp = 0;

    vector<vector<PacketSwitch::Packet>> weight_blk_sr(M_sr, vector<PacketSwitch::Packet>(K_sr)); 
    vector<vector<PacketSwitch::Packet>> activation_blk_sr(K_sr, vector<PacketSwitch::Packet>(N_sr)); 


    while(1) {

      if(sram_dram_in.PopNB(p_in)) {

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

      // when a full SRAM block (both weight and act) have been stored in SRAM, trigger sending thread 
      if((m_cnt == M_sr) && (n_cnt ==  N_sr)) {

        buffer_inp.lock();
        weight_buf = weight_blk_sr;
        activation_buf = activation_blk_sr;
        if(!send_thread_start_inp) {
          send_thread_start_inp = 1;
          send_init_inp.notify(); // starts the sending thread only after first SRAM blk has been received
        }
        buffer_inp.unlock();
        ready_send_inp.notify();        
        
        m_cnt = 0;
        n_cnt = 0;
      }

      wait();
    }
  }



  void send_to_maestro() {

    PacketSwitch::Packet p_out1;
    PacketSwitch::Packet p_out2;
    packet_out.Reset();
    wait(20.0, SC_NS);

    int act_block_cnt = 0;
    int weight_block_cnt = 0;

    int weight_cnt = 0;
    int act_cnt = 0;
    int dst_id;

    vector<vector<vector<vector<vector<int> > > > > ab_chains = AB_chains(); // AB_chains for weight packets
    vector<vector<int> > bcast = bcast_dst(); // bcast headers for data packets

    // wait for first SRAM block to arrive from DRAM
    wait(send_init_inp);

    while(1) {

      buffer_inp.lock();

      dst_id = 0;
      // Sweep through the SRAM block k-first to load SAs with weights, then send data with bcast headers
      for (int m1 = 0; m1 < (M_sr / M_ob); m1++) {

        for (int k1 = 0; k1 < (K_sr / K_ob); k1++) {

          // place tiles within each OB k-first 
          if(DEBUG) cout << "Sending OB weight component from SRAM to Maestro" << "\n";
          for (int m = 0; m < M_ob; m++) {
            for (int k = 0; k < K_ob; k++) {

              p_out1 = weight_buf[(m1 * M_ob) + m][(k1 * K_ob) + k];
              p_out1.x = (m1 * M_ob) + m; 
              p_out1.y = -1;
              p_out1.z = (k1 * K_ob) + k;
              p_out1.SB = k; 
              p_out1.SA = k + POD_SZ; 
              p_out1.SRAM = INT_MIN; // sram src
              p_out1.src = INT_MIN; // sram src
              p_out1.dst = dst_id; // k-first placement of OBs and tiles within OBs
              p_out1.d_type = 0;
              
              // set AB_chain header               
              for(int s = 0; s < NUM_LEVELS; s++) {
                p_out1.AB[s] = ab_chains[m1][k1][m][k][s];
                // cout << "chain " << p_out1.AB[s];
              }
              // cout << "\n";

              wait(lat_internal);
              packet_out.Push(p_out1); 

              if(LOG) log_packet("SRAM", "SA", INT_MIN, p_out1);

              packet_counter++;
              dst_id++;
              weight_cnt++;
            }
          }
        }
      }


      for (int n1 = 0; n1 < (N_sr / N_ob); n1++) {

        for (int k1 = 0; k1 < (K_sr / K_ob); k1++) {

          if(DEBUG) cout << "Sending OB data component from SRAM to H-tree" << "\n";
          for (int n = 0; n < N_ob; n++){
            for (int k = 0; k < K_ob; k++) {
              
              p_out2 = activation_buf[(k1 * K_ob) + k][(n1 * N_ob) + n];
              p_out2.x = -1; // dummy value for x 
              p_out2.y = (n1 * N_ob) + n;
              p_out2.z = (k1 * K_ob) + k;
              p_out2.SB = k; 
              p_out2.SA = k + POD_SZ; 
              p_out2.SRAM = INT_MIN; // sram src
              p_out2.src = INT_MIN; // sram src
              p_out2.d_type = 1;

              // set bcast header 
              for(int a = 0; a < 2*NUM_SA - 1; a++) {
                p_out2.bcast[a] = bcast[(k1 * K_ob) + k][a];
              }

              wait(lat_internal);
              packet_out.Push(p_out2);
              if(LOG) log_packet("SRAM", "SA", INT_MIN, p_out2);

              packet_counter++;
              act_cnt++;
              // cout << "SRAM send packet to maestro\n"; 
            }
          }
        }
      }

      act_block_cnt++;
      weight_block_cnt++;

      buffer_inp.unlock();
      wait(ready_send_inp);
    }
  }




  void recv_result_maestro() {

    packet_in.Reset();
    sram_dram_out.Reset();
    wait(20.0, SC_NS);

    PacketSwitch::Packet p_in;
    vector<vector<PacketSwitch::Packet>> result_blk_sr(N_sr, vector<PacketSwitch::Packet>(M_sr)); 

    bool send_thread_start_res = 0;
    int p_cnt = 0;
    int K_cnt = 0;

    while(1) {

      if(packet_in.PopNB(p_in)) {

        if(p_in.d_type == 3) { // final result packets
          sram_dram_out.Push(p_in);
          if(LOG) log_packet("SRAM", "DRAM", INT_MIN, p_in);
        } else if(p_in.d_type == 2) {    
          result_blk_sr[p_in.Y % N_sr][p_in.X % M_sr] = p_in;
          result_cnt++;
          p_cnt++;
        }
      
        wait();
      } 

      if(p_cnt == (M_sr * N_sr)) {
  
        buffer_res.lock();
        ready_send_res.unlock();
        wait();

        result_buf = result_blk_sr;

        if(!send_thread_start_res) {
          send_thread_start_res = 1;
          send_init_res.notify(); // starts the sending thread only after first SRAM blk has been received
        }

        ready_send_res.lock();
        buffer_res.unlock();
        p_cnt = 0;        
      }

      wait();
    }
  }




  void send_result() {

    sram_partial_out.Reset();
    wait(20.0, SC_NS);
    wait(send_init_res);

    while(1) {

      ready_send_res.unlock();
      buffer_res.lock();

      for(int n = 0; n < N_sr; n++) {
        for(int m = 0; m < M_sr; m++) {
          result_buf[n][m].dst = result_buf[n][m].src;
          result_buf[n][m].src = INT_MIN;
          // result_buf[n][m].dst = result_buf[n][m].AB[0];
          wait(lat_internal);
          sram_partial_out.Push(result_buf[n][m]);
          if(LOG) log_packet("SRAM", "AB", INT_MIN, result_buf[n][m]);
        }
      }

      buffer_res.unlock();
      ready_send_res.lock();
      wait();
    }
  }





};


#endif



