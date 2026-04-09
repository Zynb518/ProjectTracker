from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Sequence

PACKAGE_ROOT = Path(__file__).resolve().parents[1]
SRC_DIR = PACKAGE_ROOT / "src"
if str(SRC_DIR) not in sys.path:
    sys.path.insert(0, str(SRC_DIR))

from ai_project_manager.model_generator import generate_plan_from_local_model
from ai_project_manager.normalizer import plan_to_json


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="generate_once.py",
        description="Generate one project draft JSON from a local model.",
    )
    parser.add_argument(
        "--model-dir",
        type=Path,
        required=True,
        help="Path to the local model directory.",
    )
    parser.add_argument(
        "--input-file",
        type=Path,
        required=True,
        help="Path to a JSON request file containing the prompt field.",
    )
    return parser


def read_prompt(input_file: Path) -> str:
    payload = json.loads(input_file.read_text(encoding="utf-8"))
    prompt = payload.get("prompt")
    if not isinstance(prompt, str) or not prompt.strip():
        raise ValueError("prompt must be a non-empty string")
    return prompt.strip()


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    prompt = read_prompt(args.input_file)
    plan = generate_plan_from_local_model(args.model_dir, prompt)
    print(plan_to_json(plan))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
