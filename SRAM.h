#ifndef __SRAM_H__
#define __SRAM_H__

#include "PacketSwitch.h"


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
      vector<vector<vector<int> > >(K_ob, 
        vector<vector<int> >(M_ob, 
          vector <int>(NUM_LEVELS, 0)))));  

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
    int dst_id = 0;

    vector<vector<vector<vector<vector<int> > > > > ab_chains = AB_chains(); // AB_chains for weight packets
    vector<vector<int> > bcast = bcast_dst(); // bcast headers for data packets
   
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

      // when a full SRAM block (both weight and act) have been stored in SRAM, sweep the SRAM block K first, 
      // i.e. send each weight/act SRAM_block pair in the snaking pattern, 
      // with act being sent first, then weight. 
      // TODO : need to implement weight reuse.   
      if((m_cnt == M_sr) && (n_cnt ==  N_sr)) {
        
        weight_buf = weight_blk_sr;
        activation_buf = activation_blk_sr;

        dst_id = 0;

        // Sweep through the SRAM block k-first to load SAs with weights
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
                p_out1.SRAM = INT_MAX; // sram src
                p_out1.src = INT_MAX; // sram src
                p_out1.dst = dst_id; // k-first placement of OBs and tiles within OBs
                p_out1.d_type = 0;
                
                // set AB_chain header               
                for(int s = 0; s < NUM_LEVELS; s++) {
                  p_out1.AB[s] = ab_chains[m1][k1][m][k][s];
                }

                packet_out.Push(p_out1); 
                packet_counter++;
                wait();
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
                p_out2.SRAM = INT_MAX; // sram src
                p_out2.src = INT_MAX; // sram src
                p_out2.d_type = 1;

                // set bcast header 
                for(int a = 0; a < 2*NUM_SA - 1; a++) {
                  p_out2.bcast[a] = bcast[(k1 * K_ob) + k][a];
                }

                packet_out.Push(p_out2);
                packet_counter++;
                wait();
                act_cnt++;
                // cout << "SRAM send packet to maestro\n"; 
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



