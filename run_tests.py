import sys
import os
import subprocess
import time

import glob



arch1 = '''
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
const static bool DEBUG = 0;
const static int tile_sz = 2; // change to 8 later
const static double R = 1.5; // DRAM banwidth 
const static int alpha = (int) (1 / (R-1));
'''

arch2 = '''
const static int NUM_SA = Sx * Sy;
const static int NUM_LEVELS = (int) ceil(log(((double) NUM_SA)) / log(2));
const static int sx = 2;
const static int sy = 2;
const static int POD_SZ = sx * sy;
const static int NUM_PODS = (int) (NUM_SA / POD_SZ); // user can opt to use only a portion of the total
const static int M_ob = sy; // Size of block in the M dimension in terms of tiles
const static int K_ob = sx; // Size of operation block in the k dimension in terms of tiles
const static int N_ob = sx * alpha * NUM_PODS; // Size of operation block in N dimension in terms of tiles
'''

M=32
K=32
N=64


test_dims = [ [(2, 2), ('1', 'NUM_PODS')], 
              [(4, 2), ('1', 'NUM_PODS')], 
              [(4, 4), ('1', 'NUM_PODS')], 
              [(8, 4), ('1', 'NUM_PODS')], 
              [(8, 8), ('1', 'NUM_PODS')], 
              [(4, 2), ('NUM_PODS', '1')], 
              [(4, 4), ('NUM_PODS', '1')], 
              [(8, 4), ('NUM_PODS', '1')], 
              [(8, 8), ('NUM_PODS', '1')], 
              [(4, 4), ('2', '2')], 
              [(8, 8), ('4', '4')]]

f = open("results.txt", 'w')
f.write("number of SAs,number of packets,number of cycles,SRAM bw,M-K increase\n")
f.close()



exec_num = 0;
for i in xrange(len(test_dims)):
  a1 = '''
const static int Sx = %d;
const static int Sy = %d;
''' % (test_dims[i][0][0],test_dims[i][0][1])
  a2 = '''
const static int M_sr = M_ob * %s; 
const static int K_sr = K_ob * %s;
''' % (test_dims[i][1][0],test_dims[i][1][1])
  a3 = '''
const static int N_sr = N_ob;
const static int NUM_PODS_USED = (int) ((M_sr/M_ob) * (K_sr/K_ob)); 
const static int M = %d;
const static int K = %d;
const static int N = %d;
#endif
''' % (M,K,N)
  arch = arch1 + a1 + arch2 + a2 + a3
  os.remove("arch.h")
  f = open("arch.h", 'w')
  f.write(arch)
  f.close()
  cmd = 'make > /dev/null 2>&1; mv sim_test sim_test%d; ./sim_test%d > /dev/null 2>&1 &' % (exec_num, exec_num)
  subprocess.call(cmd, shell=True)
  exec_num += 1
    