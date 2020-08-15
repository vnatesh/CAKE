#ifndef __SA_H__
#define __SA_H__

#include "PacketSwitch.h"
#include "matops.h"


SC_MODULE(SA)
{
  public:
  sc_in_clk     clk;
  sc_in<bool>   rst;

  // Interface Ports
  Connections::In<PacketSwitch::Packet>   packet_in;
  Connections::Out<PacketSwitch::Packet>  packet_out;
  PacketSwitch::ID_type id;

  // perf counters
  double idle_time = 0;
  double io_time = 0;
  double compute_time = 0;
  int mult_cnt = 0;
  int wait_cnt = 0;


  SC_HAS_PROCESS(SA);
  SA(sc_module_name name_) : sc_module(name_) {
    SC_THREAD (run);
    sensitive << clk.pos();
    NVHLS_NEG_RESET_SIGNAL_IS(rst);
  }


  void run() {

    packet_in.Reset();
    packet_out.Reset();
    wait(20.0, SC_NS);

    bool is_act_in = 0;

    // int r = 0;

    PacketSwitch::Packet packet_reg;
    PacketSwitch::Packet weight; 
    PacketSwitch::Packet activation; 

    // timer for counters
    sc_time start, end;

    #pragma hls_pipeline_init_interval 1
    while(1) {

      start = sc_time_stamp();
      
      if (packet_in.PopNB(packet_reg)) {
          if(packet_reg.d_type == 0) { // weights
           
            if(DEBUG) cout << "SA " << packet_reg.dst << " receive weight from SB " << "\n";
            weight = packet_reg;
            id = packet_reg.dst;

          } else if(packet_reg.d_type == 1 && is_act_in == 0) { // activation
            
            if(DEBUG) cout << "SA " << id << " receive activation from SB " << "\n";
            activation = packet_reg;
            is_act_in = 1;                                    
          }
      }

      end = sc_time_stamp();
      io_time += (end - start).to_default_time_units();

      if(is_act_in) { // do matmul and send result

        // packet reg at this point contains the activation header. This allows outgoing 
        // result packet to automatically contain Y,Z,x,y,z dims
        start = sc_time_stamp();
        packet_reg.data = TileMul(weight.data, activation.data); 
        mult_cnt++;
        packet_reg.X = weight.X; // set X val in packet to that of weight. Now all dims are in result header
        packet_reg.src = id;
        packet_reg.dst = packet_reg.AB[0]; // send packet to parent switch AB i.e., the 0th ind in the AB chain
        packet_reg.d_type = 2; // result type  
        if(DEBUG) cout << "SA " << packet_reg.src << " sending result to AB\n";
        packet_out.Push(packet_reg);
        // r++;
        // if(packet_reg.x == 0 && id == POD_SZ) cout << "r_cnt = " << r << " time " << sc_time_stamp() << "\n";
        wait();
        end = sc_time_stamp();
        compute_time += (end - start).to_default_time_units();
        is_act_in = 0;
      } 

      else {
        start = sc_time_stamp();
        wait();
        end = sc_time_stamp();
        idle_time += (end - start).to_default_time_units();
        wait_cnt++;
      }
    }
  }
};
#endif


