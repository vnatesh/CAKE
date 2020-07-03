#ifndef __CB_H__
#define __CB_H__

#include "PacketSwitch.h"


SC_MODULE(CB) 
{
  // public:
  sc_in <bool> clk;
  sc_in <bool>   rst;
  
  Connections::In<PacketSwitch::Packet>  packet_in;
  Connections::Out<PacketSwitch::Packet>  packet_out;

// If any tile within the buffer has been accumulated (K / K_sr) times:
//     Send the completed accumulation tile to SRAM

  // vector<vector<vector<PacketSwitch::AccumType>>> acc_buf; 
  vector<vector<vector<PacketSwitch::Packet>>> acc_buf; 
  PacketSwitch::ID_type id;

  double io_time = 0;
  double idle_time = 0;
  double compute_time = 0;
  int packet_counter = 0;


  int wait_cnt = 0;

  SC_HAS_PROCESS(CB);
  CB(sc_module_name name_) : sc_module(name_) {
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
    int tile_cnt = 0;
    int K_cnt = 0;
    int K_cnt_sr = 0;
    int M_cnt_dr = 0;
    int N_cnt_dr = 0;
    int m_ind;
    int n_ind;

    struct timeval start, end;

    while(1) {
      if(packet_in.PopNB(p_in)) {

        gettimeofday (&start, NULL);
        if(p_in.dst == p_in.CB && p_in.d_type == 2) { // dst is cb and packet is result type
          
          m_ind = (p_in.X % (M_dr/tile_sz)) / (M_sr/tile_sz);
          n_ind = (p_in.Y % (N_dr/tile_sz)) / (N_sr/tile_sz);
          // if(DEBUG) cout << "Partial result received at CB " << p_in.dstPod << "\n";
          if(DEBUG) cout << "Partial result received at CB " << p_in.x << "\n";
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
          acc_buf[n_ind][m_ind][p_in.y].CB = p_in.CB;
          acc_buf[n_ind][m_ind][p_in.y].SRAM = p_in.SRAM;
          accum_cnt++;
        }

        gettimeofday (&end, NULL);
        compute_time += ((((end.tv_sec - start.tv_sec) * 1000000L)
            + (end.tv_usec - start.tv_usec)) / 1000000.0);
      }

      wait();

      if(accum_cnt == POD_SZ) {
        accum_cnt = 0;
        tile_cnt++;
      }

      if(tile_cnt == alpha * POD_SZ) {
        tile_cnt = 0;
        K_cnt_sr++;
      }

      if(K_cnt_sr == (K_dr / K_sr)) {
        K_cnt_sr = 0;
        M_cnt_dr++; 
      }

      if(M_cnt_dr == (M_dr / M_sr)) {
        M_cnt_dr = 0;
        N_cnt_dr++;
      }

      if(N_cnt_dr == (N_dr / N_sr)) {
        N_cnt_dr = 0;
        K_cnt++; // counter for number of DRAM blocks in K dir of full computation space
      }

      // send the completed results
      if(K_cnt == (K / K_dr)) {

        for(int m = 0; m < (M_dr / M_sr); m++) {
          for(int n = 0; n < (N_dr / N_sr); n++) {
          
            for(int t = 0; t < (alpha * POD_SZ); t++) {

              gettimeofday (&start, NULL);
              p_out = acc_buf[n][m][t]; // this sets X,Y,Z,x,y,z headers for final result output

              for (int i = 0; i < tile_sz; i++) {
                for (int j = 0; j < tile_sz; j++) {
                  acc_buf[n][m][t].data[i][j] = 0; // reset acc_buf to 0;
                }
              }

              p_out.src = p_out.CB;
              p_out.dst = p_out.SRAM;
              p_out.d_type = 2; // result type                    
              
              if(DEBUG) cout <<  "CB " << p_out.x << " sending tile " << t << " to SRAM\n";
              packet_out.Push(p_out);
              packet_counter++;

              // if(p_out.x == 0) {
              //   packet_counter++;
              //   for(int i = 0; i < tile_sz; i++) {
              //     for(int j = 0; j < tile_sz; j++) {
              //       cout << p_out.data[i][j] << " "; 
              //     }
              //     cout << "\n";
              //   }
              //   cout << "cnt = " << packet_counter << "\n";
              // }

              wait();

              gettimeofday (&end, NULL);
                io_time += ((((end.tv_sec - start.tv_sec) * 1000000L)
                    + (end.tv_usec - start.tv_usec)) / 1000000.0);
            }
          }          
        }

        K_cnt = 0;
      }

      // cout << "packet cnt = " << packet_counter << "\n";
      // // CB is not really 'idle' after its finished accumulating everything. Once its finished sending , stop counting time
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

