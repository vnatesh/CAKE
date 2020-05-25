#ifndef __CB_H__
#define __CB_H__

#include "PacketSwitch.h"


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


#endif

