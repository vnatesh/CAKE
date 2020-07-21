#ifndef __AB_H__
#define __AB_H__

#include "PacketSwitch.h"


SC_MODULE(AB) 
{
  // public:
  sc_in <bool> clk;
  sc_in <bool>   rst;
  
  Connections::In<PacketSwitch::Packet>  packet_in;
  Connections::Out<PacketSwitch::Packet>  packet_out;

  vector<vector<vector<PacketSwitch::Packet>>> acc_buf; 
  PacketSwitch::ID_type id;

  double io_time = 0;
  double idle_time = 0;
  double compute_time = 0;
  int packet_counter = 0;
  int wait_cnt = 0;

  SC_HAS_PROCESS(AB);
  AB(sc_module_name name_) : sc_module(name_) {
      SC_THREAD (run);
      sensitive << clk.pos();
      NVHLS_NEG_RESET_SIGNAL_IS(rst);
  }

  void run() {

    packet_in.Reset();
    packet_out.Reset();
    wait(20.0, SC_NS);

    PacketSwitch::Packet p_in;
    PacketSwitch::Packet p_out;
    
    int accum_cnt = 0;
    int m_ind;
    int n_ind;
    // int r = 0;

    while(1) {
      if(packet_in.PopNB(p_in)) {

        if(p_in.dst == p_in.AB && p_in.d_type == 2) { // dst is cb and packet is result type
          
          m_ind = (p_in.X % M_sr) / M_ob;
          n_ind = (p_in.Y % N_sr) / N_ob;
          // if(DEBUG) cout << "Partial result received at AB " << p_in.dstPod << "\n";
          if(DEBUG) cout << "Partial result received at AB " << p_in.x << "\n";
          for (int i = 0; i < tile_sz; i++) {
            for (int j = 0; j < tile_sz; j++) {
              acc_buf[n_ind][m_ind][p_in.y].data[i][j] += p_in.data[i][j];
            }
          }

          acc_buf[n_ind][m_ind][p_in.y].X = p_in.X;
          acc_buf[n_ind][m_ind][p_in.y].Y = p_in.Y;
          acc_buf[n_ind][m_ind][p_in.y].Z = p_in.Z;
          acc_buf[n_ind][m_ind][p_in.y].x = p_in.x;
          acc_buf[n_ind][m_ind][p_in.y].y = p_in.y;
          acc_buf[n_ind][m_ind][p_in.y].z = p_in.z;
          acc_buf[n_ind][m_ind][p_in.y].AB = p_in.AB;
          acc_buf[n_ind][m_ind][p_in.y].SRAM = p_in.SRAM;
          accum_cnt++;
        }
      }

      wait();

      // send the completed results
      // if(accum_cnt == K_ob * N_ob * (K_sr / K_ob) * 
      //             (M_sr / M_ob) * (N_sr / N_ob) * (K_mm / K_sr)) {
      if(accum_cnt == POD_SZ * alpha * POD_SZ * (K_sr / K_ob) * 
                  (M_sr / M_ob) * (N_sr / N_ob) * (K / K_sr)) {


        for(int m = 0; m < (M_sr / M_ob); m++) {
          for(int n = 0; n < (N_sr / N_ob); n++) {
          
            for(int t = 0; t < (alpha * POD_SZ); t++) {

              p_out = acc_buf[n][m][t]; // this sets X,Y,Z,x,y,z headers for final result output

              for (int i = 0; i < tile_sz; i++) {
                for (int j = 0; j < tile_sz; j++) {
                  acc_buf[n][m][t].data[i][j] = 0; // reset acc_buf to 0;
                }
              }

              p_out.src = p_out.AB;
              p_out.dst = p_out.SRAM;
              p_out.d_type = 2; // result type                    
              
              if(DEBUG) cout <<  "AB " << p_out.x << " sending tile " << t << " to SRAM\n";
              packet_out.Push(p_out);

              // r++;
              // if(r == 224) cout << "r_cnt = " << r << "\n";
              // // if(p_in.x == 0) cout << "r_cnt AB 0 = " << r << "\n";
              // // if(p_in.x == 1) cout << "r_cnt AB 1 = " << r << "\n";
              // // if(p_in.x == 2) cout << "r_cnt AB 2 = " << r << "\n";
              // // if(p_in.x == 3) cout << "r_cnt AB 3 = " << r << "\n";
              packet_counter++;
              wait();

            }
          }          
        }

        accum_cnt = 0;
      }

      // cout << "packet cnt = " << packet_counter << "\n";
      // // AB is not really 'idle' after its finished accumulating everything. Once its finished sending , stop counting time
      // if((packet_counter > 0) && (packet_counter != (((Wz/tile_sz) * (Dx/tile_sz)) / (P*P)))) {
      //   gettimeofday (&start, NULL);
      //   wait();
      //   gettimeofday (&end, NULL);
      //   idle_time += ((((end.tv_sec - start.tv_sec) * 1000000L)
      //       + (end.tv_usec - start.tv_usec)) / 1000000.0);
      //   wait_cnt++;
      // }
      wait();
    }
  }

};


#endif

