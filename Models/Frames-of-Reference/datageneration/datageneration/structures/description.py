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
    # Easier to make everything a list, even if it necessarily has 1 (or at most 1) entry
    proximity: List[str]  # One of "near" or "far"
    intrinsic: List[str]
    relative: List[str]
    absolute: List[str] # "above", "below", or neither

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
        vocabulary = vocabulary.union(self.symmetry_locatives.keys())
        vocabulary = vocabulary.union(self.body_part_locatives.keys())
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
        # Singleton values put in a list
        proximity = [self.proximity_description(scene)]
        intrinsic = self.intrinsic_descriptions(scene)
        relative = self.relative_descriptions(scene)
        if scene.figure_is_on_axis(2):
            absolute = [self.absolute_vertical_description(scene)]
        else:
            absolute = []
        return PossibleDescriptions(proximity, intrinsic, relative, absolute)

    def label_scene(self, scene):
        possible_descriptions = self.all_descriptions(scene)
        label = {}
        for word in self.vocabulary:
            label[word] = self._truth_values_by_sense(word, possible_descriptions)
        return label

    def _truth_values_by_sense(self, word, possible_descriptions):
        truth_values = {}
        # near and far only have absolute sense
        if (word == self.near or word == self.far):
            return {"Abs": (word in possible_descriptions.proximity)}
        # All other words have at least intrinsic and relative sense
        truth_values["Int"] = False
        truth_values["Rel"] = False
        if word in possible_descriptions.intrinsic:
            # TODO further split by symmetry vs meronymy
            truth_values["Int"] = True
        if word in possible_descriptions.relative:
            truth_values["Rel"] = True
        # Above and below additionally have absolute sense
        if (word == self.absolute_above or word == self.absolute_below):
            truth_values["Abs"] = (word in possible_descriptions.absolute)
            truth_values["IntRelAbs"] = (truth_values["Int"] or truth_values["Rel"] or truth_values["Abs"])
        else:
            truth_values["IntRel"] = (truth_values["Int"] or truth_values["Rel"])
        return truth_values