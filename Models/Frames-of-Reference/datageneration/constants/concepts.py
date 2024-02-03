from ..structures.language import AbstractDirection

# Symmetry locatives
ABOVE = AbstractDirection("upward", 1)
BELOW = AbstractDirection("upward", -1)
FRONT = AbstractDirection("forward", 1, reflected_relatively=True)
BEHIND = AbstractDirection("forward", -1, reflected_relatively=True)
RIGHT = AbstractDirection("rightward", 1)
LEFT = AbstractDirection("rightward", -1)
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