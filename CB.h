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

  vector<vector<vector<PacketSwitch::AccumType>>> cb_mat; 
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

    struct timeval start, end;


    while(1) {
      if(packet_in.PopNB(p_in)) {

        gettimeofday (&start, NULL);
        if(p_in.dst == id && p_in.d_type == 2) { // dst is cb and packet is result type
          
          if(DEBUG) cout << "Partial result received at CB " << p_in.dstPod << "\n";
          for (int i = 0; i < tile_sz; i++) {
             for (int j = 0; j < tile_sz; j++) {
              cb_mat[tile_cnt][i][j] += p_in.data[i][j];
            }
          }

          accum_cnt++;
          wait();
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
        accum_cnt = 0;
        tile_cnt = 0;
        K_cnt++;
      }

      if(K_cnt == (K / Wz)) {

        tile_cnt = alpha * POD_SZ;
        for(int t = 0; t < tile_cnt; t++) {
          gettimeofday (&start, NULL);
          for (int i = 0; i < tile_sz; i++) {
             for (int j = 0; j < tile_sz; j++) {
              p_out.data[i][j] = cb_mat[t][i][j];
              cb_mat[t][i][j] = 0; // set cb_mat to 0;
            }
          }
          // wait();
          p_out.src = id;
          p_out.srcPod = p_in.dstPod;
          p_out.dst = INT_MAX; // destination is SRAM
          p_out.dstPod = 0; // destination pod is default 0 for SRAM
          p_out.d_type = 2; // result type                    
          
          if(DEBUG) cout <<  "CB " << p_out.srcPod << " sending tile " << t << " to SRAM\n";
          packet_out.Push(p_out);
          packet_counter++;
          wait();

          gettimeofday (&end, NULL);
            io_time += ((((end.tv_sec - start.tv_sec) * 1000000L)
                + (end.tv_usec - start.tv_usec)) / 1000000.0);

        }
      
        accum_cnt = 0;
        tile_cnt = 0;
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

