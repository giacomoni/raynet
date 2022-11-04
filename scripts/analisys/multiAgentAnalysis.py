from os import link
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
# import seaborn as sns
import matplotlib.patches as patches
from matplotlib.patches import Ellipse
import matplotlib.transforms as transforms
import os

plt.style.use('science')

def confidence_ellipse(x, y, ax, n_std=3.0, facecolor='none', **kwargs):
    """
    Create a plot of the covariance confidence ellipse of *x* and *y*.

    Parameters
    ----------
    x, y : array-like, shape (n, )
        Input data.

    ax : matplotlib.axes.Axes
        The axes object to draw the ellipse into.

    n_std : float
        The number of standard deviations to determine the ellipse's radiuses.

    **kwargs
        Forwarded to `~matplotlib.patches.Ellipse`

    Returns
    -------
    matplotlib.patches.Ellipse
    """
    if x.size != y.size:
        raise ValueError("x and y must be the same size")

    cov = np.cov(x, y)
    pearson = cov[0, 1]/np.sqrt(cov[0, 0] * cov[1, 1])
    # Using a special case to obtain the eigenvalues of this
    # two-dimensionl dataset.
    ell_radius_x = np.sqrt(1 + pearson)
    ell_radius_y = np.sqrt(1 - pearson)
    ellipse = Ellipse((0, 0), width=ell_radius_x * 2, height=ell_radius_y * 2,
                      facecolor=facecolor, **kwargs)

    # Calculating the stdandard deviation of x from
    # the squareroot of the variance and multiplying
    # with the given number of standard deviations.
    scale_x = np.sqrt(cov[0, 0]) * n_std
    mean_x = np.mean(x)

    # calculating the stdandard deviation of y ...
    scale_y = np.sqrt(cov[1, 1]) * n_std
    mean_y = np.mean(y)

    transf = transforms.Affine2D() \
        .rotate_deg(45) \
        .scale(scale_x, scale_y) \
        .translate(mean_x, mean_y)

    ellipse.set_transform(transf + ax.transData)
    return ax.add_patch(ellipse)

def extract_metrics(data):
    metrics = {}
    metrics['episode_length'] = float(len(data))
    metrics['episode_reward'] = float(data['reward'].sum())
    metrics['step_reward_mean'] = float(data['reward'].mean())
    metrics['step_reward_std'] = float(data['reward'].std())
    metrics['time_duration'] = float(data['minRttStep'].sum()/1000)
    metrics['srtt'] =  float(data.tail(1)['sRtt'])
    return metrics

def generate_analysis_file():
    analysis = pd.DataFrame(columns=['policy','episode_length', 'episode_reward', 'step_reward_mean', 'step_reward_std', 'time_duration', 'srtt'], dtype=np.float64)
    for policy_no in range(100, 2100, 100):
        print(f"Processing csv {policy_no}...")
        data = pd.read_csv(f"rollout_{policy_no}.csv", index_col=[0]).iloc[1:,:]
        metrics = extract_metrics(data)
        metrics["policy"] = int(policy_no)
        analysis = analysis.append(metrics, ignore_index=True)
        
    analysis = analysis.set_index("policy")
    # analysis.to_csv("analysis.csv")
    return analysis

if __name__ == "__main__":
    policy = 90
    n_rows = 1
    n_cols = 3
    legend_size = 7
    
    # ---- BDP
    mss = 1500
    prop_delay = (40/6)/1000
    linkrate = 96000000
    q_size = 440
    rtt = 6*prop_delay+3*(8*mss)/linkrate + 3*(8*20)/linkrate
    BDP = (linkrate*(rtt)/8)/mss

    data =  pd.read_csv(os.getenv('HOME')+"/stacking_results"+f'/rollout_obsb_stack5_DDPG05multitrain_40_15_workers_440pkts_96mbps_40ms_det.csv', index_col=[0])
    print(data['agent'].unique())
    # ------ Data
    flow1 = data[data['agent']==2]
    flow2 = data[data['agent']==3]
    # flow3 = data[data['agent']==6]
    # flow4 = data[data['agent']==7]
    index_col = 0

    fig, axs = plt.subplots(1, 1)


    # ------ Congestion Window
    axs.plot(flow1['time'], flow1['cwnd'], label = 'Flow 1', alpha=0.75)
    axs.plot(flow2['time'], flow2['cwnd'], label = 'Flow 2', alpha=0.75)
    # axs.plot(flow3['time'], flow3['cwnd'], label = 'Flow 3', alpha=0.75)
    # axs.plot(flow4['time'], flow4['cwnd'], label = 'Flow 4', alpha=0.75)



    # axs[n][index_col].plot(withoutDelay['time'], withoutDelay['cwnd'], label = '15 Workers', alpha=0.75)

    axs.axhspan(BDP+q_size,BDP, alpha=0.2, facecolor='purple', edgecolor=None,linewidth=None)
    axs.axhline(BDP, label = 'BDP', color='red', linestyle="dashed")

    font = {'family': 'serif',
    'color':  'purple',
    'weight': 'normal',
    'size': 9,
    }

    axs.text(1.02, 0.61, 'Buffer', transform=axs.transAxes,  fontdict=font, rotation='vertical')
    axs.set(xlabel='Time (s)', ylabel='Cwnd (pkts)')
    axs.legend()
    plt.savefig(f'cwnd_multi.png', dpi=300)


    fig, axs = plt.subplots(1, 1)

    # ------ Trimmed Headers
    objects = ('Flow 1', 'Flow 2')
    y_pos = np.arange(len(objects))
    performance = [flow1['lossrate1'].iloc[-1], flow2['lossrate1'].iloc[-1]]

    axs.bar(y_pos, performance, align='center', alpha=0.5)
    axs.set_xticks(y_pos, minor=False)
    axs.set_xticklabels(objects, minor=False)
    axs.set(xlabel='Time (s)', ylabel='Loss Rate')

    plt.savefig(f'lossrate1.png', dpi=300)

    fig, axs = plt.subplots(1, 1)

    # ------ Trimmed Headers
    objects = ('Flow 1', 'Flow 2')
    y_pos = np.arange(len(objects))
    performance = [flow1['lossrate2'].iloc[-1], flow2['lossrate2'].iloc[-1]]

    axs.bar(y_pos, performance, align='center', alpha=0.5)
    axs.set_xticks(y_pos, minor=False)
    axs.set_xticklabels(objects, minor=False)
    axs.set(xlabel='Time (s)', ylabel='Loss Rate')

    plt.savefig(f'lossrate2.png', dpi=300)

    fig, axs = plt.subplots(1, 1)
    # ------ sRTT 
    axs.plot(flow1['time'], flow1['rttMean'] - 32/1000 - ((3*1500*8)/linkrate), label = 'Flow 1', alpha=0.5)
    axs.plot(flow2['time'], flow2['rttMean']- 32/1000 - ((3*1500*8)/linkrate), label = 'Flow 2', alpha=0.5)
    # axs.plot(flow3['time'], flow3['rttMean']- 32/1000 - ((3*1500*8)/linkrate), label = 'Flow 3', alpha=0.5)
    # axs.plot(flow4['time'], flow4['rttMean']- 32/1000 - ((3*1500*8)/linkrate), label = 'Flow 4', alpha=0.5)


    axs.legend()
    axs.set(xlabel='Time (s)', ylabel='Queueing Delay (s)')
    plt.savefig(f'q_del_multi.png', dpi=300)

    fig, axs = plt.subplots(1, 1)
    # ------ sRTT 
    axs.plot(flow1['time'], flow1['bwNorm'], label = 'Flow 1', alpha=0.5)
    axs.plot(flow2['time'], flow2['bwNorm'], label = 'Flow 2', alpha=0.5)
    # axs.plot(flow3['time'], flow3['bwNorm']/10, label = 'Flow 3', alpha=0.5)
    # axs.plot(flow4['time'], flow4['bwNorm']/10, label = 'Flow 4', alpha=0.5)

    axs.legend()
    axs.set(xlabel='Time (s)', ylabel='Throughput Over Max Bandwidth')
    plt.savefig(f'throughputNorm.png', dpi=300)

    fig, axs = plt.subplots(1, 1)
    # ------ sRTT 
    axs.plot(flow1['time'], flow1['goodput']/1000000000, label = 'Flow 1', alpha=0.5)
    axs.plot(flow2['time'], flow2['goodput']/1000000000, label = 'Flow 2', alpha=0.5)
    # axs.plot(flow3['time'], flow3['bwNorm']/10, label = 'Flow 3', alpha=0.5)
    # axs.plot(flow4['time'], flow4['bwNorm']/10, label = 'Flow 4', alpha=0.5)


    # axs.axhline(0.05, label = 'Fair Throughput', color='red', linestyle="dashed")
    axs.stairs([0.1,0.05,0.1], [0,5,15.75,18.5], label = 'Fair Throughput', color='red', linestyle="dashed")

    axs.legend()
    axs.set(xlabel='Time (s)', ylabel='Throughput (Gbps)')
    plt.savefig(f'throughput.png', dpi=300)


    fig, axs = plt.subplots(1, 1)

    smoothed1 = flow1[['goodput']].rolling(10).mean()
    smoothed2 = flow2[['goodput']].rolling(10).mean()
    # ------ sRTT 
    axs.plot(flow1['time'], smoothed1['goodput']/1000000000, label = 'Flow 1', alpha=0.5)
    axs.plot(flow2['time'], smoothed2['goodput']/1000000000, label = 'Flow 2', alpha=0.5)
    # axs.plot(flow3['time'], flow3['bwNorm']/10, label = 'Flow 3', alpha=0.5)
    # axs.plot(flow4['time'], flow4['bwNorm']/10, label = 'Flow 4', alpha=0.5)


    # axs.axhline(0.05, label = 'Fair Throughput', color='red', linestyle="dashed")
    axs.stairs([0.096,0.048,0.096], [0,5,15.75,18.5], label = 'Fair Throughput', color='red', linestyle="dashed")

    axs.legend()
    axs.set(xlabel='Time (s)', ylabel='Throughput (Gbps)')
    plt.savefig(f'throughputSmoothed.png', dpi=300)

    fig, axs = plt.subplots(1, 1)
    # ------ sRTT 
    axs.plot(flow1['time'], flow1['bwMax']/1000000000, label = 'Flow 1', alpha=0.5)
    axs.plot(flow2['time'], flow2['bwMax']/1000000000, label = 'Flow 2', alpha=0.5)
    # axs.plot(flow3['time'], flow3['bwNorm']/10, label = 'Flow 3', alpha=0.5)
    # axs.plot(flow4['time'], flow4['bwNorm']/10, label = 'Flow 4', alpha=0.5)

    axs.legend()
    axs.set(xlabel='Time (s)', ylabel='Estimated Max Bandwidth (Gbps)')
    plt.savefig(f'bwMax.png', dpi=300)

    fig, axs = plt.subplots(1, 1)
    # ------ sRTT 
    axs.plot(flow1['time'], flow1['bwMean']/1000000000, label = 'Flow 1', alpha=0.5)
    axs.plot(flow2['time'], flow2['bwMean']/1000000000, label = 'Flow 2', alpha=0.5)
    # axs.plot(flow3['time'], flow3['bwNorm']/10, label = 'Flow 3', alpha=0.5)
    # axs.plot(flow4['time'], flow4['bwNorm']/10, label = 'Flow 4', alpha=0.5)

    axs.legend()
    axs.set(xlabel='Time (s)', ylabel='Estimated Mean Bandwidth (Gbps)')
    plt.savefig(f'bwMean.png', dpi=300)

    fig, axs = plt.subplots(1, 1)
    # ------ sRTT 
    axs.plot(flow1['time'], flow1['trimPortion'], label = 'Flow 1', alpha=0.5)
    axs.plot(flow2['time'], flow2['trimPortion'], label = 'Flow 2', alpha=0.5)
    # axs.plot(flow3['time'], flow3['bwNorm']/10, label = 'Flow 3', alpha=0.5)
    # axs.plot(flow4['time'], flow4['bwNorm']/10, label = 'Flow 4', alpha=0.5)

    axs.legend()
    axs.set(xlabel='Time (s)', ylabel='Loss Ratio Per Step')
    plt.savefig(f'lossevolution.png', dpi=300)


    fig, axs = plt.subplots(1, 1)
    # ------ sRTT 
    axs.plot(flow1['time'], flow1['rtt'] , label = 'Flow 1', alpha=0.5)
    axs.plot(flow2['time'], flow2['rtt'], label = 'Flow 2', alpha=0.5)
    # axs.plot(flow3['time'], flow3['rttMean'], label = 'Flow 3', alpha=0.5)
    # axs.plot(flow4['time'], flow4['rttMean'], label = 'Flow 4', alpha=0.5)


    axs.legend()
    axs.set(xlabel='Time (s)', ylabel='Rtt (s)')
    plt.savefig(f'rttnorm.png', dpi=300)

    fig, axs = plt.subplots(1, 1)
    # ------ sRTT 
    axs.plot(flow1['time'], flow1['rttminEst'] , label = 'Flow 1', alpha=0.5)
    axs.plot(flow2['time'], flow2['rttminEst'], label = 'Flow 2', alpha=0.5)
    # axs.plot(flow3['time'], flow3['rttMean'], label = 'Flow 3', alpha=0.5)
    # axs.plot(flow4['time'], flow4['rttMean'], label = 'Flow 4', alpha=0.5)


    axs.legend()
    axs.set(xlabel='Time (s)', ylabel='Rtt Min (s)')
    plt.savefig(f'rttmin.png', dpi=300)




  

