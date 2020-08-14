import argparse
import sys
import os
import subprocess
import time
import numpy as np
import glob


def build_arch(M, N, K, Sx, Sy, R, lat_dram):
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
'''
	arch2 = '''
const static int alpha = (int) (1 / (R-1));
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
	a1 = '''
const static int Sx = %d;
const static int Sy = %d;
''' % (Sx, Sy)
	a2 = '''
const static double R = %.2f; 
const static int lat_dram = %d; 
''' % (R,lat_dram)
	a3 = '''
const static int M_sr = M_ob * %s; 
const static int K_sr = K_ob * %s;
''' % ('NUM_PODS', '1')
	a4 = '''
const static int N_sr = N_ob;
const static int NUM_PODS_USED = (int) ((M_sr/M_ob) * (K_sr/K_ob)); 
const static int M = %d;
const static int K = %d;
const static int N = %d;
#endif
''' % (M, K, N)
	return arch1 + a1 + a2 + arch2 + a3 + a4


def mm_simulator():
	parser = argparse.ArgumentParser(description='Matrix Multiplication Simulator')
	requiredNamed = parser.add_argument_group('required named arguments')
	requiredNamed.add_argument('-M', action="store", dest="M", type=int, required=True, help = "size of the M dimension of the computation")
	requiredNamed.add_argument('-N', action="store", dest="N", type=int, required=True, help = "size of the N dimension of the computation")
	requiredNamed.add_argument('-K', action="store", dest="K", type=int, required=True, help = "size of the K dimension of the computation")
	parser.add_argument('-w', action="store", dest="w", help = "use values in file W as the weight matrix for the MM computation")
	parser.add_argument('-d', action="store", dest="d", help = "use values in file D as the data matrix for the MM computation")
	requiredNamed.add_argument('--Sx', action="store", dest="Sx", type=int, required=True, help = "architecture has Sx element pairs in the x dimension")
	requiredNamed.add_argument('--Sy', action="store", dest="Sy", type=int, required=True, help = "architecture has Sy element pairs in the y dimension")
	requiredNamed.add_argument('-R', action="store", dest="R", type=float, required=True, help = "architecture has a DRAM bandwidth of R units")
	args = parser.parse_args()
	# print args
	sx=2
	sy=2
	bw_dr = args.R*sx*sy
	lcm = 120
	lat_dram = lcm / bw_dr
	f = open("results.txt", 'w')
	f.write("number of SAs,number of packets,number of cycles,SRAM bw,compute cycles,dram bw,M-K increase\n")
	f.close()
	arch = build_arch(args.M, args.N, args.K, args.Sx, args.Sy, args.R, lat_dram)
	os.remove("arch.h")
	f = open("arch.h", 'w')
	f.write(arch)
	f.close()
	cmd = 'make > /dev/null 2>&1; ./sim_test' 
	# cmd = 'make > /dev/null 2>&1; ./sim_test -w%s -d%s'  % (args.w, args.d)
	subprocess.call(cmd, shell=True)

if __name__ == '__main__':
	mm_simulator()