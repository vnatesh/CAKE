import sys
import os
import subprocess
import time
import numpy as np
import glob


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
  


'''
Experiment from Figure 6a)

internal bandwidth is increased linearly with the number of pods (bw_int = R*s + 2p*s)
while DRAM bw is held constant at 4 GB/s

'''
def exp1fig1(fname = "exp1fig1"):
  CAKE_params = {'M' : 96, 'K' : 96, 'N' : 96,
      'Sx' : 8, 'Sy' : 8, 's' : 2, 'NUM_PODS' : None, 'alpha' : 1, 
      'tile_sz' : 2, 'lat_ext' : 36, 'lat_int' : 1, 
      'w_file' : 'weights', 'd_file' : 'data', 
      'r_file' : 'result', 'int_bw_growth' : None, 'perf_file' : fname}
  NUM_PODS = [1,2,3,4,6,8,12,16]
  bw_growth = {'I' : [24,18,15,12,9,7,5,4], 'C' : [24,24,24,24,24,24,24,24]}
  f = open(fname, 'w')
  f.write("number of pods,number of cycles,bw growth\n")
  f.close()
  exec_num = 200;
  for k in bw_growth:
    for i in range(len(NUM_PODS)):
      CAKE_params['NUM_PODS'] = NUM_PODS[i]
      CAKE_params['int_bw_growth'] = k
      CAKE_params['lat_int'] = bw_growth[k][i]
      arch = build_arch(CAKE_params)
      # print(arch)
      run_exp(arch, exec_num)       
      exec_num += 1


'''
Experiment from Figure 6b)

internal bandwidth is increased linearly with the number of pods (bw_int = R*s + 2p*s)
while DRAM bw is varied from 1 to 4 GB/s

'''
def exp1fig2(fname = "exp1fig2"):
  CAKE_params = {'M' : 96, 'K' : 96, 'N' : 96,
      'Sx' : 8, 'Sy' : 8, 's' : 2, 'NUM_PODS' : None, 'alpha' : 1, 
      'tile_sz' : 2, 'lat_ext' : 1, 'lat_int' : 1, 
      'w_file' : 'weights', 'd_file' : 'data', 
      'r_file' : 'result', 'int_bw_growth' : 'I', 'perf_file' : fname}
#
  NUM_PODS = [1,2,3,4,6,8,12,16]
  bw_growth = [24,18,15,12,9,7,5,4]
  lat_ext = [128,64,42,36]
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
  CAKE_params = {'M' : 128, 'K' : 128, 'N' : 128,
      'Sx' : None, 'Sy' : None, 's' : None, 'NUM_PODS' : None, 'alpha' : 1, 
      'tile_sz' : 2, 'lat_ext' : 1, 'lat_int' : 1, 
      'w_file' : 'weights', 'd_file' : 'data', 
      'r_file' : 'result', 'int_bw_growth' : None, 'perf_file' : fname}
  #
  # dims = [(8,8),(16,8),(16,16),(32,16)]
  dims = {'h' : [(2,2,2,1),(4,2,2,2),(4,4,4,1),(8,4,4,2),(8,8,8,1),(16,8,8,2),(16,16,16,1)],
          'l' : [(2,2,2,1),(4,2,2,2),(4,4,2,4),(8,4,2,8),(8,8,2,16),(16,8,2,32),(16,16,2,64)],
          'm' : [(2,2,2,1),(4,2,2,2),(4,4,4,1),(8,4,4,2),(8,8,8,1),(16,8,8,2),(16,16,16,1)]}
  #bw_growth = {'h' : [32,32,16,16,8,8,4],
  #            'l' : [32,32,16,8,4,2,1],
  #            'm' : [32,32,16,8,4,2,1]}
  bw_growth = {'h' : [22,17,11,8,5,4,3],
              'l' : [22,17,11,7,4,2,1],
              'm' : [22,17,11,7,4,2,1]}
  #
  exec_num = 777;
  f = open(fname, 'w')
  f.write("NUM_SA,s,alpha,number of cycles\n")
  f.close()
  #
  for j in dims:
    for i in range(len(dims[j])):
      CAKE_params['Sx'] = dims[j][i][0]
      CAKE_params['Sy'] = dims[j][i][1]
      CAKE_params['s'] = dims[j][i][2]
      CAKE_params['NUM_PODS'] = dims[j][i][3]
      CAKE_params['lat_int'] = bw_growth[j][i]
      CAKE_params['int_bw_growth'] = j
      arch = build_arch(CAKE_params)
      # print(arch)
      run_exp(arch, exec_num)       
      exec_num += 1



if __name__ == '__main__':
  exp1fig1()
  exp1fig2()
  # exp1fig3()

