import os
import sys

import ray
from ray import tune
from ray.rllib.models import ModelCatalog
from ray.tune.registry import register_env

from griddly import gd
from griddly.util.rllib.torch import GAPAgent
from griddly.util.rllib.torch.conditional_actions.conditional_action_policy_trainer import ConditionalActionImpalaTrainer
from griddly.util.rllib.env.core import RLlibEnv

if __name__ == '__main__':
    sep = os.pathsep
    os.environ['PYTHONPATH'] = sep.join(sys.path)

    ray.init(num_gpus=1)

    env_name = "ray-griddly-env"

    register_env(env_name, RLlibEnv)
    ModelCatalog.register_custom_model("GAP", GAPAgent)

    max_training_steps = 5000000

    config = {
        'framework': 'torch',
        'num_workers': 6,
        'num_envs_per_worker': 2,

        'model': {
            'custom_model': 'GAP',
            'custom_model_config': {}
        },
        'env': env_name,
        'env_config': {
            'record_video_config': {
                'frequency': 100000
            },

            'invalid_action_masking': tune.grid_search([True, False]),
            'generate_valid_action_trees': tune.grid_search([True, False]),
            'random_level_on_reset': True,
            'yaml_file': 'Single-Player/GVGAI/clusters_partially_observable.yaml',
            'global_observer_type': gd.ObserverType.SPRITE_2D,
            'max_steps': 1000,
        },
        'entropy_coeff_schedule': [
            [0, 0.01],
            [max_training_steps, 0.0]
        ],
        'lr_schedule': [
            [0, 0.005],
            [max_training_steps, 0.0]
        ],


    }

    stop = {
        "timesteps_total": max_training_steps,
    }

    result = tune.run(ConditionalActionImpalaTrainer, config=config, stop=stop)
