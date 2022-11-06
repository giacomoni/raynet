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

ModelCatalog.register_custom_model("bn_model",KerasBatchNormModel)


ray.init(num_cpus=15)

class OmnetGymApiEnv(gym.Env):
    def __init__(self, env_config):
        self.reward_total = 0
        self.obs_list = [(0,0,0,0,0,0,0,0,0),(0,0,0,0,0,0,0,0,0),(0,0,0,0,0,0,0,0,0),(0,0,0,0,0,0,0,0,0),(0,0,0,0,0,0,0,0,0),(0,0,0,0,0,0,0,0,0),(0,0,0,0,0,0,0,0,0),(0,0,0,0,0,0,0,0,0),(0,0,0,0,0,0,0,0,0),(0,0,0,0,0,0,0,0,0)]
        self.env_config = env_config

        self.action_space = spaces.Box(
            low=-2, high=2, shape=(1,), dtype=np.float32
        )

        self.observation_space = spaces.Box(
            low=-np.finfo(np.float32).max, high=np.finfo(np.float32).max, shape = (10*9,), dtype=np.float32)
        self.runner = OmnetGymApi()

    def reset(self):

        # per = np.random.uniform(0.001,0.007)
        # delay = np.random.uniform(0.005,0.010)
        # datarate = np.random.uniform(50,100)

        per = 0.002
        delay = 0.005
        datarate = 85

        original_ini_file = self.env_config["iniPath"]
        worker_ini_file = original_ini_file + f".worker{os.getpid()}_{self.env_config.worker_index}"

        with open(original_ini_file, 'r') as fin:
            ini_string = fin.read()
        
        ini_string = ini_string.replace("PER_PLACEOLDER", str(per))
        ini_string = ini_string.replace("DELAY_PLACEHOLDER", str(delay) + "s")
        ini_string = ini_string.replace("RATE_PLACEHOLDER", str(datarate) + "Mbps")
        ini_string = ini_string.replace("HOME",  os.getenv('HOME'))

        with open(worker_ini_file, 'w') as fout:
            fout.write(ini_string)

        self.runner.initialise(worker_ini_file)

        print("paramters: ")
        print(per) 
        print(delay) 
        print(datarate)

        self.obs_list = [(0,0,0,0,0,0,0,0,0),(0,0,0,0,0,0,0,0,0),(0,0,0,0,0,0,0,0,0),(0,0,0,0,0,0,0,0,0),(0,0,0,0,0,0,0,0,0),(0,0,0,0,0,0,0,0,0),(0,0,0,0,0,0,0,0,0),(0,0,0,0,0,0,0,0,0),(0,0,0,0,0,0,0,0,0),(0,0,0,0,0,0,0,0,0)]
        self.reward_total = 0
        multi_obs = self.runner.reset()
        obs  = multi_obs["4"][:9]
        self.obs_list.pop(0)
        self.obs_list.append(np.asarray(obs))
        return np.asarray(self.obs_list, dtype=np.float32).flatten()

    def step(self, action):
        actions = {'4': action}
        obs, rewards, dones = self.runner.step(actions)
        obs_used = obs["4"][:9]
        self.obs_list.pop(0)
        self.obs_list.append(np.asarray(obs_used))
       
        self.reward_total += rewards["4"] 
        if dones["4"] == True:
            print("========================== Total reward of episode is:  " + str(self.reward_total))
        return np.asarray(self.obs_list, dtype=np.float32).flatten(), float(rewards['4']), dones['4'], {}


def omnetgymapienv_creator(env_config):
    return OmnetGymApiEnv(env_config)  # return an env instance


register_env("OmnetGymApiEnv", omnetgymapienv_creator)
config = {"env": "OmnetGymApiEnv",
          "env_config": {"iniPath": os.getenv('HOME') + "/raynet/configs/orca/orcaConfigStatic.ini"},

          "evaluation_interval": 100,
          "evaluation_config": {
              "env_config": {"iniPath": os.getenv('HOME') + "/raynet/configs/orca/orcaConfigStatic.ini"},
              "explore": False
          },
          "num_workers": 10,
          "horizon": 350,
          "no_done_at_end": True,
          "soft_horizon": False,
          "optimizer" : "Adam",
          "lr":0.001,
          "critic_lr": 0.001,
          "actor_lr": 0.0001,
           "exploration_config": {
             "type": "GaussianNoise",
             "stddev": 0.2
            },
          "train_batch_size":8096,
          "gamma": 0.995,
          "framework": 'tf',
        #   "model": {
        #     "custom_model" : "bn_model",
        #     "custom_model_config" : {}
        #   }
          }


analysis = ray.tune.run(
    "TD3", name="orca",stop={"training_iteration": 200000}, config=config, checkpoint_freq=50)

