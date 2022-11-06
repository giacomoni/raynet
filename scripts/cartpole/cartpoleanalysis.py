import pandas as pd
import matplotlib.pyplot as plt
import pandas as pd
import json
import os
from pathlib import Path
import numpy as np
from scipy import stats

plt.style.use('science')

if __name__ == "__main__":
    data = pd.read_csv('output.csv', index_col=[0])

    WORKERS = [1,2,4,8,16]
    ENVS = ['CartPole-v1', 'SimulationRunnerEnv']

    ramWorkersDf = pd.DataFrame({"env": [], "workers": [], "cpu_usage_mean": [], "cpu_usage_std": []})
    for n in WORKERS:
        for e in ENVS:
            dataSample = data[((data['env'] == e)  & (data['workers'] == n) & (data['metric'] == 'cpu_util_percent'))]
            dataSample = dataSample[['env', 'workers', 'value']]
            cpu_mean = dataSample['value'].mean()
            cpu_std = dataSample['value'].std()
            row =  pd.DataFrame({"env": [e], "workers": [n], "cpu_usage_mean": [cpu_mean], "cpu_usage_std": [cpu_std]})
            ramWorkersDf = pd.concat([ramWorkersDf, row])
    
    ramWorkersDf.to_csv('cpu_usage.csv')

    fig, axs = plt.subplots(1, 1)
    # axs.plot(ramWorkersDf[ramWorkersDf['env'] == 'SimulationRunnerEnv']['workers'], ramWorkersDf[ramWorkersDf['env'] == 'SimulationRunnerEnv']['cpu_usage_mean'], label='Omnet++')
    # axs.fill_between(ramWorkersDf[ramWorkersDf['env'] == 'SimulationRunnerEnv']['workers'],  ramWorkersDf[ramWorkersDf['env'] == 'SimulationRunnerEnv']['cpu_usage_mean'] - ramWorkersDf[ramWorkersDf['env'] == 'SimulationRunnerEnv']['cpu_usage_std'], ramWorkersDf[ramWorkersDf['env'] == 'SimulationRunnerEnv']['cpu_usage_mean'] + ramWorkersDf[ramWorkersDf['env'] == 'SimulationRunnerEnv']['cpu_usage_std'],alpha=0.25)
    # axs.plot(ramWorkersDf[ramWorkersDf['env'] == 'CartPole-v1']['workers'], ramWorkersDf[ramWorkersDf['env'] == 'CartPole-v1']['cpu_usage_mean'], label='Gym')
    # axs.fill_between(ramWorkersDf[ramWorkersDf['env'] == 'CartPole-v1']['workers'],  ramWorkersDf[ramWorkersDf['env'] == 'CartPole-v1']['cpu_usage_mean'] - ramWorkersDf[ramWorkersDf['env'] == 'CartPole-v1']['cpu_usage_std'], ramWorkersDf[ramWorkersDf['env'] == 'CartPole-v1']['cpu_usage_mean'] + ramWorkersDf[ramWorkersDf['env'] == 'CartPole-v1']['cpu_usage_std'],alpha=0.25)

    # axs.legend()
    # axs.set(xlabel='Num. Workers', ylabel='ram %',  xscale = 'log')

    index = np.arange(len(WORKERS))
    bar_width = 0.35
    opacity = 0.8

    rects1 = axs.bar(index - bar_width/2, ramWorkersDf[ramWorkersDf['env'] == 'SimulationRunnerEnv']['cpu_usage_mean'], bar_width,
    yerr=ramWorkersDf[ramWorkersDf['env'] == 'SimulationRunnerEnv']['cpu_usage_std'],
    alpha=opacity,
    label='Omnet++')

    rects2 = axs.bar(index + bar_width/2,  ramWorkersDf[ramWorkersDf['env'] == 'CartPole-v1']['cpu_usage_mean'], bar_width,
    yerr=ramWorkersDf[ramWorkersDf['env'] == 'CartPole-v1']['cpu_usage_std'],
    alpha=opacity,
    label='Gym')

    axs.set(xlabel = 'Num. Workers', ylabel = 'ram \%', title = 'ram Utilization')
    axs.set_xticks(index)
    axs.set_xticklabels(WORKERS)
    axs.legend()
    plt.savefig('cpu_usage.png', dpi=300)

