from build.omnetbind import SimulationRunner
import gym
from gym import spaces, logger
import numpy as np
import pandas as pd
import random
import ray
from ray import tune
from ray.tune.registry import get_trainable_cls
import os

from ray.tune.registry import register_env
import argparse
import copy
import math
from collections import deque, defaultdict


class SimulationRunnerEnv(gym.Env):
    def __init__(self, env_config):
        self.env_config = env_config
        self.stacking = env_config['stacking']
        self.action_space = spaces.Box(low=np.array([-2.0], dtype=np.float32), high=np.array([2.0], dtype=np.float32), dtype=np.float32)
        self.features_min =  np.array([-1000,  
                                 0,   
                                 0,   
                                 0], dtype=np.float32)

        self.obs_min = np.tile(self.features_min, self.stacking)

        self.obs_max = np.tile(np.array([10000000000, 
                                 10000000000, 
                                 10000000000, 
                                 10000000000],dtype=np.float32), self.stacking)
        self.currentRecord = defaultdict(None)
        self.observation_space = spaces.Box(low=self.obs_min, high=self.obs_max, dtype=np.float32)
        self.runner = SimulationRunner()
        self.obs = defaultdict(lambda: deque(np.zeros(len(self.obs_min)),maxlen=len(self.obs_min)))
   
    def reset(self):
       
        self.obs = defaultdict(lambda: deque(np.zeros(len(self.obs_min)),maxlen=len(self.obs_min)))

        self.runner.initialise(self.env_config["iniPath"])
        obs = self.runner.reset()
        for key, value in obs.items():
            return_obs =  np.asarray(value[:-13], dtype=np.float32)
            self.currentRecord[key] = value[-13:]
            self.current_agent = key
            self.obs[key].extend(value[:-13])
            
        return_obs = np.asarray(list(self.obs[self.current_agent]),dtype=np.float32)

        return return_obs

    def step(self, actions):
        actions = {self.current_agent: actions}
        obs, rewards, dones = self.runner.step(actions)
        assert len(obs) == 1, "More than 1 agent stepped!"
        assert len(rewards) == 1, "More than 1 agent stepped!"

        if dones['__all__']:
             self.runner.shutdown()
             self.runner.cleanup()
             done = True
        else:
            done = False


        for key, value in obs.items():
            return_obs = np.asarray(value[:-13], dtype=np.float32)
            self.currentRecord[key] = value[-13:]
            self.current_agent = key
            self.obs[key].extend(value[:-13])

        for key, value in rewards.items():
            return_reward = value
        
        return_obs = np.asarray(list(self.obs[self.current_agent]),dtype=np.float32)

        return  return_obs, return_reward, done, {}

def simulationrunnerenv_creator(env_config):
    return SimulationRunnerEnv(env_config)  # return an env instance

register_env("OmnetppEnv", simulationrunnerenv_creator)

def run_episode(agent, env, explore):
    rollout = pd.DataFrame(columns=['agent',
                                    'bwNorm', 
                                    'trimPortion',
                                    'rttNorm', 
                                    'cwndNorm' , 
                                    'time',
                                    'cwnd',
                                    'paceTime',
                                    'bwMean',
                                    'bwStd', 
                                    'rttMean', 
                                    'rttStd', 
                                    'bwMax', 
                                    'rttminEst',
                                    'action',
                                    'reward',
                                    'lossrate1',
                                    'lossrate2',
                                    'rtt',
                                    'goodput'])
    done = False
    obs = env.reset()
    step_counter = 1
    while not done:
        agent_id = env.current_agent
        trans = agent.compute_single_action(obs, explore=explore)
        new_obs, rewards, dones, _ = env.step(trans)

        rollout = pd.concat([rollout, pd.DataFrame({'agent': agent_id,
                            'bwNorm': list(obs[-len(env.features_min):])[0], 
                              'trimPortion': list(obs[-len(env.features_min):])[1], 
                              'rttNorm': round(list(obs[-len(env.features_min):])[2]),
                              'cwndNorm': list(obs[-len(env.features_min):])[3],
                              'action': trans,
                              'reward': rewards,
                              'time': env.currentRecord[agent_id][0],
                              'cwnd': env.currentRecord[agent_id][1],
                              'paceTime': env.currentRecord[agent_id][2],
                              'bwMean':  env.currentRecord[agent_id][3],
                              'bwStd':  env.currentRecord[agent_id][4],
                              'rttMean':  env.currentRecord[agent_id][5],
                              'rttStd':  env.currentRecord[agent_id][6],
                              'rttminEst':  env.currentRecord[agent_id][7],
                              'bwMax': env.currentRecord[agent_id][8],
                              'lossrate1': env.currentRecord[agent_id][9],
                              'lossrate2': env.currentRecord[agent_id][10],
                              'rtt': env.currentRecord[agent_id][11],
                              'goodput': env.currentRecord[agent_id][12]}, index=[0])])

        step_counter += 1
        obs = new_obs
        done = dones

    # env.runner.shutdown()
    return rollout

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-p','--policy', required=True, type=int, help='policy number to test')
    parser.add_argument('-r','--rate', required=True, type=int, help='bottleneck linkrate')
    parser.add_argument('-d','--delay', required=True, type=int, help='rtt')
    parser.add_argument('-b','--buffer', required=True, type=int, help='buffer size')
    parser.add_argument('-e','--explore', required=False, action='store_true', help='explore')
    args = parser.parse_args()

    HOMEPATH = os.getenv('HOME')

    os.environ["OMNETPP_NED_PATH"] = f"{HOMEPATH}/RLlibIntegration/model:{HOMEPATH}/inet/src/inet:{HOMEPATH}/inet/examples:{HOMEPATH}/rdp/src/:{HOMEPATH}/rdp/simulations:{HOMEPATH}/RLlibIntegration/rltcp/rltcp/src:{HOMEPATH}/ecmp/src"
    os.environ["NEDPATH"] = f"{HOMEPATH}/RLlibIntegration/model:{HOMEPATH}/inet/src/inet:{HOMEPATH}/inet/examples:{HOMEPATH}/rdp/src/:{HOMEPATH}/rdp/simulations:{HOMEPATH}/RLlibIntegration/rltcp/rltcp/src:{HOMEPATH}/ecmp/src"


    bw = args.rate
    rtt = args.delay
    buffer = args.buffer
    explore = args.explore

    ray.init()

    
    config = {
        "env": "OmnetppEnv",
        "env_config": {"iniPath": os.getenv('HOME') + f"/RLlibIntegration/configs/ndpconfig_single_flow_eval_with_delay_{os.getpid()}.ini",
                    "linkrate_range": [1,1000],
                    "rtt_range": [1, 500],
                    "buffer_range": [2, 64000],
                    "stacking": 5},
        "evaluation_config": {
                                "env_config": {"iniPath": os.getenv('HOME') + f"/RLlibIntegration/configs/ndpconfig_single_flow_eval_with_delay_{os.getpid()}.ini"},
                                "explore": False,
                                "stacking": 5
        },
    "num_workers": 0,
    "horizon": 400,
    "no_done_at_end":True,
    "soft_horizon":False,
    "gamma": 0.1  }

    with open(os.getenv('HOME') + "/RLlibIntegration/configs/ndpconfig_single_flow_eval_with_delay_template.ini", 'r') as fin:
        config_template = fin.read()

    ini_file = config_template.replace('DELAY_PLACEHOLDER', f'{round(rtt/6, 2)}ms')
    ini_file = ini_file.replace('RATE_PLACEHOLDER', f'{bw}Mbps')
    ini_file = ini_file.replace('BUFFER_PLACEHOLDER', f'{buffer}')

    with open(os.getenv('HOME') + f"/RLlibIntegration/configs/ndpconfig_single_flow_eval_with_delay_{os.getpid()}.ini", 'w') as fout:
        fout.write(ini_file)


    check_no = args.policy
    # Create the Trainer from config.
    cls = get_trainable_cls("APEX_DDPG")
    env = simulationrunnerenv_creator(config['env_config'])
    agent = cls(env="OmnetppEnv", config=config)

    checkpoint_path = os.getenv('HOME') + f"/ray_results/DDPGgamma05/APEX_DDPG_OmnetppEnv_70a51_00000_0_2022-08-03_18-12-23/checkpoint_{check_no:06}"
    checkpoint_file = f"/checkpoint-{check_no}" 
    agent.restore(checkpoint_path + checkpoint_file)

    rollout = run_episode(agent, env, explore)

    rollout.to_csv(os.getenv('HOME') +f"/stacking_results/rollout_obsb_stack5_DDPG05multitrain_{check_no}_15_workers_{buffer}pkts_{bw}mbps_{rtt}ms_{'stoc' if explore else 'det'}.csv")

    os.remove(os.getenv('HOME') + f"/RLlibIntegration/configs/ndpconfig_single_flow_eval_with_delay_{os.getpid()}.ini")