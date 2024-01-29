from dataclasses import dataclass
from typing import List

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

class DataGenerator:

    def __init__(self, experimental_condition, seed=None):
        self.rng = np.random.default_rng(seed)
        # Language
        self.language = const.LANGUAGES[experimental_condition.language]
        # Probabilities
        self.data_reliability = experimental_condition.data_reliability
        ## Scene probabilities
        self.p_direct = experimental_condition.p_direct
        self.p_near = experimental_condition.p_near
        self.p_axis = experimental_condition.p_axis
        self.p_biped = experimental_condition.p_biped
        ## Word probabilities
        self.p_frame = experimental_condition.p_frame
        self.p_intrinsic = experimental_condition.p_intrinsic

    def flip(self, probability):
        if self.rng.random() < probability:
            return True
        else:
            return False

    # Scene sampling
    def sample_scene(self):
        # Figure
        figure = self.sample_figure()

        # Speaker-Ground relationship
        is_direct = self.flip(self.p_direct)
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
        on_axis = self.flip(self.p_axis)
        if on_axis:
            direction = self.rng.choice(const.CARDINAL_DIRECTIONS_6)
        else:
            direction = self.rng.choice(const.OFF_AXIS_DIRECTIONS)
        distance = const.NEAR if self.flip(self.p_near) else const.FAR
        position = distance * direction
        figure = BaseObject(position)
        return figure

    def sample_ground(self):
        direction = self.rng.choice(const.CARDINAL_DIRECTIONS_4)
        body_type = "biped" if self.flip(self.p_biped) else "quadruped"
        ground = OrientedObject(
            position=const.ORIGIN,
            forward=direction,
            upward=const.UP,
            is_participant=False,
            body_type=body_type
        )
        return ground

    # Word sampling
    def sample_description(self, scene):
        is_correct = self.flip(self.data_reliability)
        if is_correct:
            word = self.sample_word_by_probs(scene)
        else:
            word = self.sample_word_randomly()
        return word

    def sample_word_by_probs(self, scene):
        uses_frame = self.flip(self.p_frame)
        is_intrinsic = self.flip(self.p_intrinsic)

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
        scene = self.sample_scene()
        description = self.sample_description(scene)
        datum = MyInput(scene, description)
        return datum

    def sample_data(self, num_samples):
        data = []
        for _ in range(num_samples):
            datum = self.sample_datum()
            data.append(datum)
        return data