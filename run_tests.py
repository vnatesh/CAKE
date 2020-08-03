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


S = [(2,2),(4,2),(4,4),(8,4),(8,8)]
M_sr = ['1', 'NUM_PODS']
K_sr = ['NUM_PODS', '1']


S = [(2,2),(4,4),(8,8)]
M_sr = ['NUM_PODS/2', 'NUM_PODS/2']
K_sr = ['NUM_PODS/2', 'NUM_PODS/2']


exec_num = 0;
for i in xrange(len(M_sr)):
  for j in S:
    a1 = '''
const static int Sx = %d;
const static int Sy = %d;
''' % (j[0],j[1])
    a2 = '''
const static int M_sr = M_ob * %s; 
const static int K_sr = K_ob * %s;
''' % (M_sr[i],K_sr[i])
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
    







MAX = 3

# for m in range(1,MAX+1):
#   for k in range(1,MAX+1):
#     for n in range(1,MAX+1):
#       a = arch + '''
# const static int M_sr = M_ob;
# const static int K_sr = K_ob;
# const static int N_sr = N_ob;

# const static int M = M_sr*%d;
# const static int K = K_sr*%d;
# const static int N = N_sr*%d;
# #endif\n\n''' % (m,k,n)
#       os.remove("arch.h")
#       f = open("arch.h", 'w')
#       f.write(a)
#       subprocess.call(['make > /dev/null 2>&1; make run > /dev/null 2>&1 &'], shell=True)
#       f.close()

for m in range(1,MAX+1):
  for k in range(1,MAX+1):
    for n in range(1,MAX+1):
      for M in range(1,MAX+1):
        for K in range(1,MAX+1):
          for N in range(1,MAX+1):
            a = arch + '''
const static int M_sr = M_ob*%d;
const static int K_sr = K_ob*%d;
const static int N_sr = N_ob*%d;
const static int NUM_PODS_USED = (int) ((M_sr/M_ob) * (K_sr/K_ob)); 
const static int M = M_sr*%d;
const static int K = K_sr*%d;
const static int N = N_sr*%d;
#endif\n\n''' % (m,k,n,M,K,N)
            os.remove("arch.h")
            f = open("arch.h", 'w')
            f.write(a)
            subprocess.call(['make > /dev/null 2>&1; make run > /dev/null 2>&1 &'], shell=True)
            f.close()


time.sleep(320)


f = open("results.txt", 'r')
results = f.read().split()
if(all([i == '1' for i in results]) and len(results) == MAX**6):
  print "ALL TESTS PASSED"
else:
  print "FAILED"

