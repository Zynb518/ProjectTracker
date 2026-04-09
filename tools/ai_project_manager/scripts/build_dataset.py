from __future__ import annotations

import sys
from pathlib import Path

PACKAGE_ROOT = Path(__file__).resolve().parents[1]
SRC_DIR = PACKAGE_ROOT / "src"
if str(SRC_DIR) not in sys.path:
    sys.path.insert(0, str(SRC_DIR))

from ai_project_manager.dataset_builder import (
    build_training_records,
    write_training_records,
)

RAW = PACKAGE_ROOT / "data" / "raw" / "seed_samples.jsonl"
PROCESSED_DIR = PACKAGE_ROOT / "data" / "processed"
OUTPUT = PROCESSED_DIR / "train.jsonl"


def main() -> None:
    PROCESSED_DIR.mkdir(parents=True, exist_ok=True)
    records = build_training_records(RAW)
    write_training_records(records, OUTPUT)
    print(f"wrote {len(records)} records to {OUTPUT}")


if __name__ == "__main__":
    main()
