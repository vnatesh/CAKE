#ifndef __PACKETSWITCH_H__
#define __PACKETSWITCH_H__

#include "arch.h"




SC_MODULE(PacketSwitch)
{
    public:
    sc_in_clk     clk;
    sc_in<bool>   rst;

    typedef NVINT8   WeightType;    
    typedef NVINT32  AccumType;
    typedef NVINT32  AddrType;
    typedef NVINT8   ID_type;
    typedef NVUINT1  Bcast;

    typedef typename nvhls::nv_scvector<nvhls::nv_scvector <AccumType, tile_sz>, tile_sz> VectorType;
    typedef typename nvhls::nv_scvector<Bcast, 2*NUM_SA - 1> BcastVectorType;
    typedef typename nvhls::nv_scvector<AddrType, NUM_LEVELS> AB_ChainType;



    class Packet: public nvhls_message {
      public:
        VectorType data;
        AddrType X;
        AddrType Y;
        AddrType Z;
        AddrType x;
        AddrType y;
        AddrType z;
        AddrType SB;
        AddrType SA;
        AB_ChainType AB;
        AddrType SRAM;
        AddrType src;
        AddrType dst;
        ID_type d_type; // weight (0), activation (1), result (2)
        BcastVectorType bcast;

        static const unsigned int width = ID_type::width + 11 * AddrType::width 
                                        + VectorType::width + BcastVectorType::width
                                        + AB_ChainType::width; // sizeof(int) * N;

        template <unsigned int Size>
        void Marshall(Marshaller<Size>& m) {
          m& data;
          m& X;
          m& Y;
          m& Z;
          m& x;
          m& y;
          m& z;
          m& SB;
          m& SA;
          m& AB;
          m& SRAM;
          m& src;
          m& dst;
          m& d_type;
          m& bcast;
        }
    };
    
    ID_type id; // id for the switch and associated AB
    ID_type level; // id for level of this switch in the h-tree

    Connections::In<Packet>   left_in;
    Connections::In<Packet>   right_in;
    Connections::In<Packet>   parent_in;
    Connections::In<Packet>   ab_in;

    Connections::Out<Packet>    left_out; 
    Connections::Out<Packet>    right_out;
    Connections::Out<Packet>    parent_out;
    Connections::Out<Packet>    ab_out;



    SC_HAS_PROCESS(PacketSwitch);
    PacketSwitch(sc_module_name name_) : sc_module(name_) {

      SC_THREAD (recv_parent); 
      sensitive << clk.pos(); 
      NVHLS_NEG_RESET_SIGNAL_IS(rst);

      SC_THREAD (recv_children); 
      sensitive << clk.pos(); 
      NVHLS_NEG_RESET_SIGNAL_IS(rst);
    }


    void recv_parent() {

      left_out.Reset();
      right_out.Reset();
      parent_in.Reset();
      wait(20.0, SC_NS); // wait for reset
      
      Packet p_in;

      while (1) {

        if(parent_in.PopNB(p_in)) {
          // if packet came from SRAM, send it down left/right children
          if(p_in.src == INT_MIN) {

            if(p_in.d_type == 1) {
              if(p_in.bcast[id]) {
                p_in.bcast[id] = 0;
                left_out.Push(p_in);
                right_out.Push(p_in);
              }
            } else if(p_in.d_type == 0) {
              if(((p_in.dst >> (NUM_LEVELS - level - 1)) & 1)) { // if bit is 1, go right
                right_out.Push(p_in);
              } else {
                left_out.Push(p_in);
              }
            }
          }
        } 

        wait();
      }
    }


    void recv_children() {

      left_in.Reset();
      right_in.Reset();
      ab_in.Reset();
      ab_out.Reset();
      parent_out.Reset();
      wait(20.0, SC_NS); // wait for reset
      
      Packet p_in;

      while(1) {

        if(left_in.PopNB(p_in)) {
          if(p_in.dst == id) {
            ab_out.Push(p_in);
          } else {
            parent_out.Push(p_in);
          }
        }

        if(right_in.PopNB(p_in)) {
          if(p_in.dst == id) {
            ab_out.Push(p_in);
          } else {
            parent_out.Push(p_in);
          }
        }

        if(ab_in.PopNB(p_in)) {
          parent_out.Push(p_in);          
        }

        wait();
      }
    }


};

#endif
