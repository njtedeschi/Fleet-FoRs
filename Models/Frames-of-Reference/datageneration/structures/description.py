from dataclasses import dataclass
from typing import Dict, List

import numpy as np
from scipy.spatial.distance import cosine

@dataclass
class AbstractDirection:
    axis: str # "upward", "forward", or "rightward"
    sign: int # -1, 0, or 1
    reflected_relatively: bool = False

    def applies(self, g_to_f, anchor, is_intrinsic=True):
        cosine_similarity = 1-cosine(g_to_f,
                                 getattr(anchor, self.axis))
        if self.sign:
            if not is_intrinsic and self.reflected_relatively:
                return (cosine_similarity == -self.sign) # e.g. relative "front", "behind"
            else:
                return (cosine_similarity == self.sign)
        else:
            return (abs(cosine_similarity) == 1) # e.g. "side"

@dataclass
class PossibleDescriptions:
    proximity: str  # One of "near" or "far"
    intrinsic: List[str]
    relative: List[str]
    absolute: str  # One out of language's "above" and "below"; can be None

@dataclass
class TruthValue:
    intrinsic: bool
    relative: bool
    absolute: bool

@dataclass
class Language:
    symmetry_locatives: Dict[str, AbstractDirection]
    body_part_locatives: Dict[str, Dict[str, AbstractDirection]]
    absolute_above: str
    absolute_below: str
    near: str = "near"
    far: str = "far"
    near_far_threshold: float = 1

    def __post_init__(self):
        self.vocabulary = self.get_vocabulary()

    def get_vocabulary(self):
        vocabulary = {self.near, self.far}
        vocabulary.union(self.symmetry_locatives.keys())
        vocabulary.union(self.body_part_locatives.keys())
        # Include absolute words if there are distinct ones
        return list(vocabulary)

    def proximity_description(self, scene):
        distance = scene.ground_figure_distance()
        if distance < self.near_far_threshold:
            word = self.near
        else:
            word = self.far
        return word

    # NOTE: Assumes figure is aligned with z-axis
    def absolute_vertical_description(self, scene):
        _, _, z = scene.ground_figure_displacement()
        if z > 0:
            word = self.absolute_above
        else:
            word = self.absolute_below
        return word

    # TODO: symmetry vs. geometry
    def intrinsic_descriptions(self, scene):
        return self._angular_descriptions(scene, is_intrinsic=True)

    def relative_descriptions(self, scene):
        return self._angular_descriptions(scene, is_intrinsic=False)
    
    # NOTE: intrinsic and relative, but not absolute
    def _angular_descriptions(self, scene, is_intrinsic=True):
        g_to_f = scene.ground_figure_displacement()
        anchor = scene.ground if is_intrinsic else scene.speaker
        words = []
        # Check possible symmetry descriptions
        for word, direction in self.symmetry_locatives.items():
            if direction and direction.applies(g_to_f, anchor, is_intrinsic=is_intrinsic): # first condition handles None direction
                words.append(word)
        # Check possible body part descriptions
        body_type = anchor.body_type
        for word, direction_dict in self.body_part_locatives.items():
            direction = direction_dict[body_type]
            if direction and direction.applies(g_to_f, anchor, is_intrinsic=is_intrinsic):
                words.append(word)
        return words

    ### Methods used in Testing

    # TODO: improve efficiency
    def all_descriptions(self, scene):
        proximity = self.proximity_description(scene)
        intrinsic = self.intrinsic_descriptions(scene)
        relative = self.relative_descriptions(scene)
        if scene.figure_is_on_axis(2):
            absolute = self.absolute_vertical_description(scene)
        else:
            absolute = None
        return PossibleDescriptions(proximity, intrinsic, relative, absolute)

    def label_scene(self, scene):
        possible_descriptions = self.all_descriptions(scene)
        label = {}
        for word in self.vocabulary:
            truth_value = self._truth_value_of_word(word, possible_descriptions)
            label[word] = truth_value
        return label

    def _truth_value_of_word(self, word, possible_descriptions):
        intrinsic = False
        relative = False
        absolute = False
        if word in possible_descriptions.proximity:
            intrinsic = True
            relative = True
            absolute = True
        if word in possible_descriptions.intrinsic:
            intrinsic = True
        if word in possible_descriptions.relative:
            relative = True
        if possible_descriptions.absolute and word in possible_descriptions.absolute:
            absolute = True
        return TruthValue(intrinsic, relative, absolute)