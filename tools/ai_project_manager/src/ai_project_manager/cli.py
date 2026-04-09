import argparse
from pathlib import Path
from typing import Sequence

from ai_project_manager.heuristic_generator import generate_plan_from_requirement
from ai_project_manager.model_generator import generate_plan_from_local_model
from ai_project_manager.normalizer import plan_to_json

DEFAULT_MODEL_DIR = Path(__file__).resolve().parents[2] / "models" / "mt5-small-offline"


def build_parser() -> argparse.ArgumentParser:
    """Build the CLI argument parser for the offline manager."""
    parser = argparse.ArgumentParser(prog="ai-project-manager")
    subparsers = parser.add_subparsers(dest="command", required=True)

    generate = subparsers.add_parser(
        "generate", help="Generate an action plan for the supplied requirement."
    )
    generate.add_argument(
        "--requirement",
        required=True,
        help="Describe the project requirement that needs planning.",
    )
    generate.add_argument(
        "--generator",
        choices=["heuristic", "model"],
        default="heuristic",
        help="Choose whether to invoke the heuristic or trained generator.",
        dest="generator",
    )
    generate.add_argument(
        "--model-dir",
        type=Path,
        default=DEFAULT_MODEL_DIR,
        help="Directory of the locally trained seq2seq model for --generator model.",
    )

    return parser


def main(argv: Sequence[str] | None = None) -> int:
    """Entry point used by console scripts."""
    parser = build_parser()
    args = parser.parse_args(argv)

    if args.command == "generate":
        if args.generator == "heuristic":
            plan = generate_plan_from_requirement(args.requirement)
            print(plan_to_json(plan))
            return 0
        if args.generator == "model":
            plan = generate_plan_from_local_model(args.model_dir, args.requirement)
            print(plan_to_json(plan))
            return 0

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
