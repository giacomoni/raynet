from build.omnetbind import OmnetGymApi
from nnmodels import KerasBatchNormModel
import gym
from gym import spaces, logger
import numpy as np
import math
from ray.tune.registry import register_env
from ray.rllib.agents.ddpg.td3 import TD3Trainer
import ray
import pandas as pd
from ray.rllib.models import ModelCatalog
import os
from collections import deque


# ModelCatalog.register_custom_model("bn_model",KerasBatchNormModel)

def uniform(low=0, high=1):
    return np.random.uniform(low, high)

class OmnetGymApiEnv(gym.Env):
    def __init__(self, env_config):
        self.env_config = env_config
        self.stacking = env_config['stacking']
        self.action_space = spaces.Box(low=np.array([-2.0], dtype=np.float32), high=np.array([2.0], dtype=np.float32), dtype=np.float32)
        self.obs_min = np.tile(np.array([-1000000000,  
                                 -1000000000,   
                                 -1000000000,   
                                 -1000000000,
                                 -1000000000,
                                 -1000000000,
                                 -1000000000], dtype=np.float32), self.stacking)

        self.obs_max = np.tile(np.array([10000000000, 
                                 10000000000, 
                                 10000000000, 
                                 10000000000,
                                 10000000000,
                                 10000000000,
                                 10000000000],dtype=np.float32), self.stacking)
        self.currentRecord = None
        self.observation_space = spaces.Box(low=self.obs_min, high=self.obs_max, dtype=np.float32)
        self.runner = OmnetGymApi()
        self.obs = deque(np.zeros(len(self.obs_min)),maxlen=len(self.obs_min))
        self.agentId = None
    def reset(self):
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
        if len(obs.keys()) > 1:
            print(f"************ ERROR: expected only 1 flow, but {len(obs.keys())} were found.") 
        self.agentId = list(obs.keys())[0]
        obs = obs[self.agentId]
        self.currentRecord = obs
        self.obs.extend(obs)
        obs = np.asarray(list(self.obs),dtype=np.float32)
        return obs

    def step(self, action):
        action = 2**action

        actions = {self.agentId: action}

        if math.isnan(action):
            print("====================================== action passed is nan =========================================")
        obs, rewards, dones, info_= self.runner.step(actions)
        if dones[self.agentId]:
             self.runner.shutdown()
             self.runner.cleanup()

        if math.isnan(rewards[self.agentId]):
            print("====================================== reward returned is nan =========================================")
        reward = round(rewards[self.agentId],4)
        if any(np.isnan(np.asarray(obs[self.agentId], dtype=np.float32))):
            print("====================================== obs returned is nan =========================================")
        

        obs = obs[self.agentId]
        self.currentRecord = obs
        self.obs.extend(obs)
        obs = np.asarray(list(self.obs),dtype=np.float32)

        if info_['simDone']:
             dones[self.agentId] = True
        return  obs, reward, dones[self.agentId], {}


def OmnetGymApienv_creator(env_config):
    return OmnetGymApiEnv(env_config)  # return an env instance

register_env("OmnetppEnv", OmnetGymApienv_creator)


config = {"env": "OmnetppEnv",
          "env_config": {"iniPath": os.getenv('HOME') + "/raynet/configs/orca/orcaConfigStatic.ini",
          "stacking": 10,
          "linkrate_range": [64,64],
          "rtt_range": [16, 16],
          "buffer_range": [250, 250],},

          "num_workers": 2,
          "horizon": 2000,
          "no_done_at_end": True,
          "soft_horizon": False,
        #   "optimizer" : "Adam",
        #   "lr":0.001,
        #   "critic_lr": 0.001,
        #   "actor_lr": 0.0001,
        #    "exploration_config": {
        #      "type": "GaussianNoise",
        #      "stddev": 0.2
        #     },
        #   "train_batch_size":8096,
          "gamma": 0.995,
          "framework": 'tf',
          }


analysis = ray.tune.run(
    "TD3", name="orca",stop={"training_iteration": 200000}, config=config, checkpoint_freq=50)

