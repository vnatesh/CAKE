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
std::string PERF_FILE = "exp1fig1";
std::string WEIGHT_FILE = "weights";
std::string DATA_FILE = "data";
std::string RESULT_FILE = "result";
static const char delims[] = " ";

const static bool DEBUG = 0;
const static bool LOG = 0;
const static bool USE_FILE = 0;

const static int tile_sz = 2; // change to 8 later
const static double R = 2; // DRAM bandwidth 
const static int alpha = (int) (1 / (R-1));
const static int lat_dram = 1; // DRAM latency 
const static int lat_internal = 1; // internal latency 
// const static double lat_internal_factor = 4.000000; 
const static char bw_growth = 'I'; // constant vs increasing     

const static int Sx = 8;
const static int Sy = 8;
const static int NUM_SA = Sx * Sy;
const static int NUM_LEVELS = (int) ceil(log(((double) NUM_SA)) / log(2));

const static int sx = 4;
const static int sy = 4;
const static int POD_SZ = sx * sy;
const static int NUM_PODS = (int) (NUM_SA / POD_SZ); // user can opt to use only a portion of the total
const static int pod_ab_hops = (int) ceil(log(((double) sx)) / log(2));

const static int M_ob = sy; // Size of block in the M dimension in terms of tiles
const static int K_ob = sx; // Size of operation block in the k dimension in terms of tiles
const static int N_ob = sx * alpha * NUM_PODS ; // Size of operation block in N dimension in terms of tiles

const static int M_sr = M_ob * NUM_PODS; 
const static int K_sr = K_ob * 1;
const static int N_sr = N_ob;

const static int NUM_PODS_USED = (int) ((M_sr/M_ob) * (K_sr/K_ob)); 
const static int M = M_sr;
const static int K = K_sr*2;
const static int N = N_sr*3;
#endif
