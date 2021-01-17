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

const static bool DEBUG = 0;
const static bool LOG = 0;
const static int tile_sz = %d;
const static double R = %.2f; 
const static int alpha = (int) (1 / (R-1));
const static int lat_dram = %d; 
const static int lat_internal = %d; // internal latency 

const static int sx = %d;
const static int sy = %d;
const static int Sx = %d;
const static int Sy = %d;
const static int NUM_SA = Sx * Sy;
const static int NUM_LEVELS = (int) ceil(log(((double) NUM_SA)) / log(2));
const static int POD_SZ = sx * sy;
const static int NUM_PODS = (int) (NUM_SA / POD_SZ); // user can opt to use only a portion of the total
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
''' % (c['perf_file'], c['w_file'], c['d_file'], c['r_file'], c['tile_sz'], 
  c['R'], c['lat_ext'], c['lat_int'], c['s'], c['s'], c['Sx'], 
  c['Sy'], 'NUM_PODS', '1', c['M'], c['K'], c['N'])
  





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





def exp1fig1(fname = "exp1fig1"):
  arch1 = include + '''
const std::string PERF_FILE = "%s"; 
const static double lat_internal_factor = 0; 
const static int tile_sz = 2; // change to 8 later
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
  # increase by NUM_PODS in the M and N dims
  # test_dims = [ [(2, 2), ('NUM_PODS', '1')], 
  # 			       [(4, 2), ('NUM_PODS', '1')], 
  #               [(4, 4), ('NUM_PODS', '1')], 
  #               [(8, 4), ('NUM_PODS', '1')], 
  #               [(8, 8), ('NUM_PODS', '1')]]
  test_dims = [ [(4, 4), ('NUM_PODS', '1')], 
                [(8, 4), ('NUM_PODS', '1')],
                [(8, 8), ('NUM_PODS', '1')],
                [(16, 8), ('NUM_PODS', '1')]]
  # increasing internal bw linearly with # SAs
  # lin_inc = [16,8,4,2,1]
  lin_inc = [16,8,4,2]
  # lin_inc = [8,5,3,2,1]
  # constant internal bw even though # SAs is increasing linearly
  constant = [16,16,16,16]
  # constant = [16,16,16]
  # constant = [24,12,6,3]
  # constant = [5,5,5,5,5]
  # lat_internals = [lin_inc] + [constant]
  lat_internals = [constant]
  # bw_growth = ['I','C']
  bw_growth = ['C']
  #
  f = open(fname, 'w')
  f.write("number of SAs,number of cycles,bw growth\n")
  f.close()
  exec_num = 200;
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
  mat_dim_test()
  # test()
  # exp1fig1()
  # exp1fig2()
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




