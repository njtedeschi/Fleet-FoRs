from . import concepts
from ..structures.description import Language

### Languages
# English
english_symmetry_locatives = {
    "above": concepts.ABOVE,
    "below": concepts.BELOW,
    "front": concepts.FRONT,
    "behind": concepts.BEHIND,
    "left": concepts.LEFT,
    "right": concepts.RIGHT,
    "side": concepts.SIDE
}
ENGLISH_CH_2 = Language(
    symmetry_locatives=english_symmetry_locatives,
    meronymy_locatives=dict(),
    absolute_above="above",
    absolute_below="below",
    near="near",
    far="far"
)

# Mixtec
mixtec_symmetry_locatives = {
    "left": concepts.LEFT,
    "right": concepts.RIGHT
}
mixtec_meronymy_locatives = {
    "head" : concepts.HEAD,
    "belly": concepts.BELLY,
    "face": concepts.FACE,
    "back": concepts.BACK,
    "flank": concepts.FLANK
}
MIXTEC_CH_2 = Language(
    symmetry_locatives=mixtec_symmetry_locatives,
    meronymy_locatives=mixtec_meronymy_locatives,
    absolute_above="head",
    absolute_below="belly",
    near="near",
    far="far"
)

# All languages
LANGUAGES_CH_2 = {
    "english": ENGLISH_CH_2,
    "mixtec": MIXTEC_CH_2
}


### CHAPTER 3

LANGUAGES_CH_3 = {}
english_symmetry_locatives_ch3 = {
    key: value for key, value in english_symmetry_locatives.items() if key != "side"
}
ENGLISH_CH_3 = Language(
    symmetry_locatives=english_symmetry_locatives_ch3,
    meronymy_locatives=dict(),
    absolute_above="above",
    absolute_below="below",
    near="",
    far=""
)

# Mixtec for CH_3: Remove "flank"
mixtec_symmetry_locatives_ch3 = mixtec_symmetry_locatives.copy()  # No changes here
mixtec_meronymy_locatives_ch3 = {
    key: value for key, value in mixtec_meronymy_locatives.items() if key != "flank"
}
MIXTEC_CH_3 = Language(
    symmetry_locatives=mixtec_symmetry_locatives_ch3,
    meronymy_locatives=mixtec_meronymy_locatives_ch3,
    absolute_above="head",
    absolute_below="belly",
    near="",
    far=""
)

LANGUAGES_CH_3 = {
    "english": ENGLISH_CH_3,
    "mixtec": MIXTEC_CH_3
}