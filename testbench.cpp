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
#include "MB.h"

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




SC_MODULE (SRAM) {

  Connections::In<PacketSwitch::Packet> packet_in;
  Connections::Out<PacketSwitch::Packet> packet_out;
  sc_in <bool> clk;
  sc_in <bool> rst;

  vector<vector<vector<PacketSwitch::AccumType>>> weight_mat;
  vector<vector<vector<PacketSwitch::AccumType>>> act_mat;
  // vector<vector<PacketSwitch::AccumType>> output_mat[POD_SZ];


  PacketSwitch::ID_type id;

  SC_CTOR(SRAM) {
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

    for(int x = 0; x < POD_SZ; x++) {

      printf("Sending weights from SRAM to MB \n");
      for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
          p_out1.data[i][j] = weight_mat[x][i][j];
        }
      }

      wait(5);

      printf("Sending activations from SRAM to MB \n");
      for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
          p_out2.data[i][j] = act_mat[x][i][j];
        }
      }

      p_out1.src = 8;
      p_out1.dst = x;
      p_out1.d_type = 0;

      p_out2.src = 8;
      p_out2.dst = x;
      p_out2.d_type = 1;

      packet_out.Push(p_out1);            
      packet_out.Push(p_out2);
      wait(10);
    }
  }
};




SC_MODULE (CB) {

  Connections::In<PacketSwitch::Packet>  packet_in;
  Connections::Out<PacketSwitch::Packet>  packet_out;
  sc_in <bool> clk;
  sc_in <bool> rst;

  vector<vector<PacketSwitch::AccumType>> cb_mat;  
  PacketSwitch::ID_type id;

  SC_CTOR(CB) {
    SC_THREAD(run);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  }

  void run() {

    packet_in.Reset();
    PacketSwitch::Packet p_in;

    int cnt = 0;
    while(1) {
      if(packet_in.PopNB(p_in)) {
        printf("Partial result received at CB\n");
        cnt++;
        for (int i = 0; i < N; i++) {
           for (int j = 0; j < N; j++) {
            cb_mat[i][j] += p_in.data[i][j];
          }
        }
      }

      if(cnt == POD_SZ) {
        printf("FINAL RESULT AT CB \n");
        for (int i = 0; i < N; i++) {
          cout << "\t";
           for (int j = 0; j < N; j++) {
            cout << cb_mat[i][j] << "\t";
          }
          cout << endl;
        }
        cout << endl;
        cnt = 0;
      }

      wait(); 
    }
  }
};










SC_MODULE (testbench) {

  NVHLS_DESIGN(PacketSwitch) p_switch;
  SRAM sram;
  MB mb0;
  MB mb1;
  MB mb2;
  MB mb3;

  SA sa0;
  SA sa1;
  SA sa2;
  SA sa3;

  CB cb;

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
    p_switch("p_switch"),
    sram("sram"),
    mb0(sc_gen_unique_name("mb")),
    mb1(sc_gen_unique_name("mb")),
    mb2(sc_gen_unique_name("mb")),
    mb3(sc_gen_unique_name("mb")),
    sa0(sc_gen_unique_name("sa")),
    sa1(sc_gen_unique_name("sa")),
    sa2(sc_gen_unique_name("sa")),
    sa3(sc_gen_unique_name("sa")),
    cb(sc_gen_unique_name("cb")),
    clk("clk", 1, SC_NS, 0.5,0,SC_NS,true),
    rst("rst")
  {
    // 0-3 is MB ind, 4-7 is SA ind, 8 is SRAM, 9 is CB

    p_switch.in_ports[0](mb_in[0]);
    p_switch.out_ports[0](mb_out[0]);
    mb0.packet_in(mb_out[0]);
    mb0.packet_out(mb_in[0]);
    mb0.clk(clk);
    mb0.rst(rst);

    p_switch.in_ports[0+POD_SZ](sa_in[0]);
    p_switch.out_ports[0+POD_SZ](sa_out[0]);
    sa0.packet_in(sa_out[0]);
    sa0.packet_out(sa_in[0]);
    sa0.clk(clk);
    sa0.rst(rst);





    p_switch.in_ports[1](mb_in[1]);
    p_switch.out_ports[1](mb_out[1]);
    mb1.packet_in(mb_out[1]);
    mb1.packet_out(mb_in[1]);
    mb1.clk(clk);
    mb1.rst(rst);

    p_switch.in_ports[1+POD_SZ](sa_in[1]);
    p_switch.out_ports[1+POD_SZ](sa_out[1]);
    sa1.packet_in(sa_out[1]);
    sa1.packet_out(sa_in[1]);
    sa1.clk(clk);
    sa1.rst(rst);





    p_switch.in_ports[2](mb_in[2]);
    p_switch.out_ports[2](mb_out[2]);
    mb2.packet_in(mb_out[2]);
    mb2.packet_out(mb_in[2]);
    mb2.clk(clk);
    mb2.rst(rst);

    p_switch.in_ports[2+POD_SZ](sa_in[2]);
    p_switch.out_ports[2+POD_SZ](sa_out[2]);
    sa2.packet_in(sa_out[2]);
    sa2.packet_out(sa_in[2]);
    sa2.clk(clk);
    sa2.rst(rst);





    p_switch.in_ports[3](mb_in[3]);
    p_switch.out_ports[3](mb_out[3]);
    mb3.packet_in(mb_out[3]);
    mb3.packet_out(mb_in[3]);
    mb3.clk(clk);
    mb3.rst(rst);

    p_switch.in_ports[3+POD_SZ](sa_in[3]);
    p_switch.out_ports[3+POD_SZ](sa_out[3]);
    sa3.packet_in(sa_out[3]);
    sa3.packet_out(sa_in[3]);
    sa3.clk(clk);
    sa3.rst(rst);

    // for(int i = 0; i < POD_SZ; i++) {
    //   p_switch.in_ports[i](mb_in[i]);
    //   p_switch.out_ports[i](mb_out[i]);
    //   mb[i].packet_in(mb_out[i]);
    //   mb[i].packet_out(mb_in[i]);
    //   mb[i].clk(clk);
    //   mb[i].rst(rst);


    //   p_switch.in_ports[i](sa_in[i]);
    //   p_switch.out_ports[i](sa_out[i]);
    //   sa[i].packet_in(sa_out[i]);
    //   sa[i].packet_out(sa_in[i]);
    //   sa[i].clk(clk);
    //   sa[i].rst(rst);
    // }

    p_switch.in_ports[2*POD_SZ](sram_in);
    p_switch.out_ports[2*POD_SZ](sram_out);
    sram.packet_in(sram_out);
    sram.packet_out(sram_in);

    p_switch.in_ports[2*POD_SZ+1](cb_in);
    p_switch.out_ports[2*POD_SZ+1](cb_out);
    cb.packet_in(cb_out);
    cb.packet_out(cb_in);

    p_switch.clk(clk);
    p_switch.rst(rst);
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


// SC_MODULE (testbench) {

//   NVHLS_DESIGN(PacketSwitch) p_switch;
//   SRAM sram;
//   MB mb[POD_SZ];
//   SA sa[POD_SZ];
//   CB cb;

//   Connections::Combinational<PacketSwitch::Packet> mb_in[POD_SZ];
//   Connections::Combinational<PacketSwitch::Packet> mb_out[POD_SZ];
//   Connections::Combinational<PacketSwitch::Packet> sa_in[POD_SZ];
//   Connections::Combinational<PacketSwitch::Packet> sa_out[POD_SZ];
//   Connections::Combinational<PacketSwitch::Packet> sram_in;
//   Connections::Combinational<PacketSwitch::Packet> sram_out;
//   Connections::Combinational<PacketSwitch::Packet> cb_in;
//   Connections::Combinational<PacketSwitch::Packet> cb_out;

//   sc_clock clk;
//   sc_signal<bool> rst;

//   // SC_HAS_PROCESS(testbench); 
//   // testbench(sc_module_name name) : sc_module(name),
//   //   // p_switch("p_switch"),

//   SC_CTOR(testbench) :
//     p_switch("p_switch");
//     for(int i = 0; i < POD_SZ; i++) {
//       mb[i](sc_gen_unique_name("mb"));
//       sa[i](sc_gen_unique_name("sa"))
//     }
//     // mb_in("mb_in"),
//     // sa_in("sa_in"),
//     // sram_out("sram_out"),
//     // sa_out("sa_out"),
//     clk("clk", 1, SC_NS, 0.5,0,SC_NS,true);
//     rst("rst")
//   {

//     // 0-3 is MB ind, 4-7 is SA ind, 8 is SRAM, 9 is CB
//     for(int i = 0; i < POD_SZ; i++) {
//       p_switch.in_ports[i](mb_in[i]);
//       p_switch.out_ports[i](mb_out[i]);
//       mb[i].packet_in(mb_out[i]);
//       mb[i].packet_out(mb_in[i]);
//       mb[i].clk(clk);
//       mb[i].rst(rst);


//       p_switch.in_ports[i](sa_in[i]);
//       p_switch.out_ports[i](sa_out[i]);
//       sa[i].packet_in(sa_out[i]);
//       sa[i].packet_out(sa_in[i]);
//       sa[i].clk(clk);
//       sa[i].rst(rst);
//     }

//     p_switch.in_ports[2*POD_SZ](sram_in);
//     p_switch.out_ports[2*POD_SZ](sram_out);
//     sram.packet_in(sram_out);
//     sram.packet_out(sram_in);

//     p_switch.in_ports[2*POD_SZ+1](cb_in);
//     p_switch.out_ports[2*POD_SZ+1](cb_out);
//     cb.packet_in(cb_out);
//     cb.packet_out(cb_in);

//     p_switch.clk(clk);
//     p_switch.rst(rst);
//     sram.clk(clk);
//     sram.rst(rst);
//     cb.clk(clk);
//     cb.rst(rst);

//     SC_THREAD(run);
//   }

//   void run() {
//     //reset
//     rst = 1;
//     wait(10.5, SC_NS);
//     rst = 0;
//     wait(1, SC_NS);
//     rst = 1;
//     wait(100,SC_NS);
//     cout << "@" << sc_time_stamp() << " Stop " << endl ;
//     sc_stop();
//   }
// };



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

  // final_out contains the true answer
  vector<vector<PacketSwitch::AccumType>> final_out(N, vector<PacketSwitch::AccumType>(N)); 
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < N; j++) {
      for(int x = 0; x < POD_SZ; x++) {
        final_out[i][j] += output_mat[x][i][j];
      }
    }
  }

  vector<vector<PacketSwitch::AccumType>> cb_mat(N, vector<PacketSwitch::AccumType>(N)); 
  // cout << "Weight Matrix " << endl; 
  // PrintMat(weight_mat);
  // cout << "Activation Matrix " << endl; 
  // PrintMat(act_mat);
  cout << "Reference Output Matrix " << endl; 
  PrintMat(final_out);
  printf("\n\n\n");

  testbench my_testbench("my_testbench");
  for(int i = 0; i < POD_SZ; i++) {
    my_testbench.sram.weight_mat.push_back(weight_mat[i]);
    my_testbench.sram.act_mat.push_back(act_mat[i]);
  }

  my_testbench.cb.cb_mat = cb_mat;

  // for(int i = 0; i < POD_SZ; i++) {
  //   my_testbench.mb[i].id = i;
  //   my_testbench.sa[i].id = i + POD_SZ;
  // }

  my_testbench.mb0.id = 0;
  my_testbench.sa0.id = 0 + POD_SZ;
  my_testbench.mb1.id = 1;
  my_testbench.sa1.id = 1 + POD_SZ;
  my_testbench.mb2.id = 2;
  my_testbench.sa2.id = 2 + POD_SZ;
  my_testbench.mb3.id = 3;
  my_testbench.sa3.id = 3 + POD_SZ;


  my_testbench.sram.id = 2*POD_SZ;
  my_testbench.cb.id = 2*POD_SZ + 1;

  sc_start();
  cout << "CMODEL PASS" << endl;
  return 0;
};

