from dataclasses import dataclass, field
from typing import Dict, Tuple

@dataclass
class PlotConfiguration:
    figsize: Tuple[int, int] = (12, 6)
    ylim: Tuple[int, int] = (0, 1)
    # xlabel: str = "Training Size"
    xlabel: str = "Amount of data"
    #
    ylabel: str = ""
    labels: Dict[str, str] = field(default_factory=dict)
    colors: Dict[str, str] = field(default_factory=dict)
    linestyles: Dict[str, str] = field(default_factory=dict)
