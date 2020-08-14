import pandas
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab

df = pandas.read_csv('results.txt')

for i in ['M','K','S']:
	plt.plot(list(df[df['M-K increase'] == i]['number of SAs'].array), 
			list(df[df['M-K increase'] == i]['SRAM bw'].array), label = i, marker = 'o')

plt.title("SRAM bw VS compute capacity for a full MM block of dim 64x64x128 (M,K,N)")
plt.xlabel("number of SAs")
plt.ylabel("SRAM bw (packets/cycle)")
plt.legend()
plt.show()


for i in ['M','K','S']:
	plt.plot(list(df[df['M-K increase'] == i]['number of SAs'].array), 
			list(df[df['M-K increase'] == i]['number of cycles'].array), label = i, marker = 'o')

plt.title("number of cycles VS compute capacity for a full MM block of dim 64x64x128 (M,K,N)")
plt.xlabel("number of SAs")
plt.ylabel("number of cycles)")
plt.legend()
plt.show()
