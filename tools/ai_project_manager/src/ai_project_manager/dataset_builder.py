from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Iterable

from pydantic import ValidationError

from ai_project_manager.schemas import GeneratedPlan
from ai_project_manager.validator import validate_generated_plan

PREFIX = (
    "请根据以下需求生成项目计划。"
    "输出格式必须为行文本："
    "第一行 PROJECT|项目名|项目描述|开始日期|结束日期，"
    "随后每个阶段输出 NODE|阶段名|阶段描述|开始日期|结束日期，"
    "阶段下的任务输出 TASK|任务名|任务描述|优先级|开始日期|结束日期，"
    "最后输出 END。"
    "不要输出解释。需求："
)


def build_training_records(source: Path) -> list[dict[str, str]]:
    records: list[dict[str, str]] = []
    with source.open("r", encoding="utf-8") as handle:
        for line_no, line in enumerate(handle, start=1):
            if not line.strip():
                continue
            try:
                row: dict[str, Any] = json.loads(line)
            except json.JSONDecodeError as exc:
                raise ValueError(f"Invalid JSON at line {line_no}: {exc}") from exc
            records.append(_build_record(row, line_no))
    return records


def _build_record(row: dict[str, Any], line_no: int) -> dict[str, str]:
    if not isinstance(row, dict):
        raise ValueError(f"Invalid seed format at line {line_no}: expected JSON object")

    requirement = row.get("requirement")
    plan_raw = row.get("plan")
    if requirement is None or plan_raw is None:
        raise ValueError(f"Missing requirement or plan at line {line_no}")
    if not isinstance(requirement, str):
        raise ValueError(f"Requirement must be a string at line {line_no}")

    plan = _validate_plan(plan_raw, line_no)
    return {
        "input": f"{PREFIX}{requirement}",
        "output": json.dumps(plan.model_dump(), ensure_ascii=False),
    }


def _validate_plan(plan_raw: Any, line_no: int) -> GeneratedPlan:
    try:
        plan = GeneratedPlan.model_validate(plan_raw)
    except ValidationError as exc:
        raise ValueError(f"Invalid plan at line {line_no}: {exc}") from exc

    errors = validate_generated_plan(plan)
    if errors:
        raise ValueError(f"Invalid plan at line {line_no}: {', '.join(errors)}")

    return plan


def write_training_records(records: Iterable[dict[str, str]], destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    with destination.open("w", encoding="utf-8") as handle:
        for record in records:
            handle.write(json.dumps(record, ensure_ascii=False))
            handle.write("\n")
