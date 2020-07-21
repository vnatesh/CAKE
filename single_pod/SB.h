#ifndef __SB_H__
#define __SB_H__

#include "PacketSwitch.h"



SC_MODULE(SB)
{
  public:
  sc_in_clk     clk;
  sc_in<bool>   rst;

  // Interface Ports
  Connections::In<PacketSwitch::Packet>   packet_in;
  Connections::Out<PacketSwitch::Packet>  packet_out;

  PacketSwitch::ID_type id;

  SC_HAS_PROCESS(SB);
  SB(sc_module_name name_) : sc_module(name_) {
    SC_THREAD (run);
    sensitive << clk.pos();
    NVHLS_NEG_RESET_SIGNAL_IS(rst);
  }


  void run() {
    packet_in.Reset();
    packet_out.Reset();

    wait(20.0, SC_NS);

    PacketSwitch::Packet packet_reg;
    PacketSwitch::Packet weight_reg; // single weight tile storage for reuse when new region starts
    vector<vector<PacketSwitch::Packet>> act_buffer(K_sr/K_ob, 
          vector<PacketSwitch::Packet>(N_ob)); // storage for a region of data (K_dr/K_sr length  
                                                              //run since same data is reused on each run) 
    // int w = 0;
    // int a = 0;

    int ind;

    #pragma hls_pipeline_init_interval 1
    while(1) {
      if (packet_in.PopNB(packet_reg)) {
        // check that src is SRAM
        if(packet_reg.src == INT_MAX && packet_reg.d_type == 1) { // activation
          
          ind = (packet_reg.Z % K_sr) / K_ob;
          act_buffer[ind][packet_reg.y] = packet_reg; //packet_reg.y varies from 0 to N_sr/tile_sz - 1
          if(DEBUG) cout <<  "received activation at SB " << packet_reg.x << " Pod " << id << " from SRAM\n";
        
          // a++;
          // if(packet_reg.x == 0 && id == 0) cout << "a_cnt = " << a << "\n";

        }

        else if(packet_reg.src == INT_MAX && packet_reg.d_type == 0) { // weights
          weight_reg = packet_reg;
          if(DEBUG) cout <<  "received weight at SB " << packet_reg.x << " Pod " << id << " from SRAM\n";
          // send weight packet to SA
          packet_reg.src = packet_reg.dst;
          packet_reg.dst = packet_reg.SA;
          packet_reg.AB = 2*POD_SZ; 
          packet_reg.d_type = 0;
          packet_out.Push(packet_reg);                  
          wait();

          // w++;
          // if(packet_reg.x == 0 && id == 0) cout << "w_cnt = " << w << "\n";

          ind = (weight_reg.Z % K_sr) / K_ob;
          // send data packets to SA, decrement ttl 
          for(int i = 0; i < N_ob; i++) {
            packet_reg = act_buffer[ind][i];
            act_buffer[ind][i].ttl--;
            packet_reg.src = weight_reg.dst;
            packet_reg.dst = weight_reg.SA;
            packet_reg.AB = 2*POD_SZ; 
            packet_reg.d_type = 1;                    
            packet_out.Push(packet_reg);                    
            wait();
          }
        }
      }

      wait();
    }
  }
};
#endif














// MB with data reuse as well as weight reuse 



// #ifndef __MB_H__
// #define __MB_H__

// #include "PacketSwitch.h"



// SC_MODULE(MB)
// {
//   public:
//   sc_in_clk     clk;
//   sc_in<bool>   rst;

//   // Interface Ports
//   Connections::In<PacketSwitch::Packet>   packet_in;
//   Connections::Out<PacketSwitch::Packet>  packet_out;

//   PacketSwitch::ID_type id;

//   SC_HAS_PROCESS(MB);
//   MB(sc_module_name name_) : sc_module(name_) {
//     SC_THREAD (run);
//     sensitive << clk.pos();
//     NVHLS_NEG_RESET_SIGNAL_IS(rst);
//   }


//   void run() {
//     packet_in.Reset();
//     packet_out.Reset();

//     wait(20.0, SC_NS);

//     PacketSwitch::Packet packet_reg;
//     vector<vector<PacketSwitch::Packet>> act_buffer(K_dr/K_sr, 
//           vector<PacketSwitch::Packet>(N_sr/tile_sz)); // storage for a region of data (K_dr/K_sr length  
//                                                               //strip since same data is used on each strip) 
//     vector<PacketSwitch::Packet> weight_buffer(K_dr/K_sr); // storage for a strip of weights (K_dr/K_sr length)  

//     int ind;
//     int w_cnt = 0;
//     int a_cnt = 0;
//     bool weight_reuse = false;



//     int w = 0;
//     int a = 0;



//     #pragma hls_pipeline_init_interval 1
//     while(1) {
//       if (packet_in.PopNB(packet_reg)) {
//         // check that src is SRAM
//         if(packet_reg.src == INT_MAX && packet_reg.d_type == 1) { // activation
          
//           ind = (packet_reg.Z % (K_dr/tile_sz)) / (K_sr/tile_sz);
//           act_buffer[ind][packet_reg.y] = packet_reg; //packet_reg.y varies from 0 to N_sr/tile_sz - 1
//           if(DEBUG) cout <<  "received activation at MB " << packet_reg.x << " Pod " << id << " from SRAM\n";
//           if(weight_reuse) {
//             a_cnt++;
//           }
//         }

//         else if(packet_reg.src == INT_MAX && packet_reg.d_type == 0) { // weights


//           ind = (packet_reg.Z % (K_dr/tile_sz)) / (K_sr/tile_sz);
//           weight_buffer[ind] = packet_reg;
//           if(DEBUG) cout <<  "received weight at MB " << packet_reg.x << " Pod " << id << " from SRAM\n";
        
//           if(packet_reg.ttl > 0) {
//             w_cnt++;
//           }
//           // send weight packet to SA
//           packet_reg.src = packet_reg.dst;
//           packet_reg.dst = packet_reg.SA;
//           packet_reg.CB = 2*POD_SZ; 
//           packet_reg.d_type = 0;
//           packet_out.Push(packet_reg);                  
//           wait();

//           // w++;
//           // if(packet_reg.x == 0 && id == 0) cout <<  "w =  " << w << "\n";

//           // send data packets to SA, decrement ttl 
//           for(int i = 0; i < N_sr/tile_sz; i++) {
//             packet_reg = act_buffer[ind][i];
//             act_buffer[ind][i].ttl--;
//             packet_reg.src = weight_buffer[ind].dst;
//             packet_reg.dst = weight_buffer[ind].SA;
//             packet_reg.CB = 2*POD_SZ; 
//             packet_reg.d_type = 1;                    
//             packet_out.Push(packet_reg);                    
//             wait();

//             // a++;
//             // if(packet_reg.x == 0 && id == 0) cout <<  "a =  " << a << "\n";



//           }

//           if(w_cnt == K_dr/K_sr) {
//             weight_reuse = true;
//           }
//         }
//       }

//       wait();

//       if(weight_reuse && a_cnt == ((N_sr/tile_sz) * (K_dr/K_sr))) {

//         for(int j = 0; j < w_cnt; j++) {
//           packet_reg = weight_buffer[j];
//           packet_reg.src = packet_reg.dst;
//           packet_reg.dst = packet_reg.SA;
//           packet_reg.CB = 2*POD_SZ; 
//           packet_reg.d_type = 0;
//           packet_out.Push(packet_reg);                  
//           wait();

//           // w++;
//           // if(packet_reg.x == 0 && id == 0) cout <<  "w =  " << w << "\n";



//           for(int i = 0; i < N_sr/tile_sz; i++) {
//             packet_reg = act_buffer[j][i];
//             act_buffer[j][i].ttl--;
//             packet_reg.src = weight_buffer[j].dst;
//             packet_reg.dst = weight_buffer[j].SA;
//             packet_reg.CB = 2*POD_SZ; 
//             packet_reg.d_type = 1;                    
//             packet_out.Push(packet_reg);                    
//             wait();

//             // a++;
//             // if(packet_reg.x == 0 && id == 0) cout <<  "a =  " << a << "\n";

//           }

//           weight_buffer[j].ttl--;
//         }

//         for(int j = 0; j < w_cnt; j++) {
//           if(weight_buffer[j].ttl == 0) {
//             w_cnt = 0;
//             weight_reuse = false;
//             break;
//           } 
//         }
//         // wait();
//         a_cnt = 0;
//       }

//       wait();
//     }
//   }
// };
// #endif







