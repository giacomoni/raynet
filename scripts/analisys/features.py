from os import link
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import seaborn as sns
import matplotlib.patches as patches
from matplotlib.patches import Ellipse
import matplotlib.transforms as transforms

plt.style.use('science')

COLOR_MAP = {8:'orange',
            
             80:'red',
             800: 'blue',
             128: 'green',
            #  60: 'purple',
            #  128: 'brown',
            #  256: 'grey'

}

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
    start_policy = 5800
    end_policy = 5800
    policies = list(range(start_policy, end_policy+1, 100))
    n_rows = 5
    n_cols = 2
    legend_size = 7
    fig, axs = plt.subplots(n_rows, n_cols, figsize=(40,20))
    bandwidths = [8, 80, 800]

    checknumber = 11800

    for bw in bandwidths:
    # ---- BDP
        mss = 1500
        prop_delay = 0.008
        linkrate = 80*1000000
        q_size = bw
        rtt = 6*prop_delay+3*(8*mss)/linkrate + 3*(8*20)/linkrate
        BDP = (linkrate*(rtt)/8)/mss
        index_row = 0
        index_col = 0
        
        # ------ Data
        withDelay = pd.read_csv(f'rollout_{checknumber}_15_workers_{bw}pkts_80mbps_8ms_stoc.csv', index_col=[0])
        # withoutDelay = pd.read_csv(f'rollout_{checknumber}_15_workers.csv')
        
        # ------ Rtt Norm
        axs[index_row][index_col].scatter(withDelay['cwnd'], withDelay['rttNorm'], label = f'{bw}mbps', alpha=0.75, color=COLOR_MAP[bw])
        # axs[n][index_col].scatter(withoutDelay['time'], withoutDelay['trimPortion'], label = '15 Workers', alpha=0.75)
        axs[index_row][index_col].set(xlabel='Time (s)', ylabel='RTT Norm')
        axs[index_row][index_col].legend(prop={'size': 7})

        index_row += 1

        # ------ Goodput norm
        axs[index_row][index_col].scatter(withDelay['cwnd'], withDelay['bwNorm'], label = f'{bw}mbps', alpha=0.75, color=COLOR_MAP[bw])
        # axs[n][index_col].scatter(withoutDelay['time'], withoutDelay['trimPortion'], label = '15 Workers', alpha=0.75)
        axs[index_row][index_col].set(xlabel='Time (s)', ylabel='BW Norm')
        axs[index_row][index_col].legend(prop={'size': 7})

        index_row += 1

        # ------ Trim portion
        axs[index_row][index_col].scatter(withDelay['cwnd'], withDelay['trimPortion'], label = f'{bw}mbps', alpha=0.75, color=COLOR_MAP[bw])
        # axs[n][index_col].scatter(withoutDelay['time'], withoutDelay['trimPortion'], label = '15 Workers', alpha=0.75)
        axs[index_row][index_col].set(xlabel='Time (s)', ylabel='Trim portion')
        axs[index_row][index_col].legend(prop={'size': 7})

        index_row += 1

        # ------ CWND norm
        axs[index_row][index_col].scatter(withDelay['cwnd'], withDelay['cwndNorm'], label = f'{bw}mbps', alpha=0.75, color=COLOR_MAP[bw])
        # axs[n][index_col].scatter(withoutDelay['time'], withoutDelay['trimPortion'], label = '15 Workers', alpha=0.75)
        axs[index_row][index_col].set(xlabel='Time (s)', ylabel='CWND Norm')
        axs[index_row][index_col].legend(prop={'size': 7})

        index_row += 1

        # ------ Bad Steps
        axs[index_row][index_col].scatter(withDelay['cwnd'], withDelay['badSteps'], label = f'{bw}mbps', alpha=0.75, color=COLOR_MAP[bw])
        # axs[n][index_col].scatter(withoutDelay['time'], withoutDelay['trimPortion'], label = '15 Workers', alpha=0.75)
        axs[index_row][index_col].set(xlabel='Time (s)', ylabel='BadSteps')
        axs[index_row][index_col].legend(prop={'size': 7})

        index_row = 0
        index_col = 1
        

        # ------ Rtt Min
        axs[index_row][index_col].scatter(withDelay['cwnd'], withDelay['rttminEst'], label = f'{bw}mbps', alpha=0.75, color=COLOR_MAP[bw])
        # axs[n][index_col].scatter(withoutDelay['time'], withoutDelay['trimPortion'], label = '15 Workers', alpha=0.75)
        axs[index_row][index_col].set(xlabel='Time (s)', ylabel='RTT min')
        axs[index_row][index_col].legend(prop={'size': 7})

        index_row += 1


        # ------ BDP Norm
        axs[index_row][index_col].scatter(withDelay['cwnd'], withDelay['bdpNorm'], label = f'{bw}mbps', alpha=0.75, color=COLOR_MAP[bw])
        # axs[n][index_col].scatter(withoutDelay['time'], withoutDelay['trimPortion'], label = '15 Workers', alpha=0.75)
        axs[index_row][index_col].set(xlabel='Time (s)', ylabel='BDP Norm')
        axs[index_row][index_col].legend(prop={'size': 7})

        index_row += 1
        
        # ------ BW Std Step
        axs[index_row][index_col].scatter(withDelay['cwnd'], withDelay['alpha'], label = f'{bw}mbps', alpha=0.75, color=COLOR_MAP[bw])
        # axs[n][index_col].scatter(withoutDelay['time'], withoutDelay['trimPortion'], label = '15 Workers', alpha=0.75)
        axs[index_row][index_col].set(xlabel='Time (s)', ylabel='alpha')
        axs[index_row][index_col].legend(prop={'size': 7})

        index_row += 1

        # ------ Rtt Std Step
        axs[index_row][index_col].scatter(withDelay['cwnd'], withDelay['reward'], label = f'{bw}mbps', alpha=0.75, color=COLOR_MAP[bw])
        # axs[n][index_col].scatter(withoutDelay['time'], withoutDelay['trimPortion'], label = '15 Workers', alpha=0.75)
        axs[index_row][index_col].set(xlabel='Time (s)', ylabel='Reward')
        axs[index_row][index_col].legend(prop={'size': 7})
        axs[index_row][index_col].axvline(0.96*BDP, label = f'BDP-{bw}', linestyle="dashed", color=COLOR_MAP[bw])
        axs[index_row][index_col].axvline(BDP, label = f'BDP-{bw}', linestyle="dashed", color=COLOR_MAP[bw])
        axs[index_row][index_col].axvline(BDP+q_size, label = f'BDP+Buffer{bw}', linestyle="dashed", color=COLOR_MAP[bw])
        axs[index_row][index_col].axhline(2, label = f'Max Reward', linestyle="dashed", color=COLOR_MAP[bw])
        axs[index_row][index_col].axhline(-1, label = f'Min Reward', linestyle="dashed", color=COLOR_MAP[bw])
        axs[index_row][index_col].axhline(1, label = f'Max thr reward', linestyle="dashed", color=COLOR_MAP[bw])


        index_row += 1

        # ------ BW Max
        axs[index_row][index_col].scatter(withDelay['cwnd'], withDelay['bwMax'], label = f'{bw}mbps', alpha=0.75, color=COLOR_MAP[bw])
        # axs[n][index_col].scatter(withoutDelay['time'], withoutDelay['trimPortion'], label = '15 Workers', alpha=0.75)
        axs[index_row][index_col].set(xlabel='Time (s)', ylabel='BW Max')
        axs[index_row][index_col].legend(prop={'size': 7})

        index_row += 1

        
    # fig.tight_layout()
    plt.savefig(f'features_80mbps_8ms_stoc.png')

  

