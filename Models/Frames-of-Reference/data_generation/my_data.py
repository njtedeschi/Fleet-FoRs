from dataclasses import dataclass
from typing import List, Dict
from contextlib import contextmanager

import numpy as np

from scene import BaseObject, OrientedObject, Scene
import constants as const
import world

# Specific combination of hyperparameters
# YAML config must match attribute names
@dataclass
class ExperimentalCondition:
    # Language
    language: str
    # Probabilities
    p_datum_is_reliable: float
    ## Scene probabilities
    p_scene_is_direct: float
    p_figure_is_on_axis: float
    p_figure_is_near: float
    p_speaker_is_canonical: float
    p_speaker_is_upside_down: float # given noncanonical speaker
    p_ground_is_biped: float # given nondirect scene
    p_ground_is_canonical: float # given nondirect scene
    p_ground_is_upside_down: float # given noncanonical ground
    ## Description probabilities
    p_description_is_angular: float
    p_description_is_intrinsic: float # given angular description
    # Name
    name: List[str]

# Datum
@dataclass
class MyInput:
    scene: Scene
    description: str
    flip_results: Dict[str, bool] # Set to None if DataGenerator.verbose=False

class DataGenerator:

    def __init__(self, experimental_condition, seed=None, verbose=False):
        self.rng = np.random.default_rng(seed)
        # Language
        self.language = const.LANGUAGES[experimental_condition.language]
        self.probs = self.set_probs(experimental_condition)
        self.verbose = verbose # Used for logging flip values

    def set_probs(self, experimental_condition):
        probs_dict = {}
        for attr in dir(experimental_condition):
            value = getattr(experimental_condition, attr)
            if attr.startswith("p_") and isinstance(value, float):
                # Remove the "p_" prefix and use the rest as a dictionary key
                key = attr[2:]
                probs_dict[key] = value
        return probs_dict

    # Keep track of flip results if self.verbose
    @contextmanager
    def flip_results_manager(self):
        if self.verbose:
            self.current_flip_results = {} # flip results for current datum being generated
            yield self.current_flip_results # pass to with block
            self.current_flip_results = None # clean up after after with block completed
        else:
            yield None

    def flip(self, p_label):
        result = self.rng.random() < self.probs[p_label]
        # Logging flip results
        if self.verbose:
            self.current_flip_results[p_label] = result
        return result

    # Scene sampling
    def sample_scene(self):
        # Sample values that are same for all scene types
        figure = self.sample_figure()
        speaker_is_canonical = self.flip("speaker_is_canonical")

        scene_is_direct = self.flip("scene_is_direct")
        if scene_is_direct:
            scene = self.sample_direct_scene(figure, speaker_is_canonical)
        else:
            scene = self.sample_nondirect_scene(figure, speaker_is_canonical)
        return scene

    ## Direct Scene
    def sample_direct_scene(self, figure, speaker_is_canonical=True):
        if speaker_is_canonical:
            speaker = const.CANONICAL_DIRECT_SPEAKER
        else:
            # Speaker is direct if specified_position=None
            speaker = self.sample_noncanonical_speaker()
        scene = Scene.direct_scene(speaker, figure)
        return scene

    ## Nondirect Scene
    def sample_nondirect_scene(self, figure, speaker_is_canonical=True):
        if speaker_is_canonical:
            speaker = const.CANONICAL_NONDIRECT_SPEAKER
        else:
            speaker = self.sample_noncanonical_speaker(specified_position=const.NONDIRECT_SPEAKER_POSITION)
        ground = self.sample_ground()
        scene = Scene(speaker, ground, figure)
        return scene

    ### Speaker
    # No sampling for canonical speakers due to EAST facing restriction

    # Possibilities directions restricted in line with canonical direct and nondirect speakers
    # Either forward or upward must face EAST
    def sample_noncanonical_speaker(self, specified_position=None):
        
        upside_down = self.flip("speaker_is_upside_down")
        if upside_down:
            speaker = OrientedObject.speaker(const.CANONICAL_SPEAKER_FORWARD, world.DOWN, specified_position)
        else:
            speaker = self.sample_lying_speaker(specified_position)
        return speaker

    # Lying down, not fibbing
    def sample_lying_speaker(self, specified_position):
        possibile_orientations = [
            # (forward, upward)
            (const.CANONICAL_SPEAKER_FORWARD, const.CANONICAL_SPEAKER_LEFTWARD), # lying on left
            (const.CANONICAL_SPEAKER_FORWARD, const.CANONICAL_SPEAKER_RIGHTWARD), # lying on right
            (world.UP, const.CANONICAL_SPEAKER_FORWARD), # lying face up
            (world.DOWN, const.CANONICAL_SPEAKER_FORWARD) # lying face down
        ]
        index = self.rng.choice(len(possibile_orientations))
        forward, upward = possibile_orientations[index]
        speaker = OrientedObject.speaker(forward, upward, specified_position)
        return speaker

    ### Ground
    def sample_ground(self):
        body_type = "biped" if self.flip("ground_is_biped") else "quadruped"
        is_canonical = self.flip("ground_is_canonical")
        if is_canonical:
            ground = self.sample_canonical_ground(body_type)
        else:
            ground = self.sample_noncanonical_ground(body_type)
        return ground

    def sample_canonical_ground(self, body_type):
        forward = self.rng.choice(const.CARDINAL_DIRECTIONS_4)
        ground = OrientedObject.ground(forward, const.UP, body_type)
        return ground

    def sample_noncanonical_ground(self, body_type):
        upside_down = self.flip("ground_is_upside_down")
        if upside_down:
            ground = self.sample_upside_down_ground(body_type)
        else:
            ground = self.sample_lying_ground(body_type)
        return ground

    def sample_upside_down_ground(self, body_type):
        forward = self.rng.choice(const.CARDINAL_DIRECTIONS_4)
        ground = OrientedObject.ground(forward, const.DOWN, body_type)
        return ground

    # TODO: Check combinatorics
    # ground lying on its face, back, or side
    def sample_lying_ground(self, body_type):
        upward = self.rng.choice(const.CARDINAL_DIRECTIONS_4)
        forward_possibilities = self._forward_possibilities(upward)
        forward = self.rng.choice(forward_possibilities)
        ground = OrientedObject.ground(forward, upward, body_type)
        return ground

    # Assuming upward is along horizontal axis
    def _forward_possibilities(self, upward):
        # forward can always be in the z-direction
        # (i.e. object lying face up or face down)
        vertical = [const.UP, const.DOWN]
        # forward can also be along the horizontal axis orthogonal to upward
        if np.dot(upward, const.NORTH) == 0:
            horizontal = [const.NORTH, const.SOUTH]
        else:
            horizontal = [const.EAST, const.WEST]
        return vertical + horizontal

    ### Figure
    def sample_figure(self):
        is_on_axis = self.flip("figure_is_on_axis")
        if is_on_axis:
            direction = self.rng.choice(const.CARDINAL_DIRECTIONS_6)
        else:
            direction = self.rng.choice(const.OFF_AXIS_DIRECTIONS)
        distance = const.NEAR if self.flip("figure_is_near") else const.FAR
        position = distance * direction
        figure = BaseObject(position)
        return figure

    # Description sampling
    def sample_true_description(self, scene):
        is_angular = self.flip("description_is_angular")

        description = None
        if (is_angular):
            is_intrinsic = self.flip("description_is_intrinsic")
            if is_intrinsic or scene.ground.is_participant:
                description = self.sample_angular_description(scene, relative=False)
            else:
                description = self.sample_angular_description(scene, relative=True)
        # If uses_frame is false or sample_angular_description returns None
        if not description:
            description = self.language.proximity_description(scene)
        return description

    def sample_angular_description(self, scene, relative=False):
        angular_descriptions = self.language.angular_descriptions(scene, relative=relative)
        if angular_descriptions:
            return self.rng.choice(angular_descriptions)
        else:
            return None

    def sample_word_randomly(self):
        word = self.rng.choice(self.language.vocabulary)
        return word

    # Data sampling
    def sample_datum(self):
        with self.flip_results_manager() as flip_results:
            scene = self.sample_scene()
            datum_is_reliable = self.flip("datum_is_reliable")
            if datum_is_reliable:
                description = self.sample_true_description(scene)
            else:
                description = self.sample_word_randomly()
            datum = MyInput(scene, description, flip_results)
            return datum

    def sample_data(self, num_samples):
        data = []
        for _ in range(num_samples):
            datum = self.sample_datum()
            data.append(datum)
        return data