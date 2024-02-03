from dataclasses import dataclass
from typing import List, Dict

from .language import TruthValue
from .world import Scene

# Specific combination of hyperparameters
# YAML config must match attribute names
@dataclass
class ExperimentalCondition:
    # Language
    language: str
    # Probabilities
    p_datum_is_reliable: float
    ## Scene probabilities
    p_scene_is_direct: float
    p_figure_is_on_axis: float
    p_figure_is_near: float
    p_speaker_is_canonical: float
    p_speaker_is_upside_down: float # given noncanonical speaker
    p_ground_is_biped: float # given nondirect scene
    p_ground_is_canonical: float # given nondirect scene
    p_ground_is_upside_down: float # given noncanonical ground
    ## Description probabilities
    p_description_is_angular: float # given figure on an axis
    p_description_is_intrinsic: float # given angular description
    p_description_is_absolute: float # given angular description and figure on z-axis
    # Name
    labels: List[str]

    def __post_init__(self):
        self.name = self.condition_name_from_hyperparameter_values(self.labels)

    def condition_name_from_hyperparameter_values(self, labels):
        return "_".join(sorted(labels))

@dataclass
class TrainingDatum:
    scene: Scene
    description: str
    flip_results: Dict[str, bool] = None

@dataclass
class TestingDatum:
    scene: Scene
    label: Dict[str, TruthValue]