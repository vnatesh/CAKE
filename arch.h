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


/*

Let's assume the number of blocks is very big.

Given the number of systolic arrays and the dram bandwidth, figure out how
much SRAM is needed to keep everything busy.

plot curve, x axis is the DRAM bandwidth and
y axis the minimum SRAM size to have high usage. For some number of
systolic arrays, we have a plot/ a line. Then add a sa... add another line.
Show how more SAs need more SRAM and relation to DRAM and SRAM sizes. Try and
get the function. Later on we can come up with the math. Not terribly important
yet.

Pod grows with systolic, with a factor of 4.


notes from 6/8/2020
Blocking and waiting, hard to know deterministic without running it.
HT says its deterministic but is a waste of time.
DRAM may hog the switch, so its not available.

Measure by checking everything going out of SA, know how many computations you have done.
Put some counters on the sender side, if receiver only receives 90 then you know theres a block
Now we have to figure out where the counters should go.

HT says make some assumptions to ensure that the program runs.

*/


/* examples:
    P = 2:    2*2 = 4 pods, each pod has 2x2 SAs    (16 SAs)
    P = 4:    4*4 = 16 pods, each pod has 4x4 SAs   (256 SAs)
    P = 8:    8*8 = 64  pods, each pod has 8x8 SAs  (4096 SAs)
*/

// Define P, each pod has (P,P) element pairs
// Number of processing elements is Wz* Wy
// const static int P = 4;
const static int P = 2;
const static int alpha = 2;
const static int tile_sz = 2; // change to 8 later
const static int NUM_PODS = P*P;
const static int POD_SZ = P*P;
const static int NUM_CB = 1;
const static int NUM_SRAM = 1;

const static int Dx = alpha*P*P*tile_sz; // Size of data block in N dimension i.e alpha*Wz
const static int Dz = P*P*tile_sz; // Size of data block in the k dimension
const static int Wz = P*P*tile_sz; // Size of block in the K dimension, same as Dz
const static int Wy = Wz; // Size of block in the M dimension


// size of SRAM block
const static int M_sr = Wy;
const static int K_sr = Wz;
const static int N_sr = Dx;
// const static int M_sr = Wy*4; // for P = 4
// const static int K_sr = Wz*4;
// const static int N_sr = Dx*4;

// size of DRAM block
const static int M_dr = M_sr*3;
const static int K_dr = K_sr*3;
const static int N_dr = N_sr*3;

// size of full matrices to be multiplied in the current layer
const static int M = M_dr*2;
const static int K = K_dr*2;
const static int N = N_dr*2;

#endif

