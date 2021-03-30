import pandas
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
import numpy as np
import os
import re





def plot_internal_bw_cpu(exp, arr_size, file_name, ncores):
	plt.rcParams.update({'font.size': 12})
	markers = ['o','v','s','d','^']
	colors = ['b','g','aqua','k','m','r']
	# labels = ['']	
	a = open(file_name,'r').read().split('\n')
	a = [i for i in a if exp in i]
	a = [i.split('\t') for i in a]
	a = [i for i in a if i[6] == 'areasize=%d' % arr_size]
	NUM_CPUs = range(1,ncores+1)
	int_bw = []
	for i in range(len(NUM_CPUs)):
		int_bw.append(float(a[i][-2][10:]) / (10**9))
	#
	plt.plot(list(NUM_CPUs), int_bw,  marker = markers[2], color = colors[1])
	plt.title('L3 Cache Bandwidth on AMD Zen3 CPU')
	plt.xlabel("Number of Cores", fontsize = 12)
	plt.ylabel("Bandwidth (GB/s)", fontsize = 12)
	plt.xticks(NUM_CPUs)
	# plt.legend(loc = "lower right", prop={'size': 12})
	# plt.savefig("./plots/%s.pdf" % fname, bbox_inches='tight')
	plt.show()
	plt.clf()
	plt.close('all')


def plot_exp1fig1(fname = 'exp1fig1'):
	plt.rcParams.update({'font.size': 12})
	markers = ['o','v','s','d','^']
	colors = ['g','b','aqua','k','m','r']
	labels = ['constant', 'linearly increasing']
	if not os.path.exists(fname):
		print("file %s not found" % fname)
		return
	#
	df = pandas.read_csv(fname)
	df = df.sort_values('number of pods')
	NUM_PODS = [1,2,3,4,6,8,12,16]
	single_pod_lat = df[(df['bw growth'] == 'C')][(df['number of pods'] == 1)]['number of cycles']._values[0]
	constant = df[(df['bw growth'] == 'C')][['number of pods','number of cycles']]
	linear_inc =  df[(df['bw growth'] == 'I')][['number of pods','number of cycles']]
	plt.plot(list(constant['number of pods']), 
			list(single_pod_lat / constant['number of cycles']), 
			label = labels[0], marker = markers[0], color = colors[0])
	plt.plot(list(linear_inc['number of pods']),
			list(single_pod_lat / linear_inc['number of cycles']), 
			label = labels[1], marker = markers[1], color = colors[1])
	plt.title("Impact of Internal BW on Speedup",fontsize = 18)
	plt.xlabel("Number of Pods",fontsize = 18)
	plt.ylabel("Speedup",fontsize = 18)
	plt.legend(title="internal bandwidth", loc = "upper left")
	# plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	plt.show()
	plt.clf()
	plt.close('all')



def plot_exp1fig2(fname = 'exp1fig2'):
	plt.rcParams.update({'font.size': 12})
	markers = ['o','v','s','d','^']
	colors = ['k','aqua','g','b','m','r']
	if not os.path.exists(fname):
		print("file %s not found" % fname)
		return
	#
	df = pandas.read_csv(fname)
	df = df.sort_values('number of pods')
	NUM_PODS = [1,2,3,4,6,8,12,16]
	single_pod_lat = df[(df['lat_dram'] == 8) & (df['number of pods'] == 1)]['number of cycles']._values[0]
	tile_sz = 8
	#
	# lat_dram = [20,10,5,1]
	lat_dram = [32,16,8,4]
	i = 0;
	for l in lat_dram:
		speedup = single_pod_lat / df[(df['lat_dram'] == l)]['number of cycles']
		plt.plot(list(NUM_PODS), list(speedup), label = (2.0/l)*(tile_sz**2), marker = markers[i], color = colors[i])
		i += 1
	#
	plt.title("Impact of External BW on Speedup", fontsize = 18)
	plt.xlabel("Number of Pods", fontsize = 18)
	plt.ylabel("Speedup", fontsize = 18)
	plt.legend(title="External bw (GB/s)", loc = "upper left")
	# plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	plt.show()
	plt.clf()




def plot_exp1fig3(fname = 'exp1fig3'):
	plt.rcParams.update({'font.size': 16})
	if not os.path.exists(fname):
		print("file %s not found" % fname)
		return
	#
	df = pandas.read_csv(fname)
	df = df.sort_values('number of SAs')
	NUM_SA = [16,32,64]
	single_pod_lat = df[(df['number of SAs'] == 16) & (df['lat_internal_factor'] == 2)]['number of cycles']._values[0]
	#
	markers = ['o','v','s','d','^']
	colors = ['b','g','aqua','k','m']
	i = 0
	for l in [2,2.5,3,4]:
		# single_pod_lat = df[(df['number of SAs'] == 4) & (df['lat_internal_factor'] == l)]['number of cycles']._values[0]
		speedup = single_pod_lat / df[(df['lat_internal_factor'] == l)]['number of cycles']
		plt.plot(list(NUM_SA), list(speedup), label = l/2.0, marker = markers[i], color = colors[i])
		i += 1
	#
	plt.title("Impact of Additional Internal BW")
	plt.xlabel("Number of SAs", fontsize = 18)
	plt.ylabel("Speedup", fontsize = 18)
	plt.legend(title="internal bw\nincrease factor", loc = "middle right", prop={'size': 13})
	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	plt.show()
	plt.clf()






def plot_exp1fig5(fname = 'exp1fig5'):
	plt.rcParams.update({'font.size': 16})
	if not os.path.exists(fname):
		print("file %s not found" % fname)
		return
	#
	df = pandas.read_csv(fname)
	df = df.sort_values('number of SAs')
	NUM_SA = [4,8,16,32,64]
	single_pod_lat = df[(df['s'] == 2) & (df['number of SAs'] == 4) & (df['int'] == 'H')]['number of cycles']._values[0]
	#
	markers = ['o','v']
	labels = ['fixed', 'max']
	i = 0
	for s in ['H','L']:
		speedup = single_pod_lat / df[(df['int'] == s)]['number of cycles']
		plt.plot(list(NUM_SA), list(speedup), label = labels[i], marker = markers[i])
		i += 1
	#
	plt.title("Impact of Local Accumulation")
	plt.xlabel("Number of SAs", fontsize = 18)
	plt.ylabel("Speedup", fontsize = 18)
	plt.legend(title="Pod size (s)", loc = "middle right")
	# plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	plt.show()
	plt.clf()





def plot_exp1fig6(fname = 'exp1fig6'):
	plt.rcParams.update({'font.size': 12})
	if not os.path.exists(fname):
		print("file %s not found" % fname)
		return
	#
	df = pandas.read_csv(fname)
	df = df.sort_values('number of SAs')
	NUM_SA = [4,8,16,32,64]
	single_pod_lat = df[(df['number of SAs'] == 4)]['number of cycles'].max()
	#
	i = 0 
	markers = ['o','v']
	n_factor = [1,2]
	for l in n_factor:
		speedup = single_pod_lat / df[(df['N factor'] == l)]['number of cycles']
		plt.plot(list(NUM_SA), list(speedup), label = l, marker = markers[i])
		i += 1
	#
	plt.title("Impact of Local Memory Size on Speedup")
	plt.xlabel("Number of SAs", fontsize = 18)
	plt.ylabel("Speedup", fontsize = 18)
	plt.legend(title="N-dim factor", loc = "middle right")
	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	plt.show()
	plt.clf()





def plot_exp1fig8(fname = 'exp1fig8'):
	plt.rcParams.update({'font.size': 16})
	if not os.path.exists(fname):
		print("file %s not found" % fname)
		return
	#
	df = pandas.read_csv(fname)
	df = df.sort_values('number of SAs')
	NUM_SA = [4,8,16,32,64]
	single_pod_lat = df[(df['bw growth'] == 'I')][(df['number of SAs'] == 4)]['number of cycles']._values[0]
	acc_s4 = df[(df['bw growth'] == 'P')][['number of SAs','number of cycles']]
	acc_s2 =  df[(df['bw growth'] == 'I')][['number of SAs','number of cycles']]
	# plt.plot(list(acc_s2['number of SAs']), 
	# 		list((single_pod_lat*100) / (acc_s2['number of cycles']*NUM_SA)), 
	# 		label = "constant", marker = 'o')
	# plt.plot(list(acc_s4['number of SAs']),
	# 		list((single_pod_lat*100) / (acc_s4['number of cycles']*NUM_SA)), 
	# 		label = "increasing", marker = 'o')
	# plt.title("Impact of Local Accumulation on Strong Scaling Efficiency")
	# plt.xlabel("Number of SAs", fontsize = 18)
	# plt.ylabel("Strong Scaling Efficiency (%)")
	# plt.legend(title="pod size", loc = "middle right")
	plt.plot(list(acc_s2['number of SAs']), 
			list(single_pod_lat / acc_s2['number of cycles']), 
			label = "constant", marker = 'o')
	plt.plot(list(acc_s4['number of SAs']),
			list(single_pod_lat / acc_s4['number of cycles']), 
			label = "increasing", marker = 'o')
	plt.title("Impact of Local Accumulation on Speedup")
	plt.xlabel("Number of SAs", fontsize = 18)
	plt.ylabel("Speedup", fontsize = 18)
	plt.legend(title="pod size", loc = "middle right")
	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	plt.show()
	plt.clf()







def plot_exp1_local_acc(fname = 'local_acc'):
	plt.rcParams.update({'font.size': 16})
	df = pandas.read_csv('exp1fig5')
	df = df.sort_values('number of SAs')
	NUM_SA = [16,32,64,128]
	single_pod_lat = df[(df['number of SAs'] == 16) & (df['bw growth'] == 'FH')]['number of cycles']._values[0]
	markers = ['o','v', '^']
	i = 0
	labels = ['max pod size + high bw', 'fixed pod size + high bw', 'max pod size + low bw']
	colors = ['g','b','aqua'] 	
	for s in ['MH','FH','ML']:
		speedup = single_pod_lat / df[(df['bw growth'] == s)]['number of cycles']
		plt.plot(list(NUM_SA), list(speedup), label = labels[i], marker = markers[i], color = colors[i])
		i += 1
	#
	plt.title('Impact of Local Accumulation')
	plt.xlabel("Number of SAs", fontsize = 18)
	plt.ylabel("Speedup", fontsize = 18)
	plt.legend(loc = "upper left", prop={'size': 13})
	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	plt.show()
	plt.clf()
	plt.close('all')








def op_intensity(M,N,K,M_sr,N_sr):
	M,N,K= float(M), float(N), float(K)
	mults = M*N*K
	P = M*N
	D = (M/M_sr)*K*N 
	W = (N/N_sr)*M*K 
	return (mults / (P+D+W))


def roofline():
	plt.rcParams.update({'font.size': 16})
	s2 = op_intensity(32,64,32,32,32)
	s4 = op_intensity(32,64,32,16,16)
	s8 = op_intensity(32,64,32,8,8)
	# Peak performance (tile mults/cycle)
	P_max = 32
	# Peak DRAM bw (tiles/cycle)
	b_s = 2
	#
	oi_list = [1/8., 1/4., 1/2., 1, 2, 3, 4, 6, 8, 16, 32, 64]
	p = [min(P_max, b_s*x) for x in oi_list]
	plt.plot(oi_list, p,'-b', label  = "roofline", linewidth=4, color = 'black')
	plt.scatter([s2],[0.893], color = 'r', s=40)
	plt.scatter([s4],[1.904], color = 'b', s=40)
	plt.scatter([s8],[3.529], color = 'g', s=40)
	#
	plt.title('Roofline Model for Various Pod Sizes')
	plt.xscale('log', basex=2)
	plt.yscale('log', basey=2)
	plt.xlabel('Operational Intensity (tile mults / tile)', fontsize = 18)
	plt.ylabel('Performance (tile mults / cycle)', fontsize = 18)
	# plt.grid()
	plt.axvline(16, label = 'memory/compute\nboundary', linestyle='dashed')
	# plt.text(16,0,'memory vs compute boundary',rotation=90)
	plt.annotate("s = 2", (3.5 ,0.8))
	plt.annotate("s = 4", (2 ,1.5))
	plt.annotate("s = 8", (1.5 ,2.3))
	plt.legend(loc = "upper left", prop={'size': 15})
	plt.savefig("roofline.pdf", bbox_inches='tight')
	plt.show()
	plt.clf()


def plot_exp1ABskip(fname = 'exp1ABskip'):
	plt.rcParams.update({'font.size': 16})
	df = pandas.read_csv('exp1fig7')
	df = df.sort_values('OP')
	# plt.figure(figsize=(3,3))
	markers = ['o','v']
	labels = {'Yes' : 'skip 1 level', 'No' : 'no skipping'}
	single_pod_lat = df[(df['skip'] == 'No') & (df['OP'] == 1)]['number of cycles']._values[0]
	i = 0
	colors = ['orange','gray']
	for d in ['Yes','No']:
		plt.plot(list(df[df['skip'] == d]['OP']), 
				list(664427 / df[(df['skip'] == d)]['number of cycles']), 
				label = labels[d], marker = markers[i], color = colors[i])
		# plt.plot(list(df[df['skip'] == d]['OP']), 
		# 		list(df[(df['skip'] == d)]['number of cycles']), label = d, marker = markers[i])
		i+=1
	#
	plt.title("Impact of AB Skipping on Speedup")
	plt.xlabel("Over-Provisioning Factor", fontsize = 18)
	plt.ylabel("Speedup", fontsize = 18)
	plt.xticks(np.arange(0,4,1))
	# plt.yticks(np.arange(0,0.15,0.01))
	plt.legend(loc = "upper left")
	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	plt.show()



 
def plot_mem_size_R(fname = 'mem_size_R', NUM_SA = 64):
	# 100 linearly spaced numbers
	R = np.linspace(1.01,2,100)
	SZ_sr = (NUM_SA*R) / (R-1)
	plt.figure(figsize=(4,3)) 
	plt.plot(R,SZ_sr, 'r')
	# plt.title("Local memory size as a function of R")
	plt.xlabel("R", fontsize = 18)
	plt.ylabel("memory size (tiles)", fontsize = 18)
	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	plt.show()
	plt.close('all')



def ext_mem_accesses(M,K,N,Sx,Sy,s,alpha,tile_sz):
	'''
		inputs : MMM dims and arch params
		return number accesses (loads and stores) to external memory (DRAM)
	'''
	M_sr = s*((Sx*Sy)/(s*s))*tile_sz
	K_sr = s*tile_sz
	N_sr = s*((Sx*Sy)/(s*s))*alpha*tile_sz
	num_cbs_blks = K/K_sr * M/M_sr * N/N_sr
	dram_transfers_per_blk = (M_sr*K_sr +  K_sr*N_sr) # CAKE transmits only weight and data
	result = M*N
	return(num_cbs_blks*dram_transfers_per_blk + result)


def local_mem_size(Sx,Sy,s,alpha,tile_sz):
	'''
		inputs : MMM dims and arch params
		return : local memory (SRAM) size in bytes
	'''
	M_sr = s*((Sx*Sy)/(s*s))*tile_sz
	K_sr = s*tile_sz
	N_sr = s*((Sx*Sy)/(s*s))*alpha*tile_sz
	weights = M_sr*K_sr
	data = K_sr*N_sr
	partial = M_sr*N_sr
	return((weights+data+partial)*4)




def plot_DRAM_access(M,K,N,tile_sz, fname = 'dram_acc'):
	plt.rcParams.update({'font.size': 12})
	# NUM_SA = [4,8,16,32,64,128,256]
	NUM_SA = [1,2,4,8]
	markers = ['o','v']
	colors = ['g','b'] 	
	# C = [(2,2),(4,2),(4,4),(8,4),(8,8),(16,8),(16,16)]
	# C = [(4,4),(8,4),(8,8),(16,8)]
	# C = [(8,8,8,2),(16,8,8,2),(16,16,8,2),(32,16,8,2)]
	C = [(8,8,8,1.25),(16,8,8,1.25),(16,16,16,1.125),(32,16,16,1.125)]
	labels = ['CAKE DRAM','Intel i9 DRAM', 'CAKE SRAM', 'i9 SRAM']
	mem_acc = [ext_mem_accesses(M,K,N,c[0],c[1],c[2],c[3],tile_sz) / (10**9) for c in C]
	mem_sz = [local_mem_size(c[0],c[1],c[2],c[3],tile_sz) for c in C]
	#
	NUM_CPUs = [1,2,3,4,5,6,7,8,9,10]
	cpu_mem_acc = [0]*len(NUM_CPUs)
	for i in NUM_CPUs:
		df1 = pandas.read_csv('reports/report_%d.csv' % i,skiprows=17,skipfooter=17)
		df2 = pandas.read_csv('reports/report_%d.csv' % i,skipfooter=20)
		avg_dram_bw = df1['Average']._values[0]
		elapsed_time = df2[df2['Metric Name']=='Elapsed Time']['Metric Value']._values[0]
		cpu_mem_acc[i-1] = (avg_dram_bw * elapsed_time)/4 # divide by 4 bytes to get number of 32bit floats transferred
	# cpu_mem_acc = [1633248996, 1548046440, 1270838124, 2028060840, 1592447772, 1208436252, 1764052920, 1372841184]
	#
	plt.plot(list(NUM_SA), list(mem_acc), label = labels[0],  marker = markers[0], color = colors[0])
	plt.plot(list(NUM_CPUs), list(cpu_mem_acc), label = labels[1],  marker = markers[1], color = colors[1])
	# plt.plot(list(NUM_SA), list(mem_sz), label = labels[2], marker = markers[0], color = colors[0])
	# plt.plot(list(NUM_CPUs),[20*(10**6)] * len(NUM_CPUs), label = labels[3], marker = markers[1], color = colors[1])
	#
	plt.title('DRAM Accesses in CAKE vs. Intel i9 CPU')
	plt.xlabel("Number of PEs", fontsize = 12)
	plt.ylabel("Number of Values Transferred (10^9)", fontsize = 12)
	plt.legend(loc = "upper right", prop={'size': 12})
	# plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	plt.show()
	plt.clf()
	plt.close('all')




def plot_cpu_speedup(fname = 'cpu_speedup'):
	plt.rcParams.update({'font.size': 12})
	NUM_SA = [1,2,4,8,16]
	markers = ['o','v','s','d','^']
	colors = ['b','g','aqua','k','m']
	labels = ['CAKE', 'Intel i9', 'Ideal']
	# mem_acc = [ext_mem_accesses(M,K,N,c[0],c[1],s,R,tile_sz) / (10**9) for c in C]
	# mem_sz = [local_mem_size(c[0],c[1],s,R,tile_sz) for c in C]
	#
	NUM_CPUs = [1,2,3,4,5,6,7,8,9,10]
	cpu_elapsed_time = [0]*len(NUM_CPUs)
	# CAKE_elapsed_time = [71305460/2,19123775,10546984,6311018]
	# CAKE_elapsed_time = [50135100,25104475,16777327,9743581,8388875,4395388]	
	# CAKE_elapsed_time = [50135100,25104475,12583052,9001763,5243533]	
	# CAKE_elapsed_time = [50135100,25104475,12583052,8695457,5243533]	
	# CAKE_elapsed_time = [50135100,25104475,12583052,8558623,4719757]	
	# CAKE_elapsed_time = [151003278,76596190,35684702,18788206]	
	# CAKE_elapsed_time = [582219,325740,164047,136049,82573]	
	CAKE_elapsed_time = [412221,280535,147790,136049,82573]	
	for i in NUM_CPUs:
		df2 = pandas.read_csv('reports/report_%d.csv' % i,skipfooter=20)
		cpu_elapsed_time[i-1] = df2[df2['Metric Name']=='Elapsed Time']['Metric Value']._values[0]
	# cpu_mem_acc = [1633248996, 1548046440, 1270838124, 2028060840, 1592447772, 1208436252, 1764052920, 1372841184]
	#
	speedup_cpu = [max(cpu_elapsed_time) / i for i in cpu_elapsed_time]
	speedup_CAKE = [max(CAKE_elapsed_time) / i for i in CAKE_elapsed_time]
	plt.plot(list(NUM_SA), list(speedup_CAKE), label = labels[0],  marker = markers[0], color = colors[0])
	plt.plot(list(NUM_CPUs), list(speedup_cpu), label = labels[1],  marker = markers[1], color = colors[1])
	plt.plot(list(NUM_CPUs), list(NUM_CPUs), label = labels[2],  linewidth=2, color = colors[2])	
	# plt.plot(list(NUM_SA), list(mem_sz), label = labels[2], marker = markers[0], color = colors[0])
	# plt.plot(list(NUM_CPUs),[20*(10**6)] * len(NUM_CPUs), label = labels[3], marker = markers[1], color = colors[1])
	#
	plt.title('Speedup for MMM on CAKE vs Intel CPU')
	plt.xlabel("Number of PEs", fontsize = 12)
	plt.ylabel("Speedup", fontsize = 12)
	plt.legend(loc = "lower right", prop={'size': 12})
	# plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	plt.show()
	plt.clf()
	plt.close('all')








def plot_cpu_dram_bw(fname = 'cpu_dram_bw'):
	plt.rcParams.update({'font.size': 12})
	# NUM_SA = [4,8,16,32,64,128,256]
	markers = ['o','v']
	colors = ['g','b'] 	
	# C = [(2,2),(4,2),(4,4),(8,4),(8,8),(16,8),(16,16)]
	# C = [(4,4),(8,4),(8,8),(16,8)]
	labels = ['MM Avg. DRAm bw', 'Peak DRAM bw']
	#
	NUM_CPUs = [1,2,3,4,5,6,7,8,9,10]
	avg_dram_bw = [0]*len(NUM_CPUs)
	for i in NUM_CPUs:
		df1 = pandas.read_csv('reports/report_%d.csv' % i,skiprows=17,skipfooter=17)
		avg_dram_bw[i-1] = df1['Average']._values[0]
	# cpu_mem_acc = [1633248996, 1548046440, 1270838124, 2028060840, 1592447772, 1208436252, 1764052920, 1372841184]
	#
	plt.plot(list(NUM_CPUs), list(avg_dram_bw), label = labels[0],  marker = markers[0], color = colors[0])
	plt.plot(list(NUM_CPUs), [40]*len(NUM_CPUs), label = labels[1],  color = colors[1],linewidth=2)
	# plt.plot(list(NUM_SA), list(mem_sz), label = labels[2], marker = markers[0], color = colors[0])
	# plt.plot(list(NUM_CPUs),[20*(10**6)] * len(NUM_CPUs), label = labels[3], marker = markers[1], color = colors[1])
	#
	plt.title('DRAM bw Utilization vs Num CPU Cores')
	plt.xlabel("Number of Cores", fontsize = 12)
	plt.ylabel("Average DRAM bw (GB/s)", fontsize = 12)
	plt.legend(loc = "lower right", prop={'size': 12})
	# plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	plt.show()
	plt.clf()
	plt.close('all')


def plot_local_mem_size(M,K,N,s,R,tile_sz):
	plt.rcParams.update({'font.size': 12})
	# NUM_SA = [4,8,16,32,64,128,256]
	NUM_SA = [1,2,4,8]
	markers = ['o','v']
	colors = ['g','b'] 	
	# C = [(2,2),(4,2),(4,4),(8,4),(8,8),(16,8),(16,16)]
	C = [(4,4),(8,4),(8,8),(16,8)]
	labels = ['CAKE SRAM', 'i9 SRAM']
	mem_sz = [local_mem_size(M,K,N,c[0],c[1],s,R,tile_sz) for c in C]
	NUM_CPUs = [1,2,3,4,5,6,7,8]
	cpu_mem_acc = [1633248996, 1548046440, 1270838124, 2028060840, 1592447772, 1208436252, 1764052920, 1372841184]
	#
	plt.plot(list(NUM_SA), list(mem_sz), label = labels[0], marker = markers[0], color = colors[0])
	plt.plot(list(NUM_CPUs),[20*(10**6)] * len(NUM_CPUs), label = labels[1], marker = markers[1], color = colors[1])
	#
	plt.title('SRAM Size in CAKE vs. Intel i9 CPU')
	plt.xlabel("Number of PEs", fontsize = 12)
	plt.ylabel("Bytes", fontsize = 12)
	plt.legend(loc = "upper right", prop={'size': 12})
	# plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	plt.show()
	plt.clf()
	plt.close('all')






def test(fname = 'all_combs'):
	plt.rcParams.update({'font.size': 12})
	if not os.path.exists(fname):
		print("file %s not found" % fname)
		return
	#
	df = pandas.read_csv(fname)
	df = df.sort_values('number of SAs')
	SAs = [4,8,16,32,64,128,256]
	sa1 = [4,8,16,32,64,128]
	sa2 = [16,32,64,128,256]
	sa3 = [64,128,256]
	sa4 = [256]
	NUM_SA = [sa1]+[sa2]+[sa3]+[sa4]
	single_pod = 4 # 2x2 pod
	single_pod_lat = df[(df['number of SAs'] == single_pod) & (df['s'] == 2)]['number of cycles']._values[0] / 2
	#
	markers = ['o','v','s','d','^']
	colors = ['b','g','aqua','k','m']
	s = [2,4,8,16]
	for i in xrange(len(s)):
		speedup = single_pod_lat / df[(df['s'] == s[i])]['number of cycles']
		plt.plot(list(NUM_SA[i]), list(speedup), label = s[i], marker = markers[i])
	#
	plt.plot(SAs, [i/single_pod for i in SAs], label = 'ideal')	
	plt.title("Impact of Local Accumulation on Speedup")
	plt.xlabel("Number of SAs", fontsize = 18)
	plt.ylabel("Speedup", fontsize = 18)
	plt.legend(title="s value", loc = "middle right")
	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	plt.show()
	plt.clf()





if __name__ == '__main__':
	plot_exp1fig1()
	plot_exp1fig2()
	plot_exp1fig3()
	plot_exp1fig4()
	roofline()





