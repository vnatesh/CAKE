import sys
import os
import subprocess
import time
import numpy as np
import glob


include = '''
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
const static bool LOG = 0;
'''



def run_exp(arch_spec, exec_num):    
    os.remove("src/arch.h")
    f = open("src/arch.h", 'w')
    f.write(arch_spec)
    f.close()
    cmd = 'make > /dev/null 2>&1; mv sim_test sim_test%d; ./sim_test%d > /dev/null 2>&1 &' % (exec_num, exec_num)
    subprocess.call(cmd, shell=True)
    



def exp1fig1(fname = "exp1fig1"):
  arch1 = include + '''
const std::string PERF_FILE = "%s"; 
const static double lat_internal_factor = 0; 
const static int tile_sz = 2; // change to 8 later
const static double R = 2; // DRAM banwidth 
const static int alpha = (int) (1 / (R-1));
const static int lat_dram = 1; // DRAM latency 

''' % fname
#
  arch2 = '''
const static int NUM_SA = Sx * Sy;
const static int NUM_LEVELS = (int) ceil(log(((double) NUM_SA)) / log(2));
const static int sx = 2;
const static int sy = 2;
const static int POD_SZ = sx * sy;
const static int NUM_PODS = (int) (NUM_SA / POD_SZ); // user can opt to use only a portion of the total
const static int pod_ab_hops = (int) ceil(log(((double) sx)) / log(2));

const static int M_ob = sy; // Size of block in the M dimension in terms of tiles
const static int K_ob = sx; // Size of operation block in the k dimension in terms of tiles
const static int N_ob = sx * alpha * NUM_PODS; // Size of operation block in N dimension in terms of tiles

'''
#
  M=32
  K=32
  N=64
  sx=2
  sy=2
  # increase by NUM_PODS in the M and N dims
  test_dims = [ [(2, 2), ('NUM_PODS', '1')], 
  			       [(4, 2), ('NUM_PODS', '1')], 
                [(4, 4), ('NUM_PODS', '1')], 
                [(8, 4), ('NUM_PODS', '1')], 
                [(8, 8), ('NUM_PODS', '1')]]
  # increasing internal bw linearly with # SAs
  lin_inc = [16,8,4,2,1]
  # constant internal bw even though # SAs is increasing linearly
  constant = [16,16,16,16,16]
  lat_internals = [lin_inc] + [constant]
  # bw_growth = ['I','C']
  bw_growth = ['I']
  #
  f = open(fname, 'w')
  f.write("number of SAs,number of cycles,bw growth\n")
  f.close()
  exec_num = 100;
  #
  for l in xrange(len(bw_growth)):
    for i in xrange(len(test_dims)):
      a1 = '''
const static char bw_growth = '%s'; // constant vs increasing     
const static int lat_internal = %d; // internal latency 
const static int Sx = %d;
const static int Sy = %d;
''' % (bw_growth[l], lat_internals[l][i], test_dims[i][0][0], test_dims[i][0][1])
      a3 = '''
const static int M_sr = M_ob * %s; 
const static int K_sr = K_ob * %s;
''' % (test_dims[i][1][0], test_dims[i][1][1])
      a4 = '''
const static int N_sr = N_ob;
const static int NUM_PODS_USED = (int) ((M_sr/M_ob) * (K_sr/K_ob)); 
const static int M = %d;
const static int K = %d;
const static int N = %d;
#endif
''' % (M,K,N)
      arch = arch1 + a1 + arch2 + a3 + a4
      run_exp(arch, exec_num)  	    
      exec_num += 1




def exp1fig2(fname = "exp1fig2"):
  arch1 = include + '''
const std::string PERF_FILE = "%s"; 
const static char bw_growth = 'N'; // constant vs increasing     
const static double lat_internal_factor = 0; 
const static int tile_sz = 2; // change to 8 later
const static double R = 2; // DRAM banwidth 
const static int alpha = (int) (1 / (R-1));
''' % fname
  #
  arch2 = '''
const static int NUM_SA = Sx * Sy;
const static int NUM_LEVELS = (int) ceil(log(((double) NUM_SA)) / log(2));
const static int sx = 2;
const static int sy = 2;
const static int POD_SZ = sx * sy;
const static int NUM_PODS = (int) (NUM_SA / POD_SZ); // user can opt to use only a portion of the total
const static int pod_ab_hops = (int) ceil(log(((double) sx)) / log(2));

const static int M_ob = sy; // Size of block in the M dimension in terms of tiles
const static int K_ob = sx; // Size of operation block in the k dimension in terms of tiles
const static int N_ob = sx * alpha * NUM_PODS; // Size of operation block in N dimension in terms of tiles
'''
  #
  M=32
  K=32
  N=32
  test_dims = [ [(2, 2), ('NUM_PODS', '1')], 
             [(4, 2), ('NUM_PODS', '1')], 
              [(4, 4), ('NUM_PODS', '1')], 
              [(8, 4), ('NUM_PODS', '1')], 
              [(8, 8), ('NUM_PODS', '1')]]
  lat_internal = [16,8,4,2,1]
  sx=2
  sy=2
  #
  f = open(fname, 'w')
  f.write("number of SAs,number of cycles,K\n")
  f.close()
  exec_num = 100;
  #
  for factor in range(1,5):
    for i in xrange(len(test_dims)):
      a1 = '''
const static int lat_dram = 1; // DRAM latency 
const static int lat_internal = %d; // internal latency 
const static int Sx = %d;
const static int Sy = %d;
''' % (lat_internal[i], test_dims[i][0][0], test_dims[i][0][1])
      a3 = '''
//const static int lat_link[NUM_LEVELS] = NULL;
const static int M_sr = M_ob * %s; 
const static int K_sr = K_ob * %s;
''' % (test_dims[i][1][0], test_dims[i][1][1])
      a4 = '''
const static int N_sr = N_ob;
const static int NUM_PODS_USED = (int) ((M_sr/M_ob) * (K_sr/K_ob)); 
const static int M = %d;
const static int K = %d;
const static int N = %d;
#endif
''' % (M , K * factor, N)
      arch = arch1 + a1 + arch2 + a3 + a4
      run_exp(arch, exec_num)       
      exec_num += 1
    # print arch




def exp1fig3(fname = "exp1fig3"):
  arch1 = include + '''
const std::string PERF_FILE = "%s"; 
const static int tile_sz = 2; // change to 8 later
const static char bw_growth = 'N'; // constant vs increasing     
const static double R = 2; // DRAM banwidth 
const static int alpha = (int) (1 / (R-1));
const static int lat_dram = 16; // DRAM latency 

''' % fname
  #
  arch2 = '''
const static int NUM_SA = Sx * Sy;
const static int NUM_LEVELS = (int) ceil(log(((double) NUM_SA)) / log(2));
const static int sx = 2;
const static int sy = 2;
const static int POD_SZ = sx * sy;
const static int NUM_PODS = (int) (NUM_SA / POD_SZ); // user can opt to use only a portion of the total
const static int pod_ab_hops = (int) ceil(log(((double) sx)) / log(2));

const static int M_ob = sy; // Size of block in the M dimension in terms of tiles
const static int K_ob = sx; // Size of operation block in the k dimension in terms of tiles
const static int N_ob = sx * alpha * NUM_PODS; // Size of operation block in N dimension in terms of tiles
'''
  #
  M=32
  K=32
  N=64
  #
  sx=2
  sy=2
  # increase by NUM_PODS in the M and N dims
  test_dims = [ [(2, 2), ('NUM_PODS', '1')], 
                [(4, 2), ('NUM_PODS', '1')], 
                [(4, 4), ('NUM_PODS', '1')], 
                [(8, 4), ('NUM_PODS', '1')], 
                [(8, 8), ('NUM_PODS', '1')]]
   # increasing internal bw linearly with # SAs
  # lat_internal1 = [256,128,64,32,16]          # 2:2
  # lat_internal2 = [256,102,41,16,7]          # 2:2.5
  # lat_internal3 = [256,85,28,9,3]          # 2:3
  # lat_internal4 = [256,64,16,4,1]          # 2:4
  lat_internal1 = [32,16,8,5,4]          # 2:2
  lat_internal2 = [26,13,6,4,3]          # 2:2.5
  lat_internal3 = [21,11,5,3,2]           # 2:3
  lat_internal4 = [16,8,4,2,1]          # 2:4
  #
  lat_internals = [lat_internal1] + [lat_internal2] + [lat_internal3] + [lat_internal4]
  factors = [2, 2.5, 3, 4]
  #
  f = open(fname, 'w')
  f.write("number of SAs,number of cycles,lat_internal_factor\n")
  f.close()
  exec_num = 300;
  #
  for l in xrange(len(lat_internals)):
    for i in xrange(len(test_dims)):
      a1 = '''
const static int lat_internal = %d; // internal latency 
const static double lat_internal_factor = %f; 
const static int Sx = %d;
const static int Sy = %d;
''' % (lat_internals[l][i], factors[l], test_dims[i][0][0],test_dims[i][0][1])
      a3 = '''
const static int M_sr = M_ob * %s; 
const static int K_sr = K_ob * %s;
''' % (test_dims[i][1][0],test_dims[i][1][1])
      a4 = '''
const static int N_sr = N_ob;
const static int NUM_PODS_USED = (int) ((M_sr/M_ob) * (K_sr/K_ob)); 
const static int M = %d;
const static int K = %d;
const static int N = %d;
#endif
  ''' % (M, K, N)
      arch = arch1 + a1 + arch2 + a3 + a4
      run_exp(arch, exec_num)       
      exec_num += 1






def exp1fig4(fname = "exp1fig4"):
  arch1 = include + '''
const std::string PERF_FILE = "%s"; 
const static char bw_growth = 'N'; // constant vs increasing     
const static double lat_internal_factor = 0; 
const static int tile_sz = 2; // change to 8 later
const static double R = 2; // DRAM banwidth 
const static int alpha = (int) (1 / (R-1));
''' % fname
  #
  arch2 = '''
const static int NUM_SA = Sx * Sy;
const static int NUM_LEVELS = (int) ceil(log(((double) NUM_SA)) / log(2));
const static int sx = 2;
const static int sy = 2;
const static int POD_SZ = sx * sy;
const static int NUM_PODS = (int) (NUM_SA / POD_SZ); // user can opt to use only a portion of the total
const static int pod_ab_hops = (int) ceil(log(((double) sx)) / log(2));

const static int M_ob = sy; // Size of block in the M dimension in terms of tiles
const static int K_ob = sx; // Size of operation block in the k dimension in terms of tiles
const static int N_ob = sx * alpha * NUM_PODS; // Size of operation block in N dimension in terms of tiles
'''
  #
  M=32
  K=32
  N=64
  # increase by NUM_PODS in the M and N dims
  test_dims = [ [(2, 2), ('NUM_PODS', '1')], 
                [(4, 2), ('NUM_PODS', '1')], 
                [(4, 4), ('NUM_PODS', '1')], 
                [(8, 4), ('NUM_PODS', '1')], 
                [(8, 8), ('NUM_PODS', '1')]]
  # increasing internal bw linearly with # SAs
  lat_internal = [16,8,4,2,1]         
  # different DRAM bws
  lat_dram = [16,8,4,2,1]
  sx=2
  sy=2
  #
  f = open(fname, 'w')
  f.write("number of SAs,number of cycles,lat_dram\n")
  f.close()
  exec_num = 400;
  #
  for l in xrange(len(lat_dram)):
    for i in xrange(len(test_dims)):
      a1 = '''
const static int lat_dram = %d; // DRAM latency 
const static int lat_internal = %d; // internal latency 
const static int Sx = %d;
const static int Sy = %d;
''' % (lat_dram[l], lat_internal[i], test_dims[i][0][0],test_dims[i][0][1])
      a3 = '''
const static int M_sr = M_ob * %s; 
const static int K_sr = K_ob * %s;
''' % (test_dims[i][1][0],test_dims[i][1][1])
      a4 = '''
const static int N_sr = N_ob;
const static int NUM_PODS_USED = (int) ((M_sr/M_ob) * (K_sr/K_ob)); 
const static int M = %d;
const static int K = %d;
const static int N = %d;
#endif
''' % (M, K, N)
      arch = arch1 + a1 + arch2 + a3 + a4
      run_exp(arch, exec_num)       
      exec_num += 1




# # exp to see if we can afford to increase bw less than proportionally if we do more
# # local accumulation (s = 4 vs s = 2)
# def exp1fig5(fname = "exp1fig5"):
#   arch1 = include + '''
# const std::string PERF_FILE = "%s"; 
# const static char bw_growth = 'N'; // constant vs increasing     
# const static double lat_internal_factor = 0; 
# const static int tile_sz = 2; // change to 8 later
# const static double R = 2; // DRAM banwidth 
# const static int alpha = (int) (1 / (R-1));
# const static int lat_dram = 1;
# ''' % fname
#   #
#   arch2 = '''
# const static int NUM_SA = Sx * Sy;
# const static int NUM_LEVELS = (int) ceil(log(((double) NUM_SA)) / log(2));
# const static int POD_SZ = sx * sy;
# const static int NUM_PODS = (int) (NUM_SA / POD_SZ); // user can opt to use only a portion of the total
# const static int pod_ab_hops = (int) ceil(log(((double) sx)) / log(2));

# const static int M_ob = sy; // Size of block in the M dimension in terms of tiles
# const static int K_ob = sx; // Size of operation block in the k dimension in terms of tiles
# const static int N_ob = sx * alpha * NUM_PODS; // Size of operation block in N dimension in terms of tiles
# '''
#   #
#   M=32
#   K=32
#   N=64
#   # increase by NUM_PODS in the M and N dims
#   test_dims = [ [(2, 2), ('NUM_PODS', '1')], 
#                 [(4, 2), ('NUM_PODS', '1')], 
#                 [(4, 4), ('NUM_PODS', '1')], 
#                 [(8, 4), ('NUM_PODS', '1')], 
#                 [(8, 8), ('NUM_PODS', '1')],
#                 [(16, 8), ('NUM_PODS', '1')]]
#   #
#   lat_internals2 = [16,8,4]       # 2:2 
#   lat_internals4 = [16,12,9]      # 2:(4/3) (increase bw_int by (4/3)x when increasing C by 2x)
#   # lat_internals4 = [16,14,12]      # 2:(4/3.5) 
#   lat_internal = [lat_internals2] + [lat_internals4]
#   s = [2,4]
#   #
#   f = open(fname, 'w')
#   f.write("number of SAs,number of cycles,s\n")
#   f.close()
#   exec_num = 500;
#   #
#   for j in xrange(len(s)):
#     for i in xrange(len(test_dims)):
#       a1 = '''
# const static int lat_internal = %d; // internal latency 
# const static int Sx = %d;
# const static int Sy = %d;
# const static int sx = %d;
# const static int sy = %d;
# ''' % (lat_internal[j][i], test_dims[i][0][0],test_dims[i][0][1], s[j], s[j])
#       a3 = '''
# const static int M_sr = M_ob * %s; 
# const static int K_sr = K_ob * %s;
# ''' % (test_dims[i][1][0],test_dims[i][1][1])
#       a4 = '''
# const static int N_sr = N_ob;
# const static int NUM_PODS_USED = (int) ((M_sr/M_ob) * (K_sr/K_ob)); 
# const static int M = %d;
# const static int K = %d;
# const static int N = %d;
# #endif
# ''' % (M, K, N)
#       arch = arch1 + a1 + arch2 + a3 + a4
#       run_exp(arch, exec_num)       
#       exec_num += 1
#       # print arch



# exp to see if we can afford to increase bw less than proportionally if we do more
# local accumulation (s = 8 vs 4, 2)
def exp1fig5(fname = "exp1fig5"):
  arch1 = include + '''
const std::string PERF_FILE = "%s"; 
const static char bw_growth = 'N'; // constant vs increasing     
const static double lat_internal_factor = 0; 
const static int tile_sz = 2; // change to 8 later
const static double R = 2; // DRAM banwidth 
const static int alpha = (int) (1 / (R-1));
const static int lat_dram = 16;
''' % fname
  #
  arch2 = '''
const static int NUM_SA = Sx * Sy;
const static int NUM_LEVELS = (int) ceil(log(((double) NUM_SA)) / log(2));
const static int POD_SZ = sx * sy;
const static int NUM_PODS = (int) (NUM_SA / POD_SZ); // user can opt to use only a portion of the total
const static int pod_ab_hops = (int) ceil(log(((double) sx)) / log(2));

const static int M_ob = sy; // Size of block in the M dimension in terms of tiles
const static int K_ob = sx; // Size of operation block in the k dimension in terms of tiles
const static int N_ob = sx * alpha * NUM_PODS; // Size of operation block in N dimension in terms of tiles
'''
  #
  M=32
  K=32
  N=64
  # increase by NUM_PODS in the M and N dims
  test_dims = [ [(2, 2), ('NUM_PODS', '1')], 
                [(4, 2), ('NUM_PODS', '1')], 
                [(4, 4), ('NUM_PODS', '1')], 
                [(8, 4), ('NUM_PODS', '1')], 
                [(8, 8), ('NUM_PODS', '1')]]
                # [(16, 8), ('NUM_PODS', '1')]]
  #
  lat_internals1 = [16,8,4,2,1]       # 2:2 
  # lat_internals2 = [32,24,18,13,10,8]      # 2:(4/3) (increase bw_int by (4/3)x when increasing C by 2x)  
  lat_internals2 = [16,10,6,3,2]      # 2:(5/3)   
  # lat_internals2 = [32,16,8,4,2]      # 2:(5/3)   
  # lat_internals2 = [32,18,10,5,3,2]      # 2:1.8 (increase bw_int by 1.8x when increasing C by 2x)
  # lat_internals4 = [16,14,12]      # 2:(4/3.5) 
  # lat_internals1 = [48,24,12,6,3,2]       # 2:2 
  # lat_internals2 = [48,32,21,14,9,6]      # 2:1.5 
  # lat_internals2 = [48,30,19,12,7,5]      # 2:1.6 
  # lat_internals2 = [48,29,17,10,6,4]      # 2:(5/3) 
  lat_internal = [lat_internals1] + [lat_internals2]
  s1 = [2,2,2,2,2,2]
  s2 = [2,2,4,4,8,8]
  s = [s1] + [s2]
  #
  f = open(fname, 'w')
  f.write("number of SAs,number of cycles,s\n")
  f.close()
  exec_num = 800;
  #
  for j in xrange(len(s)):
    for i in xrange(len(test_dims)):
      a1 = '''
const static int lat_internal = %d; // internal latency 
const static int Sx = %d;
const static int Sy = %d;
const static int sx = %d;
const static int sy = %d;
''' % (lat_internal[j][i], test_dims[i][0][0],test_dims[i][0][1], s[j][i], s[j][i])
      a3 = '''
const static int M_sr = M_ob * %s; 
const static int K_sr = K_ob * %s;
''' % (test_dims[i][1][0],test_dims[i][1][1])
      a4 = '''
const static int N_sr = N_ob;
const static int NUM_PODS_USED = (int) ((M_sr/M_ob) * (K_sr/K_ob)); 
const static int M = %d;
const static int K = %d;
const static int N = %d;
#endif
''' % (M, K, N)
      arch = arch1 + a1 + arch2 + a3 + a4
      run_exp(arch, exec_num)       
      exec_num += 1
      # print arch







# exp to see if growing memory by more than the function C + aC + aC^2/s^2 will lead to better
# speedup when increasing processing power. Memory is grown in the N dim 
def exp1fig6(fname = "exp1fig6"):
  arch1 = include + '''
const std::string PERF_FILE = "%s"; 
const static char bw_growth = 'N'; // constant vs increasing     
const static double lat_internal_factor = 0; 
const static int tile_sz = 2; // change to 8 later
const static double R = 2; // DRAM banwidth 
const static int alpha = (int) (1 / (R-1));
''' % fname
  #
  arch2 = '''
const static int NUM_SA = Sx * Sy;
const static int NUM_LEVELS = (int) ceil(log(((double) NUM_SA)) / log(2));
const static int sx = 2;
const static int sy = 2;
const static int POD_SZ = sx * sy;
const static int NUM_PODS = (int) (NUM_SA / POD_SZ); // user can opt to use only a portion of the total
const static int pod_ab_hops = (int) ceil(log(((double) sx)) / log(2));

const static int M_ob = sy; // Size of block in the M dimension in terms of tiles
const static int K_ob = sx; // Size of operation block in the k dimension in terms of tiles
'''
  #
  M=64
  K=32
  N=128
  # increase by NUM_PODS in the M and N dims
  test_dims = [ [(2, 2), ('NUM_PODS', '1')], 
                [(4, 2), ('NUM_PODS', '1')], 
                [(4, 4), ('NUM_PODS', '1')], 
                [(8, 4), ('NUM_PODS', '1')], 
                [(8, 8), ('NUM_PODS', '1')]]
  # increasing internal bw linearly with # SAs
  # lat_internal = [16,8,4,2,1]          # 2:2
  lat_internal = [16,8,4,2,1]        # 2:2
  # different N dim size 
  n_factor = [1,2]
  sx=2
  sy=2
  #
  f = open(fname, 'w')
  f.write("number of SAs,number of cycles,N_sr\n")
  f.close()
  exec_num = 600;
  #
  for l in xrange(len(n_factor)):
    for i in xrange(len(test_dims)):
      a1 = '''
const static int lat_dram = 1; // DRAM latency 
const static int lat_internal = %d; // internal latency 
const static int Sx = %d;
const static int Sy = %d;
''' % (lat_internal[i], test_dims[i][0][0],test_dims[i][0][1])
      a3 = '''
const static int N_ob = sx * alpha * NUM_PODS * %d; // Size of operation block in N dimension in terms of tiles
const static int M_sr = M_ob * %s; 
const static int K_sr = K_ob * %s;
''' % (n_factor[l], test_dims[i][1][0],test_dims[i][1][1])
      a4 = '''
const static int N_sr = N_ob;
const static int NUM_PODS_USED = (int) ((M_sr/M_ob) * (K_sr/K_ob)); 
const static int M = %d;
const static int K = %d;
const static int N = %d;
#endif
''' % (M, K, N)
      arch = arch1 + a1 + arch2 + a3 + a4
      run_exp(arch, exec_num)       
      exec_num += 1
      # print arch






# AB SKIPPING
def exp1fig7(fname = "exp1fig7"):
  arch1 = include + '''
const std::string PERF_FILE = "%s"; 
const static char bw_growth = 'N'; // constant vs increasing     
const static double lat_internal_factor = 0; 
const static int tile_sz = 2; // change to 8 later
const static double R = 2; // DRAM banwidth 
const static int alpha = (int) (1 / (R-1));
''' % fname
  #
  arch2 = '''
const static int NUM_SA = Sx * Sy;
const static int NUM_LEVELS = (int) ceil(log(((double) NUM_SA)) / log(2));
const static int sx = 2;
const static int sy = 2;
const static int POD_SZ = sx * sy;
const static int NUM_PODS = (int) (NUM_SA / POD_SZ); // user can opt to use only a portion of the total
const static int pod_ab_hops = (int) ceil(log(((double) sx)) / log(2));

const static int M_ob = sy; // Size of block in the M dimension in terms of tiles
const static int K_ob = sx; // Size of operation block in the k dimension in terms of tiles
const static int N_ob = sx * alpha * NUM_PODS; // Size of operation block in N dimension in terms of tiles
'''
  #
  M=32
  K=32
  N=64
  test_dims = [ [(8, 8), ('NUM_PODS', '1')] ]
  # const static int lat_link[NUM_LEVELS] = {27,18,12,9}; # 1x OP
  # const static int lat_link[NUM_LEVELS] = {13,9,6,5}; # 2x OP
  # const static int lat_link[NUM_LEVELS] = {9,6,4,3}; # 3x OP
  # lat_link = ["{45,36,27,18,12,9}", "{23,18,14,9,6,5}", "{15,12,9,6,4,3}"]
  # lat_link = ["{45,45,45,45,45,45}", "{45,37,32,28,25,23}", "{45,32,25,20,17,15}" ]
  # lat_link = ["{40,27,20,16,13,11}", "{20,13,10,8,7,6}", "{13,9,7,5,4,3}" ]
  # lat_link = ["{4,4,4,4,4,4}", "{23,16,11,8,6,4}", "{11,8,6,4,3,2}", "{8,5,4,3,2,1}" ]
  #
  lat_link = ["{10,10,25,4}", "{8,8,15,3}", "{5,5,5,2}" ]
  # lat_link = ["{10,10,10,10,25,4}", "{8,8,8,8,15,3}", "{5,5,5,5,5,2}" ]
  sx=2
  sy=2
  #
  f = open(fname, 'w')
  f.write("number of SAs,number of cycles,lat_link\n")
  f.close()
  exec_num = 500;
  #
  for l in xrange(len(lat_link)):
    a1 = '''
const static int lat_dram = 1; // DRAM latency 
const static int lat_internal = 1; // internal latency 
const static int Sx = %d;
const static int Sy = %d;
''' % (test_dims[0][0][0], test_dims[0][0][1])
    a3 = '''
const static int lat_link[NUM_LEVELS] = %s;
const static int M_sr = M_ob * %s; 
const static int K_sr = K_ob * %s;
''' % (lat_link[l], test_dims[0][1][0], test_dims[0][1][1])
    a4 = '''
const static int N_sr = N_ob;
const static int NUM_PODS_USED = (int) ((M_sr/M_ob) * (K_sr/K_ob)); 
const static int M = %d;
const static int K = %d;
const static int N = %d;
#endif
''' % (M, K, N)
    arch = arch1 + a1 + arch2 + a3 + a4
    run_exp(arch, exec_num)       
    exec_num += 1
    # print arch











def exp1fig8(fname = "exp1fig8"):
  arch1 = include + '''
  const std::string PERF_FILE = "%s"; 
  const static char bw_growth = 'N'; // constant vs increasing     
  const static double lat_internal_factor = 0; 
  const static int tile_sz = 2; // change to 8 later
  const static double R = 2; // DRAM banwidth 
  const static int alpha = (int) (1 / (R-1));
  ''' % fname
  #
  arch2 = '''
  const static int NUM_SA = Sx * Sy;
  const static int NUM_LEVELS = (int) ceil(log(((double) NUM_SA)) / log(2));
  const static int POD_SZ = sx * sy;
  const static int NUM_PODS = (int) (NUM_SA / POD_SZ); // user can opt to use only a portion of the total
  const static int pod_ab_hops = (int) ceil(log(((double) sx)) / log(2));

  const static int M_ob = sy; // Size of block in the M dimension in terms of tiles
  const static int K_ob = sx; // Size of operation block in the k dimension in terms of tiles
  const static int N_ob = sx * alpha * NUM_PODS; // Size of operation block in N dimension in terms of tiles
  '''
  #
  M=32
  K=32
  N=64
  test_dims = [ [(2, 2), ('NUM_PODS', '1'),2,2], 
                [(4, 2), ('NUM_PODS', '1'),2,2], 
                [(4, 4), ('NUM_PODS', '1'),4,4], 
                [(8, 4), ('NUM_PODS', '1'),4,4], 
                [(8, 8), ('NUM_PODS', '1'),8,8]]
  lat_internal = [16,8,4,2,1]
  #
  f = open(fname, 'w')
  f.write("number of SAs,number of cycles,sx\n")
  f.close()
  exec_num = 500;
  #
  for i in xrange(len(test_dims)):
    a1 = '''
  const static int lat_dram = 16; // DRAM latency 
  const static int lat_internal = %d; // internal latency 
  const static int Sx = %d;
  const static int Sy = %d;
  const static int sx = %d;
  const static int sy = %d;
  ''' % (lat_internal[i], test_dims[i][0][0], test_dims[i][0][1], test_dims[i][2], test_dims[i][3])
    a3 = '''
  //const static int lat_link[NUM_LEVELS] = NULL;
  const static int M_sr = M_ob * %s; 
  const static int K_sr = K_ob * %s;
  ''' % (test_dims[i][1][0], test_dims[i][1][1])
    a4 = '''
  const static int N_sr = N_ob;
  const static int NUM_PODS_USED = (int) ((M_sr/M_ob) * (K_sr/K_ob)); 
  const static int M = %d;
  const static int K = %d;
  const static int N = %d;
  #endif
  ''' % (M, K, N)
    arch = arch1 + a1 + arch2 + a3 + a4
    run_exp(arch, exec_num)       
    exec_num += 1
    # print arch


if __name__ == '__main__':
  # exp1fig1()
  # exp1fig2()
  exp1fig3()
  # exp1fig4()
  # exp1fig5()
  # exp1fig6()
  # exp1fig7()
  # exp1fig8()









