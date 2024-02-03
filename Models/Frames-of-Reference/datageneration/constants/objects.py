import numpy as np

from . import space
from ..structures.world import OrientedObject

# Figure
## Off axis Directions
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
## Distances
NEAR = 0.5
FAR = 1.5

# Speaker
CANONICAL_SPEAKER_FORWARD = space.EAST # Arbitrary convention
CANONICAL_SPEAKER_LEFTWARD = space.NORTH # Consequence of forward
CANONICAL_SPEAKER_RIGHTWARD = space.SOUTH # Consequence of forward
## Direct
CANONICAL_DIRECT_SPEAKER = OrientedObject.speaker(forward=CANONICAL_SPEAKER_FORWARD, upward=space.UP)
## Nondirect
NONDIRECT_SPEAKER_POSITION = np.array([-2,0,0])
CANONICAL_NONDIRECT_SPEAKER = OrientedObject.speaker(forward=CANONICAL_SPEAKER_FORWARD, upward=space.UP, specified_position=NONDIRECT_SPEAKER_POSITION)