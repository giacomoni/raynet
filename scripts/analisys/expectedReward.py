from torch import true_divide
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
import nnmodels
from ray.rllib.policy.sample_batch import SampleBatch
import matplotlib.pyplot as plt



from ray.tune.registry import register_env
import argparse
import copy
import math
from collections import deque
from ray.rllib.utils.framework import get_variable, try_import_tf
# plt.style.use('science')


tf1, tf, tfv = try_import_tf()

class SimulationRunnerEnv(gym.Env):
    def __init__(self, env_config):
        self.env_config = env_config
        self.stacking = env_config['stacking']
        self.action_space = spaces.Box(low=np.array([-2.0], dtype=np.float32), high=np.array([2.0], dtype=np.float32), dtype=np.float32)
        self.obs_min = np.tile(np.array([-1000,  
                                 0,   
                                 0,   
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 -1000], dtype=np.float32), self.stacking)

        self.obs_max = np.tile(np.array([1000, 
                                 100, 
                                 10, 
                                 100, 
                                 1,
                                 1,
                                 100,
                                 100,
                                 1000],dtype=np.float32), self.stacking)
        self.currentRecord = None
        self.observation_space = spaces.Box(low=self.obs_min, high=self.obs_max, dtype=np.float32)
        self.runner = SimulationRunner()
        self.obs = deque(np.zeros(len(self.obs_min)),maxlen=len(self.obs_min))
   
    def reset(self):
       
        self.obs = deque(np.zeros(len(self.obs_min)),maxlen=len(self.obs_min))
        self.runner.initialise(self.env_config["iniPath"])
        obs = self.runner.reset()
        obs = obs['5']
        self.currentRecord = obs[-9:]
        self.obs.extend(obs[:-9])
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
        self.currentRecord = obs[-9:]
        self.obs.extend(obs[:-9])
        obs = np.asarray(list(self.obs),dtype=np.float32)
        return  obs, reward, dones['5'], {}


def simulationrunnerenv_creator(env_config):
    return SimulationRunnerEnv(env_config)  # return an env instance

register_env("OmnetppEnv", simulationrunnerenv_creator)

if __name__ == "__main__":


    BUFFERS = [80]

    COLOR_MAP = {8:'orange',
                80:'red',
                800: 'blue',
                #  128: 'green',
                #  60: 'purple',
                #  128: 'brown',
                #  256: 'grey'
    }

    CHECKPOINT = 11800

    config = {
            "env": "OmnetppEnv",
            "env_config": {"iniPath": os.getenv('HOME') + "/RLlibIntegration/src/ndpconfig_single_flow_eval_with_delay.ini",
                        "linkrate_range": [1,1000],
                        "rtt_range": [1, 500],
                        "buffer_range": [2, 64000],
                        "stacking": 1},
            "evaluation_config": {
                                    "env_config": {"iniPath": os.getenv('HOME') + "/RLlibIntegration/src/ndpconfig_single_flow_eval_with_delay.ini"},
                                    "explore": False,
                                    "stacking": 1
            },
        "num_workers": 0,
        "horizon": 400,
        "no_done_at_end":True,
        "soft_horizon":False,
        "gamma": 0.5  }

    ray.init()

    cls = get_trainable_cls("SAC")
    env = simulationrunnerenv_creator(config['env_config'])
    agent = cls(env="OmnetppEnv", config=config)

    checkpoint_path = f"/home/luca/partial_results/checkpoint_{CHECKPOINT:06}"
    checkpoint_file = f"/checkpoint-{CHECKPOINT}" 
    agent.restore(checkpoint_path + checkpoint_file)


    policy = agent.get_policy()
    model = policy.model
    session = policy.get_session()

    # fig, axs = plt.subplots(1, 1, figsize=(10,5))
    fig = plt.figure()
    axs = fig.add_subplot(projection='3d')


    with session.graph.as_default():
        for buffer in BUFFERS:
            # ---- BDP
                mss = 1500
                prop_delay = 0.008
                linkrate = 80*1000000
                q_size = buffer
                rtt = 6*prop_delay+3*(8*mss)/linkrate + 3*(8*20)/linkrate
                BDP = (linkrate*(rtt)/8)/mss
                index_row = 0
                index_col = 0
                filename = f'rollout_{CHECKPOINT}_15_workers_{buffer}pkts_80mbps_8ms_stoc'

                data = pd.read_csv(f'{filename}.csv', index_col=[0])
                
                # action_dist_class = model._get_dist_class(policy, policy.config, policy.action_space)
                # action_dist_inputs_t, _ = model.get_action_model_outputs(model_out_t)
                # action_dist_t = action_dist_class(action_dist_inputs_t, policy.model)
                # policy_t = (
                #     action_dist_t.sample()
                # )
                # log_pis_t = tf.expand_dims(action_dist_t.logp(policy_t), -1)
                
                model_out_t, _ = model(
                SampleBatch(obs=data[['rttNorm', 'bwNorm', 'trimPortion', 'cwndNorm', 'badSteps', 'bdpNorm', 'rttminEst', 'bwMax' ,'alpha']], _is_training=False),
                [],
                None,
                )
                # Q-values for the actually selected actions.
                q_t, _ = model.get_q_values(
                    model_out_t, tf.cast(data[['action']], tf.float32)
                )

                twin_q_t, _ = model.get_twin_q_values(
                        model_out_t, tf.cast(data[['action']], tf.float32)
                    )

                # # Q-values for current policy in given current state.
                # q_t_det_policy, _ = model.get_q_values(model_out_t, policy_t)
                # twin_q_t_det_policy, _ = model.get_twin_q_values(model_out_t, policy_t)
                # q_t_det_policy = tf.reduce_min(
                #         (q_t_det_policy, twin_q_t_det_policy), axis=0
                #     )

                q_t_numpy = q_t.eval(session=session)
                twin_q_t_numpy = twin_q_t.eval(session=session)
                data = pd.concat([data.reset_index(drop=True), pd.DataFrame(q_t_numpy, columns=['q_t'])], axis=1)
                data = pd.concat([data.reset_index(drop=True), pd.DataFrame(twin_q_t_numpy, columns=['twin_q_t'])], axis=1)
                # data.to_csv(f'{filename}_q.csv')

                  # ------ Rtt Std Step
                axs.scatter(data['rttNorm'], data['bwNorm'], data['reward'], label = f'Reward {buffer} pkts', alpha=0.75, color='orange')
                axs.scatter(data['rttNorm'], data['bwNorm'], data['q_t'], label = f'Q {buffer} pkts', alpha=0.75, color='red', marker='s')
                axs.scatter(data['rttNorm'], data['bwNorm'], data['twin_q_t'], label = f'Twin Q {buffer} pkts', alpha=0.75, color='blue', marker='^')

                axs.set(xlabel='rttNorm', ylabel='bwNorm', zlabel='Value')
                axs.legend(prop={'size': 7})
                # axs.axvline(0.96*BDP, label = f'BDP-{buffer}', linestyle="dashed", color=COLOR_MAP[buffer])
                # axs.axvline(BDP, label = f'BDP-{buffer}', linestyle="dashed", color=COLOR_MAP[buffer])
                # axs.axvline(BDP+q_size, label = f'BDP+Buffer{buffer}', linestyle="dashed", color=COLOR_MAP[buffer])
                # axs.axhline(2, label = f'Max Reward', linestyle="dashed", color=COLOR_MAP[buffer])
                # axs.axhline(-1, label = f'Min Reward', linestyle="dashed", color=COLOR_MAP[buffer])
                # axs.axhline(1, label = f'Max thr reward', linestyle="dashed", color=COLOR_MAP[buffer])

        
    plt.savefig(f'q_{buffer}pkts_80mbps_8ms_stoc_3d.png', dpi=300)


            

