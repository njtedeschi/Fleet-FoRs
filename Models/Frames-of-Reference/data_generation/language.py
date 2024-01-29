from dataclasses import dataclass
from typing import Dict

import numpy as np
from scipy.spatial.distance import cosine

from scene import *

@dataclass
class AbstractDirection:
    axis: str # "upward", "forward", or "rightward"
    sign: int # -1, 0, or 1
    reflected_relatively: bool = False

    def applies(self, g_to_f, anchor, relative=False):
        cosine_distance = cosine(g_to_f,
                                 getattr(anchor, self.axis))
        if self.sign:
            if relative and self.reflected_relatively:
                return (cosine_distance == -self.sign) # e.g. relative "front", "behind"
            else:
                return (cosine_distance == self.sign)
        else:
            return (abs(cosine_distance) == 1) # e.g. "side"

@dataclass
class Language:
    symmetry_locatives: Dict[str, AbstractDirection]
    body_part_locatives: Dict[str, Dict[str, AbstractDirection]]
    near_far_threshold: float = 1

    def __post_init__(self):
        self.vocabulary = self.get_vocabulary()

    def get_vocabulary(self):
        vocabulary = {"near", "far"}
        vocabulary.union(self.symmetry_locatives.keys())
        vocabulary.union(self.body_part_locatives.keys())
        return list(vocabulary)

    def proximity_description(self, scene):
        distance = scene.ground_figure_distance()
        if distance < self.near_far_threshold:
            word = "near"
        else:
            word = "far"
        return word
    
    def angular_descriptions(self, scene, relative=False):
        g_to_f = scene.ground_figure_displacement()
        anchor = scene.speaker if relative else scene.ground
        words = []
        # Check possible symmetry descriptions
        for word, direction in self.symmetry_locatives.items():
            if direction and direction.applies(g_to_f, anchor, relative=relative): # first condition handles None direction
                words.append(word)
        # Check possible body part descriptions
        body_type = anchor.body_type
        for word, direction_dict in self.body_part_locatives.items():
            direction = direction_dict[body_type]
            if direction and direction.applies(g_to_f, anchor, relative=relative):
                words.append(word)
        return words