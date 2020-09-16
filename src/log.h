#ifndef __LOG_H__
#define __LOG_H__

#include "PacketSwitch.h"


void log_packet(const char* mod_src, const char* mod_dst, int id, PacketSwitch::Packet pac) {

  logfile << "@ " << sc_time_stamp() << "\n";
  logfile <<  mod_src << " " << id << " sending tile to " << mod_dst << " " << pac.dst << "\n";
  logfile << "Packet Header : X = " << pac.X << ", Y = " << pac.Y <<
  ", Z = " << pac.Z << ", x = " << pac.x << ", y = " << pac.y <<
  ", src = " << pac.src << ", dst = " << pac.dst << ", dtype = " << pac.d_type << "\n";

  for (int i = 0; i < tile_sz; i++) {
    for (int j = 0; j < tile_sz; j++) {
      logfile << pac.data[i][j] << " ";
    }
    logfile << "\n";
  }
  logfile << "\n";
}



#endif
