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

from ai_project_manager.model_generator import (
    generate_plan_with_local_generator,
    load_local_generator,
)
from ai_project_manager.normalizer import plan_to_payload


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="generate_worker.py",
        description="Run a persistent local model worker for project draft generation.",
    )
    parser.add_argument(
        "--model-dir",
        type=Path,
        required=True,
        help="Path to the local model directory.",
    )
    return parser


def parse_request_line(raw_line: str) -> tuple[str, str]:
    payload = json.loads(raw_line)
    request_id = payload.get("request_id")
    prompt = payload.get("prompt")

    if not isinstance(request_id, str) or not request_id.strip():
        raise ValueError("request_id must be a non-empty string")

    if not isinstance(prompt, str) or not prompt.strip():
        raise ValueError("prompt must be a non-empty string")

    return request_id.strip(), prompt.strip()


def emit_response(payload: dict[str, object]) -> None:
    print(json.dumps(payload, ensure_ascii=False), flush=True)


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    generator = load_local_generator(args.model_dir)

    for raw_line in sys.stdin:
        line = raw_line.strip()
        if not line:
            continue

        request_id = ""
        try:
            request_id, prompt = parse_request_line(line)
            plan = generate_plan_with_local_generator(generator, prompt)
            emit_response(
                {
                    "request_id": request_id,
                    "ok": True,
                    "draft": plan_to_payload(plan),
                }
            )
        except Exception as exc:  # pragma: no cover - worker must always answer
            emit_response(
                {
                    "request_id": request_id,
                    "ok": False,
                    "error": str(exc),
                }
            )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
