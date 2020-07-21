import sys
import os
import subprocess
import time

arch = '''
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
'''

MAX = 3

# for m in range(1,MAX+1):
# 	for k in range(1,MAX+1):
# 		for n in range(1,MAX+1):
# 			a = arch + '''
# const static int M_sr = M_ob;
# const static int K_sr = K_ob;
# const static int N_sr = N_ob;

# const static int M = M_sr*%d;
# const static int K = K_sr*%d;
# const static int N = N_sr*%d;
# #endif\n\n''' % (m,k,n)
# 			os.remove("arch.h")
# 			f = open("arch.h", 'w')
# 			f.write(a)
# 			subprocess.call(['make > /dev/null 2>&1; make run > /dev/null 2>&1 &'], shell=True)
# 			f.close()

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

