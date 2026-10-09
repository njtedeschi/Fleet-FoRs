#!/usr/bin/env python3
"""prepare_all.py

Run the *fast* preprocessing steps for all dissertation visualization modules.

This script exists so you can do a one-time "make plot-ready tables" pass,
then repeatedly regenerate figures via the render scripts + YAML configs,
without rerunning any expensive model inference.

It calls the per-module scripts:
  - ch2_viz_setup/scripts/prepare_ch2_figdata.py
  - ch2_viz_setup/scripts/prepare_ch2_sym_unbalance_figdata.py
  - ch3_old_viz_setup/scripts/prepare_ch3_old_figdata.py
  - ch3_new_model_viz_setup/scripts/prepare_ch3_new_model_figdata.py

Notes
-----
- These scripts only aggregate / reshape existing result tables.
- If some required input file is missing, this runner tells you exactly
  where to put it.
"""

from __future__ import annotations

import argparse
import glob
import os
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, List, Optional


@dataclass
class Task:
    name: str
    workdir: Path
    command: List[str]
    input_check: Callable[[], Optional[str]]


def _root_dir() -> Path:
    return Path(__file__).resolve().parents[1]


def _run(task: Task) -> None:
    msg = task.input_check()
    if msg is not None:
        raise SystemExit(f"[missing input] {task.name}:\n{msg}\n")

    print(f"\n=== {task.name} ===")
    print(f"$ (cd {task.workdir} && {' '.join(task.command)})")
    subprocess.run(task.command, cwd=str(task.workdir), check=True)


def main() -> None:
    ap = argparse.ArgumentParser(
        description="Run fast preprocessing for all dissertation viz modules."
    )
    ap.add_argument(
        "--only",
        action="append",
        default=[],
        help=(
            "Run only specific modules. Repeatable. "
            "Choices: ch2, ch2_sym, ch3_old, ch3_new"
        ),
    )
    args = ap.parse_args()

    only = set([s.strip() for s in (args.only or []) if s.strip()])
    root = _root_dir()

    def check_ch2() -> Optional[str]:
        p = root / "ch2_viz_setup" / "figdata" / "chapter_2_results.csv"
        if not p.exists():
            return (
                f"Expected: {p}\n"
                "Put your consolidated Chapter 2 long-format results CSV there."
            )
        return None

    def check_ch3_old() -> Optional[str]:
        raw_dir = root / "ch3_old_viz_setup" / "figdata" / "raw"
        files = sorted(raw_dir.glob("shares_*.csv"))
        if not files:
            return (
                f"Expected at least one file matching: {raw_dir / 'shares_YYYY-MM-DD.csv'}\n"
                "Put your raw 'shares_YYYY-MM-DD.csv' files there (long format)."
            )
        return None

    def check_ch3_new() -> Optional[str]:
        p = root / "ch3_new_model_viz_setup" / "figdata" / "ch3_new_model_consolidated_preprocessed.csv"
        if not p.exists():
            return (
                f"Expected: {p}\n"
                "Put your *pre-consolidated/preprocessed* new-model results CSV there.\n"
                "(If you only have raw per-condition CSVs organized by date, run:\n"
                "  python ch3_new_model_viz_setup/scripts/consolidate_ch3_new_model_results.py <experiments_root> "
                "-o ch3_new_model_viz_setup/figdata/ch3_new_model_consolidated_preprocessed.csv\n"
                ")"
            )
        return None

    tasks: List[Task] = [
        Task(
            name="Ch2: summarize learning curves (Int/Rel/IntRel)",
            workdir=root / "ch2_viz_setup" / "scripts",
            command=[sys.executable, "prepare_ch2_figdata.py"],
            input_check=check_ch2,
        ),
        Task(
            name="Ch2: summarize symmetric-unbalance mirror curves",
            workdir=root / "ch2_viz_setup" / "scripts",
            command=[sys.executable, "prepare_ch2_sym_unbalance_figdata.py"],
            input_check=check_ch2,
        ),
        Task(
            name="Ch3 old model: consolidate + summarize posterior shares",
            workdir=root / "ch3_old_viz_setup" / "scripts",
            command=[sys.executable, "prepare_ch3_old_figdata.py"],
            input_check=check_ch3_old,
        ),
        Task(
            name="Ch3 new model: summarize IntP-RelP difference curves",
            workdir=root / "ch3_new_model_viz_setup" / "scripts",
            command=[sys.executable, "prepare_ch3_new_model_figdata.py"],
            input_check=check_ch3_new,
        ),
    ]

    # Apply --only filtering
    if only:
        allow = {
            "ch2": {"Ch2: summarize learning curves (Int/Rel/IntRel)", "Ch2: summarize symmetric-unbalance mirror curves"},
            "ch2_sym": {"Ch2: summarize symmetric-unbalance mirror curves"},
            "ch3_old": {"Ch3 old model: consolidate + summarize posterior shares"},
            "ch3_new": {"Ch3 new model: summarize IntP-RelP difference curves"},
        }
        wanted_names = set()
        unknown = [x for x in only if x not in allow]
        if unknown:
            raise SystemExit(f"Unknown --only value(s): {unknown}. Choices: {sorted(allow)}")
        for key in only:
            wanted_names |= allow[key]
        tasks = [t for t in tasks if t.name in wanted_names]

    # Run
    for t in tasks:
        _run(t)

    print("\n[DONE] All requested preprocessing steps completed.")


if __name__ == "__main__":
    main()
