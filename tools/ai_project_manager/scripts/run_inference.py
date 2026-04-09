from __future__ import annotations

import argparse
import sys
from pathlib import Path

PACKAGE_ROOT = Path(__file__).resolve().parents[1]
SRC_DIR = PACKAGE_ROOT / "src"
if str(SRC_DIR) not in sys.path:
    sys.path.insert(0, str(SRC_DIR))

from ai_project_manager.model_generator import generate_plan_from_local_model
from ai_project_manager.normalizer import plan_to_json


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="run_inference.py", description="Run local model inference."
    )
    parser.add_argument(
        "--model-dir",
        type=Path,
        required=True,
        help="Path to the local model directory.",
    )
    parser.add_argument(
        "--requirement",
        required=True,
        help="Requirement prompt to generate a project plan for.",
    )
    return parser


def main() -> None:
    args = build_parser().parse_args()
    plan = generate_plan_from_local_model(args.model_dir, args.requirement)
    print(plan_to_json(plan))


if __name__ == "__main__":
    main()
