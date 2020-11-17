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

#ifndef __CCS_P2P_EVENT_H
#define __CCS_P2P_EVENT_H
#include <ccs_types.h>

//----------------------------------------------------------------------------
// Interconnect: p2p_event
//   One signal used to trigger a block from a controller.
//     <name>_vld:  Active high:  Active for one cycle when controller triggers target.
//   Underlying datatypes are bool for one-bit and sc_lv for vector types to allow clean 
//   mixed-language simulation.
//   Back pressure is not supported by this channel, so event is "missed" if target is not waiting.
//
//   The interconnect has a latency of one and supports a throughput of one.
//   Supported Methods:
//     out:
//         void nb_notify():  Notifies the target block.
//                          No status is returned, target is assumed to be waiting to be notified.
//      in:
//         void wait_for():  Blocks process until trigger is called by controller
//
//   Most common declaration examples, which result in transaction level simulation:
//     p2p_event<>::chan<> my_channel;
//     p2p_event<>::in<> my_input_port;
//     p2p_event<>::out<> my_output_port;
//   To declare pin-accurate interconnects use SYN template parameter:
//     p2p_event<SYN>::chan<> my_channel;
//     p2p_event<SYN>::in<> my_input_port;
//     p2p_event<SYN>::out<> my_output_port;



// Container Template
template <abstraction_t source_abstraction = AUTO>
class p2p_event {

#ifndef CALYPTO_SYSC
// Define a channel and ports to manage events
class simple_event_out_if: public sc_interface {
public:
  virtual void nb_notify() = 0;
};

class simple_event_in_if: public sc_interface {
public:
  virtual const sc_event& default_event() const = 0;
};

class simple_event
: public sc_prim_channel
, public simple_event_out_if
, public simple_event_in_if
{
public:
  explicit simple_event(sc_module_name name): sc_prim_channel (sc_gen_unique_name(name)) {}
  void nb_notify() { m_event.notify(); }
  const sc_event& default_event() const 
    { return m_event; }
private:
  sc_event m_event;
  simple_event (const simple_event& rhs) {}
};
#endif
public:
// Base Template Definition, override defaults for synthesis
// Do not overried setting on impl_abstraction template parameter!
// Always use SYN view for synthesis and formal
#if defined(__SYNTHESIS__) || defined (CALYPTO_SYSC)
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
  // Implemented with sc_event
  template <bool b>
  class chan <b, TLM> {
    p2p_checker wr_chk;

  public:
    simple_event event;

    chan(sc_module_name name = sc_gen_unique_name("p2p_event_chan")) 
      : wr_chk(name, "call reset_notify()", "send an event notification on this channel" )
      , event(ccs_concat(name, "event"))  
      {}

    void reset_wait_for() {}
    void reset_notify() { wr_chk.ok(); }

    void nb_notify() { wr_chk.test(); event.nb_notify(); }
    void wait_for() { wait(event.default_event()); }
    bool nb_valid() { 
      wait(event.default_event()); 
      return true;
    }  // Assumes "if(nb_event)" guards entire design
  };

  // TLM input port definition
  template <bool b>
  class in <b, TLM> {

  public:

    sc_port<simple_event_in_if> i_event;

    in(sc_module_name name = sc_gen_unique_name("p2p_event_in")) 
      : i_event(ccs_concat(name, "i_event"))
      {}

    void reset_wait_for() { }

    void wait_for() {wait(i_event->default_event()); }
    bool nb_valid() {
      wait(i_event->default_event());
      return true;
    }  // Assumes if (nb_event) guards entire design


    void bind (in<b,TLM> &x) { i_event(x.i_event); }
    template<class C>
    void bind (C &x) { i_event(x.event); }
    template <class C>
    void operator() (C &x) { bind(x); }
  };

  // TLM output port definition
  template <bool b>
  class out <b, TLM> {
    p2p_checker wr_chk;

  public:
    sc_port<simple_event_out_if> o_event;

    out(sc_module_name name = sc_gen_unique_name("p2p_event_out")) 
      : wr_chk(name, "call reset_notify()", "send an event notification on this port" )
      , o_event(ccs_concat(name,"o_event")) 
      {}

    void reset_notify(){ wr_chk.ok(); }

    void nb_notify() { wr_chk.test(); o_event->nb_notify(); }

    void bind (out<b,TLM> &x) { o_event(x.o_event); }
    template<class C>
    void bind (C &x) { o_event(x.event); }
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
      bind_out.reset_notify();
      bind_in.reset_wait_for();
      while(1) {
        bind_in.wait_for();
        bind_out.nb_notify();
      }
    }
  };
#endif

  // SYN channel definition
  template <bool b>
  class chan <b, SYN> {
    p2p_checker wr_chk;
  public:
    sc_signal <bool> vld ;

    chan(sc_module_name name = sc_gen_unique_name("p2p_event_chan")) 
      : wr_chk(name, "call reset_notify()", "send an event notification on this channel" )
      , vld( ccs_concat(name,"vld") ) 
      {}

    void reset_notify() {
      vld.write(false);
      wr_chk.ok();
    }
    void reset_wait_for() {}

    #pragma design modulario<sync>
    void wait_for() {
      do {
        wait() ;
      } while (vld.read() != true );
    }

    // Assumes "if(nb_event)" guards entire design
    #pragma design modulario<sync>
    bool nb_valid() {
      wait();
      return vld.read();
    }

    #pragma design modulario<sync>
    void nb_notify () {
      wr_chk.test();
      vld.write(true);
      wait();
      vld.write(false);
    }
  };

  // SYN input port definition
  template <bool b>
  class in <b, SYN> {
  public:
    sc_in <bool> i_vld ;

    in(sc_module_name name = sc_gen_unique_name("p2p_event_in")) :
      i_vld( ccs_concat(name,"i_vld") ) {}

    void reset_wait_for() {}

    #pragma design modulario<sync>
    void wait_for() {
      do {
        wait() ;
      } while (i_vld.read() != true );
    }

    // Assumes "if(nb_event)" guards entire design
    #pragma design modulario<sync>
    bool nb_valid() {
      wait();
      return i_vld.read();
    }

    void bind (in<b,SYN>& c) { i_vld(c.i_vld); }

    template <class C>
    void bind (C& c) { i_vld(c.vld); }

    template <class C>
    void operator() (C& c) { bind(c); }
  };

  // SYN output port definition
  template <bool b>
  class out <b, SYN> {
    p2p_checker wr_chk;

  public:
    sc_out <bool> o_vld ;

    out(sc_module_name name = sc_gen_unique_name("p2p_event_out")) 
      : wr_chk(name, "call reset_notify()", "send an event notification on this port" )
      , o_vld( ccs_concat(name,"o_vld") ) 
      {}

    void reset_notify() {
      o_vld.write(false);
      wr_chk.ok();
    }

    #pragma design modulario<sync>
    void nb_notify () {
      wr_chk.test();
      o_vld.write(true);
      wait();
      o_vld.write(false);
    }

    void bind (out<b,SYN>& c) { o_vld(c.o_vld); }

    template <class C>
    void bind (C& c) { o_vld(c.vld); }

    template <class C>
    void operator() (C& c) { bind(c); }
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
    }

    void assign_vld() { bind_out.o_vld.write(bind_in.i_vld.read()); }
  };

};
#endif
