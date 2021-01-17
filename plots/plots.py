import pandas
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
import numpy as np
import os


def plot_exp1fig1(fname = 'exp1fig1'):
	plt.rcParams.update({'font.size': 16})
	if not os.path.exists(fname):
		print("file %s not found" % fname)
		return
	#
	df = pandas.read_csv(fname)
	df = df.sort_values('number of SAs')
	NUM_SA = [16,32,64]
	single_pod_lat = df[(df['bw growth'] == 'C')][(df['number of SAs'] == 16)]['number of cycles']._values[0]
	constant = df[(df['bw growth'] == 'C')][['number of SAs','number of cycles']]
	linear_inc =  df[(df['bw growth'] == 'I')][['number of SAs','number of cycles']]
	plt.plot(list(constant['number of SAs']), 
			list(single_pod_lat / constant['number of cycles']), 
			label = "constant", marker = 'o', color = 'r')
	plt.plot(list(linear_inc['number of SAs']),
			list(single_pod_lat / linear_inc['number of cycles']), 
			label = "linearly increasing", marker = 'v', color = 'b')
	plt.title("Impact of Internal BW on Speedup")
	plt.xlabel("Number of SAs", fontsize = 20)
	plt.ylabel("Speedup", fontsize = 20)
	plt.legend(title="internal bandwidth", loc = "middle right")
	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
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
	plt.xlabel("Number of SAs", fontsize = 20)
	plt.ylabel("Speedup", fontsize = 20)
	plt.legend(title="internal bw\nincrease factor", loc = "middle right", prop={'size': 13})
	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	plt.show()
	plt.clf()



def plot_exp1fig4(fname = 'exp1fig4'):
	plt.rcParams.update({'font.size': 16})
	if not os.path.exists(fname):
		print("file %s not found" % fname)
		return
	#
	df = pandas.read_csv(fname)
	df = df.sort_values('number of SAs')
	NUM_SA = [16,32,64]
	single_pod_lat = df[(df['lat_dram'] == 4) & (df['number of SAs'] == 16)]['number of cycles']._values[0]
	#
	# lat_dram = [20,10,5,1]
	lat_dram = [32,16,8,4]
	markers = ['o','v','s','d','^']
	colors = ['brown','y','chartreuse','b']
	i = 0;
	for l in lat_dram:
		speedup = single_pod_lat / df[(df['lat_dram'] == l)]['number of cycles']
		plt.plot(list(NUM_SA), list(speedup), label = 2.0/l, marker = markers[i], color = colors[i])
		i += 1
	#
	plt.title("Impact of External BW on Speedup")
	plt.xlabel("Number of SAs", fontsize = 20)
	plt.ylabel("Speedup", fontsize = 20)
	plt.legend(title="external bw", loc = "middle right")
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
	plt.xlabel("Number of SAs", fontsize = 20)
	plt.ylabel("Speedup", fontsize = 20)
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
	plt.xlabel("Number of SAs", fontsize = 20)
	plt.ylabel("Speedup", fontsize = 20)
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
	# plt.xlabel("Number of SAs", fontsize = 20)
	# plt.ylabel("Strong Scaling Efficiency (%)")
	# plt.legend(title="pod size", loc = "middle right")
	plt.plot(list(acc_s2['number of SAs']), 
			list(single_pod_lat / acc_s2['number of cycles']), 
			label = "constant", marker = 'o')
	plt.plot(list(acc_s4['number of SAs']),
			list(single_pod_lat / acc_s4['number of cycles']), 
			label = "increasing", marker = 'o')
	plt.title("Impact of Local Accumulation on Speedup")
	plt.xlabel("Number of SAs", fontsize = 20)
	plt.ylabel("Speedup", fontsize = 20)
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
	plt.xlabel("Number of SAs", fontsize = 20)
	plt.ylabel("Speedup", fontsize = 20)
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
	plt.xlabel('Operational Intensity (tile mults / tile)', fontsize = 20)
	plt.ylabel('Performance (tile mults / cycle)', fontsize = 20)
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
	plt.xlabel("Over-Provisioning Factor", fontsize = 20)
	plt.ylabel("Speedup", fontsize = 20)
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
	plt.xlabel("R", fontsize = 20)
	plt.ylabel("memory size (tiles)", fontsize = 20)
	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	plt.show()
	plt.close('all')



def ext_mem_accesses(M,K,N,Sx,Sy,s,R,tile_sz):
	'''
		inputs : MMM dims and arch params
		return number accesses (loads and stores) to external memory (DRAM)
	'''
	M_sr = s*((Sx*Sy)/(s*s))*tile_sz
	K_sr = s*tile_sz
	N_sr = s*((Sx*Sy)/(s*s))*(1/(R-1))*tile_sz
	num_cbs_blks = K/K_sr * M/M_sr * N/N_sr
	dram_transfers_per_blk = (M_sr*K_sr +  K_sr*N_sr) # CAKE transmits only weight and data
	result = M*N
	return(num_cbs_blks*dram_transfers_per_blk + result)


def local_mem_size(Sx,Sy,s,R,tile_sz):
	'''
		inputs : MMM dims and arch params
		return : local memory (SRAM) size in bytes
	'''
	M_sr = s*((Sx*Sy)/(s*s))*tile_sz
	K_sr = s*tile_sz
	N_sr = s*((Sx*Sy)/(s*s))*(1/(R-1))*tile_sz
	weights = M_sr*K_sr
	data = K_sr*N_sr
	partial = M_sr*N_sr
	return(weights+data+partial)




def plot_DRAM_access(M,K,N,s,R,tile_sz, fname = 'dram_acc'):
	plt.rcParams.update({'font.size': 12})
	# NUM_SA = [4,8,16,32,64,128,256]
	NUM_SA = [1,2,4,8]
	markers = ['o','v']
	colors = ['g','b'] 	
	# C = [(2,2),(4,2),(4,4),(8,4),(8,8),(16,8),(16,16)]
	# C = [(4,4),(8,4),(8,8),(16,8)]
	C = [(64,64),(128,64),(128,128),(256,128)]
	labels = ['CAKE DRAM','Intel i9 DRAM', 'CAKE SRAM', 'i9 SRAM']
	mem_acc = [ext_mem_accesses(M,K,N,c[0],c[1],s,R,tile_sz) / (10**9) for c in C]
	mem_sz = [local_mem_size(c[0],c[1],s,R,tile_sz) for c in C]
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
	plt.xlabel("Number of SAs", fontsize = 20)
	plt.ylabel("Speedup", fontsize = 20)
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





