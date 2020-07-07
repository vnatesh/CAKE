import sys
import os
import subprocess


arch = '''#ifndef __ARCH_H__
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
const static int NUM_CB = 1;
const static int NUM_SRAM = 1;
const static int Dx = alpha*P*P*tile_sz; // Size of operation block in N dimension i.e alpha*Wz
const static int Dz = P*P*tile_sz; // Size of operation block in the k dimension
const static int Wz = P*P*tile_sz; // Size of operation block in the K dimension, same as Dz
const static int Wy = Wz; // Size of block in the M dimension
const static int M_sr = Wy;
const static int K_sr = Wz;
const static int N_sr = Dx;
'''


for m in range(1,4):
	for k in range(1,4):
		for n in range(1,4):
			a = arch + '''
const static int M_dr = M_sr*%d;
const static int K_dr = K_sr*%d;
const static int N_dr = N_sr*%d;

const static int M = M_dr;
const static int K = K_dr;
const static int N = N_dr;
#endif\n\n''' % (m,k,n)
			os.remove("arch.h")
			f = open("arch.h", 'w')
			f.write(a)
			subprocess.call(['make > /dev/null 2>&1; make run > /dev/null 2>&1'], shell=True)
			f.close()

# for m in range(1,4):
# 	for k in range(1,4):
# 		for n in range(1,4):
# 			for M in range(1,4):
# 				for K in range(1,4):
# 					for N in range(1,4):
# 						a = arch + '''
# const static int M_dr = M_sr*%d;
# const static int K_dr = K_sr*%d;
# const static int N_dr = N_sr*%d;

# const static int M = M_dr*%d;
# const static int K = K_dr*%d;
# const static int N = N_dr*%d;
# #endif\n\n''' % (m,k,n,M,K,N)
# 						os.remove("arch.h")
# 						f = open("arch.h", 'w')
# 						f.write(a)
# 						subprocess.call(['make > /dev/null 2>&1; make run > /dev/null 2>&1'], shell=True)
# 						f.close()


f = open("results.txt", 'r')
results = f.read().split()
if(all([i == '1' for i in results])):
	print "ALL TESTS PASSED"
else:
	print "FAILED"

