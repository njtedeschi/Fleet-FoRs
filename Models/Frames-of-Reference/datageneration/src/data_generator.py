from contextlib import contextmanager

import numpy as np

from .structures.scene import BaseObject, OrientedObject, Scene
from .structures.experiment import Datum, TrainingDatum, TestingDatum
from .constants.space import (UP, DOWN, NORTH, SOUTH, EAST, WEST,
                              CARDINAL_DIRECTIONS_4,
                              CARDINAL_DIRECTIONS_6)
from .constants.objects import (NEAR, FAR,
                                OFF_AXIS_DIRECTIONS,
                                CANONICAL_DIRECT_SPEAKER,
                                CANONICAL_NONDIRECT_SPEAKER)
from .constants.languages import LANGUAGES

class Sampler:

    def __init__(self, experimental_condition, seed=None, verbose=False):
        self.rng = np.random.default_rng(seed)
        self.probs = self.set_probs(experimental_condition)
        # Whether to log flip values
        self.verbose = verbose

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
            # flip results for current datum being generated
            self.current_flip_results = {}
            # pass to with block
            yield self.current_flip_results
            # clean up after after with block completed
            self.current_flip_results = None
        else:
            yield None

    def flip(self, p_label, adjustment_label=None):
        if(adjustment_label):
            p_adjustment = self.probs[adjustment_label]
        else:
            p_adjustment = 0
        result = self.rng.random() < (self.probs[p_label] + p_adjustment)
        # Logging flip results
        if self.verbose:
            self.current_flip_results[p_label] = result
        return result

    def choice(self, options):
        return self.rng.choice(options)


class SceneGenerator:

    def __init__(self, sampler):
        self.sampler = sampler

    def build_scene(self):
        # Sample values that are same for all scene types
        figure = self.build_figure()
        speaker_is_canonical = self.sampler.flip("speaker_is_canonical")

        if self.sampler.flip("scene_is_direct"):
            scene = self.build_direct_scene(figure, speaker_is_canonical)
        else:
            scene = self.build_nondirect_scene(figure, speaker_is_canonical)
        return scene

    ## Direct Scene
    def build_direct_scene(self, figure, speaker_is_canonical=True):
        if speaker_is_canonical:
            speaker = CANONICAL_DIRECT_SPEAKER
        else:
            # Speaker is direct when specified_position=None
            speaker = self.build_noncanonical_speaker()
        scene = Scene.direct_scene(speaker, figure)
        return scene

    ## Nondirect Scene
    def build_nondirect_scene(self, figure, speaker_is_canonical=True):
        if speaker_is_canonical:
            speaker = CANONICAL_NONDIRECT_SPEAKER
        else:
            speaker = self.build_noncanonical_speaker(
                CANONICAL_NONDIRECT_SPEAKER.position)
        ground = self.build_ground()
        scene = Scene(speaker, ground, figure)
        return scene

    ### Speaker
    # No sampling for canonical speakers due to EAST facing restriction

    # Possibilitie directions restricted in line with canonical speakers
    # Either forward or upward must face EAST
    def build_noncanonical_speaker(self, specified_position=None):

        if self.sampler.flip("speaker_is_upside_down"):
            # The speaker faces the same direction as the canonical one
            # but upward=DOWN instead of upward=UP
            speaker = OrientedObject.speaker(
                CANONICAL_DIRECT_SPEAKER.forward, DOWN, specified_position)
        else:
            speaker = self.build_lying_speaker(specified_position)
        return speaker

    # Lying down, not fibbing
    def build_lying_speaker(self, specified_position):
        cds = CANONICAL_DIRECT_SPEAKER
        # Orientations are restricted to those where either forward or upward
        # matches the direction faced by the canonical speaker
        possibile_orientations = [
            # (forward, upward)
            (cds.forward, cds.rightward), # lying on right
            (cds.forward, -cds.rightward), # lying on left
            (UP, cds.forward), # lying face up
            (DOWN, cds.forward) # lying face down
        ]
        index = self.sampler.choice(len(possibile_orientations))
        forward, upward = possibile_orientations[index]
        speaker = OrientedObject.speaker(forward, upward, specified_position)
        return speaker

    ### Ground
    def build_ground(self):
        body_type = "biped" if self.sampler.flip("ground_is_biped") else "quadruped"
        is_canonical = self.sampler.flip("ground_is_canonical")
        if is_canonical:
            ground = self.build_canonical_ground(body_type)
        else:
            ground = self.build_noncanonical_ground(body_type)
        return ground

    def build_canonical_ground(self, body_type):
        forward = self.sampler.choice(CARDINAL_DIRECTIONS_4)
        ground = OrientedObject.ground(forward, UP, body_type)
        return ground

    def build_noncanonical_ground(self, body_type):
        if self.sampler.flip("ground_is_upside_down"):
            ground = self.build_upside_down_ground(body_type)
        else:
            ground = self.build_lying_ground(body_type)
        return ground

    def build_upside_down_ground(self, body_type):
        forward = self.sampler.choice(CARDINAL_DIRECTIONS_4)
        ground = OrientedObject.ground(forward, DOWN, body_type)
        return ground

    # TODO: Check combinatorics
    # ground lying on its face, back, or side
    def build_lying_ground(self, body_type):
        upward = self.sampler.choice(CARDINAL_DIRECTIONS_4)
        forward_possibilities = self._forward_possibilities(upward)
        forward = self.sampler.choice(forward_possibilities)
        ground = OrientedObject.ground(forward, upward, body_type)
        return ground

    # Assuming upward is along horizontal axis
    def _forward_possibilities(self, upward):
        # forward can always be in the z-direction
        # (i.e. object lying face up or face down)
        vertical = [UP, DOWN]
        # forward can also be along the horizontal axis orthogonal to upward
        if np.dot(upward, NORTH) == 0:
            horizontal = [NORTH, SOUTH]
        else:
            horizontal = [EAST, WEST]
        return vertical + horizontal

    ### Figure
    def build_figure(self):
        if self.sampler.flip("figure_is_on_axis"):
            direction = self.sampler.choice(CARDINAL_DIRECTIONS_6)
        else:
            direction = self.sampler.choice(OFF_AXIS_DIRECTIONS)
        distance = NEAR if self.sampler.flip("figure_is_near") else FAR
        position = distance * direction
        figure = BaseObject(position)
        return figure


class DescriptionGenerator:

    def __init__(self, sampler, language):
        self.sampler = sampler
        self.language = language

    def sample_word(self, words):
        words = list(words)
        return self.sampler.choice(words)

    def true_description(self, scene):
        description = None
        if scene.figure_is_on_axis() and self.sampler.flip("description_is_angular"):
            description = self.angular_description(scene)
        # Default to proximity description if no angular description applies
        if not description:
            description = self.language.proximity_description(scene)
        return description

    def angular_description(self, scene):
        if scene.is_canonical():
            return self._angular_description_canonical(scene)
        else:
            return self._angular_description_noncanonical(scene)

    def _angular_description_canonical(self, scene):
        # For now, absolute descriptions are only for the vertical, and coincide with both intrinsic and relative
        return self.intrinsic_or_relative_description(scene)

    def _angular_description_noncanonical(self, scene):
        if scene.figure_is_on_axis(2) and self.sampler.flip("description_is_absolute"):
            description = self.language.absolute_vertical_description(scene)
        else:
            description = self.intrinsic_or_relative_description(scene, "noncanonical_adjustment")
        return description

    def intrinsic_or_relative_description(self, scene, adjustment=None):
        # Direct scenes automatically use intrinsic description
        # as direct frame of reference
        if scene.is_direct() or self.sampler.flip("description_is_intrinsic",
                                                  adjustment):
            descriptions = self.language.intrinsic_descriptions(scene)
        else:
            descriptions = self.language.relative_descriptions(scene)

        if descriptions:
            return self.sample_word(descriptions)
        else:
            return None

    def random_word(self):
        word = self.sample_word(self.language.vocabulary)
        return word


class DataGenerator:

    def __init__(self, experimental_condition, seed=None, verbose=False):
        self.sampler = Sampler(experimental_condition, seed, verbose)
        self.scene_generator = SceneGenerator(self.sampler)

    def generate_data(self, num_samples):
        data = []
        for _ in range(num_samples):
            datum = self.build_datum()
            data.append(datum)
        return data

    def build_datum(self):
        with self.sampler.flip_results_manager() as flip_results:
            datum = self._build_datum()
            if flip_results:
                datum.set_flip_results(flip_results)
            return datum

    def _build_datum(self):
        scene = self.scene_generator.build_scene()
        datum = Datum(scene)
        return datum


class TrainingDataGenerator(DataGenerator):

    def __init__(self, experimental_condition, seed=None, verbose=False):
        super().__init__(experimental_condition, seed, verbose)
        language = LANGUAGES[experimental_condition.language]
        self.description_generator = DescriptionGenerator(self.sampler, language)

    # Override super
    def _build_datum(self):
        scene = self.scene_generator.build_scene()
        if self.sampler.flip("datum_is_reliable"):
            description = self.description_generator.true_description(scene)
        else:
            description = self.description_generator.random_word()
        datum = TrainingDatum(scene, description)
        return datum


class TestingDataGenerator(DataGenerator):

    def __init__(self, experimental_condition, seed=None, verbose=False):
        super().__init__(experimental_condition, seed, verbose)
        self.language = LANGUAGES[experimental_condition.language]

    # Override super
    def _build_datum(self):
        scene = self.scene_generator.build_scene()
        label = self.language.label_scene(scene)
        datum = TestingDatum(scene, label)
        return datum