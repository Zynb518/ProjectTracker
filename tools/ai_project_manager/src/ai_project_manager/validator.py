from __future__ import annotations

from datetime import date
from typing import List

from ai_project_manager.schemas import GeneratedPlan


def validate_generated_plan(plan: GeneratedPlan) -> List[str]:
    errors: List[str] = []

    project_start = _parse_iso_date(plan.project.planned_start_date)
    project_end = _parse_iso_date(plan.project.planned_end_date)

    if project_start is None or project_end is None:
        errors.append("project date range is invalid")
        return errors

    if project_end < project_start:
        errors.append("project date range is invalid")
        return errors

    for node_index, node in enumerate(plan.nodes):
        node_start = _parse_iso_date(node.planned_start_date)
        node_end = _parse_iso_date(node.planned_end_date)

        if node_start is None or node_end is None:
            errors.append(f"node[{node_index}] date range must stay within project range")
            continue

        if (
            node_start < project_start
            or node_end > project_end
            or node_end < node_start
        ):
            errors.append(f"node[{node_index}] date range must stay within project range")
            continue

        for sub_index, subtask in enumerate(node.subtasks):
            sub_start = _parse_iso_date(subtask.planned_start_date)
            sub_end = _parse_iso_date(subtask.planned_end_date)

            if sub_start is None or sub_end is None:
                errors.append(
                    f"node[{node_index}].subtasks[{sub_index}] date range must stay within node range"
                )
                continue

            if (
                sub_start < node_start
                or sub_end > node_end
                or sub_end < sub_start
            ):
                errors.append(
                    f"node[{node_index}].subtasks[{sub_index}] date range must stay within node range"
                )

    return errors


def _parse_iso_date(value: str) -> date | None:
    try:
        return date.fromisoformat(value)
    except ValueError:
        return None
