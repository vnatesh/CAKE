#ifndef __ARCH_H__
#define __ARCH_H__
#include <systemc.h>
#include <nvhls_int.h>
#include <nvhls_connections.h>
#include <nvhls_vector.h>
#include <ArbitratedScratchpad.h>
#include <ArbitratedScratchpad/ArbitratedScratchpadTypes.h>
#include <sys/time.h>
#include<bits/stdc++.h>


ofstream logfile;

const static bool DEBUG = 0;
const static int tile_sz = 2; // change to 8 later

const static double R = 2; // DRAM banwidth 
const static int alpha = (int) (1 / (R-1));
const static int lat_dram = 48; // DRAM latency 
const static int lat_internal = 16; // internal latency 

// number of SAs on H-tree (square 2d array of SAs)
const static int Sx = 4;
const static int Sy = 4;
const static int NUM_SA = Sx * Sy;
const static int NUM_LEVELS = (int) ceil(log(((double) NUM_SA)) / log(2));

// shape of a pod sx x sy. For now, only 2x2 pods supported
const static int sx = 2;
const static int sy = 2;
const static int POD_SZ = sx * sy;
const static int NUM_PODS = (int) (NUM_SA / POD_SZ); // user can opt to use only a portion of the total
const static int SR_sz_factor = NUM_PODS;

// An operation block is sized/shaped to a pod and available DRAM bw
const static int M_ob = sy; // Size of block in the M dimension in terms of tiles
const static int K_ob = sx; // Size of operation block in the k dimension in terms of tiles
const static int N_ob = sx * alpha * SR_sz_factor; // Size of operation block in N dimension in terms of tiles

const static int M_sr = M_ob * SR_sz_factor; 
const static int K_sr = K_ob;
const static int N_sr = N_ob;

// the number of pods the user actually uses is based on the SRAM block shape
const static int NUM_PODS_USED = (int) ((M_sr/M_ob) * (K_sr/K_ob)); 

// Full MM block is a 3d collection of SRAM blocks
const static int M = M_sr*2;
const static int K = K_sr;
const static int N = N_sr;


#endif
