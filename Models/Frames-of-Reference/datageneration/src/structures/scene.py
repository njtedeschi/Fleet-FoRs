from dataclasses import dataclass

import numpy as np
from scipy.spatial.distance import cosine

from ..constants.space import ORIGIN, UP


@dataclass
class BaseObject:
    position: np.ndarray

@dataclass
class OrientedObject(BaseObject):
    forward: np.ndarray
    upward: np.ndarray
    is_participant: bool
    body_type: str

    # Note: forward and upward should be orthogonal
    def __post_init__(self):
        self.rightward = np.cross(self.forward, self.upward)
        self.is_upright = np.array_equal(self.upward, UP)

    @classmethod
    def speaker(cls, forward, upward, specified_position=None):
        # Could be handled with a default argument of ORIGIN,
        # but I want things to be more explicit
        if specified_position is not None:
            position = specified_position
        else:
            # Speaker is direct if specified_position=None
            position = ORIGIN

        speaker = cls(
            position=position,
            forward=forward,
            upward=upward,
            is_participant=True,
            body_type="biped"
        )
        return speaker

    @classmethod
    def ground(cls, forward, upward, body_type):
        ground = cls(
            position=ORIGIN,
            forward=forward,
            upward=upward,
            is_participant=False,
            body_type=body_type
        )
        return ground

@dataclass
class Scene:
    speaker: OrientedObject
    ground: OrientedObject
    figure: BaseObject

    def __post_init__(self):
        self.is_direct = self._is_direct()
        self.satisfies_POCO = True
        if not self.ground.is_upright:
            self.satisfies_POCO = self._satisfies_POCO()

    @classmethod
    def direct_scene(cls, speaker, figure):
        scene = cls(
            speaker=speaker,
            ground=speaker,
            figure=figure
        )
        return scene

    def _is_direct(self):
        return self.ground.is_participant

    def _satisfies_POCO(self):
        canonicity = self._ground_canonicity_by_axis()
        axis = self._intrinsic_figure_axis()
        if axis:
            return canonicity[axis]
        # TODO: error handling for off-axis case?
        return False

    def _ground_canonicity_by_axis(self):
        axis_is_canonical = {
            "rightward": True,
            "forward": True,
            "upward": True
        }
        # If object isn't upright
        if not np.array_equal(self.ground.upward, UP):
            axis_is_canonical["upward"] = False
            if np.dot(self.ground.forward, UP) != 0:
                axis_is_canonical["forward"] = False
            if np.dot(self.ground.rightward, UP) != 0:
                axis_is_canonical["rightward"] = False
        return axis_is_canonical

    # TODO: this and the import of cosine introduced redundancy with description.py
    def _intrinsic_figure_axis(self):
        g_to_f = self.ground_figure_displacement()
        for axis in ["upward", "forward", "rightward"]:
            cosine_similarity = 1-cosine(g_to_f,
                                         getattr(self.ground, axis))
            if (abs(cosine_similarity) == 1):
                return axis


    def ground_figure_displacement(self):
        return self.figure.position - self.ground.position

    def ground_figure_distance(self):
        g_to_f = self.ground_figure_displacement()
        return np.linalg.norm(g_to_f)

    def figure_is_on_axis(self, axis=None):
        displacement = self.ground_figure_displacement()
        on_an_axis = (np.count_nonzero(displacement) == 1)
        if axis:
            # Return True if figure on specified axis
            return (on_an_axis and displacement[axis] != 0)
        else:
            # If axis is None, return True if figure on any axis
            return on_an_axis