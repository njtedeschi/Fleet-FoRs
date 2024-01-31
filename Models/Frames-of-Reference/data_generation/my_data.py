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
    # p_canonical_s: float
    # p_canonical_g: float
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
            "description_is_reliable": hyper.data_reliability,
            # Scene probabilities
            "scene_is_direct": hyper.p_direct,
            "figure_is_near": hyper.p_near,
            "figure_on_axis": hyper.p_axis,
            "ground_is_biped": hyper.p_biped,
            "speaker_is_canonical": hyper.p_canonical_s,
            "ground_is_canonical": hyper.p_canonical_g,
            # Word probabilities
            "description_uses_frame": hyper.p_frame,
            "description_is_intrinsic": hyper.p_intrinsic
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

        # Speaker-Ground relationship
        scene_is_direct = self.flip("scene_is_direct")
        if scene_is_direct:
            return self.sample_direct_scene(figure,
                                            speaker_is_canonical)
        else:
            return self.sample_nondirect_scene(figure,
                                               speaker_is_canonical) 

    ## Direct Scene
    def sample_direct_scene(self, figure, speaker_is_canonical=True):
        if speaker_is_canonical:
            speaker = const.DIRECT_SPEAKER
        else:
            speaker = self.sample_noncanonical_speaker(const.ORIGIN)
        scene = Scene(
            speaker=speaker,
            ground=speaker,
            figure=figure
        )
        return scene


    ## Nondirect Scene
    def sample_nondirect_scene(self, figure, speaker_is_canonical=True):
        if speaker_is_canonical:
            speaker = const.NONDIRECT_SPEAKER
        else:
            speaker = self.sample_noncanonical_speaker(const.NONDIRECT_SPEAKER_POSITION)
        ground = self.sample_ground()
        scene = Scene(
            speaker=speaker,
            ground=ground,
            figure=figure
        )
        return scene

    ### Speaker

    def sample_noncanonical_speaker(speaker, position):
        pass

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
        direction = self.rng.choice(const.CARDINAL_DIRECTIONS_4)
        ground = OrientedObject(
            position=const.ORIGIN,
            forward=direction,
            upward=const.UP,
            is_participant=False,
            body_type=body_type
        )
        return ground

    def sample_noncanonical_ground(self, body_type):
        pass

    ### Figure
    def sample_figure(self):
        on_axis = self.flip("figure_on_axis")
        if on_axis:
            direction = self.rng.choice(const.CARDINAL_DIRECTIONS_6)
        else:
            direction = self.rng.choice(const.OFF_AXIS_DIRECTIONS)
        distance = const.NEAR if self.flip("figure_is_near") else const.FAR
        position = distance * direction
        figure = BaseObject(position)
        return figure


    # Word sampling
    def sample_word_by_probs(self, scene):
        uses_frame = self.flip("description_uses_frame")

        word = None
        if (uses_frame):
            is_intrinsic = self.flip("description_is_intrinsic")
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
            description_is_reliable = self.flip("description_is_reliable")
            if description_is_reliable:
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