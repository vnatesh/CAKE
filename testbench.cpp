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
  Connections::Combinational<PacketSwitch::Packet> mb_in[NUM_PODS][POD_SZ];
  Connections::Combinational<PacketSwitch::Packet> mb_out[NUM_PODS][POD_SZ];
  Connections::Combinational<PacketSwitch::Packet> sa_in[NUM_PODS][POD_SZ];
  Connections::Combinational<PacketSwitch::Packet> sa_out[NUM_PODS][POD_SZ];
  Connections::Combinational<PacketSwitch::Packet> cb_in[NUM_PODS];
  Connections::Combinational<PacketSwitch::Packet> cb_out[NUM_PODS];
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
        maestro.p_switch->in_ports[j][i](mb_in[j][i]);
        maestro.p_switch->out_ports[j][i](mb_out[j][i]);
        maestro.mb[j][i]->packet_in(mb_out[j][i]);
        maestro.mb[j][i]->packet_out(mb_in[j][i]);
        maestro.mb[j][i]->clk(clk);
        maestro.mb[j][i]->rst(rst);

        // SA to switch ports
        maestro.p_switch->in_ports[j][i+POD_SZ](sa_in[j][i]);
        maestro.p_switch->out_ports[j][i+POD_SZ](sa_out[j][i]);
        maestro.sa[j][i]->packet_in(sa_out[j][i]);
        maestro.sa[j][i]->packet_out(sa_in[j][i]);
        maestro.sa[j][i]->clk(clk);
        maestro.sa[j][i]->rst(rst);
      }
      // CB to switch ports
      maestro.p_switch->in_ports[j][2*POD_SZ](cb_in[j]);
      maestro.cb[j]->packet_out(cb_in[j]);
      maestro.p_switch->out_ports[j][2*POD_SZ](cb_out[j]);
      maestro.cb[j]->packet_in(cb_out[j]);
      maestro.cb[j]->clk(clk);
      maestro.cb[j]->rst(rst);
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
    sc_stop();
  }
};




// TODO : experiment with different wait() times in different places.
// TODO : print the final result in SRAM in a neat format
int sc_main(int argc, char *argv[]) {
  
  nvhls::set_random_seed();
  testbench my_testbench("my_testbench");

  // vector<vector<vector<vector<PacketSwitch::AccumType>>>> cbmat(NUM_PODS,
  //                   vector<vector<vector<PacketSwitch::AccumType>>>(alpha * POD_SZ, 
  //                           vector<vector<PacketSwitch::AccumType>>(tile_sz, 
  //                                   vector<PacketSwitch::AccumType>(tile_sz, 0))));

  for(int j = 0; j < NUM_PODS; j++) {
    for(int i = 0; i < POD_SZ; i++) {
      my_testbench.maestro.mb[j][i]->id = i;
      my_testbench.maestro.sa[j][i]->id = i + POD_SZ;
    }

    my_testbench.maestro.cb[j]->id = 2*POD_SZ;
    vector<vector<vector<PacketSwitch::AccumType>>> cbmat(alpha * POD_SZ, 
                                vector<vector<PacketSwitch::AccumType>>(tile_sz, 
                                        vector<PacketSwitch::AccumType>(tile_sz, 0)));
    my_testbench.maestro.cb[j]->cb_mat = cbmat;
  }

  // TODO :  M, N, and K are set here for now. Later, they need to be dims of the 
  // actually weight/data, which changes every DNN layer
  int M = Wy*3;
  int K = Wz*3;
  int N = Dx*3;

  // Create weight and data matrices with random values
  my_testbench.dram.weights = GetMat<PacketSwitch::AccumType>(M, K);
  my_testbench.dram.activations = GetMat<PacketSwitch::AccumType>(K, N);
  vector<vector<PacketSwitch::AccumType>> result = vector<vector<PacketSwitch::AccumType>>(M, 
                                                                vector<PacketSwitch::AccumType>(N, 0));
  
  my_testbench.dram.result = result;
  vector<vector<PacketSwitch::AccumType>> ref_out(M, vector<PacketSwitch::AccumType>(N));
  ref_out = MatMul<PacketSwitch::AccumType, PacketSwitch::AccumType>(my_testbench.dram.weights, my_testbench.dram.activations);

  cout << "Weight matrix: \n";
  PrintMat(my_testbench.dram.weights);
  cout << "Activation matrix: \n";
  PrintMat(my_testbench.dram.activations);
  cout << "Reference Output: \n";
  PrintMat(ref_out);
  sc_start();

  for(int i = 0; i < M; i++) {
    for(int j = 0; j < N; j++) {
      if(my_testbench.dram.result[i][j] != ref_out[i][j]) {
        cout << "MMM Result Incorrect! :(" << "\n";
      }
    }
  }

  cout << "MMM Result Correct!" << "\n";
  return 0;
};


