from contextlib import contextmanager

import numpy as np

from .structures.world import BaseObject, OrientedObject, Scene
from .structures.experiment import TrainingDatum, TestingDatum
from .constants.space import (UP, DOWN, NORTH, SOUTH, EAST, WEST,
                              CARDINAL_DIRECTIONS_4,
                              CARDINAL_DIRECTIONS_6)
from .constants.objects import (NEAR, FAR,
                                OFF_AXIS_DIRECTIONS,
                                CANONICAL_DIRECT_SPEAKER,
                                CANONICAL_NONDIRECT_SPEAKER)
from .constants.languages import LANGUAGES

class DataGenerator:

    def __init__(self, experimental_condition, seed=None, verbose=False):
        self.rng = np.random.default_rng(seed)
        # Language
        self.language = LANGUAGES[experimental_condition.language]
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

        if self.flip("scene_is_direct"):
            scene = self.sample_direct_scene(figure, speaker_is_canonical)
        else:
            scene = self.sample_nondirect_scene(figure, speaker_is_canonical)
        return scene

    ## Direct Scene
    def sample_direct_scene(self, figure, speaker_is_canonical=True):
        if speaker_is_canonical:
            speaker = CANONICAL_DIRECT_SPEAKER
        else:
            # Speaker is direct when specified_position=None
            speaker = self.sample_noncanonical_speaker()
        scene = Scene.direct_scene(speaker, figure)
        return scene

    ## Nondirect Scene
    def sample_nondirect_scene(self, figure, speaker_is_canonical=True):
        if speaker_is_canonical:
            speaker = CANONICAL_NONDIRECT_SPEAKER
        else:
            speaker = self.sample_noncanonical_speaker(
                CANONICAL_NONDIRECT_SPEAKER.position)
        ground = self.sample_ground()
        scene = Scene(speaker, ground, figure)
        return scene

    ### Speaker
    # No sampling for canonical speakers due to EAST facing restriction

    # Possibilities directions restricted in line with canonical direct and nondirect speakers
    # Either forward or upward must face EAST
    def sample_noncanonical_speaker(self, specified_position=None):
        
        if self.flip("speaker_is_upside_down"):
            # The speaker faces the same direction as the canonical one
            # but upward=DOWN instead of upward=UP
            speaker = OrientedObject.speaker(
                CANONICAL_DIRECT_SPEAKER.forward, DOWN, specified_position)
        else:
            speaker = self.sample_lying_speaker(specified_position)
        return speaker

    # Lying down, not fibbing
    def sample_lying_speaker(self, specified_position):
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
        forward = self.rng.choice(CARDINAL_DIRECTIONS_4)
        ground = OrientedObject.ground(forward, UP, body_type)
        return ground

    def sample_noncanonical_ground(self, body_type):
        if self.flip("ground_is_upside_down"):
            ground = self.sample_upside_down_ground(body_type)
        else:
            ground = self.sample_lying_ground(body_type)
        return ground

    def sample_upside_down_ground(self, body_type):
        forward = self.rng.choice(CARDINAL_DIRECTIONS_4)
        ground = OrientedObject.ground(forward, DOWN, body_type)
        return ground

    # TODO: Check combinatorics
    # ground lying on its face, back, or side
    def sample_lying_ground(self, body_type):
        upward = self.rng.choice(CARDINAL_DIRECTIONS_4)
        forward_possibilities = self._forward_possibilities(upward)
        forward = self.rng.choice(forward_possibilities)
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
    def sample_figure(self):
        if self.flip("figure_is_on_axis"):
            direction = self.rng.choice(CARDINAL_DIRECTIONS_6)
        else:
            direction = self.rng.choice(OFF_AXIS_DIRECTIONS)
        distance = NEAR if self.flip("figure_is_near") else FAR
        position = distance * direction
        figure = BaseObject(position)
        return figure

    # Description sampling
    def sample_true_description(self, scene):
        description = None
        if scene.figure_is_on_axis() and self.flip("description_is_angular"):
            description = self.sample_angular_description(scene)
        if not description:
            description = self.language.proximity_description(scene)
        return description

    def sample_angular_description(self, scene):
        if scene.figure_is_on_axis(2) and self.flip("description_is_absolute"):
            description = self.language.absolute_vertical_description(scene)
        else:
            description = self.sample_intrinsic_or_relative_description(scene)
        return description

    def sample_intrinsic_or_relative_description(self, scene):
        if self.flip("description_is_intrinsic"):
            descriptions = self.language.intrinsic_descriptions(scene)
        else:
            descriptions = self.language.relative_descriptions(scene)

        if descriptions:
            return self.rng.choice(descriptions)
        else:
            return None

    def sample_word_randomly(self):
        word = self.rng.choice(self.language.vocabulary)
        return word

    # Data sampling
    def sample_data(self, num_samples, for_training=True):
        data = []
        for _ in range(num_samples):
            if for_training:
                datum = self.sample_training_datum()
            else:
                datum = self.sample_testing_datum()
            data.append(datum)
        return data

    ## Training
    def sample_training_datum(self):
        with self.flip_results_manager() as flip_results:
            scene = self.sample_scene()
            if self.flip("datum_is_reliable"):
                description = self.sample_true_description(scene)
            else:
                description = self.sample_word_randomly()
            datum = TrainingDatum(scene, description, flip_results)
            return datum

    ## Testing
    def sample_testing_datum(self):
        scene = self.sample_scene()
        test_label = self.language.label_scene(scene)
        datum = TestingDatum(scene, test_label)
        return datum