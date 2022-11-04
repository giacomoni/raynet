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


fig, axs = plt.subplots(1, 1)

LABELS = {'PPO099': r'$\gamma$'+' = 0.99',
'PPO01': r'$\gamma$'+' = 0.1'}

for alg in ['PPO099', 'PPO01']:
    data =  pd.read_csv(os.getenv('HOME')+"/Downloads"+f'/{alg}.csv', index_col=[0])
    axs.plot(data['Step'], data['Value'], label = f'{LABELS[alg]}', alpha=0.75)
    axs.set(xlabel='Step', ylabel='Mean Cumulative Reward')
    axs.legend()
    plt.savefig(f'PPOTraining.png', dpi=300)