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



  // 0-3 is MB ind, 4-7 is SA ind, 8 is SRAM, 9 is CB
  SC_HAS_PROCESS(SRAM);
  SRAM(sc_module_name name_) : sc_module(name_) {
    SC_THREAD(send_block);
    sensitive << clk.pos();
    NVHLS_NEG_RESET_SIGNAL_IS(rst);

    SC_THREAD(receive_result);
    sensitive << clk.pos();
    NVHLS_NEG_RESET_SIGNAL_IS(rst);

  }


  void send_block() {

    sram_dram_in.Reset();
    packet_out.Reset();
    wait(20.0, SC_NS);

    PacketSwitch::Packet p_in;

    PacketSwitch::Packet p_out1;
    PacketSwitch::Packet p_out2;

    int m_cnt = 0; // weight row
    int k_cnt = 0; // weight col / act row
    int n_cnt = 0; // act col


    vector<vector<PacketSwitch::AccumType>> weight_block(Wy, vector<PacketSwitch::AccumType>(Wz)); 
    vector<vector<PacketSwitch::AccumType>> activation_block(Wz, vector<PacketSwitch::AccumType>(Dx)); 

    sc_time start, end;


    while(1) {
      if(sram_dram_in.PopNB(p_in)) {

        start = sc_time_stamp();
        if(p_in.d_type == 0) {

          for (int i = 0; i < tile_sz; i++) {
            for (int j = 0; j < tile_sz; j++) {
              weight_block[m_cnt * tile_sz + i][k_cnt * tile_sz + j] = p_in.data[i][j];
            }
          }

          wait();
          // wait(5);

          k_cnt++;
          if(k_cnt ==  Wz / tile_sz) {
            k_cnt = 0;
            m_cnt++;
          }

          if(m_cnt == Wy / tile_sz) {

            if(DEBUG) cout << "Sending weight block from SRAM to Maestro" << "\n";
            for (int m = 0; m < Wy/tile_sz; m++) { // row
              for (int k = 0; k < Wz/tile_sz; k++) { // col

                for (int i = 0; i < tile_sz; i++) {
                  for (int j = 0; j < tile_sz; j++) {
                    p_out1.data[i][j] = weight_block[m * tile_sz + i][k * tile_sz + j];
                  }
                }

                // wait(1);
                // wait(5);
                p_out1.src = INT_MAX; // sram src
                p_out1.srcPod = 0; // sram default src pod
                p_out1.dst = k;
                p_out1.dstPod = m;
                p_out1.d_type = 0;
                p_out1.bcast = 0;
                packet_out.Push(p_out1); 

                packet_counter++;

                // wait(2);
                // wait(10);
                wait();
              }
            }

            m_cnt = 0;
          }
        } 

        else if(p_in.d_type == 1) {

          for (int i = 0; i < tile_sz; i++) {
            for (int j = 0; j < tile_sz; j++) {
              activation_block[k_cnt * tile_sz + i][n_cnt * tile_sz + j] = p_in.data[i][j];
            }
          }

          wait(1);
          // wait(5);
          k_cnt++;
          if(k_cnt ==  Wz / tile_sz) {
            k_cnt = 0;
            n_cnt++;
          }

          if(n_cnt ==  Dx / tile_sz) {

            // TODO : this is a triple loop due to the broadcast. Change packet header to include BCAST instruction
            if(DEBUG) cout << "Sending activation block from SRAM to Maestro" << "\n";
            for (int n = 0; n < Dx/tile_sz; n++){
              for (int k = 0; k < Wz/tile_sz; k++) {
                // for (int m = 0; m < M/ tile_sz; m++) {
             
                for (int i = 0; i < tile_sz; i++) {
                  for (int j = 0; j < tile_sz; j++) {
                    p_out2.data[i][j] = activation_block[k * tile_sz + i][n * tile_sz + j];
                  }
                }

                // wait(1);
                // wait(5);
                p_out2.src = INT_MAX;
                p_out2.srcPod = 0;
                p_out2.dst = k;
                // p_out2.dstPod = m;
                p_out2.d_type = 1;
                p_out2.bcast = 1;
                packet_out.Push(p_out2);

                packet_counter++;

                // wait(2);
                wait();
                // wait(10);
              // }
              }
            }

            n_cnt = 0;
          }
        }

        end = sc_time_stamp();
        io_time += (end - start).to_default_time_units();
      }

      start = sc_time_stamp();
      wait();
      end = sc_time_stamp();
      idle_time += (end - start).to_default_time_units();

    }
  }


  void receive_result() {

    sram_dram_out.Reset();
    packet_in.Reset();
    wait(20.0, SC_NS);

    PacketSwitch::Packet p_in2;
    

    while(1) {

      if(packet_in.PopNB(p_in2)) {
        sram_dram_out.Push(p_in2);
        result_cnt++;
      } 

      wait(); 
      wait_cnt++; // use wait cnt as a way to synchronize tracking of the current time and result_cnt
      if(wait_cnt % 10 == 0) {
        cout << "SRAM " << sc_time_stamp().to_default_time_units() << " " << result_cnt << "\n";
      }

    }
  }


};


#endif



