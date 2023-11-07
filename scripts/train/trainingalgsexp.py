from multiprocessing import Pool, TimeoutError
import os
import time
import os
import itertools

def run_training(exp):
    os.system(f"python train_single_agent_single_network.py {exp[0]} {exp[1]}") 

def run_training_test(exp):
    print(f"{exp[0]} {exp[1]}")

if __name__ == '__main__':
    EXPS_PARAMS = [
        ['PPO', 'DDPG', 'SAC'],
        [1,10,100]
        ]
    EXPS = []
    for element in itertools.product(*EXPS_PARAMS):
        EXPS.append(element)

    # start 4 worker processes
    with Pool(processes=1) as pool:
        pool.map(run_training, EXPS)
        
