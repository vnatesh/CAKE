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

#include "PacketSwitch.h"
#include "SA.h"

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
      mat[i][j] = nvhls::get_rand<T::width>();
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



SC_MODULE (MB) {

  Connections::Out<PacketSwitch::Packet> packet_out;
  sc_in <bool> clk;
  sc_in <bool> rst;

  vector<vector<PacketSwitch::AccumType>> weight_mat, act_mat, output_mat;

  SC_CTOR(MB) {
    SC_THREAD(run);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  }


  void run() {

    PacketSwitch::Packet   p_out1;
    PacketSwitch::Packet   p_out2;

    packet_out.Reset();
    // Wait for initial reset.
    wait(20.0, SC_NS);
  
    printf("Sending weights from MB to SA \n");
    for (int i = 0; i < N; i++) {
      for (int j = 0; j < N; j++) {
        p_out1.data[i][j] = weight_mat[i][j];
      }
    }

    p_out1.src = 0;
    p_out1.dst = 1;
    p_out1.d_type = 0;
    
    wait(20); 

    printf("Sending activations from MB to SA \n");
    for (int i = 0; i < N; i++) {
      for (int j = 0; j < N; j++) {
        p_out2.data[i][j] = act_mat[i][j];
      }
    }
    p_out2.src = 0;
    p_out2.dst = 1;
    p_out2.d_type = 1;

    packet_out.Push(p_out1);            
    packet_out.Push(p_out2);
  }
};




SC_MODULE (Sram) {

  Connections::In<PacketSwitch::Packet>  result;
  sc_in <bool> clk;
  sc_in <bool> rst;


  SC_CTOR(Sram) {
    SC_THREAD(run);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  }

  void run() {

    result.Reset();
    PacketSwitch::Packet p_in;


    while(1) {
      if(result.PopNB(p_in)) {
        printf("receiving result at CB \n");
        for (int i = 0; i < N; i++) {
          cout << "\t";
           for (int j = 0; j < N; j++) {
            cout << p_in.data[i][j] << "\t";
          }
          cout << endl;
        }
        cout << endl;
      }
      wait(); 
    }
  }
};




// testbench

SC_MODULE (testbench) {

  NVHLS_DESIGN(PacketSwitch) p_switch;
  MB mb;
  Sram sram;
  SA sa;

  Connections::Combinational<PacketSwitch::Packet> mb_in;
  Connections::Combinational<PacketSwitch::Packet> sa_in;

  Connections::Combinational<PacketSwitch::Packet> sram_out;
  Connections::Combinational<PacketSwitch::Packet> sa_out;

  sc_clock clk;
  sc_signal<bool> rst;


  SC_CTOR(testbench) :
    p_switch("p_switch"),
    mb("mb"),
    sram("sram"),
    sa("sa"),
    // mb_in("mb_in"),
    // sa_in("sa_in"),
    // sram_out("sram_out"),
    // sa_out("sa_out"),
    clk("clk", 1, SC_NS, 0.5,0,SC_NS,true),
    rst("rst")

  {
    p_switch.clk(clk);
    p_switch.rst(rst);
    mb.clk(clk);
    mb.rst(rst);
    sram.clk(clk);
    sram.rst(rst);
    sa.clk(clk);
    sa.rst(rst);

    p_switch.in_ports[0](mb_in);
    mb.packet_out(mb_in);

    p_switch.out_ports[0](sram_out);
    sram.result(sram_out);

    p_switch.in_ports[1](sa_in);
    sa.packet_out(sa_in);

    p_switch.out_ports[1](sa_out);
    sa.packet_in(sa_out);

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



int sc_main(int argc, char *argv[])
{
  nvhls::set_random_seed();

  // Weight N*N 
  // Input N*M
  // Output N*M
  
  vector<vector<PacketSwitch::AccumType>> weight_mat = GetMat<PacketSwitch::AccumType>(N, N); 
  vector<vector<PacketSwitch::AccumType>> act_mat = GetMat<PacketSwitch::AccumType>(N, N);  
  vector<vector<PacketSwitch::AccumType>> output_mat;
  output_mat = MatMul<PacketSwitch::AccumType, PacketSwitch::AccumType>(weight_mat, act_mat); 
  
  cout << "Weight Matrix " << endl; 
  PrintMat(weight_mat);
  cout << "Activation Matrix " << endl; 
  PrintMat(act_mat);
  cout << "Reference Output Matrix " << endl; 
  PrintMat(output_mat);
  printf("\n\n\n");

  testbench my_testbench("my_testbench");
  
  my_testbench.mb.weight_mat = weight_mat;
  my_testbench.mb.act_mat = act_mat;
  my_testbench.mb.output_mat = output_mat;

  sc_start();
  cout << "CMODEL PASS" << endl;
  return 0;
};





