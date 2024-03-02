from enum import Enum

### BEGIN LABELS ###
SENSE_LABELS = {
    "Int": "Intrinsic Only",
    "Rel": "Relative Only",
    "Abs": "Absolute Only",
        ###
    "IntRel": "Intrinsic and Relative",
    "IntAbs": "Intrinsic and Absolute",
    "RelAbs": "Relative and Absolute",
        ###
    "IntRelAbs": "All frames",
    #########
    "SInt": "Intrinsic (symmetry) Only",
    "MInt": "Intrinsic (meronymy) Only",
        ###
    "SIntRel": "Intrinsic (symmetry) and Relative",
    "MIntRel": "Intrinsic (meronymy) and Relative",
    "SIntAbs": "Intrinsic (symmetry) and Absolute",
    "MIntAbs": "Intrinsic (meronymy) and Absolute",
        ###
    "SIntRelAbs": "All frames (symmetric intrinsic)",
    "MIntRelAbs":  "All frames (meronymic intrinsic)"
}
### END LABELS ###

### BEGIN COLORS ###
class Color(Enum):
    BLUE = "blue"
    RED = "red"
    YELLOW = "goldenrod"
    PURPLE = "purple"
    GREEN = "green"
    ORANGE = "orange"
    BLACK = "black"

# Color assignments dictionary
SENSE_COLORS = {
    "Int": Color.BLUE.value,
    "Rel": Color.RED.value,
    "Abs": Color.YELLOW.value,
    "IntRelAbs": Color.BLACK.value
}

def set_sense_secondary_colors():
    # Simplified color mixing logic
    color_mixing = {
        frozenset({Color.BLUE.value, Color.RED.value}): Color.PURPLE.value,
        frozenset({Color.BLUE.value, Color.YELLOW.value}): Color.GREEN.value,
        frozenset({Color.RED.value, Color.YELLOW.value}): Color.ORANGE.value
    }

    # Update secondary colors based on primary colors
    SENSE_COLORS["IntRel"] = color_mixing.get(frozenset({SENSE_COLORS["Int"], SENSE_COLORS["Rel"]}))
    SENSE_COLORS["IntAbs"] = color_mixing.get(frozenset({SENSE_COLORS["Int"], SENSE_COLORS["Abs"]}))
    SENSE_COLORS["RelAbs"] = color_mixing.get(frozenset({SENSE_COLORS["Rel"], SENSE_COLORS["Abs"]}))

# Initial update to set secondary colors
set_sense_secondary_colors()
SENSE_COLORS["SInt"] = SENSE_COLORS["Int"]
SENSE_COLORS["MInt"] = SENSE_COLORS["Int"]
SENSE_COLORS["SIntRel"] = SENSE_COLORS["IntRel"]
SENSE_COLORS["MIntRel"] = SENSE_COLORS["IntRel"]
SENSE_COLORS["SIntAbs"] = SENSE_COLORS["IntAbs"]
SENSE_COLORS["MIntAbs"] = SENSE_COLORS["IntAbs"]
SENSE_COLORS["SIntRelAbs"] = SENSE_COLORS["IntRelAbs"]
SENSE_COLORS["MIntRelAbs"] = SENSE_COLORS["IntRelAbs"]
### END COLORS ###


### BEGIN LINESTYLES ###
SENSE_PREFIX_LINESTYLES = {
    "SInt": "dotted",
    "MInt": "solid" # solid or "more solid" because "correct"
}
SENSE_LINESTYLES = {}
def set_sense_linestyles():
    for sense in SENSE_LABELS.keys():
        linestyle = "solid" # default
        for prefix, prefix_linestyle in SENSE_PREFIX_LINESTYLES.items():
            if sense.startswith(prefix):
                linestyle = prefix_linestyle
                break
        SENSE_LINESTYLES[sense] = linestyle
### END LINESTYLES ###

### BEGIN LEGEND INFO ###
FACTOR_LABELS = {
    "Sense": SENSE_LABELS
}
FACTOR_COLORS = {
    "Sense": SENSE_COLORS
}
FACTOR_LINESTYLES = {
    "Sense": SENSE_LINESTYLES
}
### END LEGEND INFO ###

METRIC_LABELS = {
    "Share": "Posterior Share",
    "Precision": "Precision",
    "Recall": "Recall",
    "Accuracy": "Accuracy",
    "F1": "F1 score"
}
