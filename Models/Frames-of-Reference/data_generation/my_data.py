from dataclasses import dataclass
from typing import List, Dict
from contextlib import contextmanager

import numpy as np

from scene import BaseObject, OrientedObject, Scene
import constants as const
import language

# Specific combination of hyperparameters
@dataclass
class ExperimentalCondition:
    # Language
    language: str
    # Probabilities
    data_reliability: float
    ## Scene probabilities
    p_direct: float
    p_biped: float
    p_near: float
    p_axis: float
    ## Word probabilities
    p_frame: float
    p_intrinsic: float
    # Name
    name: List[str]

# Datum
@dataclass
class MyInput:
    scene: Scene
    description: str
    flip_results: Dict[str, bool]

class DataGenerator:

    def __init__(self, experimental_condition, seed=None, verbose=False):
        self.rng = np.random.default_rng(seed)
        # Language
        self.language = const.LANGUAGES[experimental_condition.language]
        self.probs = self.set_probabilities(experimental_condition)
        self.verbose = verbose # Used for logging flip values

    def set_probabilities(self, hyper):
        probs = {
            "is_reliable": hyper.data_reliability,
            # Scene probabilities
            "is_direct": hyper.p_direct,
            "is_near": hyper.p_near,
            "on_axis": hyper.p_axis,
            "is_biped": hyper.p_biped,
            # Word probabilities
            "uses_frame": hyper.p_frame,
            "is_intrinsic": hyper.p_intrinsic
        }
        return probs

    # Keep track of flip results if self.verbose
    @contextmanager
    def flip_results_manager(self):
        if self.verbose:
            self.current_flip_results = {} # flip results for current datum being generated
            yield self.current_flip_results # pass to with block
            self.current_flip_results = None # clean up after after with block completed
        else:
            yield None

    def flip(self, p_label, flip_results=None):
        result = self.rng.random() < self.probs[p_label]
        # Logging flip results
        if self.verbose:
            self.current_flip_results[p_label] = result
        return result

    # Scene sampling
    def sample_scene(self):
        # Figure
        figure = self.sample_figure()

        # Speaker-Ground relationship
        is_direct = self.flip("is_direct")
        if is_direct:
            direct_speaker = const.DIRECT_SPEAKER
            direct_scene = Scene(
                speaker=direct_speaker,
                ground=direct_speaker,
                figure=figure
            )
            return direct_scene
        else:
            nondirect_speaker = const.NONDIRECT_SPEAKER
            ground = self.sample_ground()
            nondirect_scene = Scene(
                speaker=nondirect_speaker,
                ground=ground,
                figure=figure
            )
            return nondirect_scene

    def sample_figure(self):
        on_axis = self.flip("on_axis")
        if on_axis:
            direction = self.rng.choice(const.CARDINAL_DIRECTIONS_6)
        else:
            direction = self.rng.choice(const.OFF_AXIS_DIRECTIONS)
        distance = const.NEAR if self.flip("is_near") else const.FAR
        position = distance * direction
        figure = BaseObject(position)
        return figure

    def sample_ground(self):
        direction = self.rng.choice(const.CARDINAL_DIRECTIONS_4)
        body_type = "biped" if self.flip("is_biped") else "quadruped"
        ground = OrientedObject(
            position=const.ORIGIN,
            forward=direction,
            upward=const.UP,
            is_participant=False,
            body_type=body_type
        )
        return ground

    # Word sampling
    def sample_word_by_probs(self, scene):
        uses_frame = self.flip("uses_frame")
        is_intrinsic = self.flip("is_intrinsic")

        word = None
        if (uses_frame):
            if is_intrinsic or scene.ground.is_participant:
                word = self.sample_angular_description(scene, relative=False)
            else:
                word = self.sample_angular_description(scene, relative=True)
        # If uses_frame is false or sample_angular_description returns None
        if not word:
            word = self.language.proximity_description(scene)
        return word

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
            is_reliable = self.flip("is_reliable")
            if is_reliable:
                description = self.sample_word_by_probs(scene)
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