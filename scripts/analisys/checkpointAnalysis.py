from os import link
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import seaborn as sns
import matplotlib.patches as patches
from matplotlib.patches import Ellipse
import matplotlib.transforms as transforms

# plt.style.use('science')

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
    start_policy = 11700
    end_policy = 12100
    policies = list(range(start_policy, end_policy+1, 100))
    n_rows = len(policies)
    n_cols = 9
    legend_size = 7
    fig, axs = plt.subplots(n_rows, n_cols, figsize=(100,20)  ,sharex='col')
    
    
    for n, checknumber in enumerate(policies):
        # ---- BDP
        mss = 1500
        prop_delay = 0.002
        linkrate = 80000000
        q_size = 8
        rtt = 6*prop_delay+3*(8*mss)/linkrate + 3*(8*20)/linkrate
        BDP = (linkrate*(rtt)/8)/mss

        # ------ Data
        withDelay = pd.read_csv(f'rollout_{checknumber}_15_workers.csv', index_col=[0])
        # withoutDelay = pd.read_csv(f'rollout_{checknumber}_15_workers.csv')
        index_col = 0

        # ------ Congestion Window
        axs[n][index_col].plot(withDelay['time'], withDelay['cwndReport'], label = '30 Workers', alpha=0.75)
        # axs[n][index_col].plot(withoutDelay['time'], withoutDelay['cwndReport'], label = '15 Workers', alpha=0.75)

        axs[n][index_col].axhspan(BDP+q_size,BDP, alpha=0.2, facecolor='purple', edgecolor=None,linewidth=None)
        axs[n][index_col].axhline(BDP, label = 'BDP', color='red', linestyle="dashed")

        font = {'family': 'serif',
        'color':  'purple',
        'weight': 'normal',
        'size': 9,
        }

        axs[n][index_col].text(1.02, 0.31, 'Buffer', transform=axs[n][0].transAxes,  fontdict=font, rotation='vertical')
        axs[n][index_col].text(0.5, 0.8, f'Iteration {checknumber}', transform=axs[n][0].transAxes, ha='center')
        axs[n][index_col].set(xlabel='Time (s)', ylabel='Cwnd (pkts)')
        axs[n][index_col].legend()

        index_col += 1

        # ------ Trimmed Headers
        axs[n][index_col].plot(withDelay['time'], withDelay['trimPortion'], label = '30 Workers', alpha=0.75)
        # axs[n][index_col].plot(withoutDelay['time'], withoutDelay['trimPortion'], label = '15 Workers', alpha=0.75)
        axs[n][index_col].set(xlabel='Time (s)', ylabel='Trim Fraction')
        axs[n][index_col].legend()

        index_col += 1


        # Throughput Delay Plot
        axs[n][index_col].scatter(withDelay['sRttStep'].mean(), withDelay['goodputStep'].mean()/(linkrate/1000000000), label = '30 Workers', alpha=0.75, marker='s')
        # axs[n][index_col].scatter(withoutDelay['sRttStep'].mean(), withoutDelay['goodputStep'].mean()/(linkrate/1000000000), label = '15 Workers', alpha=0.75, marker='^')
        # confidence_ellipse(withDelay['sRttStep'], withDelay['goodputStep']/(linkrate/1000000000), axs[n][index_col], n_std=1.0, facecolor='blue', alpha=0.2)
        # confidence_ellipse(withoutDelay['sRttStep'], withoutDelay['goodputStep'], axs[n][index_col], n_std=1.0, facecolor='green', alpha=0.2)

        axs[n][index_col].scatter(round(rtt*1000.0, 4), 1.0, label = 'Optimum', alpha=0.75, color='red', marker='*')
        axs[n][index_col].scatter(round(rtt*1000.0, 4) + (q_size*mss*8/linkrate)*1000, 1.0, label = 'Full Buffer', alpha=0.75, color='red')


        axs[n][index_col].set(xlabel='Delay (ms)', ylabel='Throughput')
        axs[n][index_col].legend(prop={'size': legend_size})

        index_col+=1

         # ------ Transmission Rate
        axs[n][index_col].plot(withDelay['time'], ((withDelay['cwndReport']*mss*8)/(withDelay['sRttStep']/1000))/linkrate, label = '30 Workers', alpha=0.75)
        # axs[n][index_col].plot(withoutDelay['time'], ((withoutDelay['cwndReport']*mss*8)/(withoutDelay['sRttStep']/1000))/linkrate, label = '30 Workers', alpha=0.75)
        axs[n][index_col].set(xlabel='Time (s)', ylabel='Rate/Bottleneck')
        axs[n][index_col].legend(prop={'size': legend_size})

        index_col+=1

        # ------ Reward
        axs[n][index_col].plot(withDelay['time'], withDelay['reward'], label = '30 Workers', alpha=0.75)
        # axs[n][index_col].plot(withoutDelay['time'], withoutDelay['reward'], label = '15 Workers', alpha=0.75)
        axs[n][index_col].set(xlabel='Time (s)', ylabel='Reward')
        axs[n][index_col].legend(prop={'size': legend_size})

        index_col+=1

        # # ------ Estimated BW
        # axs[n][index_col].plot(withDelay['time'], withDelay['estRate'], label = 'Est. BW', alpha=0.75)
        # # axs[n][index_col].plot(withoutDelay['time'],((withoutDelay['cwndReport']*1500*8)/(withoutDelay['sRtt']/1000))/1000000000, label = 'Throughput only', alpha=0.75)
        # axs[n][index_col].set(xlabel='Time (s)', ylabel='Est. BW.')
        # axs[n][index_col].legend(prop={'size': legend_size})

        # index_col+=1

        # ------ Pace 
        axs[n][index_col].plot(withDelay['time'], withDelay['paceTime'], label = '30 Workers', alpha=0.75)
        # axs[n][index_col].plot(withoutDelay['time'], withoutDelay['paceTime'], label = '15 Workers', alpha=0.75)
        axs[n][index_col].set(xlabel='Time (s)', ylabel='Pace Time')
        axs[n][index_col].legend(prop={'size': legend_size})

        index_col+=1

        # ------ sRTT 
        axs[n][index_col].plot(withDelay['time'], withDelay['sRttStep']/1000, label = 'sRttStep 30W', alpha=0.5)
        # axs[n][index_col].plot(withDelay['time'],withDelay['sRtt'] - 6*prop_delay -(3*(8*20/80000000)*1000) -(3*(8*1500/80000000)*1000), label = 'sRtt', alpha=0.5)
        axs[n][index_col].plot(withDelay['time'],withDelay['rttminEst'], label = 'minRtt', alpha=0.5)
        # axs[n][index_col].plot(withDelay['time'],withDelay['minRttStep'] - 12 -(3*(8*20/80000000)*1000) -(3*(8*1500/80000000)*1000), label = 'minRttStep', alpha=0.5)
        axs[n][index_col].set(xlabel='Time (s)', ylabel='Rtt Values')
        axs[n][index_col].axhline((q_size*mss*8/linkrate) + rtt, label = 'Max Queue Delay', color='red', linestyle="dashed")
        axs[n][index_col].axhline((mss*8/linkrate)+ rtt, label = '1 Pkt Queue Delay', color='red', linestyle="dashed")
        axs[n][index_col].axhline((2*mss*8/linkrate)+ rtt, color='red', linestyle="dashed")
        axs[n][index_col].axhline((3*mss*8/linkrate)+ rtt, color='red', linestyle="dashed")
        axs[n][index_col].axhline((4*mss*8/linkrate)+ rtt, color='red', linestyle="dashed")
        axs[n][index_col].axhline((5*mss*8/linkrate)+ rtt, color='red', linestyle="dashed")
        axs[n][index_col].axhline((6*mss*8/linkrate)+ rtt, color='red', linestyle="dashed")
        axs[n][index_col].axhline((7*mss*8/linkrate)+ rtt,  color='red', linestyle="dashed")


        axs[n][index_col].legend(prop={'size': legend_size})

        index_col+=1

        axs[n][index_col].plot(withDelay['time'], withDelay['rttMean'], label = 'rttMean', alpha=0.75)
        axs[n][index_col].plot(withDelay['time'], withDelay['rttminEst'], label = 'rttMin', alpha=0.75)
        axs[n][index_col].fill_between(withDelay['time'],  withDelay['rttMean'] + withDelay['rttStd'],  withDelay['rttMean'] - withDelay['rttStd'], alpha=0.2)
        # axs[n][3].plot(withoutDelay['time'],((withoutDelay['cwndReport']*1500*8)/(withoutDelay['sRtt']/1000))/1000000000, label = 'Throughput only', alpha=0.75)
        axs[n][index_col].set(xlabel='Time (s)', ylabel='rttPropEst')
        axs[n][index_col].legend(prop={'size': legend_size})

        index_col+=1

        axs[n][index_col].plot(withDelay['time'], withDelay['bwMean'], label = 'bwMean', alpha=0.75)
        axs[n][index_col].plot(withDelay['time'], withDelay['bwMax'], label = 'bwMax', alpha=0.75)
        axs[n][index_col].fill_between(withDelay['time'],  withDelay['bwMean'] + withDelay['bwStd'],  withDelay['bwMean'] - withDelay['bwStd'], alpha=0.2) 
        axs[n][index_col].set(xlabel='Time (s)', ylabel='bwEst')
        axs[n][index_col].legend(prop={'size': legend_size})


    axs[0][2].invert_xaxis()
    # fig.tight_layout()
    plt.savefig(f'cwnd_{start_policy}_{end_policy}.png')

  

