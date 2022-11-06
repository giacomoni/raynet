import itertools
import os

if __name__ == "__main__":
    
    SEEDS =  [1]
    ENVS = ["OmnetGymApiEnv"]
    WORKERS = [1]
    
    
    for params in itertools.product(ENVS, WORKERS, SEEDS):
         os.system(f"python3 cartpoleexp.py {params[0]} {params[1]} {params[2]}")