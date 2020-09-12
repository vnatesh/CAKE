/*
 * Copyright (c) 2016-2019, NVIDIA CORPORATION.  All rights reserved.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License")
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "DRAM.h"
#include "SRAM.h"
#include "maestro.h"

#include <systemc.h>
#include <mc_scverify.h>
#include <nvhls_int.h>

#include <vector>

#define NVHLS_VERIFY_BLOCKS (PacketSwitch)
#include <nvhls_verify.h>
using namespace::std;

#include <testbench/nvhls_rand.h>

// module load ~/cs148/catapult

#include "matops.h"

SC_MODULE (testbench) {

  DRAM dram;
  SRAM sram;
  Maestro maestro;

  // DRAM-SRAM connections
  Connections::Combinational<PacketSwitch::Packet> dram_sram_out;  
  Connections::Combinational<PacketSwitch::Packet> dram_sram_in;  

  // SRAM-maestro connections
  Connections::Combinational<PacketSwitch::Packet> sram_maestro_out;  
  Connections::Combinational<PacketSwitch::Packet> sram_maestro_in;  
  Connections::Combinational<PacketSwitch::Packet> sram_maestro_partial_out;  

  // switch connections
  Connections::Combinational<PacketSwitch::Packet> left_in[Sx*Sy-1];
  Connections::Combinational<PacketSwitch::Packet> left_out[Sx*Sy-1];
  Connections::Combinational<PacketSwitch::Packet> right_in[Sx*Sy-1];
  Connections::Combinational<PacketSwitch::Packet> right_out[Sx*Sy-1];
  Connections::Combinational<PacketSwitch::Packet> parent_in[Sx*Sy-1];
  Connections::Combinational<PacketSwitch::Packet> parent_out[Sx*Sy-1];

  // SB, SA, AB, and SRAM connections
  Connections::Combinational<PacketSwitch::Packet> sb_in[Sx*Sy];
  Connections::Combinational<PacketSwitch::Packet> sb_out[Sx*Sy];
  Connections::Combinational<PacketSwitch::Packet> sa_in[Sx*Sy];
  Connections::Combinational<PacketSwitch::Packet> sa_out[Sx*Sy];
  Connections::Combinational<PacketSwitch::Packet> ab_in[Sx*Sy-1];
  Connections::Combinational<PacketSwitch::Packet> ab_out[Sx*Sy-1];  
  Connections::Combinational<PacketSwitch::Packet> sram_in;
  Connections::Combinational<PacketSwitch::Packet> sram_out;

  Connections::Combinational<PacketSwitch::Packet> ab_partial_out[Sx*Sy-1];

  Connections::Combinational<PacketSwitch::Packet> left_partial_out[Sx*Sy-1];
  Connections::Combinational<PacketSwitch::Packet> right_partial_out[Sx*Sy-1];
  Connections::Combinational<PacketSwitch::Packet> sram_partial_in;


  sc_clock clk;
  sc_signal<bool> rst;

  SC_CTOR(testbench) :
    dram("dram"),
    sram("sram"),
    maestro("maestro"),
    clk("clk", 1, SC_NS, 0.5, 0, SC_NS, true),
    rst("rst")
  {
    maestro.p_switch[0]->parent_in(sram_in);
    maestro.packet_out(sram_in);
    maestro.p_switch[0]->parent_out(sram_out);
    maestro.packet_in(sram_out);


    maestro.p_switch[0]->parent_partial_in(sram_partial_in);
    maestro.partial_out(sram_partial_in);


    // connect the interior switches together as a binary tree
    for(int i = 0; i < ((Sx*Sy)/2) - 1; i++) {
      maestro.p_switch[i]->left_in(left_in[i]); 
      maestro.p_switch[2*i+1]->parent_out(left_in[i]); 
      maestro.p_switch[i]->left_out(left_out[i]); 
      maestro.p_switch[2*i+1]->parent_in(left_out[i]); 

      maestro.p_switch[i]->left_partial_out(left_partial_out[i]); 
      maestro.p_switch[2*i+1]->parent_partial_in(left_partial_out[i]); 

      maestro.p_switch[i]->right_in(right_in[i]); 
      maestro.p_switch[2*i+2]->parent_out(right_in[i]); 
      maestro.p_switch[i]->right_out(right_out[i]); 
      maestro.p_switch[2*i+2]->parent_in(right_out[i]); 

      maestro.p_switch[i]->right_partial_out(right_partial_out[i]); 
      maestro.p_switch[2*i+2]->parent_partial_in(right_partial_out[i]); 

    }

    // connect the last level of interior switches to the leaf switches
    int j = 0;
    for(int i = ((Sx*Sy)/2) - 1; i < Sx*Sy-1; i++) {
      maestro.p_switch[i]->left_in(left_in[i]); 
      maestro.leaf_switch[j]->parent_out(left_in[i]);
      maestro.p_switch[i]->left_out(left_out[i]); 
      maestro.leaf_switch[j]->parent_in(left_out[i]);

      maestro.p_switch[i]->right_in(right_in[i]); 
      maestro.leaf_switch[j+1]->parent_out(right_in[i]);
      maestro.p_switch[i]->right_out(right_out[i]); 
      maestro.leaf_switch[j+1]->parent_in(right_out[i]);

      maestro.p_switch[i]->left_partial_out(left_partial_out[i]); 
      maestro.leaf_switch[j]->partial_in(left_partial_out[i]);
      maestro.p_switch[i]->right_partial_out(right_partial_out[i]); 
      maestro.leaf_switch[j+1]->partial_in(right_partial_out[i]);


      j += 2;
    }


    // connect an AB to eat switch 
    for(int i = 0; i < Sx*Sy - 1; i++) {

      maestro.p_switch[i]->ab_in(ab_in[i]);
      maestro.ab[i]->packet_out(ab_in[i]);
      maestro.p_switch[i]->ab_out(ab_out[i]);
      maestro.ab[i]->packet_in(ab_out[i]);
      
      maestro.p_switch[i]->ab_partial_out(ab_partial_out[i]);
      maestro.ab[i]->partial_in(ab_partial_out[i]);

      maestro.ab[i]->clk(clk);
      maestro.ab[i]->rst(rst);
      maestro.p_switch[i]->clk(clk);
      maestro.p_switch[i]->rst(rst);
    }

    // connect each SA and SB pair to a leaf switch
    for(int i = 0; i < Sx*Sy; i++) {
      maestro.leaf_switch[i]->sb_in(sb_in[i]);
      maestro.sb[i]->packet_out(sb_in[i]); 
      maestro.leaf_switch[i]->sb_out(sb_out[i]);
      maestro.sb[i]->packet_in(sb_out[i]); 

      maestro.leaf_switch[i]->sa_in(sa_in[i]);
      maestro.sa[i]->packet_out(sa_in[i]); 
      maestro.leaf_switch[i]->sa_out(sa_out[i]);
      maestro.sa[i]->packet_in(sa_out[i]); 

      maestro.sa[i]->clk(clk); 
      maestro.sa[i]->rst(rst); 
      maestro.sb[i]->clk(clk); 
      maestro.sb[i]->rst(rst); 
      maestro.leaf_switch[i]->clk(clk);
      maestro.leaf_switch[i]->rst(rst);
    }

    // dram to sram ports
    dram.packet_out(dram_sram_out);
    sram.sram_dram_in(dram_sram_out);
    dram.packet_in(dram_sram_in);
    sram.sram_dram_out(dram_sram_in);

    // sram to maestro ports
    maestro.maestro_sram_in(sram_maestro_out);
    sram.packet_out(sram_maestro_out);  
    maestro.maestro_sram_out(sram_maestro_in);
    sram.packet_in(sram_maestro_in);  

    maestro.maestro_partial_in(sram_maestro_partial_out);
    sram.sram_partial_out(sram_maestro_partial_out);  


    maestro.clk(clk);
    maestro.rst(rst);
    sram.clk(clk);
    sram.rst(rst);
    dram.clk(clk);
    dram.rst(rst);

    SC_THREAD(run);
  }

  void run() {

    //reset
    rst = 1;
    wait(10.5, SC_NS);
    rst = 0;
    wait(1, SC_NS);
    rst = 1;
    wait(1000000,SC_NS);
    cout << "@" << sc_time_stamp() << " Stop " << "\n" ;
  }
};



int sc_main(int argc, char *argv[]) {
  
  remove("log/logfile.txt"); 
  logfile.open("log/logfile.txt", ios::app);
  nvhls::set_random_seed();
  testbench my_testbench("my_testbench");

  int lev;
  for(int i = 0; i < NUM_SA - 1; i++) {
    lev = (int) floor(log(i+1) / log(2));
    my_testbench.maestro.p_switch[i]->id = i;
    my_testbench.maestro.p_switch[i]->level = lev;
    my_testbench.maestro.ab[i]->id = i;
    my_testbench.maestro.ab[i]->level = lev;
    // ABs in each level have acc_buf with M_ob*N_sr tiles
    vector<vector<PacketSwitch::Packet>> acc_buf(M_ob, vector<PacketSwitch::Packet>(N_sr)); 
    my_testbench.maestro.ab[i]->acc_buf = acc_buf;
    vector<PacketSwitch::AddrType> chain_buf(NUM_LEVELS+1, 0);
    my_testbench.maestro.ab[i]->chain_buf = chain_buf;
  }

  for (int i = 0; i < NUM_SA; i++) {
    my_testbench.maestro.leaf_switch[i]->id = i;
  }

  vector<vector<PacketSwitch::Packet>> weight_buf(M_sr, vector<PacketSwitch::Packet>(K_sr)); 
  my_testbench.sram.weight_buf = weight_buf;

  vector<vector<PacketSwitch::Packet>> activation_buf(K_sr, vector<PacketSwitch::Packet>(N_sr)); 
  my_testbench.sram.activation_buf = activation_buf;

  vector<vector<PacketSwitch::Packet>> result_buf(M_sr, vector<PacketSwitch::Packet>(N_sr)); 
  my_testbench.sram.result_buf = result_buf;



  cout << "M = " << M*tile_sz << ", K = " << K*tile_sz << ", N = " << N*tile_sz << ", DRAM bw = " << R << endl;
  // Create weight and activation matrices with random values
  my_testbench.dram.weights = GetMat<PacketSwitch::AccumType>(M*tile_sz, K*tile_sz);
  my_testbench.dram.activations = GetMat<PacketSwitch::AccumType>(K*tile_sz, N*tile_sz);
  vector<vector<PacketSwitch::AccumType>> result = vector<vector<PacketSwitch::AccumType>>(M*tile_sz, 
                                                                vector<PacketSwitch::AccumType>(N*tile_sz, 0));
  my_testbench.dram.result = result;
  vector<vector<PacketSwitch::AccumType>> ref_out(M*tile_sz, vector<PacketSwitch::AccumType>(N*tile_sz));
  ref_out = MatMul<PacketSwitch::AccumType, PacketSwitch::AccumType>(my_testbench.dram.weights, my_testbench.dram.activations);


  // if(DEBUG) {
    cout << "Weight matrix: \n";
    PrintMat(my_testbench.dram.weights);
    cout << "Activation matrix: \n";
    PrintMat(my_testbench.dram.activations);
    cout << "Reference Output: \n";
    PrintMat(ref_out);
  // }

  sc_time start, end;
  start = sc_time_stamp();
  sc_start();

  end = sc_time_stamp();
  int total_time = (end - start).to_default_time_units();
  cout << "TOTAL SIMULATION TIME = " << total_time << "\n";


  logfile.close();

  int  SRAM_sz = M_sr*K_sr + K_sr*N_sr;
  int num_sr_blk = (M/M_sr)*(K/K_sr)*(N/N_sr);
  int num_packets = (SRAM_sz * num_sr_blk);
  double SRAM_bw = (double) (((double) num_packets) / ((double) total_time));
  cout << "packets sent = " << num_packets << "\n";
  int comp_time = my_testbench.maestro.sa[0]->compute_time;
  cout << "SA compute cycles = " << comp_time << "\n";


  int IO_d_ideal = NUM_PODS * alpha * POD_SZ;
  int IO_w = NUM_PODS * POD_SZ;
  int SRAM_sz_ideal = IO_d_ideal + IO_w;
  double SRAM_growth = ((double) SRAM_sz) / ((double) SRAM_sz_ideal);

  bool CORRECT = 1;
  for(int i = 0; i < M*tile_sz; i++) {
    for(int j = 0; j < N*tile_sz; j++) {
      if(my_testbench.dram.result[i][j] != ref_out[i][j]) {
        CORRECT = 0;
      }
    }
  }

  ofstream myfile;
  myfile.open ("results.txt", ios::app);
  if(CORRECT) {
    if(M_sr >= K_sr) {
      myfile << NUM_SA << "," << num_packets << "," << 
      total_time << "," << SRAM_bw << "," << comp_time << "," 
      << SRAM_growth << "," << R << ",M"<< "\n";
    } // else if(M_sr < K_sr) {
    //   myfile << NUM_SA << "," << num_packets << "," << total_time << "," << SRAM_bw << "," << comp_time << ",K"<< "\n";
    // } else if(M_sr == K_sr) {
    //   myfile << NUM_SA << "," << num_packets << "," << total_time << "," << SRAM_bw << "," << comp_time << ",S"<< "\n";
    // }
  } 
  myfile.close();


  // ofstream myfile;
  // myfile.open ("results.txt", ios::app);
  if(CORRECT) {
    // myfile << "1 ";
    cout << "\nMMM Result Correct!\n\n";
  } else {
    // myfile << "0 ";
    cout << "\nMMM Result Incorrect! :( \n\n";
  }
  // myfile.close();


  cout << "\nDRAM PERF\n";
  cout << "Packets sent = " << my_testbench.dram.packet_counter_send << "\n";
  cout << "IO Time send = " << my_testbench.dram.io_time_send << "\n";
  cout << "Packets received = " << my_testbench.dram.packet_counter_recv << "\n";
  cout << "IO Time recv = " << my_testbench.dram.io_time_recv << "\n\n";


  // cout << "\nSRAM PERF\n";
  // cout << "Packets sent = " << my_testbench.sram.packet_counter << "\n";
  // cout << "IO Time = " << my_testbench.sram.io_time << "\n";
  // cout << "Idle Time = " << my_testbench.sram.idle_time << "\n\n";



  return 0;
};