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

template<typename T> vector<vector<T>> GetMat(int rows, int cols) {

  vector<vector<T>> mat(rows, vector<T>(cols)); 

  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      mat[i][j] = (nvhls::get_rand<8>()) % 4; // 8 bit numbers for weight and activation
    }
  }

  return mat;
}


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

  // switch connections
  Connections::Combinational<PacketSwitch::Packet> sb_in[NUM_PODS][POD_SZ];
  Connections::Combinational<PacketSwitch::Packet> sb_out[NUM_PODS][POD_SZ];
  Connections::Combinational<PacketSwitch::Packet> sa_in[NUM_PODS][POD_SZ];
  Connections::Combinational<PacketSwitch::Packet> sa_out[NUM_PODS][POD_SZ];
  Connections::Combinational<PacketSwitch::Packet> ab_in[NUM_PODS];
  Connections::Combinational<PacketSwitch::Packet> ab_out[NUM_PODS];
  Connections::Combinational<PacketSwitch::Packet> sram_in;
  Connections::Combinational<PacketSwitch::Packet> sram_out;

  sc_clock clk;
  sc_signal<bool> rst;

  SC_CTOR(testbench) :
    dram("dram"),
    sram("sram"),
    maestro("maestro"),
    clk("clk", 1, SC_NS, 0.5, 0, SC_NS, true),
    rst("rst")
  {

    // 0-3 is MB ind, 4-7 is SA ind, 8 is CB, 
    for (int j = 0; j < NUM_PODS; j++) {
      for (int i = 0; i < POD_SZ; i++) {
        // MB to switch ports
        maestro.p_switch->in_ports[j][i](sb_in[j][i]);
        maestro.p_switch->out_ports[j][i](sb_out[j][i]);
        maestro.sb[j][i]->packet_in(sb_out[j][i]);
        maestro.sb[j][i]->packet_out(sb_in[j][i]);
        maestro.sb[j][i]->clk(clk);
        maestro.sb[j][i]->rst(rst);

        // SA to switch ports
        maestro.p_switch->in_ports[j][i+POD_SZ](sa_in[j][i]);
        maestro.p_switch->out_ports[j][i+POD_SZ](sa_out[j][i]);
        maestro.sa[j][i]->packet_in(sa_out[j][i]);
        maestro.sa[j][i]->packet_out(sa_in[j][i]);
        maestro.sa[j][i]->clk(clk);
        maestro.sa[j][i]->rst(rst);
      }
      // CB to switch ports
      maestro.p_switch->in_ports[j][2*POD_SZ](ab_in[j]);
      maestro.ab[j]->packet_out(ab_in[j]);
      maestro.p_switch->out_ports[j][2*POD_SZ](ab_out[j]);
      maestro.ab[j]->packet_in(ab_out[j]);
      maestro.ab[j]->clk(clk);
      maestro.ab[j]->rst(rst);
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

    maestro.p_switch->maestro_in_port(sram_in);
    maestro.packet_out(sram_in);
    maestro.p_switch->maestro_out_port(sram_out);
    maestro.packet_in(sram_out);

    maestro.p_switch->clk(clk);
    maestro.p_switch->rst(rst);
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




// TODO : experiment with different wait() times in different places.
// TODO : print the final result in SRAM in a neat format
int sc_main(int argc, char *argv[]) {
  
  nvhls::set_random_seed();
  testbench my_testbench("my_testbench");

  for(int j = 0; j < NUM_PODS; j++) {
    for(int i = 0; i < POD_SZ; i++) {
      my_testbench.maestro.sb[j][i]->id = i;
      my_testbench.maestro.sa[j][i]->id = i + POD_SZ;
    }

    my_testbench.maestro.ab[j]->id = 2*POD_SZ;

    vector<vector<vector<PacketSwitch::Packet>>> acc_buf(N_sr / N_ob,
          vector<vector<PacketSwitch::Packet>>(M_sr / M_ob, 
              vector<PacketSwitch::Packet>(N_ob)));

    my_testbench.maestro.ab[j]->acc_buf = acc_buf;
  }

  // TODO :  M, N, and K are set in arch.h for now. Later, they need to be dims of the 
  // actual weight/data, which changes every DNN layer
  cout << "M = " << M*tile_sz << ", K = " << K*tile_sz << ", N = " << N*tile_sz << endl;
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
  cout << "TOTAL SIMULATION TIME = " << (end - start).to_default_time_units() << "\n";

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
    myfile << "1 ";
    cout << "\nMMM Result Correct!\n\n";
  } else {
    myfile << "0 ";
    cout << "\nMMM Result Incorrect! :( \n\n";
  }
  myfile.close();


  // PERFORMANCE COUNTERS
  // int tput = 0;

  // tput = (double) (((double) my_testbench.dram.packet_counter_send) / 
  //         ((double) my_testbench.dram.io_time_send));
  // cout << "\nDRAM send throughput = " << tput << "\n";

  // tput = (double) (((double) my_testbench.dram.packet_counter_recv) / 
  //         ((double) my_testbench.dram.io_time_recv));
  // cout << "\nDRAM receive throughput = " << tput << "\n";

  cout << "\nDRAM PERF\n";
  cout << "Packets sent = " << my_testbench.dram.packet_counter_send << "\n";
  cout << "IO Time send = " << my_testbench.dram.io_time_send << "\n";
  cout << "Packets received = " << my_testbench.dram.packet_counter_recv << "\n";
  cout << "IO Time recv = " << my_testbench.dram.io_time_recv << "\n\n";


  cout << "\nSRAM PERF\n";
  cout << "Packets sent = " << my_testbench.sram.packet_counter << "\n";
  cout << "IO Time = " << my_testbench.sram.io_time << "\n";
  cout << "Idle Time = " << my_testbench.sram.idle_time << "\n\n";


  int mult_cnt = 0;
  for(int j = 0; j < NUM_PODS; j++) {
    for(int i = 0; i < POD_SZ; i++) {
      cout << "SA idle time = " << my_testbench.maestro.sa[j][i]->idle_time << "\n";
      cout << "SA IO time = " << my_testbench.maestro.sa[j][i]->io_time << "\n";
      cout << "SA compute time = " << my_testbench.maestro.sa[j][i]->compute_time << "\n";
      cout << "SA Wait cnt = " << my_testbench.maestro.sa[j][i]->wait_cnt << "\n";
      mult_cnt += my_testbench.maestro.sa[j][i]->mult_cnt;
    }
  }

  cout << "\nSA PERF\n";
  cout << "Tiles Multiplied = " << mult_cnt << "\n";
  // cout << "IO Time = " << io_time << "\n";
  // cout << "Idle Time = " << idle_time << "\n";
  // cout << "Compute Time = " << compute_time << "\n\n";

  int packet_cnt = 0;
  for(int j = 0; j < NUM_PODS; j++) {
    // cout << "CB idle time = " << my_testbench.maestro.ab[j]->idle_time << "\n";
    // cout << "CB IO time = " << my_testbench.maestro.ab[j]->io_time << "\n";
    // cout << "CB compute time = " << my_testbench.maestro.ab[j]->compute_time << "\n";
    // cout << "CB Wait cnt = " << my_testbench.maestro.ab[j]->wait_cnt << "\n";
    packet_cnt += my_testbench.maestro.ab[j]->packet_counter;
  }

  cout << "\nCB PERF\n";
  cout << "Result Tiles Sent = " << packet_cnt << "\n";
  // cout << "IO Time = " << io_time << "\n";
  // cout << "Idle Time = " << idle_time << "\n";
  // cout << "Compute Time = " << compute_time << "\n\n";


  return 0;
};


