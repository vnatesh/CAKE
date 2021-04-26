import pandas
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
import numpy as np
import os
import re


def ext_mem_accesses(M,K,N,Sx,Sy,s,alpha,tile_sz,p=None):
	if p == None:
		p = ((Sx*Sy)/(s*s))
	M_sr = s*p*tile_sz
	K_sr = s*tile_sz
	N_sr = s*p*alpha*tile_sz
	num_cbs_blks = K/K_sr * M/M_sr * N/N_sr
	dram_transfers_per_blk = (M_sr*K_sr +  K_sr*N_sr) # CAKE transmits only weight and data
	result = M*N
	return(num_cbs_blks , num_cbs_blks*dram_transfers_per_blk + result)


def num_cb_blks(M,K,N,Sx,Sy,s,alpha,tile_sz,p=None):
	if p == None:
		p = ((Sx*Sy)/(s*s))
	M_sr = s*p*tile_sz
	K_sr = s*tile_sz
	N_sr = s*p*alpha*tile_sz
	return(K/K_sr * M/M_sr * N/N_sr)


'''
	inputs : MMM dims and arch params
	return : local memory (SRAM) size in bytes
'''
def local_mem_size(Sx,Sy,s,alpha,tile_sz,p=None):
	if p == None:
		p = ((Sx*Sy)/(s*s))	
	M_sr = s*p*tile_sz
	K_sr = s*tile_sz
	N_sr = s*p*alpha*tile_sz
	weights = M_sr*K_sr
	data = K_sr*N_sr
	partial = M_sr*N_sr
	return(weights+data+partial)

def plot_local_mem_sz(fname = 'local_mem_sz'):
	plt.rcParams.update({'font.size': 12})
	markers = ['o','v','s','d','^']
	colors = ['g','b','aqua','k','m','r']
	plt.figure(figsize = (6,4))
	plt.plot(list(range(1,17)), [local_mem_size(8,8,2,1,8,i) for i in range(1,17)],
		marker = markers[0], color = colors[0])
	plt.title("Local Memory Size vs. Compute Power",fontsize = 18)
	plt.xlabel("Number of SAs",fontsize = 18)
	plt.xlim(xmin=0)
	plt.ylabel("Local Memory Size (bytes)",fontsize = 18)
	plt.ylim(ymin=0)
	# plt.legend(title="Internal BW", loc = "upper left")
	# plt.savefig("%s.pdf" % fname, bbox_inches='tight')
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
	plt.figure(figsize = (6,4))
	plt.plot(list(constant['number of pods']), 
			list(single_pod_lat / constant['number of cycles']), 
			label = labels[0], marker = markers[0], color = colors[0])
	plt.plot(list(linear_inc['number of pods']),
			list(single_pod_lat / linear_inc['number of cycles']), 
			label = labels[1], marker = markers[1], color = colors[1])
	plt.title("(a) Impact of Internal BW on Speedup",fontsize = 18)
	plt.xlabel("Number of Pods",fontsize = 18)
	plt.xlim(xmin=0)
	plt.ylabel("Speedup",fontsize = 18)
	plt.ylim(ymin=0)
	plt.legend(title="Internal BW", loc = "upper left")
	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	# plt.show()
	# plt.clf()
	# plt.close('all')


def plot_mem_accesses(fname = 'mem_access_cell'):
	plt.rcParams.update({'font.size': 12})
	markers = ['o','v','s','d','^']
	colors = ['g','b','k','r','m','aqua']	
	labels = ['staging buffers, s=2', 'local memory, s=2','staging buffers, s=4', 'local memory, s=4']
	NUM_PODS = [2,3,4,6,8,12,16]
	NUM_SA = [i*2*2 for i in NUM_PODS]
	NUM_PODS1 = [1,2,3,4]
	NUM_SA1 = [i*4*4 for i in NUM_PODS1]
	plt.figure(figsize = (6,4))
	plt.plot(NUM_SA, [2*2*num_cb_blks(768,768,768,8,8,2,1,8,i) for i in NUM_PODS], 
			label = labels[0], marker = markers[0], color = colors[0])
	plt.plot(NUM_SA, [2*3*num_cb_blks(768,768,768,8,8,2,1,8,i) for i in NUM_PODS], 
			label = labels[1], marker = markers[1], color = colors[1])
	plt.plot(NUM_SA1, [2*2*num_cb_blks(768,768,768,8,8,4,1,8,i) for i in NUM_PODS1], 
			label = labels[2], marker = markers[0], color = colors[2])
	plt.plot(NUM_SA1, [2*3*num_cb_blks(768,768,768,8,8,4,1,8,i) for i in NUM_PODS1], 
			label = labels[3], marker = markers[1], color = colors[3])
	plt.title("Memory Accesses Per Cell In Maestro",fontsize = 18)
	plt.xlabel("Number of SAs",fontsize = 18)
	plt.xlim(xmin=0)
	plt.xticks(NUM_SA)
	plt.ylabel("Number Memory Accesses",fontsize = 18)
	plt.ylim(ymin=0)
	plt.legend(title="Memory Type", loc = "center right")
	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	plt.show()
	plt.clf()
	plt.close('all')



def plot_access_per_cycle(fname = 'access_per_cycle_cell'):
	plt.rcParams.update({'font.size': 12})
	markers = ['o','v','s','d','^']
	colors = ['g','b','r','k','m','aqua']
	labels = ['staging buffers, s=2', 'local memory, s=2','staging buffers, s=4', 'local memory, s=4']
	#
	df = pandas.read_csv('exp1fig1')
	df = df.sort_values('number of pods')
	NUM_PODS = [1,2,3,4,6,8,12,16]
	single_pod_lat = df[(df['bw growth'] == 'C')][(df['number of pods'] == 1)]['number of cycles']._values[0]
	constant = df[(df['bw growth'] == 'C')][['number of pods','number of cycles']]
	linear_inc =  df[(df['bw growth'] == 'I')][['number of pods','number of cycles']]
	NUM_SA = [i*2*2 for i in NUM_PODS]
	plt.figure(figsize = (6,4))
	plt.plot(NUM_SA, [(2*2*num_cb_blks(768,768,768,8,8,2,1,8,NUM_PODS[i])) / 
		(list(linear_inc['number of cycles'])[i] / (10**9)) for i in range(len(NUM_PODS))], 
		label = labels[0], marker = markers[0], color = colors[0])
	plt.plot(NUM_SA, [(2*3*num_cb_blks(768,768,768,8,8,2,1,8,NUM_PODS[i])) / 
		(list(linear_inc['number of cycles'])[i] / (10**9)) for i in range(len(NUM_PODS))], 
		label = labels[1], marker = markers[1], color = colors[1])
	plt.title("Avg. Memory Accesses/Sec Per Cell",fontsize = 18)
	plt.xlabel("Number of SAs",fontsize = 18)
	plt.xlim(xmin=0)
	plt.xticks(NUM_SA)
	plt.ylabel("Avg. Memory Accesses/sec",fontsize = 18)
	plt.ylim(ymin=0)
	plt.legend(title="memory type", loc = "upper right")
	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
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
	single_pod_lat = df[(df['lat_dram'] == 36) & (df['number of pods'] == 1)]['number of cycles']._values[0]
	tile_sz = 8
	#
	lat_dram = [128,64,42,36]
	i = 0;
	plt.figure(figsize = (6, 4))
	for l in lat_dram:
		speedup = single_pod_lat / df[(df['lat_dram'] == l)]['number of cycles']
		plt.plot(list(NUM_PODS), list(speedup), label = round((2.0/l)*(tile_sz**2)), marker = markers[i], color = colors[i])
		i += 1
	#
	plt.title("(b) Impact of External BW on Speedup", fontsize = 18)
	plt.xlabel("Number of Pods", fontsize = 18)
	plt.xlim(xmin=0)
	plt.ylabel("Speedup",fontsize = 18)
	plt.ylim(ymin=0)
	plt.legend(title="External BW (GB/s)", loc = "upper left")
	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	# plt.show()
	# plt.clf()
	# plt.close('all')



if __name__ == '__main__':
	plot_exp1fig1()
	plot_exp1fig2()
	# plot_exp1fig3()
	# plot_exp1fig4()
	# roofline()





#------------ OLD PLOTS--------------#


# def plot_exp1fig3(fname = 'exp1fig3'):
# 	plt.rcParams.update({'font.size': 12})
# 	markers = ['o','v','s','d','^']
# 	colors = ['b','g','aqua','k','m']
# 	labels = ['max pod size + low bw', 'fixed pod size + high bw', 'max pod size + high bw']
# 	if not os.path.exists(fname):
# 		print("file %s not found" % fname)
# 		return
# 	#
# 	df = pandas.read_csv(fname)
# 	df = df.sort_values('NUM_SA')
# 	NUM_SA = [4,8,16,32,64,128,256]
# 	single_pod_lat = df[(df['s'] == 'l') & (df['NUM_SA'] == 4)]['number of cycles']._values[0]
# 	#
# 	i = 0
# 	for s in ['h','l','m']:
# 		speedup = single_pod_lat / df[(df['s'] == s)]['number of cycles']
# 		plt.plot(list(NUM_SA), list(speedup), label = labels[i], marker = markers[i])
# 		i += 1
# 	#
# 	plt.title("Impact of Local Accumulation")
# 	plt.xlabel("Number of SAs", fontsize = 18)
# 	plt.ylabel("Speedup", fontsize = 18)
# 	plt.legend(loc = "upper left")
# 	# plt.savefig("%s.pdf" % fname, bbox_inches='tight')
# 	plt.show()
# 	plt.clf()



# def plot_exp1fig6(fname = 'exp1fig6'):
# 	plt.rcParams.update({'font.size': 12})
# 	if not os.path.exists(fname):
# 		print("file %s not found" % fname)
# 		return
# 	#
# 	df = pandas.read_csv(fname)
# 	df = df.sort_values('number of SAs')
# 	NUM_SA = [4,8,16,32,64]
# 	single_pod_lat = df[(df['number of SAs'] == 4)]['number of cycles'].max()
# 	#
# 	i = 0 
# 	markers = ['o','v']
# 	n_factor = [1,2]
# 	for l in n_factor:
# 		speedup = single_pod_lat / df[(df['N factor'] == l)]['number of cycles']
# 		plt.plot(list(NUM_SA), list(speedup), label = l, marker = markers[i])
# 		i += 1
# 	#
# 	plt.title("Impact of Local Memory Size on Speedup")
# 	plt.xlabel("Number of SAs", fontsize = 18)
# 	plt.ylabel("Speedup", fontsize = 18)
# 	plt.legend(title="N-dim factor", loc = "middle right")
# 	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
# 	plt.show()
# 	plt.clf()



# def plot_exp1fig8(fname = 'exp1fig8'):
# 	plt.rcParams.update({'font.size': 16})
# 	if not os.path.exists(fname):
# 		print("file %s not found" % fname)
# 		return
# 	#
# 	df = pandas.read_csv(fname)
# 	df = df.sort_values('number of SAs')
# 	NUM_SA = [4,8,16,32,64]
# 	single_pod_lat = df[(df['bw growth'] == 'I')][(df['number of SAs'] == 4)]['number of cycles']._values[0]
# 	acc_s4 = df[(df['bw growth'] == 'P')][['number of SAs','number of cycles']]
# 	acc_s2 =  df[(df['bw growth'] == 'I')][['number of SAs','number of cycles']]
# 	# plt.plot(list(acc_s2['number of SAs']), 
# 	# 		list((single_pod_lat*100) / (acc_s2['number of cycles']*NUM_SA)), 
# 	# 		label = "constant", marker = 'o')
# 	# plt.plot(list(acc_s4['number of SAs']),
# 	# 		list((single_pod_lat*100) / (acc_s4['number of cycles']*NUM_SA)), 
# 	# 		label = "increasing", marker = 'o')
# 	# plt.title("Impact of Local Accumulation on Strong Scaling Efficiency")
# 	# plt.xlabel("Number of SAs", fontsize = 18)
# 	# plt.ylabel("Strong Scaling Efficiency (%)")
# 	# plt.legend(title="pod size", loc = "middle right")
# 	plt.plot(list(acc_s2['number of SAs']), 
# 			list(single_pod_lat / acc_s2['number of cycles']), 
# 			label = "constant", marker = 'o')
# 	plt.plot(list(acc_s4['number of SAs']),
# 			list(single_pod_lat / acc_s4['number of cycles']), 
# 			label = "increasing", marker = 'o')
# 	plt.title("Impact of Local Accumulation on Speedup")
# 	plt.xlabel("Number of SAs", fontsize = 18)
# 	plt.ylabel("Speedup", fontsize = 18)
# 	plt.legend(title="pod size", loc = "middle right")
# 	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
# 	plt.show()
# 	plt.clf()



# def plot_exp1_local_acc(fname = 'local_acc'):
# 	plt.rcParams.update({'font.size': 16})
# 	df = pandas.read_csv('exp1fig5')
# 	df = df.sort_values('number of SAs')
# 	NUM_SA = [16,32,64,128]
# 	single_pod_lat = df[(df['number of SAs'] == 16) & (df['bw growth'] == 'FH')]['number of cycles']._values[0]
# 	markers = ['o','v', '^']
# 	i = 0
# 	labels = ['max pod size + high bw', 'fixed pod size + high bw', 'max pod size + low bw']
# 	colors = ['g','b','aqua'] 	
# 	for s in ['MH','FH','ML']:
# 		speedup = single_pod_lat / df[(df['bw growth'] == s)]['number of cycles']
# 		plt.plot(list(NUM_SA), list(speedup), label = labels[i], marker = markers[i], color = colors[i])
# 		i += 1
# 	#
# 	plt.title('Impact of Local Accumulation')
# 	plt.xlabel("Number of SAs", fontsize = 18)
# 	plt.ylabel("Speedup", fontsize = 18)
# 	plt.legend(loc = "upper left", prop={'size': 13})
# 	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
# 	plt.show()
# 	plt.clf()
# 	plt.close('all')



# def op_intensity(M,N,K,M_sr,N_sr):
# 	M,N,K= float(M), float(N), float(K)
# 	mults = M*N*K
# 	P = M*N
# 	D = (M/M_sr)*K*N 
# 	W = (N/N_sr)*M*K 
# 	return (mults / (P+D+W))


# def roofline():
# 	plt.rcParams.update({'font.size': 16})
# 	s2 = op_intensity(32,64,32,32,32)
# 	s4 = op_intensity(32,64,32,16,16)
# 	s8 = op_intensity(32,64,32,8,8)
# 	# Peak performance (tile mults/cycle)
# 	P_max = 32
# 	# Peak DRAM bw (tiles/cycle)
# 	b_s = 2
# 	#
# 	oi_list = [1/8., 1/4., 1/2., 1, 2, 3, 4, 6, 8, 16, 32, 64]
# 	p = [min(P_max, b_s*x) for x in oi_list]
# 	plt.plot(oi_list, p,'-b', label  = "roofline", linewidth=4, color = 'black')
# 	plt.scatter([s2],[0.893], color = 'r', s=40)
# 	plt.scatter([s4],[1.904], color = 'b', s=40)
# 	plt.scatter([s8],[3.529], color = 'g', s=40)
# 	#
# 	plt.title('Roofline Model for Various Pod Sizes')
# 	plt.xscale('log', basex=2)
# 	plt.yscale('log', basey=2)
# 	plt.xlabel('Operational Intensity (tile mults / tile)', fontsize = 18)
# 	plt.ylabel('Performance (tile mults / cycle)', fontsize = 18)
# 	# plt.grid()
# 	plt.axvline(16, label = 'memory/compute\nboundary', linestyle='dashed')
# 	# plt.text(16,0,'memory vs compute boundary',rotation=90)
# 	plt.annotate("s = 2", (3.5 ,0.8))
# 	plt.annotate("s = 4", (2 ,1.5))
# 	plt.annotate("s = 8", (1.5 ,2.3))
# 	plt.legend(loc = "upper left", prop={'size': 15})
# 	plt.savefig("roofline.pdf", bbox_inches='tight')
# 	plt.show()
# 	plt.clf()


# def plot_exp1ABskip(fname = 'exp1ABskip'):
# 	plt.rcParams.update({'font.size': 16})
# 	df = pandas.read_csv('exp1fig7')
# 	df = df.sort_values('OP')
# 	# plt.figure(figsize=(3,3))
# 	markers = ['o','v']
# 	labels = {'Yes' : 'skip 1 level', 'No' : 'no skipping'}
# 	single_pod_lat = df[(df['skip'] == 'No') & (df['OP'] == 1)]['number of cycles']._values[0]
# 	i = 0
# 	colors = ['orange','gray']
# 	for d in ['Yes','No']:
# 		plt.plot(list(df[df['skip'] == d]['OP']), 
# 				list(664427 / df[(df['skip'] == d)]['number of cycles']), 
# 				label = labels[d], marker = markers[i], color = colors[i])
# 		# plt.plot(list(df[df['skip'] == d]['OP']), 
# 		# 		list(df[(df['skip'] == d)]['number of cycles']), label = d, marker = markers[i])
# 		i+=1
# 	#
# 	plt.title("Impact of AB Skipping on Speedup")
# 	plt.xlabel("Over-Provisioning Factor", fontsize = 18)
# 	plt.ylabel("Speedup", fontsize = 18)
# 	plt.xticks(np.arange(0,4,1))
# 	# plt.yticks(np.arange(0,0.15,0.01))
# 	plt.legend(loc = "upper left")
# 	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
# 	plt.show()



 
# def plot_mem_size_R(fname = 'mem_size_R', NUM_SA = 64):
# 	# 100 linearly spaced numbers
# 	R = np.linspace(1.01,2,100)
# 	SZ_sr = (NUM_SA*R) / (R-1)
# 	plt.figure(figsize=(4,3)) 
# 	plt.plot(R,SZ_sr, 'r')
# 	# plt.title("Local memory size as a function of R")
# 	plt.xlabel("R", fontsize = 18)
# 	plt.ylabel("memory size (tiles)", fontsize = 18)
# 	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
# 	plt.show()
# 	plt.close('all')





