import functools
from typing import Callable, Dict, List, Optional, Tuple, Type, Union

import tree
from ray.rllib import SampleBatch, Policy
from ray.rllib.models.torch.torch_action_dist import TorchDistributionWrapper
from ray.rllib.utils import override
from ray.rllib.utils.torch_ops import convert_to_torch_tensor, convert_to_non_torch_type
from ray.rllib.utils.typing import TensorType

import numpy as np
import torch

from griddly.util.rllib.torch.conditional_masking_exploration import TorchConditionalMaskingExploration
from griddly.util.rllib.torch.torch_gridnet_masked_categorical_distribution import GridnetMaskedCategoricalDistribution


class InvalidActionMaskingPolicyMixin:
    """
    The info_batch contains the valid action trees. compute_actions is the only part of rllib that can has access to the
    info_batch, therefore we have to override it to explore/exploit valid actions.
    """

    @override(Policy)
    def compute_actions(
            self,
            obs_batch: Union[List[TensorType], TensorType],
            state_batches: Optional[List[TensorType]] = None,
            prev_action_batch: Union[List[TensorType], TensorType] = None,
            prev_reward_batch: Union[List[TensorType], TensorType] = None,
            info_batch: Optional[Dict[str, list]] = None,
            episodes: Optional[List["MultiAgentEpisode"]] = None,
            explore: Optional[bool] = None,
            timestep: Optional[int] = None,
            **kwargs) -> \
            Tuple[TensorType, List[TensorType], Dict[str, TensorType]]:

        if not self.config['env_config'].get('invalid_action_masking', False):
            raise RuntimeError('invalid_action_masking must be set to True in env_config to use this mixin')

        explore = explore if explore is not None else self.config["explore"]
        timestep = timestep if timestep is not None else self.global_timestep

        with torch.no_grad():
            seq_lens = torch.ones(len(obs_batch), dtype=torch.int32)
            input_dict = self._lazy_tensor_dict({
                SampleBatch.CUR_OBS: np.asarray(obs_batch),
                "is_training": False,
            })
            if prev_action_batch is not None:
                input_dict[SampleBatch.PREV_ACTIONS] = \
                    np.asarray(prev_action_batch)
            if prev_reward_batch is not None:
                input_dict[SampleBatch.PREV_REWARDS] = \
                    np.asarray(prev_reward_batch)
            state_batches = [
                convert_to_torch_tensor(s, self.device)
                for s in (state_batches or [])
            ]

            # Call the exploration before_compute_actions hook.
            self.exploration.before_compute_actions(
                explore=explore, timestep=timestep)

            dist_inputs, state_out = self.model(input_dict, state_batches,
                                                seq_lens)
            # Extract the tree from the info batch
            valid_action_trees = []
            for info in info_batch:
                if 'valid_action_tree' in info:
                    valid_action_trees.append(info['valid_action_tree'])
                else:
                    valid_action_trees.append({0: {0: {0: [0]}}})

            if hasattr(self.model, 'grid_channels'):

                self.dist_class = functools.partial(GridnetMaskedCategoricalDistribution, valid_action_trees=valid_action_trees)
                action_dist = self.dist_class(dist_inputs, self.model)

                # Get the exploration action from the forward results.
                actions, logp = \
                    self.exploration.get_exploration_action(
                        action_distribution=action_dist,
                        timestep=timestep,
                        explore=explore)

                masked_logits = action_dist.sampled_masked_logits()
                mask = action_dist.sampled_action_masks()

                # exploration = TorchConditionalMaskingGridnetExploration(
                #     self.model,
                #     self.dist_class,
                #     dist_inputs,
                #     valid_action_trees,
                # )
            else:
                exploration = TorchConditionalMaskingExploration(
                    self.model,
                    self.dist_class,
                    dist_inputs,
                    valid_action_trees,
                )

                actions, masked_logits, logp, mask = exploration.get_actions_and_mask()

            input_dict[SampleBatch.ACTIONS] = actions

            extra_fetches = {
                'valid_action_mask': mask,
                'valid_action_trees': valid_action_trees,
            }

            # Action-dist inputs.
            if dist_inputs is not None:
                extra_fetches[SampleBatch.ACTION_DIST_INPUTS] = masked_logits

            # Action-logp and action-prob.
            if logp is not None:
                extra_fetches[SampleBatch.ACTION_PROB] = \
                    torch.exp(logp.float())
                extra_fetches[SampleBatch.ACTION_LOGP] = logp

            # Update our global timestep by the batch size.
            self.global_timestep += len(input_dict[SampleBatch.CUR_OBS])

            return convert_to_non_torch_type((actions, state_out, extra_fetches))
