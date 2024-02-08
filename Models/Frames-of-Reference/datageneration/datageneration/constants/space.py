import numpy as np

ORIGIN = np.array([0,0,0])
EAST = np.array([1,0,0])
WEST = np.array([-1,0,0])
NORTH = np.array([0,1,0])
SOUTH = np.array([0,-1,0])
UP = np.array([0,0,1])
DOWN = np.array([0,0,-1])

CARDINAL_DIRECTIONS_6 = [EAST, WEST, NORTH, SOUTH, UP, DOWN]
CARDINAL_DIRECTIONS_4 = [EAST, WEST, NORTH, SOUTH]