from os import link
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import seaborn as sns
import matplotlib.patches as patches
from matplotlib.patches import Ellipse
import matplotlib.transforms as transforms

# plt.style.use('science')

COLOR_MAP = {'stoc':'orange',
            
             'a':'red',
             'b': 'blue',
             128: 'green',
            #  60: 'purple',
            #  128: 'brown',
            #  256: 'grey'

}

POLICY_MAP = {'a': 1900,
                'b': 1300}

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
    legend_size = 7
    fig, axs = plt.subplots(3, 3, figsize=(20,10))
    bandwidth = 85
    # rtt_raw = 10
    buffer = 80

    

    STACKS = ['a', 'b']
    BANDWIDTHS = [40, 85, 170]
    RTT_RAWS = [1,10,20]
    BUFFERS = [8, 80, 800]

    index_col = 0
    
    for rtt_raw in RTT_RAWS:
        
        for stack in STACKS:
        # ---- BDP
            mss = 1500
            prop_delay = (rtt_raw/1000)/6
            linkrate = bandwidth*1000000
            q_size = buffer
            rtt = 6*prop_delay+3*(8*mss)/linkrate + 3*(8*20)/linkrate
            BDP = (linkrate*(rtt)/8)/mss
            checknumber = POLICY_MAP[stack]
           
            # ------ Data
            data = pd.read_csv(f"/home/luca/stacking_results/rollout_obs{stack}_stack{10}_best_{checknumber}_15_workers_{buffer}pkts_{bandwidth}mbps_{rtt_raw}ms_det.csv", index_col=[0])
            # withoutDelay = pd.read_csv(f'rollout_{checknumber}_15_workers.csv')
            
            row_n = 0

            # ------ Congestion Window
            axs[row_n][index_col].plot(data['time'], data['cwnd'], label = f"OBS {stack}", alpha=0.75, color=COLOR_MAP[stack])
            # axs[n][index_col].plot(withoutDelay['time'], withoutDelay['cwndReport'], label = '15 Workers', alpha=0.75)

            axs[row_n][index_col].axhspan(BDP+q_size,BDP, alpha=0.2, facecolor=COLOR_MAP[stack], edgecolor=None,linewidth=None)
            axs[row_n][index_col].axhline(BDP, label = f'BDP', linestyle="dashed", color=COLOR_MAP[stack])

            font = {'family': 'serif',
            'color':  'purple',
            'weight': 'normal',
            'size': 9,
            }

            # axs[n][index_col].text(1.02, 0.31, 'Buffer', transform=axs[n][0].transAxes,  fontdict=font, rotation='vertical')
            axs[row_n][index_col].set(xlabel='Time (s)', ylabel='Cwnd (pkts)')
            axs[row_n][index_col].legend(prop={'size': 7})

            row_n += 1

            # ------ Trimmed Headers
            # axs.plot(withDelay['time'], withDelay['trimPortion'], label = f'{bw}ms', alpha=0.75, color=COLOR_MAP[bw])
            # # axs[n][index_col].plot(withoutDelay['time'], withoutDelay['trimPortion'], label = '15 Workers', alpha=0.75)
            # axs.set(xlabel='Time (s)', ylabel='Trim Fraction', xlim=[0,17.5])
            # axs.legend(prop={'size': 7})

            # row_n += 1
            
            # # Throughput Delay Plot
            # axs.scatter(withDelay['sRttStep'].mean(), withDelay['goodputStep'].mean()/(linkrate/1000000000), label = f'{bw}ms', color=COLOR_MAP[bw],alpha=0.75, marker='s')
            # # axs[n][index_col].scatter(withoutDelay['sRttStep'].mean(), withoutDelay['goodputStep'].mean()/(linkrate/1000000000), label = '15 Workers', alpha=0.75, marker='^')
            # confidence_ellipse(withDelay['sRttStep'], withDelay['goodputStep']/(linkrate/1000000000), axs, n_std=1.0, facecolor=COLOR_MAP[bw], alpha=0.2)
            # # confidence_ellipse(withoutDelay['sRttStep'], withoutDelay['goodputStep'], axs[n][index_col], n_std=1.0, facecolor='green', alpha=0.2)

            # axs.scatter(round(rtt*1000.0, 4), 1.0, label = 'Optimum', alpha=0.75, color=COLOR_MAP[bw], marker='*')
            # axs.scatter(round(rtt*1000.0, 4) + (q_size*mss*8/linkrate)*1000, 1.0, label = 'Full Buffer', alpha=0.75, color=COLOR_MAP[bw], marker='o')


            # axs.set(xlabel='Delay (ms)', ylabel='Throughput')
    

            # axs.legend(prop={'size': legend_size})

            # index_col+=1

            axs[row_n][index_col].plot(data['time'], data['rttMean'], label = f"Obs {stack}", alpha=0.75, color=COLOR_MAP[stack])
            axs[row_n][index_col].axhline(0, label = f'Min Queue Delay', color=COLOR_MAP[stack], linestyle="dashed")

            # axs[n][3].plot(withoutDelay['time'],((withoutDelay['cwndReport']*1500*8)/(withoutDelay['sRtt']/1000))/1000000000, label = 'Throughput only', alpha=0.75)
            axs[row_n][index_col].set(xlabel='Time (s)', ylabel='RTT Estimate')
            axs[row_n][index_col].legend(prop={'size': legend_size})

            row_n += 1

            axs[row_n][index_col].plot(data['time'], data['bwNorm'], label = f"OBS {stack}", alpha=0.75, color=COLOR_MAP[stack])
            # axs[n][3].plot(withoutDelay['time'],((withoutDelay['cwndReport']*1500*8)/(withoutDelay['sRtt']/1000))/1000000000, label = 'Throughput only', alpha=0.75)
            axs[row_n][index_col].axhline(1, label = f'Max Throughput', color=COLOR_MAP[stack], linestyle="dashed")

            axs[row_n][index_col].set(xlabel='Time (s)', ylabel='Throughput (Gbps)')
            axs[row_n][index_col].legend(prop={'size': legend_size})
            
        index_col +=1



        
    # fig.tight_layout()
    plt.savefig(f'obs_varying_rtt.png')
  

