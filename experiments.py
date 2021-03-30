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
std::string WEIGHT_FILE = "weights";
std::string DATA_FILE = "data";
std::string RESULT_FILE = "result";
static const char delims[] = " ";
const static bool DEBUG = 0;
const static bool LOG = 0;
const static bool USE_FILE = 0;
'''



def run_exp(arch_spec, exec_num):    
    os.remove("src/arch.h")
    f = open("src/arch.h", 'w')
    f.write(arch_spec)
    f.close()
    cmd = 'make > /dev/null 2>&1; mv sim_test sim_test%d; ./sim_test%d > /dev/null 2>&1 &' % (exec_num, exec_num)
    subprocess.call(cmd, shell=True)
    



def build_arch(c):
  return '''
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
static char *PERF_FILE = "%s";
static char *WEIGHT_FILE = "%s";
static char *DATA_FILE = "%s";
static char *RESULT_FILE = "%s";
static const char delims[] = " ";
const static char int_bw_growth = '%s'; // constant vs increasing     

const static bool DEBUG = 0;
const static bool LOG = 0;
const static bool USE_FILE = 0;
const static int tile_sz = %d;
const static int alpha = %d;
const static int lat_dram = %d; 
const static int lat_internal = %d; // internal latency 

const static int sx = %d;
const static int sy = %d;
const static int Sx = %d;
const static int Sy = %d;
const static int NUM_SA = Sx * Sy;
const static int NUM_LEVELS = (int) ceil(log(((double) NUM_SA)) / log(2));
const static int POD_SZ = sx * sy;
const static int NUM_PODS = %d; //(int) (NUM_SA / POD_SZ); // user can opt to use only a portion of the total
const static int pod_ab_hops = (int) ceil(log(((double) sx)) / log(2));

const static int M_ob = sy; // Size of block in the M dimension in terms of tiles
const static int K_ob = sx; // Size of operation block in the k dimension in terms of tiles
const static int N_ob = sx * alpha * NUM_PODS; // Size of operation block in N dimension in terms of tiles

const static int M_sr = M_ob * %s; 
const static int K_sr = K_ob * %s;
const static int N_sr = N_ob;

const static int NUM_PODS_USED = (int) ((M_sr/M_ob) * (K_sr/K_ob)); 
const static int M = %d;
const static int K = %d;
const static int N = %d;
#endif
''' % (c['perf_file'], c['w_file'], c['d_file'], c['r_file'], c['int_bw_growth'], c['tile_sz'], 
  c['alpha'], c['lat_ext'], c['lat_int'], c['s'], c['s'], c['Sx'], 
  c['Sy'], c['NUM_PODS'], 'NUM_PODS', '1', c['M'], c['K'], c['N'])
  





def mat_dim_test(fname = "mat_dim_test"):
  CAKE_params = {'M' : None, 'K' : None, 'N' : None,
      'Sx' : 16, 'Sy' : 8, 's' : 4, 'R' : 2, 
      'tile_sz' : 2, 'lat_ext' : 4, 'lat_int' : 1, 
      'w_file' : 'weights', 'd_file' : 'data', 
      'r_file' : 'result', 'perf_file' : fname}
  #
  dims = [(64,64,64),(64,64,128),(64,128,64),(128,64,64),
          (128,128,64),(128,64,128),(64,128,128),(128,128,128),
          (128,128,256),(128,256,128),(256,128,128)]
  exec_num = 900;
  f = open(fname, 'w')
  f.write("M,K,N,number of cycles\n")
  f.close()
  #
  for i in dims:
    CAKE_params['M'] = i[0]
    CAKE_params['K'] = i[1]
    CAKE_params['N'] = i[2]
    arch = build_arch(CAKE_params)
    print(arch)
    run_exp(arch, exec_num)       
    exec_num += 1



# def cpu_comp_test(fname = "cpu_comp_test"):
#   CAKE_params = {'M' : 1024, 'K' : 1024, 'N' : 1024,
#       'Sx' : None, 'Sy' : None, 's' : 8, 'R' : 2, 
#       'tile_sz' : 16, 'lat_ext' : 1, 'lat_int' : 1, 
#       'w_file' : 'weights', 'd_file' : 'data', 
#       'r_file' : 'result', 'perf_file' : fname}
#   #
#   dims = [(8,8),(16,8),(16,16),(32,16)]
#   exec_num = 3742;
#   f = open(fname, 'w')
#   f.write("Sx,Sy,number of cycles\n")
#   f.close()
#   #
#   for i in dims:
#     CAKE_params['Sx'] = i[0]
#     CAKE_params['Sy'] = i[1]
#     arch = build_arch(CAKE_params)
#     print(arch)
#     run_exp(arch, exec_num)       
#     exec_num += 1



def cpu_comp_test(fname = "cpu_comp_test3"):
  CAKE_params = {'M' : 256, 'K' : 256, 'N' : 256,
      'Sx' : None, 'Sy' : None, 's' : None, 'NUM_PODS' : None, 'alpha' : None, 
      'tile_sz' : 16, 'lat_ext' : 1, 'lat_int' : 1, 
      'w_file' : 'weights', 'd_file' : 'data', 
      'r_file' : 'result', 'int_bw_growth' : None, 'perf_file' : fname}
  #
  # dims = [(8,8),(16,8),(16,16),(32,16)]
  dims = [(2,2,2,1),(4,2,2,1),(4,4,4,2),(8,4,4,2),(8,8,8,4),(16,8,8,4),(16,16,16,8)]
  exec_num = 777;
  f = open(fname, 'w')
  f.write("Sx,Sy,s,alpha,number of cycles\n")
  f.close()
  #
  for i in range(len(dims)):
    CAKE_params['Sx'] = dims[i][0]
    CAKE_params['Sy'] = dims[i][1]
    CAKE_params['s'] = dims[i][2]
    CAKE_params['alpha'] = dims[i][3]
    arch = build_arch(CAKE_params)
    print(arch)
    run_exp(arch, exec_num)       
    exec_num += 1



def exp1fig1(fname = "exp1fig1"):
  CAKE_params = {'M' : 96, 'K' : 96, 'N' : 96,
      'Sx' : 8, 'Sy' : 8, 's' : 2, 'NUM_PODS' : None, 'alpha' : 1, 
      'tile_sz' : 2, 'lat_ext' : 4, 'lat_int' : 1, 
      'w_file' : 'weights', 'd_file' : 'data', 
      'r_file' : 'result', 'int_bw_growth' : None, 'perf_file' : fname}
  # NUM_PODS = [1,2,3,4,6,8,12,16]
  # POD_SZ = [(2,2),(4,2),(4,4),(4,4),(8,4),(8,4),(8,8),(8,8)]
  # bw_growth = {'I' : [1,1,1,1,1,1,1,1], 'C' : [1,2,3,4,6,8,12,16]}
  #
  # NUM_PODS = [1,2,3,4]
  # POD_SZ = [(4,4),(8,4),(8,8),(8,8)]
  # bw_growth = {'I' : [4,4,3,2]}
  #
#   [128.0, 64.0, 42.666666666666664, 32.0, 25.6, 21.333333333333332, 18.285714285714285, 16.0, 14.222222222222221, 12.8, 11.636363636363637, 10.666666666666666, 9.846153846153847, 9.142857142857142, 8.533333333333333, 8.0]
# [32.0, 16.0, 10.666666666666666, 8.0, 6.4, 5.333333333333333, 4.571428571428571, 4.0, 3.5555555555555554, 3.2, 2.909090909090909, 2.6666666666666665, 2.4615384615384617, 2.2857142857142856, 2.1333333333333333, 2.0]
  NUM_PODS = [1,2,3,4,6,8,12,16]
  # POD_SZ = [(2,2),(4,2),(4,4),(4,4),(8,4),(8,4),(8,8),(8,8)]
  bw_growth = {'I' : [16,16,11,8,5,4,3,2], 'C' : [16,16,16,16,16,16,16,16]}
  f = open(fname, 'w')
  f.write("number of pods,number of cycles,bw growth\n")
  f.close()
  exec_num = 200;
  for k in bw_growth:
    for i in range(len(NUM_PODS)):
      CAKE_params['NUM_PODS'] = NUM_PODS[i]
      # CAKE_params['Sx'] = POD_SZ[i][0]
      # CAKE_params['Sy'] = POD_SZ[i][1]
      CAKE_params['int_bw_growth'] = k
      CAKE_params['lat_int'] = bw_growth[k][i]
      arch = build_arch(CAKE_params)
      # print(arch)
      run_exp(arch, exec_num)       
      exec_num += 1



def exp1fig2(fname = "exp1fig2"):
  CAKE_params = {'M' : 96, 'K' : 96, 'N' : 96,
      'Sx' : 8, 'Sy' : 8, 's' : 2, 'NUM_PODS' : None, 'alpha' : 1, 
      'tile_sz' : 2, 'lat_ext' : 1, 'lat_int' : 1, 
      'w_file' : 'weights', 'd_file' : 'data', 
      'r_file' : 'result', 'int_bw_growth' : 'I', 'perf_file' : fname}
#
  NUM_PODS = [1,2,3,4,6,8,12,16]
  # POD_SZ = [(2,2),(4,2),(4,4),(4,4),(8,4),(8,4),(8,8),(8,8)]
  bw_growth = [16,16,11,8,5,4,3,2]
  lat_ext = [32,16,8,4]
  f = open(fname, 'w')
  f.write("number of pods,number of cycles,lat_dram\n")
  f.close()
  exec_num = 300;
  for j in range(len(lat_ext)):
    for i in range(len(NUM_PODS)):
      CAKE_params['NUM_PODS'] = NUM_PODS[i]
      # CAKE_params['Sx'] = POD_SZ[i][0]
      # CAKE_params['Sy'] = POD_SZ[i][1]
      CAKE_params['lat_int'] = bw_growth[i]
      CAKE_params['lat_ext'] = lat_ext[j]
      arch = build_arch(CAKE_params)
      # print(arch)
      run_exp(arch, exec_num)       
      exec_num += 1



def exp1fig3(fname = "exp1fig3"):
  arch1 = include + '''
const std::string PERF_FILE = "%s"; 
const static int tile_sz = 2; // change to 8 later
const static char bw_growth = 'N'; // constant vs increasing     
const static double R = 2; // DRAM banwidth 
const static int alpha = (int) (1 / (R-1));
const static int lat_dram = 4; // DRAM latency 

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
  # increase by NUM_PODS in the M and N dims
  # test_dims = [ [(2, 2), ('NUM_PODS', '1')], 
  #               [(4, 2), ('NUM_PODS', '1')], 
  #               [(4, 4), ('NUM_PODS', '1')], 
  #               [(8, 4), ('NUM_PODS', '1')], 
  #               [(8, 8), ('NUM_PODS', '1')]]
  test_dims = [ [(4, 4), ('NUM_PODS', '1')], 
                [(8, 4), ('NUM_PODS', '1')], 
                [(8, 8), ('NUM_PODS', '1')]]
   # increasing internal bw linearly with # SAs
  # lat_internal1 = [256,128,64,32,16]          # 2:2
  # lat_internal2 = [256,102,41,16,7]          # 2:2.5
  # lat_internal3 = [256,85,28,9,3]          # 2:3
  # lat_internal4 = [256,64,16,4,1]          # 2:4
  #
  # lat_internal1 = [32,16,8,5,4]          # 2:2
  # lat_internal2 = [26,13,6,4,3]          # 2:2.5
  # lat_internal3 = [21,11,5,3,2]           # 2:3
  # lat_internal4 = [16,8,4,2,1]          # 2:4
  #
  lat_internal1 = [32,16,8]          # 2:2
  lat_internal2 = [26,13,6]          # 2:2.5
  lat_internal3 = [21,11,5]           # 2:3
  lat_internal4 = [16,8,4]          # 2:4
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
  # test_dims = [ [(2, 2), ('NUM_PODS', '1')], 
  #               [(4, 2), ('NUM_PODS', '1')], 
  #               [(4, 4), ('NUM_PODS', '1')], 
  #               [(8, 4), ('NUM_PODS', '1')], 
  #               [(8, 8), ('NUM_PODS', '1')]]
  test_dims = [ [(4, 4), ('NUM_PODS', '1')], 
                [(8, 4), ('NUM_PODS', '1')], 
                [(8, 8), ('NUM_PODS', '1')]]
  # increasing internal bw linearly with # SAs
  # lat_internal = [8,4,2]         
  # lat_internal = [20,10,5]         
  lat_internal = [16,8,4]         
  # different DRAM bws
  # lat_dram = [32,16,8,4]
  # lat_dram = [20,10,5,1]
  # lat_internal = [16,8,4]         
  lat_dram = [32,16,8,4]
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
const static int lat_dram = 1;
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
  test_dims = [ [(4, 4), ('NUM_PODS', '1')], 
                [(8, 4), ('NUM_PODS', '1')], 
                [(8, 8), ('NUM_PODS', '1')],
                [(16, 8), ('NUM_PODS', '1')]]
  #
  # lat_internals1 = [32,16,8,4]       # 2:2 
  # lat_internals2 = [40,24,10,5]      # 2:(5/3)   
  lat_internals1 = [16,8,4,2]       # 2:2 
  lat_internals2 = [24,12,6,3]      # 2:(5/3)   
  # lat_internals2 = [32,16,8,4,2]      # 2:(5/3)   
  # lat_internals4 = [16,14,12]      # 2:(4/3.5) 
  lat_internal = [lat_internals1] + [lat_internals2] + [lat_internals1]
  s1 = [2,2,2,2]
  s2 = [2,2,4,4]
  s2 = [2,2,4,4]
  s = [s1] + [s2] + [s2]
  #
  f = open(fname, 'w')
  f.write("number of SAs,number of cycles,s,lat_internal\n")
  f.close()
  exec_num = 500;
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
const static int sx = 4;
const static int sy = 4;
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
  #
  # lat_link = ["{10,10,25,4}", "{8,8,15,3}", "{5,5,5,2}" ]
  # lat_link = ["{10,10,10,50,10,4}", "{8,8,8,25,8,3}", "{5,5,5,5,5,2}" ]
  # lat_link = ["{10,10,10,32,10,4}", "{8,8,8,16,8,3}", "{5,5,5,5,5,2}" ]
  #
  lat_link = ["{10,10,10,30,10,4}", "{8,8,8,20,8,3}", "{5,5,5,8,5,2}" ]
  # lat_link = ["{10,10,10,24,10,4}", "{8,8,8,20,8,3}", "{5,5,5,8,5,2}"]
  sx=4
  sy=4
  #
  f = open(fname, 'w')
  f.write("number of SAs,number of cycles,lat_link\n")
  f.close()
  exec_num = 800;
  #
  for l in xrange(len(lat_link)):
    a1 = '''
const static int lat_dram = 1; // DRAM latency 
const static int lat_internal = 4; // internal latency 
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







def test(fname = "exp1fig5"):
  arch1 = include + '''
const std::string PERF_FILE = "%s"; 
const static char bw_growth = 'N'; // constant vs increasing     
const static double lat_internal_factor = 0; 
const static int tile_sz = 2; // change to 8 later
const static double R = 2; // DRAM banwidth 
const static int alpha = (int) (1 / (R-1));
const static int lat_dram = 1;
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
  M=64
  K=64
  N=128
  # increase by NUM_PODS in the M and N dims
  test_dims = [ [(2, 2), ('NUM_PODS', '1')], 
                [(4, 2), ('NUM_PODS', '1')], 
                [(4, 4), ('NUM_PODS', '1')], 
                [(8, 4), ('NUM_PODS', '1')], 
                [(8, 8), ('NUM_PODS', '1')],
                [(16, 8), ('NUM_PODS', '1')],
                [(16, 16), ('NUM_PODS', '1')]]
  #
  s1 = [2]
  s2 = [2]
  s3 = [2,4]
  s4 = [2,4]
  s5 = [2,4,8]
  s6 = [2,4,8]
  s7 = [4,8,16]
  s = [s1] + [s2] + [s3] + [s4] + [s5] + [s6] + [s7]
  #
  f = open(fname, 'w')
  f.write("number of SAs,number of cycles,s,lat_internal\n")
  f.close()
  exec_num = 900;
  #
  for i in xrange(len(test_dims)):
    for j in xrange(len(s[i])):
      a1 = '''
const static int lat_internal = %d; // internal latency 
const static int Sx = %d;
const static int Sy = %d;
const static int sx = %d;
const static int sy = %d;
''' % (64 / (test_dims[i][0][0]*test_dims[i][0][1] / s[i][j]),
      test_dims[i][0][0],test_dims[i][0][1], s[i][j], s[i][j])
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




if __name__ == '__main__':
  # cpu_comp_test()
  # mat_dim_test()
  # test()
  exp1fig1()
  exp1fig2()
  # exp1fig3()
  # exp1fig4()
  # exp1fig5()
  # exp1fig6()
  # exp1fig7()
  # exp1fig8()





# number of SAs,number of cycles,bw growth
# 16,574294,FH
# 32,299410,FH
# 64,165702,FH
# 128,     ,FH
# 16,574294,MH
# 32,299410,MH
# 64,84466,MH
# 128,52866,MH
# 16,841586,ML
# 32,431746,ML
# 64,117698,ML
# 128,67438,ML




