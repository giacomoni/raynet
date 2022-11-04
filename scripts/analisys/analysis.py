import matplotlib.pyplot as plt
import pandas as pd
import json
import os
from pathlib import Path
import numpy as np
from scipy import stats

plt.style.use('science')


def get_vectors(data, module_vector_pairs):
    data_vectors = data[list(data.keys())[0]]['vectors']
    ret = {}
    stats = set()
    for vector in data_vectors:
        for pair in module_vector_pairs:
            stats.add(vector['name'])
            if (pair[0] in vector['module']) and (pair[1] in vector['name']):
                ret.update({pair[0]:{'module': vector['module'], 'name': vector['name'] ,'time': vector['time'], 'value': vector['value']}})

    return ret, stats

if __name__ == "__main__":
	COLUMNS = ['obs', 'stack', 'type','explore', 'bw', 'buffer', 'rtt', 'goodput', 'avgDelay', 'stdDelay', 'trimPortion', 'bdp', 'bdp_buffer', 'spearmanr']
	dirname = os.getenv('HOME')+"/stacking_results/"
	directory = os.fsencode(dirname)
	analysis = pd.DataFrame(columns=COLUMNS)
	stack5 = []
	stack10 = []

	for file in os.listdir(directory):
		filename = os.fsdecode(file)
		if filename.endswith('.csv'):
			results = pd.read_csv(dirname +filename)
			obstype = filename.split('_')[1]
			bw = int(filename.split('_')[8].replace('mbps', ''))
			rtt = int(filename.split('_')[9].replace('ms', ''))
			buffer = int(filename.split('_')[7].replace('pkts', ''))
			stack = int(filename.split('_')[2].replace('stack', ''))
			explore = filename.split('_')[-1].split('.')[0]
			type = filename.split('_')[3]
			min_transmission_time = (1500*100000*8)/(bw*1000000)
			final_time = results['time'].iloc[-1]
			bdp = (bw*1000000*(rtt/1000))/(1500*8)
			bdp_buffer = bdp/buffer
			if final_time < min_transmission_time:
				goodput = 0
			else:
				goodput = ((1500*100000*8)/final_time)/(bw*1000000)

			avg_delay = (results['rttMean'] - (rtt/1000) - ((3*8*1500)/(bw*1000000))).mean()
			std_delay = (results['rttMean'] - (rtt/1000) - ((3*8*1500)/(bw*1000000))).std()
			trim = results['trimPortion'].mean()
			spearmanr, _ = stats.spearmanr(results['time'], results['trimPortion']) if not (results['trimPortion'] == results['trimPortion'][0]).all() else (0,0)
			

			analysis = pd.concat([analysis, pd.DataFrame([[obstype, stack, type, explore, bw, buffer, rtt, goodput, avg_delay, std_delay, trim, bdp, bdp_buffer, spearmanr]], columns=COLUMNS)])

	analysis['bdp_buffer_ratio'] = analysis['buffer']/analysis['bdp']
	analysis.to_csv('analysis.csv')


	fig, axs = plt.subplots(1, 3, figsize=(15,2))

	det = analysis[((analysis['explore'] == 'det')  & (analysis['type'] == 'DDPG01') & (analysis['obs'] == 'obsb')  & (analysis['rtt'] >= 16) & (analysis['rtt'] <= 64) & (analysis['buffer'] >= 80) & (analysis['buffer'] <= 800) )]
	# stoc = analysis[((analysis['explore'] == 'det')  & (analysis['type'] == 'PPO01') &  (analysis['obs'] == 'obsb') & (analysis['rtt'] >= 16) & (analysis['rtt'] <= 64) & (analysis['buffer'] >= 80) & (analysis['buffer'] <= 800) )]


	meanBandwidthDet = det.groupby('bw')[['goodput', 'avgDelay', 'trimPortion', 'spearmanr']].mean()
	# meanBandwidthStoc = stoc.groupby('bw')[['goodput', 'avgDelay', 'trimPortion', 'spearmanr']].mean()
	stdBandwidthDet = det.groupby('bw')[['goodput', 'avgDelay', 'trimPortion', 'spearmanr']].std()
	# stdBandwidthStoc = stoc.groupby('bw')[['goodput', 'avgDelay', 'trimPortion', 'spearmanr']].std()

	axs[0].plot(meanBandwidthDet.index, meanBandwidthDet['goodput'], label='DDPG')
	axs[0].fill_between(meanBandwidthDet.index,  meanBandwidthDet['goodput'] - stdBandwidthDet['goodput'], meanBandwidthDet['goodput'] + stdBandwidthDet['goodput'],alpha=0.25)
	# axs[0].plot(meanBandwidthStoc.index, meanBandwidthStoc['goodput'],  label='PPO')
	# axs[0].fill_between(meanBandwidthStoc.index,  meanBandwidthStoc['goodput'] - stdBandwidthStoc['goodput'], meanBandwidthStoc['goodput'] + stdBandwidthStoc['goodput'],alpha=0.25)

	axs[0].axvspan(64,128, color='red', alpha=0.25)
		# axs[n][index_col].text(1.02, 0.31, 'Buffer', transform=axs[n][0].transAxes,  fontdict=font, rotation='vertical')
	axs[0].set(xlabel='Bottleneck Rate (Mbps)', ylim=[0.5,1], ylabel='Normalised Goodput',  xscale = 'log')
	# axs[0].legend(prop={'size': 7})


	axs[1].plot(meanBandwidthDet.index, meanBandwidthDet['avgDelay'], label='DDPG')
	axs[1].fill_between(meanBandwidthDet.index,  meanBandwidthDet['avgDelay'] - stdBandwidthDet['avgDelay'], meanBandwidthDet['avgDelay'] + stdBandwidthDet['avgDelay'],alpha=0.25)
	# axs[1].plot(meanBandwidthStoc.index, meanBandwidthStoc['avgDelay'],  label='PPO')
	# axs[1].fill_between(meanBandwidthStoc.index,  meanBandwidthStoc['avgDelay'] - stdBandwidthStoc['avgDelay'], meanBandwidthStoc['avgDelay'] + stdBandwidthStoc['avgDelay'],alpha=0.25)

	# axs[1][0].axhline(1, label = f'Min Avg Delay', linestyle="dashed")
	# axs[1][0].axhline(((rtt/1000) + (buffer*1500*8/(bw*1000000)) + (3*1500*8/(bw*1000000)))/(rtt/1000), label = f'Max Avg Delay', linestyle="dashed")
	axs[1].axvspan(64,128, color='red', alpha=0.25)
	# maxQueueDelay = ((80*1500*8)/(meanBandwidthDet.index.values*1000000))
	# axs[1][0].plot(meanBandwidthDet.index, maxQueueDelay, label='Max Q Delay')


		# axs[n][index_col].text(1.02, 0.31, 'Buffer', transform=axs[n][0].transAxes,  fontdict=font, rotation='vertical')
	axs[1].set(xlabel='Bottleneck Rate (Mbps)', ylabel='Mean Queue Delay', xscale = 'log')
	# axs[1].legend(prop={'size': 7})

	axs[2].plot(meanBandwidthDet.index, meanBandwidthDet['trimPortion'], label='DDPG')
	axs[2].fill_between(meanBandwidthDet.index,  meanBandwidthDet['trimPortion'] - stdBandwidthDet['trimPortion'], meanBandwidthDet['trimPortion'] + stdBandwidthDet['trimPortion'],alpha=0.25)
	# axs[2].plot(meanBandwidthStoc.index, meanBandwidthStoc['trimPortion'],  label='PPO')
	# axs[2].fill_between(meanBandwidthStoc.index,  meanBandwidthStoc['trimPortion'] - stdBandwidthStoc['trimPortion'], meanBandwidthStoc['trimPortion'] + stdBandwidthStoc['trimPortion'],alpha=0.25)

	axs[2].axvspan(64,128, color='red', alpha=0.25)

		# axs[n][index_col].text(1.02, 0.31, 'Buffer', transform=axs[n][0].transAxes,  fontdict=font, rotation='vertical')
	axs[2].set(xlabel='Bottleneck Rate (Mbps)', ylim=[0,0.15], ylabel='Trim Portion',  xscale = 'log')
	# axs[2].legend(prop={'size': 7})

	plt.savefig('bandwidth.png', dpi=300)

	fig, axs = plt.subplots(1, 3, figsize=(15,2))

	det = analysis[((analysis['explore'] == 'det')  & (analysis['type'] == 'DDPG01') & (analysis['obs'] == 'obsb')  & (analysis['bw'] >= 64) & (analysis['bw'] <= 128) & (analysis['buffer'] >= 80) & (analysis['buffer'] <= 800) )]
	# stoc = analysis[((analysis['explore'] == 'det')  & (analysis['type'] == 'PPO01') &  (analysis['obs'] == 'obsb') & (analysis['bw'] >= 64) & (analysis['bw'] <= 128) & (analysis['buffer'] >= 80) & (analysis['buffer'] <= 800) )]


	meanBandwidthDet = det.groupby('rtt')[['goodput', 'avgDelay', 'trimPortion']].mean()
	# meanBandwidthStoc = stoc.groupby('rtt')[['goodput', 'avgDelay', 'trimPortion']].mean()
	stdBandwidthDet = det.groupby('rtt')[['goodput', 'avgDelay', 'trimPortion']].std()
	# stdBandwidthStoc = stoc.groupby('rtt')[['goodput', 'avgDelay', 'trimPortion']].std()

	axs[0].plot(meanBandwidthDet.index, meanBandwidthDet['goodput'], label='DDPG')
	axs[0].fill_between(meanBandwidthDet.index,  meanBandwidthDet['goodput'] - stdBandwidthDet['goodput'], meanBandwidthDet['goodput'] + stdBandwidthDet['goodput'],alpha=0.25)
	# axs[0].plot(meanBandwidthStoc.index, meanBandwidthStoc['goodput'],  label='PPO')
	# axs[0].fill_between(meanBandwidthStoc.index,  meanBandwidthStoc['goodput'] - stdBandwidthStoc['goodput'], meanBandwidthStoc['goodput'] + stdBandwidthStoc['goodput'],alpha=0.25)

	axs[0].axvspan(16,64, color='red', alpha=0.25)
		# axs[n][index_col].text(1.02, 0.31, 'Buffer', transform=axs[n][0].transAxes,  fontdict=font, rotation='vertical')
	axs[0].set(xlabel='RTT (ms)', ylim=[0,1], ylabel='Normalised Goodput',  xscale = 'log')
	# axs[0].legend(prop={'size': 7})


	axs[1].plot(meanBandwidthDet.index, meanBandwidthDet['avgDelay'], label='DDPG')
	axs[1].fill_between(meanBandwidthDet.index,  meanBandwidthDet['avgDelay'] - stdBandwidthDet['avgDelay'], meanBandwidthDet['avgDelay'] + stdBandwidthDet['avgDelay'],alpha=0.25)
	# axs[1].plot(meanBandwidthStoc.index, meanBandwidthStoc['avgDelay'],  label='PPO')
	# axs[1].fill_between(meanBandwidthStoc.index,  meanBandwidthStoc['avgDelay'] - stdBandwidthStoc['avgDelay'], meanBandwidthStoc['avgDelay'] + stdBandwidthStoc['avgDelay'],alpha=0.25)

	# maxQueueNorm = ((80*1500*8)/(85*1000000))*np.ones(len(meanBandwidthDet.index))
	# axs[1][1].plot(meanBandwidthDet.index, maxQueueNorm, label='Max Delay')
	axs[1].axvspan(16,64, color='red', alpha=0.25)
	# axs[1][1].axhline(1, label = f'Min Avg Delay', linestyle="dashed")
	# axs[1][1].axhline(((rtt/1000) + (buffer*1500*8/(bw*1000000)) + (3*1500*8/(bw*1000000)))/(rtt/1000), label = f'Max Avg Delay', linestyle="dashed")


		# axs[n][index_col].text(1.02, 0.31, 'Buffer', transform=axs[n][0].transAxes,  fontdict=font, rotation='vertical')
	axs[1].set(xlabel='RTT (ms)', ylabel='Mean Queue Delay', xscale = 'log')
	# axs[1].legend(prop={'size': 7})

	axs[2].plot(meanBandwidthDet.index, meanBandwidthDet['trimPortion'], label='DDPG')
	axs[2].fill_between(meanBandwidthDet.index,  meanBandwidthDet['trimPortion'] - stdBandwidthDet['trimPortion'], meanBandwidthDet['trimPortion'] + stdBandwidthDet['trimPortion'],alpha=0.25)
	# axs[2].plot(meanBandwidthStoc.index, meanBandwidthStoc['trimPortion'],  label='PPO')
	# axs[2].fill_between(meanBandwidthStoc.index,  meanBandwidthStoc['trimPortion'] - stdBandwidthStoc['trimPortion'], meanBandwidthStoc['trimPortion'] + stdBandwidthStoc['trimPortion'],alpha=0.25)

	axs[2].axvspan(16,64, color='red', alpha=0.25)		# axs[n][index_col].text(1.02, 0.31, 'Buffer', transform=axs[n][0].transAxes,  fontdict=font, rotation='vertical')
	axs[2].set(xlabel='RTT (ms)', ylim=[0,0.15], ylabel='Trim Portion',  xscale = 'log')
	# axs[2].legend(prop={'size': 7})

	plt.savefig('rtt.png', dpi=300)

	fig, axs = plt.subplots(1, 3, figsize=(15,2))

	det = analysis[((analysis['explore'] == 'det')  & (analysis['type'] == 'DDPG01') & (analysis['obs'] == 'obsb')  & (analysis['bw'] >= 64) & (analysis['bw'] <= 128) & (analysis['rtt'] >= 16) & (analysis['rtt'] <= 64) )]
	# stoc = analysis[((analysis['explore'] == 'det')  & (analysis['type'] == 'PPO01') &  (analysis['obs'] == 'obsb') & (analysis['bw'] >= 64) & (analysis['bw'] <= 128) & (analysis['rtt'] >= 16) & (analysis['rtt'] <= 64) )]


	meanBandwidthDet = det.groupby('buffer')[['goodput', 'avgDelay', 'trimPortion']].mean()
	# meanBandwidthStoc = stoc.groupby('buffer')[['goodput', 'avgDelay', 'trimPortion']].mean()
	stdBandwidthDet = det.groupby('buffer')[['goodput', 'avgDelay', 'trimPortion']].std()
	# stdBandwidthStoc = stoc.groupby('buffer')[['goodput', 'avgDelay', 'trimPortion']].std()

	axs[0].plot(meanBandwidthDet.index, meanBandwidthDet['goodput'], label='DDPG')
	axs[0].fill_between(meanBandwidthDet.index,  meanBandwidthDet['goodput'] - stdBandwidthDet['goodput'], meanBandwidthDet['goodput'] + stdBandwidthDet['goodput'],alpha=0.25)
	# axs[0].plot(meanBandwidthStoc.index, meanBandwidthStoc['goodput'],  label='PPO')
	# axs[0].fill_between(meanBandwidthStoc.index,  meanBandwidthStoc['goodput'] - stdBandwidthStoc['goodput'], meanBandwidthStoc['goodput'] + stdBandwidthStoc['goodput'],alpha=0.25)

	axs[0].axvspan(80,800, color='red',alpha=0.25)


		# axs[n][index_col].text(1.02, 0.31, 'Buffer', transform=axs[n][0].transAxes,  fontdict=font, rotation='vertical')
	axs[0].set(xlabel='Buffer (pkts)', ylabel='Normalised Goodput', ylim=[0,1], xlim=[10,10**3],xscale = 'log')
	# axs[0].legend(prop={'size': 7})


	axs[1].plot(meanBandwidthDet.index, meanBandwidthDet['avgDelay'], label='DDPG')
	axs[1].fill_between(meanBandwidthDet.index,  meanBandwidthDet['avgDelay'] - stdBandwidthDet['avgDelay'], meanBandwidthDet['avgDelay'] + stdBandwidthDet['avgDelay'],alpha=0.25)
	# axs[1].plot(meanBandwidthStoc.index, meanBandwidthStoc['avgDelay'],  label='PPO')
	# axs[1].fill_between(meanBandwidthStoc.index,  meanBandwidthStoc['avgDelay'] - stdBandwidthStoc['avgDelay'], meanBandwidthStoc['avgDelay'] + stdBandwidthStoc['avgDelay'],alpha=0.25)

	axs[1].axvspan(80,800, color='red',alpha=0.25)

	# maxQueueDelay = ((meanBandwidthDet.index.values*1500*8)/(85*1000000))
	# axs[1][2].plot(meanBandwidthDet.index, maxQueueDelay, label='Max Delay')

	# axs[1][2].axhline(1, label = f'Min Avg Delay', linestyle="dashed")
	# axs[1][2].axhline(((rtt/1000) + (buffer*1500*8/(bw*1000000)) + (3*1500*8/(bw*1000000)))/(rtt/1000), label = f'Max Avg Delay', linestyle="dashed")


		# axs[n][index_col].text(1.02, 0.31, 'Buffer', transform=axs[n][0].transAxes,  fontdict=font, rotation='vertical')
	axs[1].set(xlabel='Buffer (pkts)', ylabel='Mean Queue Delay' , xlim=[10,10**3],xscale = 'log', ylim=[0,0.01])
	# axs[1].legend(prop={'size': 7})

	axs[2].plot(meanBandwidthDet.index, meanBandwidthDet['trimPortion'], label='DDPG')
	axs[2].fill_between(meanBandwidthDet.index,  meanBandwidthDet['trimPortion'] - stdBandwidthDet['trimPortion'], meanBandwidthDet['trimPortion'] + stdBandwidthDet['trimPortion'],alpha=0.25)
	# axs[2].plot(meanBandwidthStoc.index, meanBandwidthStoc['trimPortion'],  label='PPO')
	# axs[2].fill_between(meanBandwidthStoc.index,  meanBandwidthStoc['trimPortion'] - stdBandwidthStoc['trimPortion'], meanBandwidthStoc['trimPortion'] + stdBandwidthStoc['trimPortion'],alpha=0.25)

	axs[2].axvspan(80,800, color='red',alpha=0.25)


		# axs[n][index_col].text(1.02, 0.31, 'Buffer', transform=axs[n][0].transAxes,  fontdict=font, rotation='vertical')
	axs[2].set(xlabel='Buffer (pkts)', ylabel='Trim Portion', ylim=[0,0.15], xlim=[10,10**3],xscale = 'log')
	# axs[2].legend(prop={'size': 7})

	plt.savefig('buffer.png', dpi=300)

	fig, axs = plt.subplots(1, 3, figsize=(15,2))

	det = analysis[((analysis['explore'] == 'det')  & (analysis['type'] == 'DDPG01') & (analysis['obs'] == 'obsb')  & (analysis['rtt'] >= 16) & (analysis['rtt'] <= 64) & (analysis['buffer'] >= 80) & (analysis['buffer'] <= 800) )]
	meanBandwidthDet = det.groupby('bw')[['goodput', 'avgDelay', 'trimPortion', 'spearmanr']].mean()
	# meanBandwidthStoc = stoc.groupby('bw')[['goodput', 'avgDelay', 'trimPortion', 'spearmanr']].mean()
	stdBandwidthDet = det.groupby('bw')[['goodput', 'avgDelay', 'trimPortion', 'spearmanr']].std()
	# stdBandwidthStoc = stoc.groupby('bw')[['goodput', 'avgDelay', 'trimPortion', 'spearmanr']].std()
	axs[0].plot(meanBandwidthDet.index, meanBandwidthDet['spearmanr'], label='DDPG')
	axs[0].fill_between(meanBandwidthDet.index,  meanBandwidthDet['spearmanr'] - stdBandwidthDet['spearmanr'], meanBandwidthDet['spearmanr'] + stdBandwidthDet['spearmanr'],alpha=0.25)
	# axs[0].plot(meanBandwidthStoc.index, meanBandwidthStoc['goodput'],  label='PPO')
	# axs[0].fill_between(meanBandwidthStoc.index,  meanBandwidthStoc['goodput'] - stdBandwidthStoc['goodput'], meanBandwidthStoc['goodput'] + stdBandwidthStoc['goodput'],alpha=0.25)

	axs[0].axvspan(64,128, color='red', alpha=0.25)
		# axs[n][index_col].text(1.02, 0.31, 'Buffer', transform=axs[n][0].transAxes,  fontdict=font, rotation='vertical')
	axs[0].set(xlabel='Bottleneck Rate (Mbps)', ylim=[-1,1], ylabel='Spearman corr',  xscale = 'log')
	# axs.legend(prop={'size': 7})

	det = analysis[((analysis['explore'] == 'det')  & (analysis['type'] == 'DDPG01') & (analysis['obs'] == 'obsb')  & (analysis['bw'] >= 64) & (analysis['bw'] <= 128) & (analysis['buffer'] >= 80) & (analysis['buffer'] <= 800) )]
	# stoc = analysis[((analysis['explore'] == 'det')  & (analysis['type'] == 'PPO01') &  (analysis['obs'] == 'obsb') & (analysis['bw'] >= 64) & (analysis['bw'] <= 128) & (analysis['buffer'] >= 80) & (analysis['buffer'] <= 800) )]
	meanBandwidthDet = det.groupby('rtt')[['goodput', 'avgDelay', 'trimPortion', 'spearmanr']].mean()
	# meanBandwidthStoc = stoc.groupby('rtt')[['goodput', 'avgDelay', 'trimPortion']].mean()
	stdBandwidthDet = det.groupby('rtt')[['goodput', 'avgDelay', 'trimPortion', 'spearmanr']].std()
	# stdBandwidthStoc = stoc.groupby('rtt')[['goodput', 'avgDelay', 'trimPortion']].std()

	axs[1].plot(meanBandwidthDet.index, meanBandwidthDet['spearmanr'], label='DDPG')
	axs[1].fill_between(meanBandwidthDet.index,  meanBandwidthDet['spearmanr'] - stdBandwidthDet['spearmanr'], meanBandwidthDet['spearmanr'] + stdBandwidthDet['spearmanr'],alpha=0.25)
	# axs[0].plot(meanBandwidthStoc.index, meanBandwidthStoc['goodput'],  label='PPO')
	# axs[0].fill_between(meanBandwidthStoc.index,  meanBandwidthStoc['goodput'] - stdBandwidthStoc['goodput'], meanBandwidthStoc['goodput'] + stdBandwidthStoc['goodput'],alpha=0.25)

	axs[1].axvspan(16,64, color='red', alpha=0.25)
		# axs[n][index_col].text(1.02, 0.31, 'Buffer', transform=axs[n][0].transAxes,  fontdict=font, rotation='vertical')
	axs[1].set(xlabel='Rtt (ms)', ylim=[-1,1], ylabel='Spearman corr',  xscale = 'log')
	# axs.legend(prop={'size': 7})

	det = analysis[((analysis['explore'] == 'det')  & (analysis['type'] == 'DDPG01') & (analysis['obs'] == 'obsb')  & (analysis['bw'] >= 64) & (analysis['bw'] <= 128) & (analysis['rtt'] >= 16) & (analysis['rtt'] <= 64) )]
	# stoc = analysis[((analysis['explore'] == 'det')  & (analysis['type'] == 'PPO01') &  (analysis['obs'] == 'obsb') & (analysis['bw'] >= 64) & (analysis['bw'] <= 128) & (analysis['rtt'] >= 16) & (analysis['rtt'] <= 64) )]


	meanBandwidthDet = det.groupby('buffer')[['goodput', 'avgDelay', 'trimPortion', 'spearmanr']].mean()
	# meanBandwidthStoc = stoc.groupby('buffer')[['goodput', 'avgDelay', 'trimPortion']].mean()
	stdBandwidthDet = det.groupby('buffer')[['goodput', 'avgDelay', 'trimPortion', 'spearmanr']].std()
	# stdBandwidthStoc = stoc.groupby('buffer')[['goodput', 'avgDelay', 'trimPortion']].std()

	axs[2].plot(meanBandwidthDet.index, meanBandwidthDet['spearmanr'], label='DDPG')
	axs[2].fill_between(meanBandwidthDet.index,  meanBandwidthDet['spearmanr'] - stdBandwidthDet['spearmanr'], meanBandwidthDet['spearmanr'] + stdBandwidthDet['spearmanr'],alpha=0.25)
	# axs[0].plot(meanBandwidthStoc.index, meanBandwidthStoc['goodput'],  label='PPO')
	# axs[0].fill_between(meanBandwidthStoc.index,  meanBandwidthStoc['goodput'] - stdBandwidthStoc['goodput'], meanBandwidthStoc['goodput'] + stdBandwidthStoc['goodput'],alpha=0.25)

	axs[2].axvspan(80,800, color='red', alpha=0.25)
		# axs[n][index_col].text(1.02, 0.31, 'Buffer', transform=axs[n][0].transAxes,  fontdict=font, rotation='vertical')
	axs[2].set(xlabel='Buffer (pkts)', ylim=[-1,1], ylabel='Spearman corr',  xscale = 'log')
	# axs.legend(prop={'size': 7})


	plt.savefig('rhos.png', dpi=300)

	fig, axs = plt.subplots(1, 1)

	det = analysis[((analysis['explore'] == 'det')  & (analysis['type'] == 'DDPG01') & (analysis['obs'] == 'obsb') )]
	# stoc = analysis[((analysis['explore'] == 'det')  & (analysis['type'] == 'PPO01') &  (analysis['obs'] == 'obsb') & (analysis['bw'] >= 64) & (analysis['bw'] <= 128) & (analysis['rtt'] >= 16) & (analysis['rtt'] <= 64) )]


	meanBandwidthDet = det.groupby('bdp')[['goodput', 'avgDelay', 'trimPortion', 'spearmanr']].mean()
	# meanBandwidthStoc = stoc.groupby('buffer')[['goodput', 'avgDelay', 'trimPortion']].mean()
	stdBandwidthDet = det.groupby('bdp')[['goodput', 'avgDelay', 'trimPortion', 'spearmanr']].std()
	# stdBandwidthStoc = stoc.groupby('buffer')[['goodput', 'avgDelay', 'trimPortion']].std()

	axs.plot(meanBandwidthDet.index, meanBandwidthDet['spearmanr'], label='DDPG')
	axs.fill_between(meanBandwidthDet.index,  meanBandwidthDet['spearmanr'] - stdBandwidthDet['spearmanr'], meanBandwidthDet['spearmanr'] + stdBandwidthDet['spearmanr'],alpha=0.25)
	# axs[0].plot(meanBandwidthStoc.index, meanBandwidthStoc['goodput'],  label='PPO')
	# axs[0].fill_between(meanBandwidthStoc.index,  meanBandwidthStoc['goodput'] - stdBandwidthStoc['goodput'], meanBandwidthStoc['goodput'] + stdBandwidthStoc['goodput'],alpha=0.25)

	# axs[2].axvspan(80,800, color='red', alpha=0.25)
		# axs[n][index_col].text(1.02, 0.31, 'Buffer', transform=axs[n][0].transAxes,  fontdict=font, rotation='vertical')
	axs.set(xlabel='BDP (pkts)', ylim=[-1,1], ylabel='Spearman corr',  xscale = 'log')
	# axs.legend(prop={'size': 7})


	plt.savefig('rhos_bdp.png', dpi=300)

	fig, axs = plt.subplots(1, 1)

	det = analysis[((analysis['explore'] == 'det')  & (analysis['type'] == 'DDPG01') & (analysis['obs'] == 'obsb') )]
	# stoc = analysis[((analysis['explore'] == 'det')  & (analysis['type'] == 'PPO01') &  (analysis['obs'] == 'obsb') & (analysis['bw'] >= 64) & (analysis['bw'] <= 128) & (analysis['rtt'] >= 16) & (analysis['rtt'] <= 64) )]


	meanBandwidthDet = det.groupby('bdp_buffer')[['goodput', 'avgDelay', 'trimPortion', 'spearmanr']].mean()
	# meanBandwidthStoc = stoc.groupby('buffer')[['goodput', 'avgDelay', 'trimPortion']].mean()
	stdBandwidthDet = det.groupby('bdp_buffer')[['goodput', 'avgDelay', 'trimPortion', 'spearmanr']].std()
	# stdBandwidthStoc = stoc.groupby('buffer')[['goodput', 'avgDelay', 'trimPortion']].std()

	axs.plot(meanBandwidthDet.index, meanBandwidthDet['spearmanr'], label='DDPG')
	axs.fill_between(meanBandwidthDet.index,  meanBandwidthDet['spearmanr'] - stdBandwidthDet['spearmanr'], meanBandwidthDet['spearmanr'] + stdBandwidthDet['spearmanr'],alpha=0.25)
	# axs[0].plot(meanBandwidthStoc.index, meanBandwidthStoc['goodput'],  label='PPO')
	# axs[0].fill_between(meanBandwidthStoc.index,  meanBandwidthStoc['goodput'] - stdBandwidthStoc['goodput'], meanBandwidthStoc['goodput'] + stdBandwidthStoc['goodput'],alpha=0.25)

	# axs[2].axvspan(80,800, color='red', alpha=0.25)
		# axs[n][index_col].text(1.02, 0.31, 'Buffer', transform=axs[n][0].transAxes,  fontdict=font, rotation='vertical')
	axs.set(xlabel='BDP/Buffer (pkts)', ylim=[-1,1], ylabel='Spearman corr',  xscale = 'log')
	# axs.legend(prop={'size': 7})


	plt.savefig('rhos_bdp_buffer.png', dpi=300)

	# det = analysis[((analysis['explore'] == 'det')  & (analysis['type'] == 'DDPG01') & (analysis['obs'] == 'obsb'))]
	# stoc = analysis[((analysis['explore'] == 'det')  & (analysis['type'] == 'PPO01') & (analysis['obs'] == 'obsb'))]


	# meanBandwidthDet = det.groupby('bdp').mean()
	# meanBandwidthStoc = stoc.groupby('bdp').mean()

	# axs[0][3].plot(meanBandwidthDet.index, meanBandwidthDet['goodput'], label='DDPG')
	# axs[0][3].plot(meanBandwidthStoc.index, meanBandwidthStoc['goodput'],  label='PPO')


	# 	# axs[n][index_col].text(1.02, 0.31, 'Buffer', transform=axs[n][0].transAxes,  fontdict=font, rotation='vertical')
	# axs[0][3].set(xlabel='BDP (pkts)', ylabel='Normalised Goodput', ylim=[0,1], xlim=[10,10**3],xscale = 'log')
	# axs[0][3].legend(prop={'size': 7})


	# axs[1][3].plot(meanBandwidthDet.index, meanBandwidthDet['avgDelay'], label='DDPG')
	# axs[1][3].plot(meanBandwidthStoc.index, meanBandwidthStoc['avgDelay'],  label='PPO')
	# # maxQueueDelay = ((meanBandwidthDet.index.values*1500*8)/(85*1000000))
	# # axs[1][3].plot(meanBandwidthDet.index, maxQueueDelay, label='Max Delay')

	# # axs[1][2].axhline(1, label = f'Min Avg Delay', linestyle="dashed")
	# # axs[1][2].axhline(((rtt/1000) + (buffer*1500*8/(bw*1000000)) + (3*1500*8/(bw*1000000)))/(rtt/1000), label = f'Max Avg Delay', linestyle="dashed")


	# 	# axs[n][index_col].text(1.02, 0.31, 'Buffer', transform=axs[n][0].transAxes,  fontdict=font, rotation='vertical')
	# axs[1][3].set(xlabel='BDP (pkts)', ylabel='Mean Smoothed Q Delay' , xlim=[10,10**3],xscale = 'log')
	# axs[1][3].legend(prop={'size': 7})

	# axs[2][3].plot(meanBandwidthDet.index, meanBandwidthDet['trimPortion'], label='DDPG')
	# axs[2][3].plot(meanBandwidthStoc.index, meanBandwidthStoc['trimPortion'],  label='PPO')


	# 	# axs[n][index_col].text(1.02, 0.31, 'Buffer', transform=axs[n][0].transAxes,  fontdict=font, rotation='vertical')
	# axs[2][3].set(xlabel='BDP (pkts)', ylabel='Trim Portion', ylim=[0,0.15], xlim=[10,10**3],xscale = 'log')
	# axs[2][3].legend(prop={'size': 7})

	# det = analysis[((analysis['explore'] == 'det')  & (analysis['type'] == 'DDPG01') & (analysis['obs'] == 'obsb'))]
	# stoc = analysis[((analysis['explore'] == 'det')  & (analysis['type'] == 'PPO01') & (analysis['obs'] == 'obsb'))]


	# meanBandwidthDet = det.groupby('bdp_buffer').mean()
	# meanBandwidthStoc = stoc.groupby('bdp_buffer').mean()

	# axs[0][4].plot(meanBandwidthDet.index, meanBandwidthDet['goodput'], label='DDPG')
	# axs[0][4].plot(meanBandwidthStoc.index, meanBandwidthStoc['goodput'],  label='PPO')


	# 	# axs[n][index_col].text(1.02, 0.31, 'Buffer', transform=axs[n][0].transAxes,  fontdict=font, rotation='vertical')
	# axs[0][4].set(xlabel='BDP+Buffer (pkts)', ylabel='Normalised Goodput', ylim=[0,1], xlim=[10,10**3],xscale = 'log')
	# axs[0][4].legend(prop={'size': 7})


	# axs[1][4].plot(meanBandwidthDet.index, meanBandwidthDet['avgDelay'], label='DDPG')
	# axs[1][4].plot(meanBandwidthStoc.index, meanBandwidthStoc['avgDelay'],  label='PPO')
	# # maxQueueDelay = ((meanBandwidthDet.index.values*1500*8)/(85*1000000))
	# # axs[1][3].plot(meanBandwidthDet.index, maxQueueDelay, label='Max Delay')

	# # axs[1][2].axhline(1, label = f'Min Avg Delay', linestyle="dashed")
	# # axs[1][2].axhline(((rtt/1000) + (buffer*1500*8/(bw*1000000)) + (3*1500*8/(bw*1000000)))/(rtt/1000), label = f'Max Avg Delay', linestyle="dashed")


	# 	# axs[n][index_col].text(1.02, 0.31, 'Buffer', transform=axs[n][0].transAxes,  fontdict=font, rotation='vertical')
	# axs[1][4].set(xlabel='BDP+Buffer (pkts)', ylabel='Mean Smoothed Q Delay' , xlim=[10,10**3],xscale = 'log')
	# axs[1][4].legend(prop={'size': 7})

	# axs[2][4].plot(meanBandwidthDet.index, meanBandwidthDet['trimPortion'], label='DDPG')
	# axs[2][4].plot(meanBandwidthStoc.index, meanBandwidthStoc['trimPortion'],  label='PPO')


	# 	# axs[n][index_col].text(1.02, 0.31, 'Buffer', transform=axs[n][0].transAxes,  fontdict=font, rotation='vertical')
	# axs[2][4].set(xlabel='BDP+Buffer (pkts)', ylabel='Trim Portion', ylim=[0,0.15], xlim=[10,10**3],xscale = 'log')
	# axs[2][4].legend(prop={'size': 7})


	# # det = analysis[((analysis['explore'] == 'det')  & (analysis['type'] == 'best') & (analysis['obs'] == 'obsb'))]
	# stoc = analysis[((analysis['explore'] == 'det')  & (analysis['type'] == 'best') &  (analysis['obs'] == 'obsb'))]

	# # meanBandwidthDet = det.groupby('bdp_buffer_ratio').mean()
	# meanBandwidthStoc = stoc.groupby('bdp_buffer_ratio').mean()

	# # axs[0].plot(meanBandwidthDet.index, meanBandwidthDet['goodput'], label='Best')
	# axs[0].plot(meanBandwidthStoc.index, meanBandwidthStoc['goodput'],  label='Last')
	# # axs[0].axvline(85, color='red',label = f'Train', linestyle="dashed")

	# 	# axs[n][index_col].text(1.02, 0.31, 'Buffer', transform=axs[n][0].transAxes,  fontdict=font, rotation='vertical')
	# axs[0].set(xlabel='BDP x', ylim=[0,1], ylabel='Normalised Goodput', xscale='log')
	# axs[0].legend(prop={'size': 7})


	# # axs[1].plot(meanBandwidthDet.index, meanBandwidthDet['avgDelay'], label='Best')
	# axs[1].plot(meanBandwidthStoc.index, meanBandwidthStoc['avgDelay'],  label='Last')
	# # axs[1][0].axhline(1, label = f'Min Avg Delay', linestyle="dashed")
	# # axs[1][0].axhline(((rtt/1000) + (buffer*1500*8/(bw*1000000)) + (3*1500*8/(bw*1000000)))/(rtt/1000), label = f'Max Avg Delay', linestyle="dashed")
	# # axs[1].axvline(85, color='red', label = f'Train', linestyle="dashed")
	# # maxQueueDelay = ((80*1500*8)/(meanBandwidthDet.index.values*1000000))
	# # axs[1].plot(meanBandwidthDet.index, maxQueueDelay, label='Max Q Delay')


	# 	# axs[n][index_col].text(1.02, 0.31, 'Buffer', transform=axs[n][0].transAxes,  fontdict=font, rotation='vertical')
	# axs[1].set(xlabel='BDP x', ylabel='Mean Smoothed Q Delay', xscale='log')
	# axs[1].legend(prop={'size': 7})

	# # axs[2].plot(meanBandwidthDet.index, meanBandwidthDet['trimPortion'], label='Best')
	# axs[2].plot(meanBandwidthStoc.index, meanBandwidthStoc['trimPortion'],  label='Last')
	# # axs[2].axvline(85, color='red',label = f'Train', linestyle="dashed")

	# 	# axs[n][index_col].text(1.02, 0.31, 'Buffer', transform=axs[n][0].transAxes,  fontdict=font, rotation='vertical')
	# axs[2].set(xlabel='BDP x', ylabel='Trim Portion', xscale='log')
	# axs[2].legend(prop={'size': 7})






		

	# meanBandwidthDet = det.groupby(['bw', 'buffer'])[['goodput', 'avgDelay', 'trimPortion']].mean()
	# meanBandwidthStoc = stoc.groupby(['bw', 'buffer'])[['goodput', 'avgDelay', 'trimPortion']].mean()
	# # print(meanBandwidthDet.index.get_level_values(0))
	# # print(meanBandwidthDet.index.get_level_values(1))

	# LIST = []
	# for i, group in meanBandwidthDet.groupby(level=1)['trimPortion']:
	# 	LIST.append(group.values)
	# z = np.hstack(LIST).ravel()

	# xlabels = meanBandwidthDet.index.get_level_values(0).unique()
	# ylabels = meanBandwidthDet.index.get_level_values(1).unique()
	# x = np.arange(xlabels.shape[0])
	# y = np.arange(ylabels.shape[0])

	# x_M, y_M = np.meshgrid(x, y, copy=False)


	# fig = plt.figure(figsize=(10, 10))
	# ax = fig.add_subplot(111, projection='3d')
	
	# ax.w_xaxis.set_ticklabels(xlabels)
	# ax.w_yaxis.set_ticklabels(ylabels)

	# ax.scatter3D(x_M.ravel(), y_M.ravel(), z,  c=z, cmap='Greens')

	# plt.savefig('3dplot.png', dpi=300)
