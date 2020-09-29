import pandas
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
import numpy as np
import os


def plot_exp1fig1(fname = 'exp1fig1'):
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
			label = "linearly increasing", marker = 'o')
	plt.title("Impact of Internal Bandwidth on Speedup")
	plt.xlabel("number of SAs")
	plt.ylabel("speedup")
	plt.legend(title="internal bandwidth", loc = "middle right")
	plt.savefig("%s.pdf" % fname)
	plt.show()
	plt.clf()




def plot_exp1fig3(fname = 'exp1fig3'):
	if not os.path.exists(fname):
		print("file %s not found" % fname)
		return
	#
	df = pandas.read_csv(fname)
	df = df.sort_values('number of SAs')
	NUM_SA = [4,8,16,32,64]
	single_pod_lat = df[(df['number of SAs'] == 4)]['number of cycles']._values[0]
	#
	for l in [2,2.5,3,4]:
		speedup = single_pod_lat / df[(df['lat_internal_factor'] == l)]['number of cycles']
		plt.plot(list(NUM_SA), list(speedup), label = l/2.0, marker = 'o')
	#
	plt.title("Impact of Internal Bandwidth on Speedup")
	plt.xlabel("number of SAs")
	plt.ylabel("speedup")
	plt.legend(title="internal bw linear increase factor", loc = "middle right")
	plt.savefig("%s.pdf" % fname)
	plt.show()
	plt.clf()





def plot_exp1fig4(fname = 'exp1fig4'):
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
	for l in lat_dram:
		speedup = single_pod_lat / df[(df['lat_dram'] == l)]['number of cycles']
		plt.plot(list(NUM_SA), list(speedup), label = 16.0/l, marker = 'o')
	#
	plt.title("Impact of External Bandwidth on Speedup")
	plt.xlabel("number of SAs")
	plt.ylabel("speedup")
	plt.legend(title="external bw (tiles/cycle)", loc = "middle right")
	plt.savefig("%s.pdf" % fname)
	plt.show()
	plt.clf()




def op_intensity(M,N,K,M_sr,N_sr):
	M,N,K= float(M), float(N), float(K)
	mults = M*N*K
	P = M*N
	D = (M/M_sr)*K*N 
	W = (N/N_sr)*M*K 
	return (mults / (P+D+W))


def roofline():
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
	plt.title('Roofline model of 64 SAs for different pod architectures (log-log scale)')
	plt.xscale('log', basex=2)
	plt.yscale('log', basey=2)
	plt.xlabel('operational intensity (tile mults / tile)')
	plt.ylabel('performance (tile mults / cycle)')
	# plt.grid()
	plt.axvline(16, label = 'memory vs compute boundary')
	# plt.text(16,0,'memory vs compute boundary',rotation=90)
	plt.annotate("s = 2", (5 ,0.893))
	plt.annotate("s = 4", (3 ,1.904))
	plt.annotate("s = 8", (1.8 ,3.09))
	plt.legend(loc = "middle right")
	plt.savefig("roofline.pdf")
	plt.show()
	plt.clf()







if __name__ == '__main__':
	plot_exp1fig1()
	plot_exp1fig2()
	plot_exp1fig3()
	plot_exp1fig4()
	roofline()





