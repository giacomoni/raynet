#!/usr/bin/env python

import sys
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import random

if __name__ == "__main__":
    results = []
    plt.rcParams['font.family'] = 'serif'
    plt.rcParams["font.serif"] = "Times New Roman"
    resultsX = [100, 200, 300, 350, 400, 450,600,800,1200,1600, 2400, 2800, 3200]
    resultsY = [0.29842, 0.59568, 0.88657, 0.916863, 0.977423, 0.98142, 0.93894, 0.91364, 0.869601, 0.841692, 0.82215947, 0.811752,0.81548]
    plt.figure(figsize=(6,3.7))
    plt.xticks(fontsize=20)
    plt.yticks(fontsize=20)
    plt.plot(resultsX,resultsY, c="red", linewidth=1, label="")
    axes = plt.gca()

    axes.set_xlim([100, 3200])
    axes.set_ylim([0,1])
    plt.xlabel('Initial Window', fontsize=21)
    plt.ylabel('Goodput (Gbps)', fontsize=21)
    plt.title("Varying Initial Window - One to One", fontsize=22)
    plt.xticks([100,800,1600,2400,3200])
    #plt.legend(loc="lower right")
    #x_ticks = [1,10,20,30,40,50]
    #plt.xticks(x_ticks)
    plt.subplots_adjust(bottom=0.2, left=0.14)
    plt.savefig('SimpleNDP.png')
    plt.savefig('../../../../../ResearchWork/GIPReport/ProjectImages/SimpleNDP.pgf', format='pgf')
    

