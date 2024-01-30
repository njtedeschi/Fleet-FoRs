import numpy as np

from scene import BaseObject, OrientedObject, Scene
from language import AbstractDirection, Language

### Vectors
ORIGIN = np.array([0,0,0])
EAST = np.array([1,0,0])
WEST = np.array([-1,0,0])
NORTH = np.array([0,1,0])
SOUTH = np.array([0,-1,0])
UP = np.array([0,0,1])
DOWN = np.array([0,0,-1])

CARDINAL_DIRECTIONS_6 = [EAST, WEST, NORTH, SOUTH, UP, DOWN]
CARDINAL_DIRECTIONS_4 = [EAST, WEST, NORTH, SOUTH]
# Compute unit directions in diagonal directions
def _compute_off_axis_directions():
    sqrt_val = np.sqrt(2) / 2
    off_axis_directions = []
    for i in range(3):
        for sign1 in [-1, 1]:
            for sign2 in [-1, 1]:
                p = np.zeros(3)
                p[(i + 1) % 3] = sign1 * sqrt_val
                p[(i + 2) % 3] = sign2 * sqrt_val
                off_axis_directions.append(p)
    return off_axis_directions
OFF_AXIS_DIRECTIONS = _compute_off_axis_directions()

### Distances
NEAR = 0.5
FAR = 1.5

### Speakers
# Direct
DIRECT_SPEAKER = OrientedObject(
    position=ORIGIN,
    forward=EAST,
    upward=UP,
    is_participant=True,
    body_type="biped"
)
# Nondirect
NONDIRECT_SPEAKER_POSITION = np.array([-2,0,0])
NONDIRECT_SPEAKER = OrientedObject(
    position=NONDIRECT_SPEAKER_POSITION,
    forward=EAST,
    upward=UP,
    is_participant=True,
    body_type="biped"
)

### Concepts
# Symmetry locatives
ABOVE = AbstractDirection("upward", 1)
BELOW = AbstractDirection("upward", -1)
FRONT = AbstractDirection("forward", 1, reflected_relatively=True)
BEHIND = AbstractDirection("forward", -1, reflected_relatively=True)
LEFT = AbstractDirection("rightward", 1)
RIGHT = AbstractDirection("rightward", -1)
SIDE = AbstractDirection("rightward", 0)

# Body part locatives
HEAD = {
    "biped": AbstractDirection("upward", 1),
    "quadruped": None
}
BELLY = {
    "biped": AbstractDirection("upward", -1),
    "quadruped": AbstractDirection("upward", -1)
}
FACE = {
    "biped": AbstractDirection("forward", 1, reflected_relatively=True),
    "quadruped": AbstractDirection("forward", 1)
}
BACK = {
    "biped": AbstractDirection("forward", -1, reflected_relatively=True),
    "quadruped": AbstractDirection("upward", 1)
}
FLANK = {
    "biped": AbstractDirection("rightward", 0),
    "quadruped": AbstractDirection("rightward", 0)
}


### Languages
# English
english_symmetry_locatives = {
    "above": ABOVE,
    "below": BELOW,
    "front": FRONT,
    "behind": BEHIND,
    "left": LEFT,
    "right": RIGHT,
    "side": SIDE
}
english_body_part_locatives = dict()
ENGLISH = Language(
    symmetry_locatives=english_symmetry_locatives,
    body_part_locatives=english_body_part_locatives
)
# Mixtec
mixtec_symmetry_locatives = {
    "left": LEFT,
    "right": RIGHT
}
mixtec_body_part_locatives = {
    "head" : HEAD,
    "belly": BELLY,
    "face": FACE,
    "back": BACK,
    "flank": FLANK
}
MIXTEC = Language(
    symmetry_locatives=mixtec_symmetry_locatives,
    body_part_locatives=mixtec_body_part_locatives
)
# All languages
LANGUAGES = {
    "english": ENGLISH,
    "mixtec": MIXTEC
}