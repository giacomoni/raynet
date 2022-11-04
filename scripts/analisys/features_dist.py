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
            
             32:'red',
             64: 'blue',
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
    bandwidths = [128]

    checknumber = 6100

    for bw in bandwidths:
    # ---- BDP
        mss = 1500
        prop_delay = 0.004
        linkrate = bw*1000000
        q_size = 8
        rtt = 6*prop_delay+3*(8*mss)/linkrate + 3*(8*20)/linkrate
        BDP = (linkrate*(rtt)/8)/mss
        index_row = 0
        index_col = 0
        
        # ------ Data
        withDelay = pd.read_csv(f'rollout_{checknumber}_15_workers_{bw}mbps_stoc.csv', index_col=[0]).reset_index(drop=True)
        afterBdp = withDelay[withDelay['cwnd'] > BDP + q_size]
        inBdp = withDelay[(withDelay['cwnd'] <= BDP + q_size) & (withDelay['cwnd'] >= BDP)]
        beforeBdp = withDelay[withDelay['cwnd'] < BDP]
        # withoutDelay = pd.read_csv(f'rollout_{checknumber}_15_workers.csv')
        
        # ------ Rtt Norm
        sns.kdeplot(beforeBdp['rttNorm'], x='rttNorm', ax=axs[index_row][index_col], color='blue')
        sns.kdeplot(inBdp['rttNorm'], x='rttNorm', ax=axs[index_row][index_col],color='green')
        sns.kdeplot(afterBdp['rttNorm'], x='rttNorm', ax=axs[index_row][index_col], color='red')
        # axs[n][index_col].plot(withoutDelay['time'], withoutDelay['trimPortion'], label = '15 Workers', alpha=0.75)
        # axs[index_row][index_col].set(xlabel='rttNorm')
        axs[index_row][index_col].legend(prop={'size': 7})

        index_row += 1

         # ------ Rtt Norm
        sns.kdeplot(beforeBdp['bwNorm'], x='bwNorm', ax=axs[index_row][index_col], color='blue')
        sns.kdeplot(inBdp['bwNorm'], x='bwNorm',ax=axs[index_row][index_col], color='green')
        sns.kdeplot(afterBdp['bwNorm'], x='bwNorm',ax=axs[index_row][index_col], color='red')
        # axs[n][index_col].plot(withoutDelay['time'], withoutDelay['trimPortion'], label = '15 Workers', alpha=0.75)
        # axs[index_row][index_col].set(xlabel='bwNorm')
        axs[index_row][index_col].legend(prop={'size': 7})

        index_row += 1
          # ------ Rtt Norm
        sns.kdeplot(beforeBdp['trimPortion'], x='trimPortion', ax=axs[index_row][index_col], color='blue')
        sns.kdeplot(inBdp['trimPortion'],  x='trimPortion', ax=axs[index_row][index_col], color='green')
        sns.kdeplot(afterBdp['trimPortion'],  x='trimPortion', ax=axs[index_row][index_col], color='red')
        # axs[n][index_col].plot(withoutDelay['time'], withoutDelay['trimPortion'], label = '15 Workers', alpha=0.75)
        # axs[index_row][index_col].set(xlabel='trimPortion')
        axs[index_row][index_col].legend(prop={'size': 7})

        index_row += 1

               # ------ Rtt Norm
        sns.kdeplot(beforeBdp['cwndNorm'], x='cwndNorm', ax=axs[index_row][index_col], color='blue')
        sns.kdeplot(inBdp['cwndNorm'], x='cwndNorm',ax=axs[index_row][index_col], color='green')
        sns.kdeplot(afterBdp['cwndNorm'], x='cwndNorm',ax=axs[index_row][index_col], color='red')
        # axs[n][index_col].plot(withoutDelay['time'], withoutDelay['trimPortion'], label = '15 Workers', alpha=0.75)
        # axs[index_row][index_col].set(xlabel='cwndNorm')
        axs[index_row][index_col].legend(prop={'size': 7})

        index_row += 1
        # ------ Rtt Norm
        sns.kdeplot(beforeBdp['badSteps'], x='badSteps', ax=axs[index_row][index_col], color='blue')
        sns.kdeplot(inBdp['badSteps'],  x='badSteps',ax=axs[index_row][index_col], color='green')
        sns.kdeplot(afterBdp['badSteps'], x='badSteps',ax=axs[index_row][index_col], color='red')
        # axs[n][index_col].plot(withoutDelay['time'], withoutDelay['trimPortion'], label = '15 Workers', alpha=0.75)
        # axs[index_row][index_col].set(xlabel='badSteps')
        axs[index_row][index_col].legend(prop={'size': 7})

        index_row = 0
        index_col = 1
        

               # ------ Rtt Norm
        sns.kdeplot(beforeBdp['rttminEst'], x='rttminEst', ax=axs[index_row][index_col], color='blue')
        sns.kdeplot(inBdp['rttminEst'],  x='rttminEst',ax=axs[index_row][index_col], color='green')
        sns.kdeplot(afterBdp['rttminEst'],  x='rttminEst',ax=axs[index_row][index_col], color='red')
        # axs[n][index_col].plot(withoutDelay['time'], withoutDelay['trimPortion'], label = '15 Workers', alpha=0.75)
        # axs[index_row][index_col].set(xlabel='rttminEst')
        axs[index_row][index_col].legend(prop={'size': 7})
        index_row += 1


                 # ------ Rtt Norm
        sns.kdeplot(beforeBdp['bdpNorm'], x='bdpNorm',ax=axs[index_row][index_col], color='blue')
        sns.kdeplot(inBdp['bdpNorm'], x='bdpNorm',ax=axs[index_row][index_col],color='green')
        sns.kdeplot(afterBdp['bdpNorm'], x='bdpNorm',ax=axs[index_row][index_col], color='red')
        # axs[n][index_col].plot(withoutDelay['time'], withoutDelay['trimPortion'], label = '15 Workers', alpha=0.75)
        # axs[index_row][index_col].set(xlabel='bdpNorm')
        axs[index_row][index_col].legend(prop={'size': 7})

        index_row += 1
        
        sns.kdeplot(beforeBdp['alpha'], x= 'alpha', ax=axs[index_row][index_col], color='blue')
        sns.kdeplot(inBdp['alpha'], x= 'alpha',ax=axs[index_row][index_col],color='green')
        sns.kdeplot(afterBdp['alpha'], x= 'alpha',ax=axs[index_row][index_col], color='red')
        # axs[n][index_col].plot(withoutDelay['time'], withoutDelay['trimPortion'], label = '15 Workers', alpha=0.75)
        # axs[index_row][index_col].set(xlabel='alpha')
        axs[index_row][index_col].legend(prop={'size': 7})


        index_row += 1

        sns.kdeplot(beforeBdp['reward'], x='reward', ax=axs[index_row][index_col], color='blue')
        sns.kdeplot(inBdp['reward'], x='reward',ax=axs[index_row][index_col],color='green')
        sns.kdeplot(afterBdp['reward'], x='reward',ax=axs[index_row][index_col],color='red')
        # axs[n][index_col].plot(withoutDelay['time'], withoutDelay['trimPortion'], label = '15 Workers', alpha=0.75)
        # axs[index_row][index_col].set(xlabel='reward')
        axs[index_row][index_col].legend(prop={'size': 7})
        index_row += 1

        # ------ BW Max
        sns.kdeplot(beforeBdp['bwMax'], x='bwMax', ax=axs[index_row][index_col], color='blue')
        sns.kdeplot(inBdp['bwMax'], x='bwMax',ax=axs[index_row][index_col], color='green')
        sns.kdeplot(afterBdp['bwMax'], x='bwMax',ax=axs[index_row][index_col], color='red')
        # axs[n][index_col].plot(withoutDelay['time'], withoutDelay['trimPortion'], label = '15 Workers', alpha=0.75)
        # axs[index_row][index_col].set(xlabel='bwMax')
        axs[index_row][index_col].legend(prop={'size': 7})

        index_row += 1

        
    # fig.tight_layout()
    plt.savefig(f'features_dist_stoc.png')

  

