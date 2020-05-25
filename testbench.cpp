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

#include "SA.h"
#include "MB.h"
#include "CB.h"
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

template<typename T> void PrintMat(vector<vector<T>> mat) {
  int rows = (int) mat.size();
  int cols = (int) mat[0].size();
  for (int i = 0; i < rows; i++) {
    cout << "\t";
    for (int j = 0; j < cols; j++) {
      cout << mat[i][j] << "\t";
    }
    cout << endl;
  }
  cout << endl;
}



SC_MODULE (testbench) {

  SRAM sram;
  Pod pod;
  CB cb;


  Connections::Combinational<PacketSwitch::Packet> sram_pod_out;  
  Connections::Combinational<PacketSwitch::Packet> sram_pod_in;  

  // switch connections
  Connections::Combinational<PacketSwitch::Packet> mb_in[POD_SZ];
  Connections::Combinational<PacketSwitch::Packet> mb_out[POD_SZ];
  Connections::Combinational<PacketSwitch::Packet> sa_in[POD_SZ];
  Connections::Combinational<PacketSwitch::Packet> sa_out[POD_SZ];
  Connections::Combinational<PacketSwitch::Packet> sram_in;
  Connections::Combinational<PacketSwitch::Packet> sram_out;
  Connections::Combinational<PacketSwitch::Packet> cb_in;
  Connections::Combinational<PacketSwitch::Packet> cb_out;


  sc_clock clk;
  sc_signal<bool> rst;

  SC_CTOR(testbench) :
    sram("sram"),
    pod("pod"),
    cb("cb"),
    clk("clk", 1, SC_NS, 0.5, 0, SC_NS, true),
    rst("rst")
  {
    // 0-3 is MB ind, 4-7 is SA ind, 8 is SRAM, 9 is CB

    for (int i = 0; i < POD_SZ; i++) {
      // MB to switch ports
      pod.p_switch->in_ports[i](mb_in[i]);
      pod.p_switch->out_ports[i](mb_out[i]);
      pod.mb[i]->packet_in(mb_out[i]);
      pod.mb[i]->packet_out(mb_in[i]);
      pod.mb[i]->clk(clk);
      pod.mb[i]->rst(rst);

      // SA to switch ports
      pod.p_switch->in_ports[i+POD_SZ](sa_in[i]);
      pod.p_switch->out_ports[i+POD_SZ](sa_out[i]);
      pod.sa[i]->packet_in(sa_out[i]);
      pod.sa[i]->packet_out(sa_in[i]);
      pod.sa[i]->clk(clk);
      pod.sa[i]->rst(rst);
    }

    // cb to switch ports
    pod.p_switch->in_ports[2*POD_SZ+1](cb_in);
    pod.p_switch->out_ports[2*POD_SZ+1](cb_out);
    cb.packet_in(cb_out);
    cb.packet_out(cb_in);

    // pod to switch ports
    pod.p_switch->in_ports[2*POD_SZ](sram_in);
    pod.packet_out(sram_in);
    pod.p_switch->out_ports[2*POD_SZ](sram_out);
    pod.packet_in(sram_out);

    // sram to pod ports
    sram.packet_out(sram_pod_out);  
    pod.pod_sram_in(sram_pod_out);
    sram.packet_in(sram_pod_in);  
    pod.pod_sram_out(sram_pod_in);

    pod.clk(clk);
    pod.rst(rst);
    pod.p_switch->clk(clk);
    pod.p_switch->rst(rst);
    sram.clk(clk);
    sram.rst(rst);
    cb.clk(clk);
    cb.rst(rst);

    SC_THREAD(run);
  }

  void run() {
    //reset
    rst = 1;
    wait(10.5, SC_NS);
    rst = 0;
    wait(1, SC_NS);
    rst = 1;
    wait(100,SC_NS);
    cout << "@" << sc_time_stamp() << " Stop " << endl ;
    sc_stop();
  }
};




int sc_main(int argc, char *argv[]) {
  
  nvhls::set_random_seed();
  // Weight N*N 
  // Input N*M
  // Output N*M
  vector<vector<vector<PacketSwitch::AccumType>>> weight_mat;
  vector<vector<vector<PacketSwitch::AccumType>>> act_mat;
  vector<vector<vector<PacketSwitch::AccumType>>>  output_mat;

  for(int i = 0; i < POD_SZ; i++) {
    weight_mat.push_back(GetMat<PacketSwitch::AccumType>(N, N));
    act_mat.push_back(GetMat<PacketSwitch::AccumType>(N, N));
    output_mat.push_back(MatMul<PacketSwitch::AccumType, PacketSwitch::AccumType>(weight_mat[i], act_mat[i])); 
  }

  // initialize final_out and cb_mat to 0 
  vector<vector<PacketSwitch::AccumType>> final_out(N, vector<PacketSwitch::AccumType>(N,0));
  vector<vector<PacketSwitch::AccumType>> cb_mat(N, vector<PacketSwitch::AccumType>(N,0)); 

  for(int i = 0; i < N; i++) {
    for(int j = 0; j < N; j++) {
      for(int x = 0; x < POD_SZ; x++) {
        final_out[i][j] += output_mat[x][i][j];
      }
    }
  }

  cout << "Reference Output Matrix " << endl; 
  PrintMat(final_out);
  printf("\n\n\n");

  testbench my_testbench("my_testbench");
  for(int i = 0; i < POD_SZ; i++) {
    my_testbench.sram.weight_mat.push_back(weight_mat[i]);
    my_testbench.sram.act_mat.push_back(act_mat[i]);
  }

  my_testbench.cb.cb_mat = cb_mat;

  for(int i = 0; i < POD_SZ; i++) {
    my_testbench.pod.mb[i]->id = i;
    my_testbench.pod.sa[i]->id = i + POD_SZ;
  }


  my_testbench.sram.id = 2*POD_SZ;
  my_testbench.cb.id = 2*POD_SZ + 1;

  sc_start();
  cout << "CMODEL PASS" << endl;
  return 0;
};

