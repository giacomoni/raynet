#import the simulation model with cart-pole
from build.omnetbind import OmnetGymApi
import gymnasium as gym
from gymnasium import spaces, logger
import numpy as np
import math
from ray.tune.registry import register_env
import ray
from ray import tune
import random
import sys
import os
import math
from ray.rllib.algorithms.dqn import DQNConfig
from ns3gym import ns3env
from datetime import datetime



class OmnetGymApiEnv(gym.Env):
    def __init__(self,env_config):
        
        self.action_space = spaces.Discrete(2)
        self.runner = OmnetGymApi()
        self.env_config = env_config
        self.max_episode_len = 500

        
        high = np.array(
            [
                2.4 * 2,
                np.finfo(np.float32).max,
                (12 * 2 * math.pi / 360) * 2,
                np.finfo(np.float32).max,],
            dtype=np.float64,)
        self.observation_space = spaces.Box(-high, high, dtype=np.float64)

       
    def reset(self, *, seed=None, options=None):

        original_ini_file = self.env_config["iniPath"]

        with open(original_ini_file, 'r') as fin:
            ini_string = fin.read()
        
        ini_string = ini_string.replace("HOME",  os.getenv('HOME'))

        with open(original_ini_file + f".worker{os.getpid()}", 'w') as fout:
            fout.write(ini_string)

        self.runner.initialise(original_ini_file + f".worker{os.getpid()}")
        obs = self.runner.reset()

        obs = np.asarray(list(obs['cartpole']),dtype=np.float32)
        return  obs, {}

    def step(self, action):
        actions = {'cartpole': action}
        theta_threshold_radians = 12 * 2 * math.pi / 360
        x_threshold = 2.4
        obs, rewards, terminateds, info_ = self.runner.step(actions)
        reward = round(rewards['cartpole'],4)
        obs = obs['cartpole']

        if (obs[0] < x_threshold * -1) or (obs[0] > x_threshold) or (obs[2] < theta_threshold_radians * -1) or (obs[2] > theta_threshold_radians):
            terminateds['cartpole'] = True
            reward = 0

        if terminateds['cartpole']:
             self.runner.shutdown()
             self.runner.cleanup()
       
        obs = np.asarray(list(obs),dtype=np.float32)
    
        return  obs, reward, terminateds['cartpole'], False,{}

def omnetgymapienv_creator(env_config):
    return OmnetGymApiEnv(env_config)  # return an env instance

def ns3gymapienv_creator(env_config):

    port = 5555 + env_config.worker_index
    simTime = 500 # seconds
    stepTime = 1  # seconds
    seed = 0 + env_config.worker_index
    simArgs = {"--simTime": simTime,
            "--testArg": 123}
    debug = False
    startSim = 1
    return ns3env.Ns3Env(port=port, stepTime=stepTime, startSim=startSim, simSeed=seed, simArgs=simArgs, debug=debug)  # return an env instance

register_env("ns3-v0", ns3gymapienv_creator)
register_env("OmnetGymApiEnv", omnetgymapienv_creator)

if __name__ == '__main__':

    env = sys.argv[1]
    num_workers = int(sys.argv[2])
    seed = int(sys.argv[3])


    random.seed(seed)
    np.random.seed(seed)

    ray.init(num_cpus=64)
    
    env_config = {"iniPath": os.getenv('HOME') + "/raynet/configs/cartpole/cartpole.ini"}

    algo = (
    DQNConfig()
    .rollouts(num_rollout_workers=num_workers)
    .resources(num_gpus=0)
    .environment(env, env_config=env_config) # "ns3-v0"
    .build()
)

    t1 = datetime.now()
    t2 = datetime.now()
    while (t2 - t1).total_seconds() <= 2000:
        print(f"Total elpsed: {(t2 - t1).total_seconds()}")
        result = algo.train()
        print(result['episode_reward_mean'])
        if result['episode_reward_mean'] >= 450:
            break
        t2 = datetime.now()

    ray.shutdown()


