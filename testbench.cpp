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

#include "SRAM.h"
#include "pod.h"


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
      mat[i][j] = nvhls::get_rand<8>(); // 8 bit numbers for weight and activation
    }
  }

  return mat;
}


SC_MODULE (testbench) {

  SRAM sram;
  Pod pod;

  // SRAM connections
  Connections::Combinational<PacketSwitch::Packet> sram_pod_out;  
  // Connections::Combinational<PacketSwitch::Packet> sram_pod_in;  

  // switch connections
  Connections::Combinational<PacketSwitch::Packet> mb_in[NUM_PODS][POD_SZ];
  Connections::Combinational<PacketSwitch::Packet> mb_out[NUM_PODS][POD_SZ];
  Connections::Combinational<PacketSwitch::Packet> sa_in[NUM_PODS][POD_SZ];
  Connections::Combinational<PacketSwitch::Packet> sa_out[NUM_PODS][POD_SZ];
  // Connections::Combinational<PacketSwitch::Packet> cb_in[NUM_PODS];
  Connections::Combinational<PacketSwitch::Packet> cb_out[NUM_PODS];

  Connections::Combinational<PacketSwitch::Packet> sram_in;
  // Connections::Combinational<PacketSwitch::Packet> sram_out;


  sc_clock clk;
  sc_signal<bool> rst;

  SC_CTOR(testbench) :
    sram("sram"),
    pod("pod"),
    clk("clk", 1, SC_NS, 0.5, 0, SC_NS, true),
    rst("rst")
  {

    // 0-3 is MB ind, 4-7 is SA ind, 8 is CB, 
    for (int j = 0; j < NUM_PODS; j++) {
      for (int i = 0; i < POD_SZ; i++) {
        // MB to switch ports
        pod.p_switch->in_ports[j][i](mb_in[j][i]);
        pod.p_switch->out_ports[j][i](mb_out[j][i]);
        pod.mb[j][i]->packet_in(mb_out[j][i]);
        pod.mb[j][i]->packet_out(mb_in[j][i]);
        pod.mb[j][i]->clk(clk);
        pod.mb[j][i]->rst(rst);

        // SA to switch ports
        pod.p_switch->in_ports[j][i+POD_SZ](sa_in[j][i]);
        pod.p_switch->out_ports[j][i+POD_SZ](sa_out[j][i]);
        pod.sa[j][i]->packet_in(sa_out[j][i]);
        pod.sa[j][i]->packet_out(sa_in[j][i]);
        pod.sa[j][i]->clk(clk);
        pod.sa[j][i]->rst(rst);
      }

      pod.p_switch->out_ports[j][2*POD_SZ](cb_out[j]);
      pod.cb[j]->packet_in(cb_out[j]);
      pod.cb[j]->clk(clk);
      pod.cb[j]->rst(rst);

    }

    // sram to pod ports
    pod.pod_sram_in(sram_pod_out);
    sram.packet_out(sram_pod_out);  
    // pod.pod_sram_out(sram_pod_in);
    // sram.packet_in(sram_pod_in);  

    pod.p_switch->pod_in_port(sram_in);
    pod.packet_out(sram_in);
    // pod.p_switch->pod_out_port(sram_out);
    // pod.packet_in(sram_out);

    pod.p_switch->clk(clk);
    pod.p_switch->rst(rst);
    pod.clk(clk);
    pod.rst(rst);
    sram.clk(clk);
    sram.rst(rst);

    SC_THREAD(run);
  }

  void run() {
    //reset
    rst = 1;
    wait(10.5, SC_NS);
    rst = 0;
    wait(1, SC_NS);
    rst = 1;
    wait(10000,SC_NS);
    cout << "@" << sc_time_stamp() << " Stop " << endl ;
    sc_stop();
  }
};




int sc_main(int argc, char *argv[]) {
  
  nvhls::set_random_seed();
  testbench my_testbench("my_testbench");

  // vector<vector<vector<vector<PacketSwitch::AccumType>>>> cbmat(NUM_PODS,
  //                   vector<vector<vector<PacketSwitch::AccumType>>>(alpha * POD_SZ, 
  //                           vector<vector<PacketSwitch::AccumType>>(tile_sz, 
  //                                   vector<PacketSwitch::AccumType>(tile_sz, 0))));


  // vector<vector<PacketSwitch::AccumType>> tmp;

  for(int j = 0; j < NUM_PODS; j++) {
    for(int i = 0; i < POD_SZ; i++) {
      my_testbench.pod.mb[j][i]->id = i;
      my_testbench.pod.sa[j][i]->id = i + POD_SZ;
    }

    my_testbench.pod.cb[j]->id = 2*POD_SZ;
    vector<vector<vector<PacketSwitch::AccumType>>> cbmat(alpha * POD_SZ, 
                                vector<vector<PacketSwitch::AccumType>>(tile_sz, 
                                        vector<PacketSwitch::AccumType>(tile_sz, 0)));
    my_testbench.pod.cb[j]->cb_mat = cbmat;
    // for(int x = 0; x < alpha * POD_SZ; x++) {
    //   tmp = GetMat<PacketSwitch::AccumType>(tile_sz, tile_sz);
    //   for(int p = 0; p < tile_sz; p++) {
    //     for(int q = 0; q < tile_sz; q++) {
    //       tmp[p][q] = 0;
    //     }
    //   }

    //   my_testbench.pod.cb[j]->cb_mat.push_back(tmp);
    // }
  }


  int M = Wy;
  int K = Wz;
  int N = Dx;

  // Create weight and data matrices with random values
  my_testbench.sram.weights = GetMat<PacketSwitch::AccumType>(M, K);
  my_testbench.sram.activations = GetMat<PacketSwitch::AccumType>(K, N);

  vector<vector<PacketSwitch::AccumType>> ref_out(M, vector<PacketSwitch::AccumType>(K));
  ref_out = MatMul<PacketSwitch::AccumType, PacketSwitch::AccumType>(my_testbench.sram.weights, my_testbench.sram.activations);

  cout << "Weight matrix: \n";
  PrintMat(my_testbench.sram.weights);
  cout << "Activation matrix: \n";
  PrintMat(my_testbench.sram.activations);
  cout << "Reference Output: \n";
  PrintMat(ref_out);

  sc_start();
  cout << "CMODEL PASS" << endl;
  return 0;
};


