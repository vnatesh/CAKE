#ifndef __AB_H__
#define __AB_H__

#include "PacketSwitch.h"
#include "log.h"



SC_MODULE(AB) 
{
  public:
  sc_in_clk     clk;
  sc_in<bool>   rst;
  
  Connections::In<PacketSwitch::Packet>  partial_in;
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

      // SC_THREAD (send_switch);
      // sensitive << clk.pos();
      // NVHLS_NEG_RESET_SIGNAL_IS(rst);
  }




  void recv_switch() {
    packet_in.Reset();    
    packet_out.Reset();   
    partial_in.Reset();   
    wait(20.0, SC_NS); // wait for reset

    PacketSwitch::Packet p_in1;
    PacketSwitch::Packet p_in2;

    PacketSwitch::Packet p_out;
    bool send_thread_start = 0;
    bool first_blk = 1;
    bool ready = 0;


    bool partial_reg_in = 0;
    int K_cnt = 0;

    PacketSwitch::Packet acc_buf;
    vector<PacketSwitch::AddrType> chain_buf(NUM_LEVELS+1, 0);
    PacketSwitch::Packet zero_acc_buf; 
    vector<PacketSwitch::AddrType> zero_chain_buf(NUM_LEVELS+1, 0);

    for (int i = 0; i < tile_sz; i++) {
      for (int j = 0; j < tile_sz; j++) {
        acc_buf.data[i][j] = 0; 
        zero_acc_buf.data[i][j] = 0;
      }
    }


    int n_ind;
    p_in2.cycle_cnt = 0;


    while(1) {

      if(accum_cnt1 != K_ob) {
        if(packet_in.PopNB(p_in1)) {

          if(DEBUG) cout << "AB " << id << " received partial\n";

          for (int i = 0; i < tile_sz; i++) {
            for (int j = 0; j < tile_sz; j++) {
              acc_buf.data[i][j] += p_in1.data[i][j];
            }
          }

          acc_buf.X = p_in1.X;
          acc_buf.Y = p_in1.Y;
          acc_buf.Z = p_in1.Z;
          acc_buf.x = p_in1.x;
          acc_buf.y = p_in1.y;
          acc_buf.z = p_in1.z;
          acc_buf.SRAM = p_in1.SRAM;
          accum_cnt1++;
          n_ind = p_in1.Y % N_sr; 
          // cout << "n_ind = " << n_ind << " accum_cnt1 = " << accum_cnt1 << " accum_cnt2 = " << accum_cnt2 << "\n";
          wait();
        }
      }
      


      // if(the two partials from leaves haven't been accumed yet, then dont read from partial_in yet) {}
      if(accum_cnt1 == K_ob && !first_blk) {

        if(partial_in.PopNB(p_in2)) {

          if(p_in2.Y % N_sr == n_ind) {

            if(DEBUG) cout << "AB " << id << " received partial from SRAM\n";

            for (int i = 0; i < tile_sz; i++) {
              for (int j = 0; j < tile_sz; j++) {
                acc_buf.data[i][j] += p_in2.data[i][j];
              }
            }

            partial_reg_in = 1;
          }

          wait();
        }
      }

      
      if(level == NUM_LEVELS - 1) { // second to last level holds accumulates partials the longest when K_ob = 2
      
        if(first_blk) {
          if(accum_cnt1 == K_ob) { 
            ready = 1;
            // first_blk = 0;
          }
        } else {
          if(accum_cnt1 == K_ob && partial_reg_in == 1) {
            ready = 1;
          } 
        }         

        if(ready) { // accumulate full MM block in the K dim 
          accum_cnt2++;
          // cout << "ready now\n";
          for(int s = 0; s < NUM_LEVELS+1; s++) {
            chain_buf[s] = p_in1.AB[s];
          }

          p_out = acc_buf; // this sets X,Y,Z,x,y,z headers for final result output                    
          p_out.src = id;

          for(int s = 0; s < NUM_LEVELS+1; s++) {
            p_out.AB[s] = chain_buf[s];
          }

          p_out.dst = chain_buf[1]; // send to next AB (or SRAM) in chain


          p_out.cycle_cnt = p_in2.cycle_cnt + 1; // partial result type
          if(p_out.cycle_cnt == K/K_sr) {
            p_out.d_type = 3; //  result type
            p_out.cycle_cnt = 0;
            p_in2.cycle_cnt = 0;
          } else {
            p_out.d_type = 2; // partial result type
          }

          packet_out.Push(p_out);
          // cout <<  "AB " << id << " sending tile to " << p_out.dst << "\n";
          
          log_packet("AB", "SRAM", id, p_out);


          // if(accum_cnt2 == K_ob * N_sr * (K/K_sr)) {
          if(accum_cnt2 == N_sr ) {
            if(first_blk) {
              first_blk = 0;
            }
            accum_cnt2 = 0;
            K_cnt++;
          }

          if(K_cnt == K/K_sr) {
            first_blk = 1;
            K_cnt = 0;
          }

          partial_reg_in = 0;
          accum_cnt1 = 0;
          ready = 0;
          chain_buf = zero_chain_buf;
          acc_buf = zero_acc_buf;
          wait();
        }
      }

        

        // else { // at levels other than 2nd to last, accumulate each partial only twice

        //   n_ind = p_in.Y % N_sr; 
        //   m_ind = p_in.X % M_ob; 

        //   for (int i = 0; i < tile_sz; i++) {
        //     for (int j = 0; j < tile_sz; j++) {
        //       acc_buf_local[m_ind][n_ind].data[i][j] += p_in.data[i][j];
        //     } 
        //   }

        //   acc_buf_local[m_ind][n_ind].X = p_in.X;
        //   acc_buf_local[m_ind][n_ind].Y = p_in.Y;
        //   acc_buf_local[m_ind][n_ind].Z = p_in.Z;
        //   acc_buf_local[m_ind][n_ind].x = p_in.x;
        //   acc_buf_local[m_ind][n_ind].y = p_in.y;
        //   acc_buf_local[m_ind][n_ind].z = p_in.z;
        //   acc_buf_local[m_ind][n_ind].SRAM = p_in.SRAM;
        //   accum_cnt2++;

        //   wait();

        //   if(accum_cnt2 == (2 * M_ob * N_sr)) { // At this level only 
        //                      //accumulate each of the M_ob*N_sr tiles twice
        //     buffer.lock();
        //     acc_buf = acc_buf_local;

        //     // set the AB chain buffer
        //     for(int s = 0; s < NUM_LEVELS+1; s++) {
        //       chain_buf_local[s] = p_in.AB[s];
        //     }
        //     chain_buf = chain_buf_local;

        //     // check if this is the first SRAM block
        //     if(!send_thread_start) {
        //       send_thread_start = 1;
        //       send_init.notify(); // starts the sending thread only after first SRAM blk has been received
        //     }

        //     buffer.unlock();
        //     ready_send.notify();       

        //     // reset accum, acc_buf_local, and chain_buf to 0
        //     accum_cnt2 = 0;
        //     chain_buf_local = zero_chain_buf;
        //     acc_buf_local = zero_acc_buf; 
        //     // for (int m = 0; m < M_ob; m++) {
        //     //   for (int n = 0; n < N_sr; n++) {
        //     //     for (int i = 0; i < tile_sz; i++) {
        //     //       for (int j = 0; j < tile_sz; j++) {
        //     //         acc_buf_local[m][n].data[i][j] = 0; 
        //     //       }
        //     //     }
        //     //   }
        //     // }
        //   } 
        // }

      wait();
    }
  }



  // void send_switch() {

  //   packet_out.Reset();   
  //   wait(20.0, SC_NS); // wait for reset

  //   PacketSwitch::Packet p_out;

  //   wait(send_init);

  //   // printf("noooooooo\n");

  //   while(1) {

  //     buffer.lock();

  //     if(level == NUM_LEVELS - 1) {

  //       for(int n = 0; n < N_sr; n++) { 

  //         p_out = acc_buf[m_ind][n]; // this sets X,Y,Z,x,y,z headers for final result output                    
  //         p_out.src = id;

  //         for(int s = 0; s < NUM_LEVELS+1; s++) {
  //           p_out.AB[s] = chain_buf[s];
  //         }

  //         p_out.dst = chain_buf[1]; // send to next AB (or SRAM) in chain
  //         p_out.d_type = 2; // result type                    
  //         // if(DEBUG) cout <<  "AB " << id << " sending tile to " << p_out.dst << "\n";
  //         packet_out.Push(p_out);
  //         // cout <<  "AB " << id << " sending tile to " << p_out.dst << "\n";
  //         for (int i = 0; i < tile_sz; i++) {
  //           for (int j = 0; j < tile_sz; j++) {
  //             acc_buf[m_ind][n].data[i][j] = 0; // reset acc_buf to 0;
  //           }
  //         }

  //         wait();   
  //       }
  //     }

  //     else {

  //       for(int m = 0; m < M_ob; m++) { 
  //         for(int n = 0; n < N_sr; n++) { 
           
  //           p_out = acc_buf[m][n]; // this sets X,Y,Z,x,y,z headers for final result output
  //           p_out.src = id;

  //           // set AB chain in outupt packet
  //           for(int s = 0; s < NUM_LEVELS+1; s++) {
  //             p_out.AB[s] = chain_buf[s];
  //           }
  //           // cout << "\n";                      

  //           for(int s = 0; s < NUM_LEVELS+1; s++) {
  //             if(chain_buf[s] < id) {
  //               p_out.dst = chain_buf[s];
  //               break;
  //             }
  //           }

  //           p_out.d_type = 2; // result type                    
  //           // if(DEBUG) cout <<  "AB " << id << " sending tile to " << p_out.dst << "\n";
  //           packet_out.Push(p_out);

  //           for (int i = 0; i < tile_sz; i++) {
  //             for (int j = 0; j < tile_sz; j++) {
  //               acc_buf[m][n].data[i][j] = 0; // reset acc_buf to 0;
  //             }
  //           }

  //           wait();   
  //         }
  //       }
  //     }

  //     buffer.unlock();
  //     wait(ready_send);
  //   }
  // }




};


#endif


