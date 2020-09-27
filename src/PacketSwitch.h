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
    typedef NVINT32   ID_type;
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
        AddrType cycle_cnt;
        ID_type d_type; // weight (0), activation (1), result (2)
        BcastVectorType bcast;

        static const unsigned int width = ID_type::width + 12 * AddrType::width 
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
          m& cycle_cnt;
          m& d_type;
          m& bcast;
        }
    };

    // AddrType ab_id; // id for the associated AB

    
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

    Connections::Out<Packet>    left_partial_out; 
    Connections::Out<Packet>    right_partial_out;
    Connections::In<Packet>     parent_partial_in;
    Connections::Out<Packet>    ab_partial_out;


    SC_HAS_PROCESS(PacketSwitch);
    PacketSwitch(sc_module_name name_) : sc_module(name_) {

      SC_THREAD (recv_parent); 
      sensitive << clk.pos(); 
      NVHLS_NEG_RESET_SIGNAL_IS(rst);

      SC_THREAD (recv_children); 
      sensitive << clk.pos(); 
      NVHLS_NEG_RESET_SIGNAL_IS(rst);

      SC_THREAD (recv_partial); 
      sensitive << clk.pos(); 
      NVHLS_NEG_RESET_SIGNAL_IS(rst);

    }



    void recv_partial() {

      left_partial_out.Reset();
      right_partial_out.Reset();
      parent_partial_in.Reset();
      ab_partial_out.Reset();
      wait(20.0, SC_NS); // wait for reset
      
      Packet p_in;

      while (1) {

        if(parent_partial_in.PopNB(p_in)) {
          // if packet came from SRAM, send it down left/right children
          if(p_in.src == INT_MIN && p_in.d_type == 2) {
            if(p_in.dst == id) {
              ab_partial_out.Push(p_in);        // send partial result to AB
            } else {

              int dst = p_in.dst;
              while(1) {
                if(dst == (2*id + 1)) { // if addr is left child, go left
                  left_partial_out.Push(p_in);
                  break;
                } else if(dst == (2*id + 2)) {   // if addr is right child, go right
                  right_partial_out.Push(p_in);
                  break;
                } else {
                  dst = (dst - 1) / 2; // parent addr
                }
              }
            }
          }
        }

        wait();
      }
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

            // wait(lat_internal);  
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
      
      Packet p_in_left;
      Packet p_in_right;
      Packet p_in_ab;


      int turn = 3;

      // read first from right before left since left SA processes packets before right SA
      while(1) {

        if(turn % 3 == 0) {

          if(right_in.PopNB(p_in_right)) {
            if(p_in_right.dst == id) {
              ab_out.Push(p_in_right);
            } else {
              parent_out.Push(p_in_right);
            }
          }

          turn = 1;

        } else if (turn % 3 == 1) {

          if(left_in.PopNB(p_in_left)) {
            if(p_in_left.dst == id) {
              ab_out.Push(p_in_left);
            } else {
              parent_out.Push(p_in_left);
            }
          }

          turn = 2;

        } else if(turn % 3 == 2) {

          if(ab_in.PopNB(p_in_ab)) {
            parent_out.Push(p_in_ab); 
          }

          turn = 3;
        }

        wait();
      }
    }





};

#endif
