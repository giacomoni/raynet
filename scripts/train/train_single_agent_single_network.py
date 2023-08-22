from build.omnetbind import OmnetGymApi 
import gymnasium as gym
from gymnasium import spaces, logger
import numpy as np
import pandas as pd
import random
import ray
import math
from ray import tune
import copy
import nnmodels
import os
from collections import deque
from ray.tune.registry import get_trainable_cls
import sys
from ray.rllib.algorithms.ppo import PPOConfig


from ray.tune.registry import register_env

def uniform(low=0, high=1):
    return np.random.uniform(low, high)

def lognuniform(low=0, high=1, size=None, base=np.e):
    return np.power(base, np.random.uniform(low, high, size))

class OmnetGymApiEnv(gym.Env):
    def __init__(self, env_config):
        self.spec.max_episode_steps = 400
        self.env_config = env_config
        self.stacking = env_config['stacking']
        self.action_space = spaces.Box(low=np.array([-2.0], dtype=np.float32), high=np.array([2.0], dtype=np.float32), dtype=np.float32)
        self.obs_min = np.tile(np.array([-1000000000,  
                                 -1000000000,   
                                 -1000000000,   
                                 -1000000000], dtype=np.float32), self.stacking)

        self.obs_max = np.tile(np.array([10000000000, 
                                 10000000000, 
                                 10000000000, 
                                 10000000000],dtype=np.float32), self.stacking)
        self.currentRecord = None
        self.observation_space = spaces.Box(low=self.obs_min, high=self.obs_max, dtype=np.float32)
        self.runner = OmnetGymApi()
        self.obs = deque(np.zeros(len(self.obs_min)),maxlen=len(self.obs_min))
        self.agentId = None
        
    def  reset(self, *, seed=None, options=None):
        self.obs = deque(np.zeros(len(self.obs_min)),maxlen=len(self.obs_min))
        # Draw network parameters from space
        linkrate_range = self.env_config["linkrate_range"]
        rtt_range = self.env_config["rtt_range"]
        buffer_range = self.env_config["buffer_range"]

        linkrate = uniform(low=linkrate_range[0], high=linkrate_range[1])
        rtt = uniform(low=rtt_range[0], high=rtt_range[1])/2.0
        buffer = uniform(low=buffer_range[0], high=buffer_range[1])

        original_ini_file = self.env_config["iniPath"]
        worker_ini_file = original_ini_file + f".worker{os.getpid()}_{self.env_config.worker_index}"

        with open(original_ini_file, 'r') as fin:
            ini_string = fin.read()
        
        ini_string = ini_string.replace("DELAY_PLACEOLDER", f'{round(rtt,2)}ms')
        ini_string = ini_string.replace("LINKRATE_PLACEHOLDER", f'{round(linkrate)}Mbps')
        ini_string = ini_string.replace("Q_PLACEHOLDER", str(round(buffer)))
        ini_string = ini_string.replace("HOME",  os.getenv('HOME'))

        with open(worker_ini_file, 'w') as fout:
            fout.write(ini_string)

        self.runner.initialise(worker_ini_file)
        obs = self.runner.reset()
        # obs = np.asarray(obs['5'], dtype=np.float32)
        if len(obs.keys()) > 1:
            print(f"************ ERROR: expected only 1 flow, but {len(obs.keys())} were found.") 
        self.agentId = list(obs.keys())[0]
        obs = obs[self.agentId]
        self.currentRecord = obs[-13:]
        self.obs.extend(obs[:-13])
        obs = np.asarray(list(self.obs),dtype=np.float32)
        return obs, {}

    def step(self, action):
        actions = {self.agentId: action}

        if math.isnan(action):
            print("====================================== action passed is nan =========================================")
        obs, rewards, dones = self.runner.step(actions)
        if dones[self.agentId]:
             self.runner.shutdown()
             self.runner.cleanup()

        if math.isnan(rewards[self.agentId]):
            print("====================================== reward returned is nan =========================================")
        reward = round(rewards[self.agentId],4)
        if any(np.isnan(np.asarray(obs[self.agentId], dtype=np.float32))):
            print("====================================== obs returned is nan =========================================")
        

        obs = obs[self.agentId]
        self.currentRecord = obs[-13:]
        self.obs.extend(obs[:-13])
        obs = np.asarray(list(self.obs),dtype=np.float32)
        return  obs, reward, dones[self.agentId], False, {}

def OmnetGymApienv_creator(env_config):
    return OmnetGymApiEnv(env_config)  # return an env instance

register_env("OmnetppEnv", OmnetGymApienv_creator)


if __name__ == "__main__":
    
    alg = float(sys.argv[1])
    seed = int(sys.argv[2])

    env_config = {"iniPath": os.getenv('HOME') + "/raynet/configs/ndpconfig_single_flow_train_with_delay.ini",
                       "linkrate_range": [64,128],
                       "rtt_range": [16, 64],
                       "buffer_range": [80, 800],
                       "stacking": 10}

    evaluation_config =  {
                                "env_config": {"iniPath": os.getenv('HOME') + "/raynet/configs/ndpconfig_single_flow_train_with_delay.ini"},
                                "explore": False,
                                "stacking": 10

    }

    config = (PPOConfig()
    .debugging(seed=seed)
    .training(gamma=alg)
    .rollouts(num_rollout_workers=2)
    .resources(num_gpus=0)
    .environment("OmnetppEnv", env_config=env_config)
    .evaluation(evaluation_config=evaluation_config) # "ns3-v0"
    .build())

    ray.init(num_gpus=0, object_store_memory=1000000000)
    
    # # Create the Trainer from config.
    # cls = get_trainable_cls("SAC")
    # env = OmnetGymApienv_creator(config['env_config'])
    # agent = cls(env="OmnetppEnv", config=config)

    # checkpoint_path = f"/its/home/lg317/ray_results/explicitstate5/SAC_OmnetppEnv_4700e_00000_0_2022-07-05_16-51-05/checkpoint_009000"
    # checkpoint_file = f"/checkpoint-9000" 
    # agent.restore(checkpoint_path + checkpoint_file)

    tune.run(
        "PPO",
        name=f"PPOgamma_{alg}_{seed}",
        stop={"timesteps_total": 1000000},
        config=config,
        checkpoint_freq=100,
        checkpoint_at_end=True,
        resume=False
    )



