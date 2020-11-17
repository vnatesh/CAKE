//------------------------------------------------------------------------------
// Catapult Synthesis - Sample I/O Port Library
//
// Copyright (c) 2003-2015 Mentor Graphics Corp.
//       All Rights Reserved
//
// This document may be used and distributed without restriction provided that
// this copyright statement is not removed from the file and that any derivative
// work contains this copyright notice.
//
// The design information contained in this file is intended to be an example
// of the functionality which the end user may study in preparation for creating
// their own custom interfaces. This design does not necessarily present a 
// complete implementation of the named protocol or standard.
//
//------------------------------------------------------------------------------

#ifndef __CCS_P2P_SYNC_H
#define __CCS_P2P_SYNC_H
#include <ccs_types.h>

//----------------------------------------------------------------------------
// Interconnect: p2p_sync
//   Two signals used to directly connect two blocks for a peer-to-peer handshake
//     <name>_vld:  Active high: Driven active by the source when it is ready
//     <name>_rdy:  Active high: Driven active by the target when it is ready
//   Causes source or target to stall until both blocks are ready.
//   Underlying datatypes are bool for one-bit and sc_lv for vector types to allow clean
//   mixed-language simulation.
//
//   The interconnect has a latency of one and supports a throughput of one.
//   Supported Methods:
//     out:
//         start(): Stalls design until source is done.
//      in:
//         done():  Stalls design until target is ready to start.
//   Most common declaration examples, which result in transaction level simulation:
//     p2p_sync<>::chan<> my_channel;
//     p2p_sync<>::in<> my_input_port;
//     p2p_sync<>::out<> my_output_port;
//   To declare pin-accurate interconnects use SYN template parameter:
//     p2p_sync<SYN>::chan<> my_channel;
//     p2p_sync<SYN>::in<> my_input_port;
//     p2p_sync<SYN>::out<> my_output_port;

// Container Template
template <abstraction_t source_abstraction = AUTO>
class p2p_sync {

public:
// Base Template Definition, override defaults for synthesis
// Do not overried setting on impl_abstraction template parameter!
// Always use SYN view for synthesis and formal
// NOTE: "b" parameter added to work around limitations in C++.
//       Can't fully specialize templates in templatized class.
#if defined(__SYNTHESIS__)|| defined (CALYPTO_SYSC)
  template <bool b = false, abstraction_t impl_abstraction = SYN> class chan {};
  template <bool b = false, abstraction_t impl_abstraction = SYN> class assign {};
  template <bool b = false, abstraction_t impl_abstraction = SYN> class in {};
  template <bool b = false, abstraction_t impl_abstraction = SYN> class out {};
// Default to SYN view in SCVerify.  Channels connecting testbench blocks should be hard-coded to TLM.
#elif defined (CCS_DUT_CYCLE) || defined (CCS_DUT_RTL)
  template <bool b = false, abstraction_t impl_abstraction = (source_abstraction==AUTO)? SYN : source_abstraction > class chan {};
  template <bool b = false, abstraction_t impl_abstraction = (source_abstraction==AUTO)? SYN : source_abstraction > class assign {};
  template <bool b = false, abstraction_t impl_abstraction = (source_abstraction==AUTO)? SYN : source_abstraction > class in {};
  template <bool b = false, abstraction_t impl_abstraction = (source_abstraction==AUTO)? SYN : source_abstraction > class out {};
#else
  template <bool b = false, abstraction_t impl_abstraction = (source_abstraction==AUTO)? P2P_DEFAULT_VIEW : source_abstraction > class chan {};
  template <bool b = false, abstraction_t impl_abstraction = (source_abstraction==AUTO)? P2P_DEFAULT_VIEW : source_abstraction > class assign {};
  template <bool b = false, abstraction_t impl_abstraction = (source_abstraction==AUTO)? P2P_DEFAULT_VIEW : source_abstraction > class in {};
  template <bool b = false, abstraction_t impl_abstraction = (source_abstraction==AUTO)? P2P_DEFAULT_VIEW : source_abstraction > class out {};
#endif
#ifndef CALYPTO_SYSC
  // TLM channel definition
  // NOTE:  Minimum FIFO depth is one or SystemC simulation will hang!
  // Implemented using TLM fifo for consistency with other p2p types
  template <bool b>
  class chan <b, TLM> {
    p2p_checker rd_chk, wr_chk;

  public:
    tlm::tlm_fifo<bool> fifo;

    chan(sc_module_name name = sc_gen_unique_name("p2p_sync_chan")) 
      : rd_chk(name, "call reset_sync_in()", "synchronize from this channel")
      , wr_chk(name, "call reset_sync_out()", "synchronize to this channel")
      , fifo(ccs_concat(name, "fifo"), 1)  
      {}

    // Empty FIFO on reset
    void reset_sync_in() { 
      bool temp; 
      while (fifo.nb_get(temp)); 
      rd_chk.ok();
    }
    void reset_sync_out() {wr_chk.ok();}

    bool nb_sync_in() { 
      rd_chk.test();
      bool temp; 
      return fifo.nb_get(temp); 
    }

    void sync_in() { 
      rd_chk.test();
      fifo.get(); 
    }

    void sync_out() { 
      wr_chk.test();
      bool temp; 
      fifo.put(temp); 
      wait(fifo.ok_to_put()); 
    }
  };

  // TLM input port definition
  template <bool b>
  class in <b, TLM> {
    p2p_checker rd_chk;
  public:
    sc_port<tlm::tlm_fifo_get_if<bool> > i_fifo;

    in(sc_module_name name = sc_gen_unique_name("p2p_sync_in")) 
      : rd_chk(name, "call reset_sync_in()", "synchronize from this port")
      , i_fifo(ccs_concat(name, "i_fifo"))
      {}

    // Empty FIFO on reset
    void reset_sync_in() { 
      bool temp; 
      while (i_fifo->nb_get(temp)); 
      rd_chk.ok();
    }

    bool nb_sync_in() { 
      rd_chk.test();
      bool temp; 
      return i_fifo->nb_get(temp); 
    }

    void sync_in() { 
      rd_chk.test();
      i_fifo->get(); 
    }

    void bind (in<b,TLM> &x) { i_fifo(x.i_fifo); }
    template<class C>
    void bind (C &x) { i_fifo(x.fifo); }
    template <class C>
    void operator() (C &x) { bind(x); }
  };

  // TLM output port definition
  template <bool b>
  class out <b, TLM> {
    p2p_checker wr_chk;

  public:
    sc_port<tlm::tlm_fifo_put_if<bool> > o_fifo;

    out(sc_module_name name = sc_gen_unique_name("p2p_sync_out")) 
      : wr_chk(name, "call reset_sync_out()", "synchronize to this port")
      , o_fifo(ccs_concat(name,"o_fifo")) 
      {}

    void reset_sync_out(){wr_chk.ok();}

    void sync_out() { 
      wr_chk.test();
      bool temp; 
      o_fifo->put(temp); 
      wait(o_fifo->ok_to_put());
    }

    void bind (out<b,TLM> &x) { o_fifo(x.o_fifo); }
    template<class C>
    void bind (C &x) { o_fifo(x.fifo); }
    template <class C>
    void operator() (C &x) { bind(x); }
  };

  // TLM assignemnt definition
  // Used to connect in->chan, chan->chan, chan->out
  template < bool b >
  struct assign <b, TLM> : ::sc_core::sc_module {
    in<b,TLM> bind_in;
    out<b,TLM> bind_out;

    SC_CTOR (assign)
      :  bind_in ("bind_in")
      ,  bind_out ("bind_out")
    {
      SC_THREAD(pass_through);
    }

    void pass_through () {
      bind_out.reset_sync_out();
      bind_in.reset_sync_in();
      while(1) {
        bind_in.sync_in();
        bind_out.sync_out();
      }
    }
  };
#endif

  // SYN channel definition
  template <bool b>
  class chan <b, SYN> {
    p2p_checker rd_chk, wr_chk;

  public:
    sc_signal <bool> vld ;
    sc_signal <bool> rdy ;

    chan(sc_module_name name = sc_gen_unique_name("p2p_sync_chan")) 
      : rd_chk(name, "call reset_sync_out()", "synchronize from this channel")
      , wr_chk(name, "call reset_sync_in()", "synchronize to this channel")
      , vld( ccs_concat(name,"vld") )
      , rdy( ccs_concat(name,"rdy") ) 
      {}

    void reset_sync_out() {
      vld.write(false);
      wr_chk.ok();
    }
    void reset_sync_in() {
      rdy.write(false);
      rd_chk.ok();
    }

    #pragma design modulario<sync>
    bool nb_sync_in() {
      rd_chk.test();
      rdy.write(true);
      wait();
      rdy.write(false);
      return vld.read();
    }

    #pragma design modulario<sync>
    void sync_in() {
      rd_chk.test();
      do {
        rdy.write(true) ;
        wait() ;
      } while (vld.read() != true );
      rdy.write(false);
    }

    #pragma design modulario<sync>
    void sync_out() {
      wr_chk.test();
      do {
        vld.write(true);
        wait();
      } while (rdy.read() != true );
      vld.write(false);
    }

  };

  // SYN input port definition
  template <bool b>
  class in <b, SYN> {
    p2p_checker rd_chk;

  public:
    sc_in <bool> i_vld ;
    sc_out <bool> o_rdy ;

    in(sc_module_name name = sc_gen_unique_name("p2p_sync_in")) 
      : rd_chk(name, "call reset_sync_in()", "synchronize from this port")
      , i_vld( ccs_concat(name,"i_vld") )
      , o_rdy( ccs_concat(name,"o_rdy") ) 
      {}

    void reset_sync_in() {
      o_rdy.write(false);
      rd_chk.ok();
    }

    #pragma design modulario<sync>
    bool nb_sync_in() {
      rd_chk.test();
      o_rdy.write(true);
      wait();
      o_rdy.write(false);
      return i_vld.read();
    }

    #pragma design modulario<sync>
    void sync_in() {
      rd_chk.test();
      do {
        o_rdy.write(true) ;
        wait() ;
      } while (i_vld.read() != true );
      o_rdy.write(false);
    }

    void bind (in<b,SYN>& c) {
      i_vld(c.i_vld);
      o_rdy(c.o_rdy);
    }

    template <class C>
    void bind (C& c) {
      i_vld(c.vld);
      o_rdy(c.rdy);
    }

    template <class C>
    void operator() (C& c) {
      bind(c);
    }

  };

  // SYN output port definition
  template <bool b>
  class out <b, SYN> {
    p2p_checker wr_chk;

  public:
    sc_out <bool> o_vld ;
    sc_in <bool> i_rdy ;

    out(sc_module_name name = sc_gen_unique_name("p2p_sync_out")) 
      : wr_chk(name, "call reset_sync_out()", "synchronize to this port")
      , o_vld( ccs_concat(name,"o_vld") )
      , i_rdy( ccs_concat(name,"i_rdy") ) 
      {}

    void reset_sync_out() {
      o_vld.write(false);
      wr_chk.ok();
    }

    #pragma design modulario<sync>
    void sync_out() {
      wr_chk.test();
      do {
        o_vld.write(true);
        wait();
      } while (i_rdy.read() != true );
      o_vld.write(false);
    }

    void bind (out<b,SYN>& c) {
      o_vld(c.o_vld);
      i_rdy(c.i_rdy);
    }

    template <class C>
    void bind (C& c) {
      o_vld(c.vld);
      i_rdy(c.rdy);
    }

    template <class C>
    void operator() (C& c) {
      bind(c);
    }
  };

  // SYN assignemnt definition
  // Used to connect in->chan, chan->chan, chan->out
  #pragma ungroup
  template < bool b >
  struct assign <b, SYN> : ::sc_core::sc_module {
    in<b,SYN> bind_in;
    out<b,SYN> bind_out;

    SC_CTOR (assign)
      :  bind_in ("bind_in")
      ,  bind_out ("bind_out")
    {
      SC_METHOD(assign_vld);
      sensitive << bind_in.i_vld;
      SC_METHOD(assign_rdy);
      sensitive << bind_out.i_rdy;
    }

    void assign_vld() { bind_out.o_vld.write(bind_in.i_vld.read()); }
    void assign_rdy() { bind_in.o_rdy.write(bind_out.i_rdy.read()); }
  };

};
#endif
