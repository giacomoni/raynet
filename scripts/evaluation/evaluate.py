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
from collections import deque

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
        self.currentRecord = None
        self.observation_space = spaces.Box(low=self.obs_min, high=self.obs_max, dtype=np.float32)
        self.runner = SimulationRunner()
        self.obs = deque(np.zeros(len(self.obs_min)),maxlen=len(self.obs_min))
   
    def reset(self):
       
        self.obs = deque(np.zeros(len(self.obs_min)),maxlen=len(self.obs_min))
        self.runner.initialise(self.env_config["iniPath"])
        obs = self.runner.reset()
        obs = obs['5']
        self.currentRecord = obs[-11:]
        self.obs.extend(obs[:-11])
        obs = np.asarray(list(self.obs),dtype=np.float32)
        return obs
        
    def step(self, action):
        actions = {'5': action}

        if math.isnan(action):
            print("====================================== action passed is nan =========================================")
        obs, rewards, dones = self.runner.step(actions)
        if dones['5']:
             self.runner.shutdown()
             self.runner.cleanup()

        if math.isnan(rewards['5']):
            print("====================================== reward returned is nan =========================================")

        if any(np.isnan(np.asarray(obs['5'], dtype=np.float32))):
            print("====================================== obs returned is nan =========================================")
        reward = round(rewards['5'],4)

        obs = obs['5']
        self.currentRecord = obs[-11:]
        self.obs.extend(obs[:-11])
        obs = np.asarray(list(self.obs),dtype=np.float32)
        return  obs, reward, dones['5'], {}


def simulationrunnerenv_creator(env_config):
    return SimulationRunnerEnv(env_config)  # return an env instance

register_env("OmnetppEnv", simulationrunnerenv_creator)

def run_episode(agent, env, explore):
    rollout = pd.DataFrame(columns=[
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
                                    'lossrate2'])
    done = False
    obs = env.reset()
    
    # rollout = pd.concat([rollout, pd.DataFrame({'rttNorm': list(obs[-9:])[0],
    #                           'bwNorm': list(obs[-9:])[1], 
    #                           'trimPortion': list(obs[-9:])[2], 
    #                           'cwndNorm': round(list(obs[-9:])[3], 4),
    #                           'badSteps': list(obs[-9:])[4],
    #                           'bdpNorm': list(obs[-9:])[5],
    #                           'alpha':list(obs[-9:])[8],
    #                           'time': env.currentRecord[0],
    #                           'cwnd': env.currentRecord[1],
    #                           'paceTime': env.currentRecord[2],
    #                           'bwMean':  env.currentRecord[3],
    #                           'bwStd':  env.currentRecord[4],
    #                           'rttMean':  env.currentRecord[5],
    #                           'rttStd':  env.currentRecord[6],
    #                           'rttminEst':  env.currentRecord[7],
    #                           'bwMax': env.currentRecord[8]},index=[0])])

    step_counter = 1
    while not done:
        trans = agent.compute_single_action(obs, explore=explore)
        new_obs, rewards, dones, _ = env.step(trans)
        rollout = pd.concat([rollout, pd.DataFrame({
                              'bwNorm': list(obs[-len(env.features_min):])[0], 
                              'trimPortion': list(obs[-len(env.features_min):])[1], 
                              'rttNorm': round(list(obs[-len(env.features_min):])[2]),
                              'cwndNorm': list(obs[-len(env.features_min):])[3],
                              'action': trans,
                              'reward': rewards,
                              'time': env.currentRecord[0],
                              'cwnd': env.currentRecord[1],
                              'paceTime': env.currentRecord[2],
                              'bwMean':  env.currentRecord[3],
                              'bwStd':  env.currentRecord[4],
                              'rttMean':  env.currentRecord[5],
                              'rttStd':  env.currentRecord[6],
                              'rttminEst':  env.currentRecord[7],
                              'bwMax': env.currentRecord[8],
                              'lossrate1': env.currentRecord[9],
                              'lossrate2': env.currentRecord[10]}, index=[0])])

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

    ray.init(address='auto')

    
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

    rollout.to_csv(os.getenv('HOME') +f"/stacking_results/rollout_obsb_stack5_SAC05_{check_no}_15_workers_{buffer}pkts_{bw}mbps_{rtt}ms_{'stoc' if explore else 'det'}.csv")

    os.remove(os.getenv('HOME') + f"/RLlibIntegration/configs/ndpconfig_single_flow_eval_with_delay_{os.getpid()}.ini")