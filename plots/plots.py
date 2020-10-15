import pandas
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
import numpy as np
import os


def plot_exp1fig1(fname = 'exp1fig1'):
	plt.rcParams.update({'font.size': 14})
	if not os.path.exists(fname):
		print("file %s not found" % fname)
		return
	#
	df = pandas.read_csv(fname)
	df = df.sort_values('number of SAs')
	NUM_SA = [4,8,16,32,64]
	single_pod_lat = df[(df['bw growth'] == 'C')][(df['number of SAs'] == 4)]['number of cycles']._values[0]
	constant = df[(df['bw growth'] == 'C')][['number of SAs','number of cycles']]
	linear_inc =  df[(df['bw growth'] == 'I')][['number of SAs','number of cycles']]
	plt.plot(list(constant['number of SAs']), 
			list(single_pod_lat / constant['number of cycles']), 
			label = "constant", marker = 'o')
	plt.plot(list(linear_inc['number of SAs']),
			list(single_pod_lat / linear_inc['number of cycles']), 
			label = "linearly increasing", marker = 'v')
	plt.title("Impact of Internal Bandwidth on Speedup")
	plt.xlabel("number of SAs")
	plt.ylabel("speedup")
	plt.legend(title="internal bandwidth", loc = "middle right")
	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	plt.show()
	plt.clf()




def plot_exp1fig3(fname = 'exp1fig3'):
	plt.rcParams.update({'font.size': 14})
	if not os.path.exists(fname):
		print("file %s not found" % fname)
		return
	#
	df = pandas.read_csv(fname)
	df = df.sort_values('number of SAs')
	NUM_SA = [4,8,16,32,64]
	single_pod_lat = df[(df['number of SAs'] == 4) & (df['lat_internal_factor'] == 2)]['number of cycles']._values[0]/2
	#
	markers = ['o','v','s','d','^']
	i = 0
	for l in [2,2.5,3,4]:
		# single_pod_lat = df[(df['number of SAs'] == 4) & (df['lat_internal_factor'] == l)]['number of cycles']._values[0]
		speedup = single_pod_lat / df[(df['lat_internal_factor'] == l)]['number of cycles']
		plt.plot(list(NUM_SA), list(speedup), label = l/2.0, marker = markers[i])
		i += 1
	#
	plt.title("Impact of Internal Bandwidth on Speedup")
	plt.xlabel("number of SAs")
	plt.ylabel("speedup")
	plt.legend(title="internal bw\nincrease factor", loc = "middle right")
	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	plt.show()
	plt.clf()





def plot_exp1fig4(fname = 'exp1fig4'):
	plt.rcParams.update({'font.size': 14})
	if not os.path.exists(fname):
		print("file %s not found" % fname)
		return
	#
	df = pandas.read_csv(fname)
	df = df.sort_values('number of SAs')
	NUM_SA = [4,8,16,32,64]
	single_pod_lat = df[(df['lat_dram'] == 16) & (df['number of SAs'] == 4)]['number of cycles']._values[0]
	#
	lat_dram = [16,8,4,2,1]
	markers = ['o','v','s','d','^']
	i = 0;
	for l in lat_dram:
		speedup = single_pod_lat / df[(df['lat_dram'] == l)]['number of cycles']
		plt.plot(list(NUM_SA), list(speedup), label = 2.0/l, marker = markers[i])
		i += 1
	#
	plt.title("Impact of External Bandwidth on Speedup")
	plt.xlabel("number of SAs")
	plt.ylabel("speedup")
	plt.legend(title="external bw", loc = "middle right")
	# plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	plt.show()
	plt.clf()




def plot_exp1fig5(fname = 'exp1fig5'):
	plt.rcParams.update({'font.size': 14})
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
	plt.xlabel("number of SAs")
	plt.ylabel("speedup")
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
	plt.xlabel("number of SAs")
	plt.ylabel("speedup")
	plt.legend(title="N-dim factor", loc = "middle right")
	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	plt.show()
	plt.clf()





def plot_exp1fig8(fname = 'exp1fig8'):
	plt.rcParams.update({'font.size': 14})
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
	# plt.xlabel("number of SAs")
	# plt.ylabel("Strong Scaling Efficiency (%)")
	# plt.legend(title="pod size", loc = "middle right")
	plt.plot(list(acc_s2['number of SAs']), 
			list(single_pod_lat / acc_s2['number of cycles']), 
			label = "constant", marker = 'o')
	plt.plot(list(acc_s4['number of SAs']),
			list(single_pod_lat / acc_s4['number of cycles']), 
			label = "increasing", marker = 'o')
	plt.title("Impact of Local Accumulation on Speedup")
	plt.xlabel("number of SAs")
	plt.ylabel("speedup")
	plt.legend(title="pod size", loc = "middle right")
	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	plt.show()
	plt.clf()






def plot_exp1_local_acc(fname = 'local_acc'):
	plt.rcParams.update({'font.size': 14})
	markers = ['o','v', '^']
	i = 0
	labels = ['max pod size + high bw', 'fixed pod size + high bw', 'max pod size + low bw']
	#
	# fig, (ax1, ax2) = plt.subplots(nrows = 1, ncols = 2, sharex=True, sharey=True,)
	# fig.suptitle('Impact of Local Accumulation')
	plt.title('Impact of Local Accumulation')
	#
	# add a big axes, hide frame
	# fig.add_subplot(111, frameon=False)
	#
	df = pandas.read_csv('exp1fig8')
	df = df.sort_values('number of SAs')
	NUM_SA = [4,8,16,32,64]
	single_pod_lat = df[(df['bw growth'] == 'I')][(df['number of SAs'] == 4)]['number of cycles']._values[0]
	acc_s4 = df[(df['bw growth'] == 'P')][['number of SAs','number of cycles']]
	acc_s2 =  df[(df['bw growth'] == 'I')][['number of SAs','number of cycles']]
	#
	# plt.plot(list(acc_s2['number of SAs']), 
	# 		list(single_pod_lat / acc_s2['number of cycles']), 
	# 		label = "constant", marker = 'o')
	plt.plot(list(acc_s4['number of SAs']),
			list(single_pod_lat / acc_s4['number of cycles']), 
			label = labels[i], marker = markers[i])
	i += 1
	#
	df = pandas.read_csv('exp1fig5')
	df = df.sort_values('number of SAs')
	NUM_SA = [4,8,16,32,64]
	single_pod_lat = df[(df['s'] == 2) & (df['number of SAs'] == 4) & (df['int'] == 'H')]['number of cycles']._values[0]
	#
	for s in ['H','L']:
		speedup = single_pod_lat / df[(df['int'] == s)]['number of cycles']
		plt.plot(list(NUM_SA), list(speedup), label = labels[i], marker = markers[i])
		i += 1
	#
	# plt.tick_params(labelcolor='none', top=False, bottom=False, left=False, right=False)
	# plt.grid(False)
	plt.xlabel("number of SAs")
	plt.ylabel("speedup")
	# plt.sca(ax1)
	# plt.xticks(NUM_SA[1:])
	# plt.yticks(np.arange(0,70,10))
	plt.legend(loc = "upper left")
	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	plt.show()
	plt.clf()
	plt.close('all')




# def plot_exp1_local_acc(fname = 'local_acc'):
# 	plt.rcParams.update({'font.size': 14})
# 	#
# 	fig, (ax1, ax2) = plt.subplots(nrows = 1, ncols = 2, sharex=True, sharey=True,)
# 	fig.suptitle('Impact of Local Accumulation')
# 	#
# 	# add a big axes, hide frame
# 	fig.add_subplot(111, frameon=False)
# 	#
# 	df = pandas.read_csv('exp1fig8')
# 	df = df.sort_values('number of SAs')
# 	NUM_SA = [4,8,16,32,64]
# 	single_pod_lat = df[(df['bw growth'] == 'I')][(df['number of SAs'] == 4)]['number of cycles']._values[0]
# 	acc_s4 = df[(df['bw growth'] == 'P')][['number of SAs','number of cycles']]
# 	acc_s2 =  df[(df['bw growth'] == 'I')][['number of SAs','number of cycles']]
# 	#
# 	ax1.plot(list(acc_s2['number of SAs']), 
# 			list(single_pod_lat / acc_s2['number of cycles']), 
# 			label = "constant", marker = 'o')
# 	ax1.plot(list(acc_s4['number of SAs']),
# 			list(single_pod_lat / acc_s4['number of cycles']), 
# 			label = "increasing", marker = 'v')
# 	#
# 	df = pandas.read_csv('exp1fig5')
# 	df = df.sort_values('number of SAs')
# 	NUM_SA = [4,8,16,32,64]
# 	single_pod_lat = df[(df['s'] == 2) & (df['number of SAs'] == 4) & (df['int'] == 'H')]['number of cycles']._values[0]
# 	#
# 	markers = ['o','v']
# 	labels = ['fixed', 'max']
# 	i = 0
# 	for s in ['H','L']:
# 		speedup = single_pod_lat / df[(df['int'] == s)]['number of cycles']
# 		ax2.plot(list(NUM_SA), list(speedup), label = labels[i], marker = markers[i])
# 		i += 1
# 	#
# 	plt.tick_params(labelcolor='none', top=False, bottom=False, left=False, right=False)
# 	plt.grid(False)
# 	plt.xlabel("number of SAs")
# 	plt.ylabel("speedup")
# 	plt.sca(ax1)
# 	plt.xticks(NUM_SA[1:])
# 	plt.yticks(np.arange(0,70,10))
# 	ax2.legend(title="Pod size (s)", loc = "upper right")
# 	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
# 	plt.show()
# 	plt.clf()
# 	plt.close('all')






def op_intensity(M,N,K,M_sr,N_sr):
	M,N,K= float(M), float(N), float(K)
	mults = M*N*K
	P = M*N
	D = (M/M_sr)*K*N 
	W = (N/N_sr)*M*K 
	return (mults / (P+D+W))


def roofline():
	plt.rcParams.update({'font.size': 14})
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
	plt.plot(oi_list, p,'-b', label  = "roofline")
	plt.scatter([s2],[0.893])
	plt.scatter([s4],[1.904])
	plt.scatter([s8],[3.529])
	#
	plt.title('Roofline model of 64 SAs for different pod sizes')
	plt.xscale('log', basex=2)
	plt.yscale('log', basey=2)
	plt.xlabel('operational intensity (tile mults / tile)')
	plt.ylabel('performance (tile mults / cycle)')
	# plt.grid()
	plt.axvline(16, label = 'memory/compute\nboundary')
	# plt.text(16,0,'memory vs compute boundary',rotation=90)
	plt.annotate("s = 2", (4.5 ,0.8))
	plt.annotate("s = 4", (2.7 ,1.5))
	plt.annotate("s = 8", (1.6 ,2.5))
	plt.legend(loc = "upper left")
	plt.savefig("roofline.pdf", bbox_inches='tight')
	plt.show()
	plt.clf()








def plot_exp1ABskip(fname = 'exp1ABskip'):
	plt.rcParams.update({'font.size': 14})
	df = pandas.read_csv('exp1fig7')
	df = df.sort_values('OP')
	# plt.figure(figsize=(3,3))
	markers = ['o','v']
	labels = {'Yes' : 'skip 1 level', 'No' : 'no skipping'}
	i = 0
	for d in ['Yes','No']:
		plt.plot(list(df[df['skip'] == d]['OP']), 
				list(1638460 / df[(df['skip'] == d)]['number of cycles']), label = labels[d], marker = markers[i])
		# plt.plot(list(df[df['skip'] == d]['OP']), 
		# 		list(df[(df['skip'] == d)]['number of cycles']), label = d, marker = markers[i])
		i+=1
	#
	plt.title("Impact of AB skipping on Speedup")
	plt.xlabel("over-provisioning factor")
	plt.ylabel("speedup")
	plt.xticks(np.arange(0,4,1))
	# plt.yticks(np.arange(0,0.15,0.01))
	plt.legend(loc = "upper left")
	plt.savefig("%s.pdf" % fname, bbox_inches='tight')
	plt.show()







if __name__ == '__main__':
	plot_exp1fig1()
	plot_exp1fig2()
	plot_exp1fig3()
	plot_exp1fig4()
	roofline()





