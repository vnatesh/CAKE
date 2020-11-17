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

#ifndef __CCS_P2P_H
#define __CCS_P2P_H
/* This file implements a library of point-to-point, one or two-wire 
   handshaking interconnects for synchronizing blocks in SystemC.
   TLM views are implemented with TLM FIFO or derived from sc_interface.
   Synthesis views are implemented with signals of type sc_lv for 
   cross-language compatibility.
  
   All interconnects are implemented in a class containing three components.
    - chan: The interconnect object, which contains signals and any hardware 
            to connect two blocks.  Threads in SystemC can interact directly
            with channels.
    - in: The port used by an sc_module to read from the channel.
    - out: The port used by an sc_module to write to the channel.
  
   The "in" and "out" ports define an API for interacting with the channel 
   and the channel implements the API for both "in" and "out" ports.
  
   This file defines the following interconnects, including a synthesis(SYN) 
   and TLM view and any hardware that is required to implement the channel.  
   The interconnects are listed below and the definition of the API is defined
   before the implementation of each interconnect.
    - p2p:       Three signals used to directly connect two blocks using the
                  same protocol as a FIFO.  Defined in ccs_p2p_wire.h.
    - p2p_valid: Two signals used to directly connect two blocks without back
                 pressure.
    - p2p_fifo:  A FIFO used to connect two blocks that provides empty and full
                 status.
    - p2p_sync:  Two signals used to synchronize two blocks.  Both blocks must
                 reach the syncronization point before they can both continue
                 past it.
    - p2p_event: One signal used to syncronize two blocks without back pressure.
                 One block waits and the other notifies.
*/

#include "ccs_p2p/ccs_p2p_wire.h"
#include "ccs_p2p/ccs_p2p_valid.h"
#include "ccs_p2p/ccs_p2p_fifo.h"
#include "ccs_p2p/ccs_p2p_sync.h"
#include "ccs_p2p/ccs_p2p_event.h"

#endif
