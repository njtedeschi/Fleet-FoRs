# Colors
BLUE = "blue"
RED = "red"
YELLOW = "goldenrod"
PURPLE = "purple"
GREEN = "green"
ORANGE = "orange"
BLACK = "black"

COLOR_I = BLUE
COLOR_R = RED
COLOR_A = YELLOW
COLOR_IRA = BLACK
def update_secondary_colors():
    global COLOR_IR, COLOR_IA, COLOR_RA
    # Simplified color mixing logic
    color_mixing = {
        frozenset({BLUE, RED}): PURPLE,
        frozenset({BLUE, YELLOW}): GREEN,
        frozenset({RED, YELLOW}): ORANGE
    }
    # Update secondary colors based on primary colors
    COLOR_IR = color_mixing.get(frozenset({COLOR_I, COLOR_R}))
    COLOR_IA = color_mixing.get(frozenset({COLOR_I, COLOR_A}))
    COLOR_RA = color_mixing.get(frozenset({COLOR_R, COLOR_A}))
update_secondary_colors()
# COLOR_IR = "purple"
# COLOR_IA = "green"
# COLOR_RA = "orange"
# COLOR_IRA = "black"

# Linestyles
LINESTYLE_S = "dotted"
LINESTYLE_M = "solid" # solid or "more solid" because correct

# Factor info
SENSE_INFO = {
    "Int": {
        "color": COLOR_I,
        "label": "Intrinsic Only",
        "linestyle": "solid"
        },
    "Rel": {
        "color": COLOR_R,
        "label": "Relative Only",
        "linestyle": "solid"
        },
    "Abs": {
        "color": COLOR_A,
        "label": "Absolute Only",
        "linestyle": "solid"
        },
        ###
    "IntRel": {
        "color": COLOR_IR,
        "label": "Intrinsic and Relative",
        "linestyle": "solid"
        },
    "IntAbs": {
        "color": COLOR_IA,
        "label": "Intrinsic and Absolute",
        "linestyle": "solid"
        },
    "RelAbs": {
        "color": COLOR_RA,
        "label": "Relative and Absolute",
        "linestyle": "solid"
        },
        ###
    "IntRelAbs": {
        "color": COLOR_IRA,
        "label": "All frames",
        "linestyle": "solid"
    },
    #########
    "SInt": {
        "color": COLOR_I,
        "label": "Intrinsic (symmetry) Only",
        "linestyle": LINESTYLE_S
        },
    "MInt": {
        "color": COLOR_I,
        "label": "Intrinsic (meronymy) Only",
        "linestyle": LINESTYLE_M
        },
        ###
    "SIntRel": {
        "color": COLOR_IR,
        "label": "Intrinsic (symmetry) and Relative",
        "linestyle": LINESTYLE_S
        },
    "MIntRel": {
        "color": COLOR_IR,
        "label": "Intrinsic (meronymy) and Relative",
        "linestyle": LINESTYLE_M
        },
    "SIntAbs": {
        "color": COLOR_IA,
        "label": "Intrinsic (symmetry) and Absolute",
        "linestyle": LINESTYLE_S
        },
    "MIntAbs": {
        "color": COLOR_IA,
        "label": "Intrinsic (meronymy) and Absolute",
        "linestyle": LINESTYLE_M
        },
        ###
    "SIntRelAbs": {
        "color": COLOR_IRA,
        "label": "All frames (symmetric intrinsic)",
        "linestyle": LINESTYLE_S
    },
    "MIntRelAbs": {
        "color": COLOR_IRA,
        "label": "All frames (meronymic intrinsic)",
        "linestyle": LINESTYLE_M
    }
}

# Legend info
LEGEND_INFO = {
    "Sense": SENSE_INFO
}

# Metric labels
METRIC_LABELS = {
    "Share": "Posterior Share",
    "Precision": "Precision",
    "Recall": "Recall",
    "Accuracy": "Accuracy",
    "F1": "F1 score"
}

# Senses by language and word
HORIZONTAL_SM_COMBINED = [
    "Int",
    "Rel",
    "IntRel"
]
HORIZONTAL_SM_SEPARATE = [
    "SInt",
    "MInt",
    "Rel",
    "SIntRel",
    "MIntRel"
]
VERTICAL_SM_COMBINED = [
    "Int",
    "Rel",
    "Abs",
    "IntRel",
    "IntAbs",
    "RelAbs",
    "IntRelAbs"
]
VERTICAL_SM_SEPARATE= [
    "SInt",
    "MInt",
    "Rel",
    "Abs",
    "SIntRel",
    "MIntRel",
    "SIntAbs",
    "MIntAbs",
    "RelAbs",
    "SIntRelAbs"
    "MIntRelAbs"
]

ENGLISH_WORD_SENSES = {
    "above/below": VERTICAL_SM_COMBINED,
    "front/behind": HORIZONTAL_SM_COMBINED,
    "left/right": HORIZONTAL_SM_COMBINED,
    "side": HORIZONTAL_SM_COMBINED,
}

MIXTEC_WORD_SENSES_SM_COMBINED = {
    "head": VERTICAL_SM_COMBINED,
    "belly": VERTICAL_SM_COMBINED,
    "face": HORIZONTAL_SM_COMBINED,
    "back": HORIZONTAL_SM_COMBINED,
    "left/right": HORIZONTAL_SM_COMBINED,
    "flank": HORIZONTAL_SM_COMBINED,
}

MIXTEC_WORD_SENSES_SM_SEPARATE = {
    "head": VERTICAL_SM_SEPARATE,
    "belly": VERTICAL_SM_COMBINED,
    "face": HORIZONTAL_SM_COMBINED,
    "back": HORIZONTAL_SM_SEPARATE,
    "left/right": HORIZONTAL_SM_COMBINED,
    "flank": HORIZONTAL_SM_COMBINED,
}