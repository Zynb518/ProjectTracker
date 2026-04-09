from __future__ import annotations

import re

from ai_project_manager.schemas import (
    GeneratedPlan,
    ProjectDraft,
    ProjectNodeDraft,
    SubTaskDraft,
)


def serialize_plan_for_model(plan: GeneratedPlan) -> str:
    lines = [
        _join_fields(
            "PROJECT",
            plan.project.name,
            plan.project.description,
            plan.project.planned_start_date,
            plan.project.planned_end_date,
        )
    ]

    for node in plan.nodes:
        lines.append(
            _join_fields(
                "NODE",
                node.name,
                node.description,
                node.planned_start_date,
                node.planned_end_date,
            )
        )
        for subtask in node.subtasks:
            lines.append(
                _join_fields(
                    "TASK",
                    subtask.name,
                    subtask.description,
                    str(subtask.priority),
                    subtask.planned_start_date,
                    subtask.planned_end_date,
                )
            )

    lines.append("END")
    return "\n".join(lines)


def parse_model_plan_text(text: str) -> GeneratedPlan:
    project: ProjectDraft | None = None
    nodes: list[ProjectNodeDraft] = []
    current_node: ProjectNodeDraft | None = None
    pending_subtasks: list[SubTaskDraft] = []

    for raw_line in _split_records(text):
        line = raw_line.strip()
        if not line:
            continue
        if line == "END":
            break

        parts = _split_fields(line)
        record_type = parts[0]

        if record_type == "PROJECT":
            project = _build_project(parts)
            continue

        if record_type == "NODE":
            current_node = _build_node(parts)
            if pending_subtasks:
                current_node.subtasks.extend(pending_subtasks)
                pending_subtasks = []
            nodes.append(current_node)
            continue

        if record_type == "TASK":
            subtask = _build_task(parts)
            if current_node is None:
                pending_subtasks.append(subtask)
            else:
                current_node.subtasks.append(subtask)
            continue

        raise ValueError(f"unsupported record type: {record_type}")

    if project is None:
        raise ValueError("missing PROJECT line")

    if pending_subtasks:
        nodes.append(
            ProjectNodeDraft(
                name="执行阶段",
                description="根据模型输出恢复的默认阶段",
                planned_start_date=project.planned_start_date,
                planned_end_date=project.planned_end_date,
                subtasks=pending_subtasks,
            )
        )

    return GeneratedPlan(project=project, nodes=nodes)


def _join_fields(*fields: str) -> str:
    return "|".join(_escape_field(field) for field in fields)


def _escape_field(value: str) -> str:
    return value.replace("\\", "\\\\").replace("\n", "\\n").replace("|", "\\|")


def _split_fields(line: str) -> list[str]:
    fields: list[str] = []
    current: list[str] = []
    escaped = False

    for char in line:
        if escaped:
            if char == "n":
                current.append("\n")
            else:
                current.append(char)
            escaped = False
            continue

        if char == "\\":
            escaped = True
            continue

        if char == "|":
            fields.append("".join(current))
            current = []
            continue

        current.append(char)

    if escaped:
        current.append("\\")

    fields.append("".join(current))
    return fields


def _parse_priority(value: str) -> int:
    try:
        return int(value)
    except ValueError as exc:
        raise ValueError(f"invalid task priority: {value}") from exc


def _split_records(text: str) -> list[str]:
    normalized = text.replace("\r\n", "\n").replace("\r", "\n")
    normalized = re.sub(r"\s+(?=(?:PROJECT|NODE|TASK)\|)", "\n", normalized)
    normalized = re.sub(r"\s+(?=END(?:\s|$))", "\n", normalized)
    return normalized.splitlines()


def _build_project(parts: list[str]) -> ProjectDraft:
    values = parts[1:]
    if not values:
        raise ValueError("PROJECT line must contain a project name")
    name = values[0]
    description_fields, start_date, end_date = _extract_description_and_dates(values[1:])
    return ProjectDraft(
        name=name,
        description="|".join(description_fields),
        planned_start_date=start_date,
        planned_end_date=end_date,
    )


def _build_node(parts: list[str]) -> ProjectNodeDraft:
    values = parts[1:]
    if not values:
        raise ValueError("NODE line must contain a node name")
    name = values[0]
    description_fields, start_date, end_date = _extract_description_and_dates(values[1:])
    return ProjectNodeDraft(
        name=name,
        description="|".join(description_fields),
        planned_start_date=start_date,
        planned_end_date=end_date,
        subtasks=[],
    )


def _build_task(parts: list[str]) -> SubTaskDraft:
    values = parts[1:]
    if not values:
        raise ValueError("TASK line must contain a task name")

    name = values[0]
    middle_fields, start_date, end_date = _extract_description_and_dates(values[1:])
    description_fields = middle_fields
    priority = 2

    for index in range(len(middle_fields) - 1, -1, -1):
        field = middle_fields[index]
        if field in {"1", "2", "3"}:
            priority = _parse_priority(field)
            description_fields = middle_fields[:index] + middle_fields[index + 1 :]
            break

    return SubTaskDraft(
        name=name,
        description="|".join(description_fields),
        priority=priority,
        planned_start_date=start_date,
        planned_end_date=end_date,
    )


def _extract_description_and_dates(values: list[str]) -> tuple[list[str], str, str]:
    if not values:
        return [], "", ""

    date_indexes = [index for index, value in enumerate(values) if _looks_like_date(value)]
    if len(date_indexes) >= 2:
        start_index = date_indexes[-2]
        end_index = date_indexes[-1]
        start_date = values[start_index]
        end_date = values[end_index]
        description_fields = [
            value
            for index, value in enumerate(values)
            if index not in {start_index, end_index}
        ]
        return description_fields, start_date, end_date

    if len(date_indexes) == 1:
        date_index = date_indexes[0]
        date_value = values[date_index]
        description_fields = [
            value for index, value in enumerate(values) if index != date_index
        ]
        return description_fields, date_value, date_value

    if len(values) >= 2:
        return values[:-2], values[-2], values[-1]

    return [], values[0], values[0]


def _looks_like_date(value: str) -> bool:
    return bool(re.fullmatch(r"\d{4}-\d{2}-\d{2}", value))
