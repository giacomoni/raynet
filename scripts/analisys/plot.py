import sys
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import random
import matplotlib.lines as mlines
from pathlib import Path

if __name__ == "__main__":

    LATENCIES = [['0ms', '0.01ms'], ['0.1ms', '1ms']]
    fig, axs = plt.subplots(2,2, figsize=(12,7.4))

    plt.rcParams['font.family'] = 'serif'
    plt.rcParams["font.serif"] = "Times New Roman"

    plt.setp(axs, xlabel='Initial Window', ylabel='Trimmed Packet / Flow Size', ylim=[0,0.5])
    red_line = mlines.Line2D([],[], color='red', linewidth=1, label='S1')
    blue_line = mlines.Line2D([],[], color='blue', linewidth=1, label='S2')
    green_line = mlines.Line2D([],[], color='green', linewidth=1, label='S3, S4, S5')

    for i in range(2):
        for j in range(2):
            file_name = str(Path.home())+f"/RLlibIntegration/latency_{LATENCIES[i][j]}.csv"
            results_df = (pd.read_csv(file_name, index_col=[0])/20000)
            results_df = results_df.rename(columns={"ndpcongestionnetwork.server1.app[0]":"F1", "ndpcongestionnetwork.server2.app[0]":"F2", 
                                                    "ndpcongestionnetwork.server3.app[0]":"F3", "ndpcongestionnetwork.server3.app[1]":"F4", 
                                                    "ndpcongestionnetwork.server3.app[2]":"F5", "ndpcongestionnetwork.server3.app[3]":"F6"} ) 


            axs[i][j].plot(list(results_df.index),results_df['F1'].values, c="red", linewidth=0.5, marker='s', markersize=2, label="F1")
            axs[i][j].plot(list(results_df.index),results_df['F2'].values, c="blue", linewidth=0.5, marker='s',markersize=2, label="F2")
            axs[i][j].plot(list(results_df.index),results_df['F3'].values, c="green", linewidth=0.5, marker='s',markersize=2,label="F3")
            axs[i][j].plot(list(results_df.index),results_df['F4'].values, c="green", linewidth=0.5, marker='s',markersize=2,label="F4")
            axs[i][j].plot(list(results_df.index),results_df['F5'].values, c="green", linewidth=0.5, marker='s',markersize=2,label="F5")
            axs[i][j].plot(list(results_df.index),results_df['F6'].values, c="green", linewidth=0.5, marker='s',markersize=2,label="F5")

            axs[i][j].title.set_text(f"{LATENCIES[i][j]}")
            axs[i][j].set_xscale('log', base=2)
           

    fig.suptitle("Varying Initial Window on different BDP", fontsize=22)
    fig.legend(handles=[red_line,blue_line,green_line])
    
    #x_ticks = [1,10,20,30,40,50]
    #plt.xticks(x_ticks)
    # plt.subplots_adjust(bottom=0.2, left=0.14)
    plt.tight_layout()
    plt.savefig('InitialWindowBDP_trimmed.png')
 
