#ifndef __AB_H__
#define __AB_H__

#include "PacketSwitch.h"



SC_MODULE(AB) 
{
  public:
  sc_in_clk     clk;
  sc_in<bool>   rst;
  
  Connections::In<PacketSwitch::Packet>  packet_in;
  Connections::Out<PacketSwitch::Packet>  packet_out;

  vector<vector<PacketSwitch::Packet>> acc_buf; 
  vector<PacketSwitch::AddrType>     chain_buf; 

  PacketSwitch::ID_type id;
  PacketSwitch::ID_type level; // id for level of this AB in the h-tree

  int accum_cnt1 = 0;
  int accum_cnt2 = 0;
  int n_ind;
  int m_ind;

  sc_mutex  buffer;
  sc_event  ready_send;
  sc_event  send_init;

  SC_HAS_PROCESS(AB);
  AB(sc_module_name name_) : sc_module(name_) {
      SC_THREAD (recv_switch);
      sensitive << clk.pos();
      NVHLS_NEG_RESET_SIGNAL_IS(rst);

      SC_THREAD (send_switch);
      sensitive << clk.pos();
      NVHLS_NEG_RESET_SIGNAL_IS(rst);
  }




  void recv_switch() {
    packet_in.Reset();
    wait(20.0, SC_NS); // wait for reset
    PacketSwitch::Packet p_in;

    bool send_thread_start = 0;

    vector<vector<PacketSwitch::Packet>> acc_buf_local(M_ob, vector<PacketSwitch::Packet>(N_sr)); 
    vector<PacketSwitch::AddrType> chain_buf_local(NUM_LEVELS+1, 0);
    vector<vector<PacketSwitch::Packet>> zero_acc_buf(M_ob, vector<PacketSwitch::Packet>(N_sr)); 
    vector<PacketSwitch::AddrType> zero_chain_buf(NUM_LEVELS+1, 0);

    // initialize buffers to zero
    for (int m = 0; m < M_ob; m++) {
      for (int n = 0; n < N_sr; n++) {
        for (int i = 0; i < tile_sz; i++) {
          for (int j = 0; j < tile_sz; j++) {
            zero_acc_buf[m][n].data[i][j] = 0; 
            acc_buf_local[m][n].data[i][j] = 0; 
          }
        }
      }
    }

    while(1) {

      if(packet_in.PopNB(p_in)) {

        if(level == NUM_LEVELS - 1) { // second to last level holds accumulates partials the longest

          n_ind = p_in.Y % N_sr; 
          m_ind = 0; 
    
          for (int i = 0; i < tile_sz; i++) {
            for (int j = 0; j < tile_sz; j++) {
              acc_buf_local[m_ind][n_ind].data[i][j] += p_in.data[i][j];
            }
          }

          acc_buf_local[m_ind][n_ind].X = p_in.X;
          acc_buf_local[m_ind][n_ind].Y = p_in.Y;
          acc_buf_local[m_ind][n_ind].Z = p_in.Z;
          acc_buf_local[m_ind][n_ind].x = p_in.x;
          acc_buf_local[m_ind][n_ind].y = p_in.y;
          acc_buf_local[m_ind][n_ind].z = p_in.z;
          acc_buf_local[m_ind][n_ind].SRAM = p_in.SRAM;
          accum_cnt1++;

          wait();

          if(accum_cnt1 == K_ob * N_sr * (K/K_sr)) { // accumulate full MM block in the K dim 

            // double buffering implemented using a mutex lock and sc_event
            buffer.lock();
            acc_buf = acc_buf_local;

            for(int s = 0; s < NUM_LEVELS+1; s++) {
              chain_buf_local[s] = p_in.AB[s];
            }
            chain_buf = chain_buf_local;

            if(!send_thread_start) {
              send_thread_start = 1;
              send_init.notify(); // starts the sending thread only after first SRAM blk has been received
            }
            buffer.unlock();
            ready_send.notify();       

            // reset accum, acc_buf_local, and chain_buf to 0
            accum_cnt1 = 0;
            chain_buf_local = zero_chain_buf;
            acc_buf_local = zero_acc_buf;
          }
        }


        else { // at levels other than 2nd to last, accumulate each partial only twice

          n_ind = p_in.Y % N_sr; 
          m_ind = p_in.X % M_ob; 

          for (int i = 0; i < tile_sz; i++) {
            for (int j = 0; j < tile_sz; j++) {
              acc_buf_local[m_ind][n_ind].data[i][j] += p_in.data[i][j];
            } 
          }

          acc_buf_local[m_ind][n_ind].X = p_in.X;
          acc_buf_local[m_ind][n_ind].Y = p_in.Y;
          acc_buf_local[m_ind][n_ind].Z = p_in.Z;
          acc_buf_local[m_ind][n_ind].x = p_in.x;
          acc_buf_local[m_ind][n_ind].y = p_in.y;
          acc_buf_local[m_ind][n_ind].z = p_in.z;
          acc_buf_local[m_ind][n_ind].SRAM = p_in.SRAM;
          accum_cnt2++;

          wait();

          if(accum_cnt2 == (2 * M_ob * N_sr)) { // At this level only 
                             //accumulate each of the M_ob*N_sr tiles twice
            buffer.lock();
            acc_buf = acc_buf_local;

            // set the AB chain buffer
            for(int s = 0; s < NUM_LEVELS+1; s++) {
              chain_buf_local[s] = p_in.AB[s];
            }
            chain_buf = chain_buf_local;

            // check if this is the first SRAM block
            if(!send_thread_start) {
              send_thread_start = 1;
              send_init.notify(); // starts the sending thread only after first SRAM blk has been received
            }

            buffer.unlock();
            ready_send.notify();       

            // reset accum, acc_buf_local, and chain_buf to 0
            accum_cnt2 = 0;
            chain_buf_local = zero_chain_buf;
            acc_buf_local = zero_acc_buf; 
          } 
        }
      }

      wait();
    }
  }



  void send_switch() {

    packet_out.Reset();   
    wait(20.0, SC_NS); // wait for reset

    PacketSwitch::Packet p_out;

    wait(send_init);

    // printf("noooooooo\n");

    while(1) {

      buffer.lock();

      if(level == NUM_LEVELS - 1) {

        for(int n = 0; n < N_sr; n++) { 

          p_out = acc_buf[m_ind][n]; // this sets X,Y,Z,x,y,z headers for final result output                    
          p_out.src = id;

          for(int s = 0; s < NUM_LEVELS+1; s++) {
            p_out.AB[s] = chain_buf[s];
          }

          p_out.dst = chain_buf[1]; // send to next AB (or SRAM) in chain
          p_out.d_type = 2; // result type                    
          // if(DEBUG) cout <<  "AB " << id << " sending tile to " << p_out.dst << "\n";
          packet_out.Push(p_out);
          // cout <<  "AB " << id << " sending tile to " << p_out.dst << "\n";
          for (int i = 0; i < tile_sz; i++) {
            for (int j = 0; j < tile_sz; j++) {
              acc_buf[m_ind][n].data[i][j] = 0; // reset acc_buf to 0;
            }
          }

          wait();   
        }
      }

      else {

        for(int m = 0; m < M_ob; m++) { 
          for(int n = 0; n < N_sr; n++) { 
           
            p_out = acc_buf[m][n]; // this sets X,Y,Z,x,y,z headers for final result output
            p_out.src = id;

            // set AB chain in outupt packet
            for(int s = 0; s < NUM_LEVELS+1; s++) {
              p_out.AB[s] = chain_buf[s];
            }
            // cout << "\n";                      

            for(int s = 0; s < NUM_LEVELS+1; s++) {
              if(chain_buf[s] < id) {
                p_out.dst = chain_buf[s];
                break;
              }
            }

            p_out.d_type = 2; // result type                    
            // if(DEBUG) cout <<  "AB " << id << " sending tile to " << p_out.dst << "\n";
            packet_out.Push(p_out);

            for (int i = 0; i < tile_sz; i++) {
              for (int j = 0; j < tile_sz; j++) {
                acc_buf[m][n].data[i][j] = 0; // reset acc_buf to 0;
              }
            }

            wait();   
          }
        }
      }

      buffer.unlock();
      wait(ready_send);
    }
  }




};


#endif


