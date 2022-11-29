#import the simulation model with cart-pole
from build.omnetbind import OmnetGymApi
import gym
from gym import spaces, logger
import numpy as np
import math
from ray.tune.registry import register_env
import ray
from ray import tune
import random
import sys
import os
import math

class OmnetGymApiEnv(gym.Env):
    def __init__(self,env_config):
        
        self.action_space = spaces.Discrete(2)
        self.runner = OmnetGymApi()
        self.env_config = env_config

        
        high = np.array(
            [
                2.4 * 2,
                np.finfo(np.float32).max,
                (12 * 2 * math.pi / 360) * 2,
                np.finfo(np.float32).max,],
            dtype=np.float64,)
        self.observation_space = spaces.Box(-high, high, dtype=np.float64)

       
    def reset(self):

        original_ini_file = self.env_config["iniPath"]

        with open(original_ini_file, 'r') as fin:
            ini_string = fin.read()
        
        ini_string = ini_string.replace("HOME",  os.getenv('HOME'))

        with open(original_ini_file, 'w') as fout:
            fout.write(ini_string)

        self.runner.initialise(original_ini_file)
        obs = self.runner.reset()

        obs = np.asarray(list(obs['cartpole']),dtype=np.float32)
        return  obs

    def step(self, action):
        actions = {'cartpole': action}
        theta_threshold_radians = 12 * 2 * math.pi / 360
        x_threshold = 2.4
        obs, rewards, dones = self.runner.step(actions)
        reward = round(rewards['cartpole'],4)
        obs = obs['cartpole']

        if (obs[0] < x_threshold * -1) or (obs[0] > x_threshold) or (obs[2] < theta_threshold_radians * -1) or (obs[2] > theta_threshold_radians):
            dones['cartpole'] = True
            reward = 0

        if dones['cartpole']:
             self.runner.shutdown()
             self.runner.cleanup()
       
        obs = np.asarray(list(obs),dtype=np.float32)
    
        return  obs, reward, dones['cartpole'], {}

def omnetgymapienv_creator(env_config):
    return OmnetGymApiEnv(env_config)  # return an env instance

register_env("OmnetGymApiEnv", omnetgymapienv_creator)

if __name__ == '__main__':

    env = sys.argv[1]
    num_workers = int(sys.argv[2])
    seed = int(sys.argv[3])


    random.seed(seed)
    np.random.seed(seed)

    ray.init(num_cpus=256)

    config = {"env": env,
            "num_workers" : num_workers,
            "env_config": {"iniPath": os.getenv('HOME') + "/raynet/configs/cartpole/cartpole.ini"},
            "evaluation_config": {
                                "env_config": {"iniPath": os.getenv('HOME') + "/raynet/configs/cartpole/cartpole.ini"},
                                "explore": False

        },
        "horizon": 500,
            "seed":seed}

    ray.tune.run(
        "DQN",  
        name=f"{env}_{num_workers}_{seed}",
        config=config, 
        stop={"episode_reward_mean": 500.0},
        time_budget_s=2000, 
        checkpoint_at_end= True)
    

    ray.shutdown()


