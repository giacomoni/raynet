from build.omnetbind import SimulationRunner
import gym
from gym import spaces, logger
import numpy as np
import pandas as pd
import random
import ray
import math
from ray import tune
import copy
from ray.rllib.policy.policy import PolicySpec
from ray.rllib.env.multi_agent_env import MultiAgentEnv

from ray.tune.registry import register_env

class SimulationRunnerEnv(MultiAgentEnv):
    def __init__(self, env_config):
        self.env_config = env_config
        self.action_space = spaces.Box(-2,2,shape=(1,), dtype=np.float32)
        self.obs_min = np.array([0,   0,   0,   0,   0,   -3, 0,    -2],dtype=np.float32)
        self.obs_max = np.array([0.1, 0.1, 0.1, 0.1, 1000, 1, 500000,2],dtype=np.float32)
        self._agent_ids = set([str(x) for x in range(20)])

        self.observation_space = spaces.Box(low=self.obs_min, high=self.obs_max, dtype=np.float32)
        self.runner = SimulationRunner()

    def reset(self):
       
        # if not isinstance(self.runner, type(None)):
        #     del self.runner


        self.runner.initialise(self.env_config["iniPath"])
        obs = self.runner.reset()

        for key, value in obs.items():
            obs[key] = np.asarray(value, dtype=np.float32)

        return obs

    def step(self, actions):
        obs, rewards, dones = self.runner.step(actions)
        if dones['__all__']:
             self.runner.shutdown()
             self.runner.cleanup()
        
        for key, value in obs.items():
            obs[key] = np.asarray(value, dtype=np.float32)

        return  obs, rewards, dones, {}


def simulationrunnerenv_creator(env_config):
    return SimulationRunnerEnv(env_config)  # return an env instance

register_env("OmnetppEnv", simulationrunnerenv_creator)


def single_agent_train():
    config = {
        "env": "OmnetppEnv",
        "env_config": {"iniPath": "/home/luca/RLlibIntegration/src/ndpconfig_single_flow_train.ini"},
        "evaluation_config": {
                                "env_config": {"iniPath": "/home/luca/RLlibIntegration/src/ndpconfig_single_flow_train.ini"},
                                "explore": False
        },
     "horizon": 400   }

    ray.init()
    tune.run(
        "SAC",
        name="MIMD-Experiment3",
        stop={"timesteps_total": 300000},
        config=config,
        checkpoint_freq=100,
        checkpoint_at_end=True
    )

def multi_agent_train():
    config={
        "multiagent": {
                 "count_steps_by": "env_steps",
                "policies": {
            # Use the PolicySpec namedtuple to specify an individual policy:
            "default_policy": PolicySpec(),  # alternatively, simply do: `PolicySpec(config={"gamma": 0.85})`
        },
        "policy_mapping_fn":
            lambda agent_id, episode, worker, **kwargs:
                "default_policy"  #
        },
        "env": "OmnetppEnv",
        "env_config": {"iniPath": "/home/luca/RLlibIntegration/src/ndpconfig_single_flow_train.ini"},
        "evaluation_config": {
                                "env_config": {"iniPath": "/home/luca/RLlibIntegration/src/ndpconfig_single_flow_train.ini"},
                                "explore": False
        },
}
    ray.init()
    tune.run(
        "SAC",
        name="MultiAgent",
        stop={"timesteps_total": 500000},
        config=config,
        checkpoint_freq=100,
        checkpoint_at_end=True
    )


if __name__ == "__main__":
    multi_agent_train()

    

