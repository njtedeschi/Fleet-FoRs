from dataclasses import dataclass

import numpy as np

@dataclass
class BaseObject:
    position: np.ndarray

@dataclass
class OrientedObject(BaseObject):
    forward: np.ndarray
    upward: np.ndarray
    is_participant: bool
    body_type: str

    def __post_init__(self):
        self.rightward = np.cross(self.forward, self.upward)

@dataclass
class Scene:
    speaker: OrientedObject
    ground: OrientedObject
    figure: BaseObject

    def ground_figure_displacement(self):
        return self.figure.position - self.ground.position

    def ground_figure_distance(self):
        g_to_f = self.ground_figure_displacement()
        return np.linalg.norm(g_to_f)