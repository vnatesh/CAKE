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

    while(1) {
      if(packet_in.PopNB(p_in)) {
        if(p_in.dst == id && p_in.d_type == 2) { // dst is cb and packet is result type
          
          if(DEBUG) cout << "Partial result received at CB " << p_in.dstPod << "\n";
          for (int i = 0; i < tile_sz; i++) {
             for (int j = 0; j < tile_sz; j++) {
              cb_mat[tile_cnt][i][j] += p_in.data[i][j];
            }
          }

          accum_cnt++;
        }
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
          for (int i = 0; i < tile_sz; i++) {
             for (int j = 0; j < tile_sz; j++) {
              p_out.data[i][j] = cb_mat[t][i][j];
              cb_mat[t][i][j] = 0; // set cb_mat to 0;
            }
          }
          wait();
          p_out.src = id;
          p_out.srcPod = p_in.dstPod;
          p_out.dst = INT_MAX; // destination is SRAM
          p_out.dstPod = 0; // destination pod is default 0 for SRAM
          p_out.d_type = 2; // result type                    
          
          if(DEBUG) cout <<  "CB " << p_out.srcPod << " sending tile " << t << " to SRAM\n";
          packet_out.Push(p_out);
          wait();
        }
      
        accum_cnt = 0;
        tile_cnt = 0;
        K_cnt = 0;
      } 

      wait(1);
    }
  }


};


#endif

