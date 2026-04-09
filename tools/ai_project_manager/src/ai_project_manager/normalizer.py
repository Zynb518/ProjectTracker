from __future__ import annotations

import json
from datetime import date

from ai_project_manager.schemas import (
    GeneratedPlan,
    ProjectDraft,
    ProjectNodeDraft,
    SubTaskDraft,
)
from ai_project_manager.validator import validate_generated_plan

_DEFAULT_FALLBACK_DATE = date(2026, 1, 1)


def normalize_generated_plan(plan: GeneratedPlan) -> GeneratedPlan:
    project_start = _parse_date(
        plan.project.planned_start_date, fallback=_DEFAULT_FALLBACK_DATE
    )
    project_end = _parse_date(
        plan.project.planned_end_date, fallback=project_start
    )
    if project_end < project_start:
        project_start, project_end = project_end, project_start

    normalized_nodes: list[ProjectNodeDraft] = []
    for index, node in enumerate(plan.nodes):
        normalized_nodes.append(
            _normalize_node(
                node=node,
                project_start=project_start,
                project_end=project_end,
                index=index,
            )
        )

    return GeneratedPlan(
        project=ProjectDraft(
            name=plan.project.name or "项目计划",
            description=plan.project.description,
            planned_start_date=project_start.isoformat(),
            planned_end_date=project_end.isoformat(),
        ),
        nodes=normalized_nodes,
    )


def plan_to_json(plan: GeneratedPlan) -> str:
    errors = validate_generated_plan(plan)
    if errors:
        raise ValueError("; ".join(errors))
    return json.dumps(plan.model_dump(exclude_none=True), indent=2, ensure_ascii=False)


def _normalize_node(
    node: ProjectNodeDraft,
    project_start: date,
    project_end: date,
    index: int,
) -> ProjectNodeDraft:
    node_start = _parse_date(node.planned_start_date, fallback=project_start)
    node_end = _parse_date(node.planned_end_date, fallback=project_end)
    if node_end < node_start:
        node_start, node_end = node_end, node_start

    node_start = max(node_start, project_start)
    node_end = min(node_end, project_end)
    if node_end < node_start:
        node_start = project_start
        node_end = project_end

    subtasks = node.subtasks or [
        SubTaskDraft(
            name=f"{node.name or f'阶段{index + 1}'}任务",
            planned_start_date=node_start.isoformat(),
            planned_end_date=node_end.isoformat(),
        )
    ]

    normalized_subtasks: list[SubTaskDraft] = []
    for sub_index, subtask in enumerate(subtasks):
        sub_start = _parse_date(subtask.planned_start_date, fallback=node_start)
        sub_end = _parse_date(subtask.planned_end_date, fallback=node_end)
        if sub_end < sub_start:
            sub_start, sub_end = sub_end, sub_start

        sub_start = max(sub_start, node_start)
        sub_end = min(sub_end, node_end)
        if sub_end < sub_start:
            sub_start = node_start
            sub_end = node_start

        priority = subtask.priority if subtask.priority in {1, 2, 3} else 2
        normalized_subtasks.append(
            SubTaskDraft(
                name=subtask.name or f"{node.name or f'阶段{index + 1}'}子任务{sub_index + 1}",
                description=subtask.description,
                responsible_user_id=subtask.responsible_user_id,
                priority=priority,
                planned_start_date=sub_start.isoformat(),
                planned_end_date=sub_end.isoformat(),
            )
        )

    return ProjectNodeDraft(
        name=node.name or f"阶段{index + 1}",
        description=node.description,
        planned_start_date=node_start.isoformat(),
        planned_end_date=node_end.isoformat(),
        subtasks=normalized_subtasks,
    )


def _parse_date(value: str, fallback: date) -> date:
    try:
        return date.fromisoformat(value)
    except ValueError:
        return fallback
