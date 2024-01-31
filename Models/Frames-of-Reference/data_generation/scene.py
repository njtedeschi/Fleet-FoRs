from dataclasses import dataclass

import numpy as np

from constants import ORIGIN

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

    @classmethod
    def speaker(cls, position, forward, upward):
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

    @classmethod
    def direct_scene(cls, speaker, figure):
        scene = cls(
            speaker=speaker,
            ground=speaker,
            figure=figure
        )
        return scene

    def ground_figure_displacement(self):
        return self.figure.position - self.ground.position

    def ground_figure_distance(self):
        g_to_f = self.ground_figure_displacement()
        return np.linalg.norm(g_to_f)