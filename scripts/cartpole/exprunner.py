import itertools
import os

if __name__ == "__main__":
    
    SEEDS =  [1,10]
    ENVS = ["OmnetGymApiEnv", "CartPole-v1"]
    WORKERS = [2,4,8,16,31]
    
    
    for params in itertools.product(ENVS, WORKERS, SEEDS):
         os.system(f"python3 cartpoleexp.py {params[0]} {params[1]} {params[2]}")