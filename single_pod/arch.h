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
const static bool DEBUG = false;
const static int P = 2;
const static int alpha = 2;
const static int tile_sz = 2; // change to 8 later
const static int NUM_PODS = P*P;
const static int POD_SZ = P*P;
const static int NUM_AB = 1;
const static int NUM_SRAM = 1;

// All shapes are in terms of tiles
const static int Dx = alpha*P*P; // Size of operation block in N dimension i.e alpha*Wz
const static int Dz = P*P; // Size of operation block in the k dimension
const static int Wz = P*P; // Size of operation block in the K dimension, same as Dz
const static int Wy = Wz; // Size of block in the M dimension

const static int M_ob = Wy;
const static int K_ob = Wz;
const static int N_ob = Dx;

const static int M_sr = M_ob*3;
const static int K_sr = K_ob*3;
const static int N_sr = N_ob*3;

const static int M = M_sr*3;
const static int K = K_sr*3;
const static int N = N_sr*6;
#endif

