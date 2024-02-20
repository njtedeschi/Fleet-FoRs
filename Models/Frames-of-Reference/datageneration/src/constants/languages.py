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
english_meronymy_locatives = dict()
ENGLISH = Language(
    symmetry_locatives=english_symmetry_locatives,
    meronymy_locatives=english_meronymy_locatives,
    absolute_above="above",
    absolute_below="below"
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
MIXTEC = Language(
    symmetry_locatives=mixtec_symmetry_locatives,
    meronymy_locatives=mixtec_meronymy_locatives,
    absolute_above="head",
    absolute_below="belly"
)

# All languages
LANGUAGES = {
    "english": ENGLISH,
    "mixtec": MIXTEC
}