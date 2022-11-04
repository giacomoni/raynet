import argparse
import os
from multiprocessing import Pool
import multiprocessing as mp
import itertools
import ray

def evaluate(params):
    os.system(f"python3 evaluateMulti.py -p {params[0]} -r {params[1]} -d {params[2]} -b {params[3]} {'-e' if params[4] else ''} ")



if __name__ == "__main__":
    mp.set_start_method('forkserver')
    parser = argparse.ArgumentParser()
    parser.add_argument('-p','--policy', type=int, help='policy number to test')
    args = parser.parse_args()

    BANDWIDTHS = [96]
    BUFFERS = [440]
    RTTS = [40]
    EXPLORES = [False]

    N_PROCESSES=1

    SINGLE_PARAMS = [
        [args.policy],
        BANDWIDTHS,
        RTTS,
        BUFFERS,
        EXPLORES
    ]

    EVAL_PARAMS = []

    for element in itertools.product(*SINGLE_PARAMS):
        EVAL_PARAMS.append(element)
    

    ray.init(num_cpus=N_PROCESSES)

    with Pool(processes=N_PROCESSES) as pool:
        pool.map(evaluate, EVAL_PARAMS)
    