from dataclasses import dataclass
from typing import Dict, Set

import numpy as np
from scipy.spatial.distance import cosine

@dataclass(frozen=True)
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
    proximity: Set[str]  # One of "near" or "far"
    intrinsic_symmetry: Set[str]
    intrinsic_meronymy: Set[str]
    intrinsic_sym_mer: Set[str] # Symmetric interpretation of meronymic locatives
    relative: Set[str]
    absolute: Set[str] # "above", "below", or neither

@dataclass
class Language:
    symmetry_locatives: Dict[str, AbstractDirection]
    meronymy_locatives: Dict[str, Dict[str, AbstractDirection]]
    absolute_above: str
    absolute_below: str
    near: str = "near"
    far: str = "far"
    near_far_threshold: float = 1

    def __post_init__(self):
        self.vocabulary = self._get_vocabulary()
        self.true_meronymy_locatives = self._get_true_meronymy_locatives()

    def _get_vocabulary(self):
        vocabulary = {self.near, self.far}
        vocabulary = vocabulary.union(self.symmetry_locatives.keys())
        vocabulary = vocabulary.union(self.meronymy_locatives.keys())
        # Include absolute words if there are distinct ones
        return vocabulary

    # Meronmy locatives that actually exhibit a difference depending on
    # body type of anchor
    def _get_true_meronymy_locatives(self):
        return {word : direction_dict
                for word, direction_dict in self.meronymy_locatives.items()
                if len(set(direction_dict.values())) > 1
                }

    def proximity_description(self, scene):
        distance = scene.ground_figure_distance()
        return self.near if distance < self.near_far_threshold else self.far

    # NOTE: Assumes figure is aligned with z-axis
    # The check for this occurs in:
    # - `DescriptionGenerator.angular_description` for training data
    # - `Language.all_descriptions` for testing data
    def absolute_vertical_description(self, scene):
        _, _, z = scene.ground_figure_displacement()
        return self.absolute_above if z > 0 else self.absolute_below

    ## Intrinsic

    def intrinsic_descriptions(self, scene):
        g_to_f = scene.ground_figure_displacement()
        anchor = scene.ground
        symmetry_words = self._intrinsic_descriptions_symmetry(
            g_to_f, anchor)
        meronymy_words = self._intrinsic_descriptions_meronymy(
            g_to_f, anchor)
        return symmetry_words.union(meronymy_words)

    def _intrinsic_descriptions_symmetry(self, g_to_f, anchor):
        words = set()
        for word, direction in self.symmetry_locatives.items():
            if direction and direction.applies(g_to_f, anchor, is_intrinsic=True): # first condition handles None direction
                words.add(word)
        return words

    def _intrinsic_descriptions_meronymy(self, g_to_f, anchor):
        body_type = anchor.body_type
        words = set()
        for word, direction_dict in self.meronymy_locatives.items():
            direction = direction_dict[body_type]
            if direction and direction.applies(g_to_f, anchor, is_intrinsic=True):
                words.add(word)
        return words

    # Testing only
    # Treats all anchors as having bipedal body type
    def _intrinsic_descriptions_symmetry_with_meronym(self, g_to_f, anchor):
        words = set()
        for word, direction_dict in self.meronymy_locatives.items():
            direction = direction_dict["biped"]
            if direction and direction.applies(g_to_f, anchor, is_intrinsic=True):
                words.add(word)
        return words

    ## Relative

    def relative_descriptions(self, scene):
        # Direct FoRs treated as intrinsic
        # This condition shouldn't be reached due to a previous check
        if scene.is_direct():
            return self.intrinsic_descriptions(scene)
        else:
            g_to_f = scene.ground_figure_displacement()
            anchor = scene.speaker
            return self._relative_descriptions(g_to_f, anchor)

    def _relative_descriptions(self, g_to_f, anchor):
        words = set()
        for word, direction in self.symmetry_locatives.items():
            if direction and direction.applies(g_to_f, anchor, is_intrinsic=False): # first condition handles None direction
                words.add(word)
        for word, direction_dict in self.meronymy_locatives.items():
            direction = direction_dict["biped"]
            if direction and direction.applies(g_to_f, anchor, is_intrinsic=False):
                words.add(word)
        return words

    ### Methods used in Testing

    # TODO: improve efficiency
    def all_descriptions(self, scene):
        # Singleton values put in a set
        proximity = {self.proximity_description(scene)}
        if scene.figure_is_on_axis(2):
            absolute = {self.absolute_vertical_description(scene)}
        else:
            absolute = set()
        relative = self.relative_descriptions(scene)
        # Intrinsic descriptions
        g_to_f = scene.ground_figure_displacement()
        anchor = scene.ground
        int_sym = self._intrinsic_descriptions_symmetry(g_to_f, anchor)
        int_mer = self._intrinsic_descriptions_meronymy(g_to_f, anchor)
        int_sym_mer = self._intrinsic_descriptions_symmetry_with_meronym(g_to_f, anchor)
        return PossibleDescriptions(proximity, int_sym, int_mer, int_sym_mer, relative, absolute)

    def label_scene(self, scene):
        possible_descriptions = self.all_descriptions(scene)
        label = {}
        # Proximity
        for word in [self.near, self.far]:
            label[word] = {"Top": (word in possible_descriptions.proximity)}
        for word in self.symmetry_locatives:
            label[word] = self._truth_values(word, possible_descriptions)
        for word in self.meronymy_locatives:
            # Meronymy locatives that lead to different predictions if used
            # as symmetry locatives receive special labels
            if word in self.true_meronymy_locatives:
                label[word] = self._truth_values_true_meronyms(word, possible_descriptions)
            else:
                label[word] = self._truth_values(word, possible_descriptions)
        return label

    def _truth_values(self, word, possible_descriptions):
        truth_values = {}
        truth_values["Int"] = False
        truth_values["Rel"] = False
        if word in possible_descriptions.intrinsic_symmetry or word in possible_descriptions.intrinsic_meronymy:
            truth_values["Int"] = True
        if word in possible_descriptions.relative:
            truth_values["Rel"] = True
        # Above and below additionally have absolute sense
        if (word in [self.absolute_above, self.absolute_below]):
            truth_values["Abs"] = (word in possible_descriptions.absolute)
            truth_values["IntRel"] = (truth_values["Int"] or truth_values["Rel"])
            truth_values["IntAbs"] = (truth_values["Int"] or truth_values["Abs"])
            truth_values["RelAbs"] = (truth_values["Rel"] or truth_values["Abs"])
            truth_values["IntRelAbs"] = (truth_values["Int"] or truth_values["Rel"] or truth_values["Abs"])
        else:
            truth_values["IntRel"] = (truth_values["Int"] or truth_values["Rel"])
        return truth_values

    def _truth_values_true_meronyms(self, word, possible_descriptions):
        truth_values = {}
        truth_values["SInt"] = False
        truth_values["MInt"] = False
        truth_values["Rel"] = False
        if word in possible_descriptions.intrinsic_sym_mer:
            truth_values["SInt"] = True
        if word in possible_descriptions.intrinsic_meronymy:
            truth_values["MInt"] = True
        if word in possible_descriptions.relative:
            truth_values["Rel"] = True
        # Above and below additionally have absolute sense
        if (word in [self.absolute_above, self.absolute_below]):
            truth_values["Abs"] = (word in possible_descriptions.absolute)
            truth_values["SIntRel"] = (truth_values["SInt"] or truth_values["Rel"])
            truth_values["SIntAbs"] = (truth_values["SInt"] or truth_values["Abs"])
            truth_values["MIntRel"] = (truth_values["MInt"] or truth_values["Rel"])
            truth_values["MIntAbs"] = (truth_values["MInt"] or truth_values["Abs"])
            truth_values["RelAbs"] = (truth_values["Rel"] or truth_values["Abs"])
            truth_values["SIntRelAbs"] = (truth_values["SInt"] or truth_values["Rel"] or truth_values["Abs"])
            truth_values["MIntRelAbs"] = (truth_values["MInt"] or truth_values["Rel"] or truth_values["Abs"])
        else:
            truth_values["SIntRel"] = (truth_values["SInt"] or truth_values["Rel"])
            truth_values["MIntRel"] = (truth_values["MInt"] or truth_values["Rel"])
        return truth_values
