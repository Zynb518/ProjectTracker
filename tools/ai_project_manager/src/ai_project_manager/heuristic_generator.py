from __future__ import annotations

import re
from calendar import monthrange
from datetime import date, timedelta

from ai_project_manager.normalizer import normalize_generated_plan
from ai_project_manager.schemas import (
    GeneratedPlan,
    ProjectDraft,
    ProjectNodeDraft,
    SubTaskDraft,
)

_PHASE_CATALOG: list[tuple[str, tuple[str, ...]]] = [
    ("需求分析", ("需求", "分析", "调研")),
    ("设计阶段", ("设计", "原型", "视觉")),
    ("开发实现", ("开发", "实现", "编码")),
    ("测试上线", ("测试", "上线", "验收", "发布")),
]

_DEFAULT_PHASES = ["需求分析", "设计阶段", "开发实现", "测试上线"]

_SUBTASK_TEMPLATES: dict[str, list[str]] = {
    "需求分析": ["梳理业务需求", "确认验收标准"],
    "设计阶段": ["输出界面原型", "评审视觉方案"],
    "开发实现": ["完成核心功能开发", "联调关键流程"],
    "测试上线": ["执行回归测试", "准备发布上线"],
}

_REFERENCE_YEAR = 2026
_DEFAULT_START_MONTH = 1
_DEFAULT_END_MONTH = 2


def generate_plan_from_requirement(requirement: str) -> GeneratedPlan:
    project_start, project_end = _infer_project_dates(requirement)
    phases = _infer_phases(requirement)
    project_name = _infer_project_name(requirement)

    nodes = _build_nodes(
        phases=phases,
        project_start=project_start,
        project_end=project_end,
    )

    raw_plan = GeneratedPlan(
        project=ProjectDraft(
            name=project_name,
            description=requirement,
            planned_start_date=project_start.isoformat(),
            planned_end_date=project_end.isoformat(),
        ),
        nodes=nodes,
    )
    return normalize_generated_plan(raw_plan)


def _infer_project_name(requirement: str) -> str:
    prefix = re.split(r"[，,。；;]|(?:包含|包括|覆盖|分为|分成|分)", requirement, maxsplit=1)[0]
    prefix = prefix.strip()

    for verb in ("做一个", "做一套", "做个", "规划一个", "上线一个", "建设", "搭建", "发布", "改版"):
        if prefix.startswith(verb):
            prefix = prefix[len(verb) :].strip()
            break

    if not prefix:
        return "项目执行计划"
    if prefix.endswith("项目"):
        return prefix
    return f"{prefix}项目"


def _infer_project_dates(requirement: str) -> tuple[date, date]:
    year = _REFERENCE_YEAR
    month_numbers = [int(value) for value in re.findall(r"(\d{1,2})月", requirement)]

    if not month_numbers:
        start_month = _DEFAULT_START_MONTH
        end_month = _DEFAULT_END_MONTH
    elif len(month_numbers) == 1:
        start_month = month_numbers[0]
        end_month = min(12, start_month + 1)
    else:
        start_month = month_numbers[0]
        end_month = month_numbers[1]

    start_month = _clamp_month(start_month)
    end_month = _clamp_month(end_month)
    start_date = date(year, start_month, 1)
    end_day = 15 if ("中结束" in requirement or "中旬" in requirement) else monthrange(year, end_month)[1]
    end_date = date(year, end_month, end_day)
    if end_date < start_date:
        end_date = date(year + 1, end_month, end_day)
    return start_date, end_date


def _infer_phases(requirement: str) -> list[str]:
    explicit_phases = infer_requirement_phases(requirement)
    if len(explicit_phases) >= 2:
        return explicit_phases

    selected: list[str] = []
    for phase_name, keywords in _PHASE_CATALOG:
        if any(keyword in requirement for keyword in keywords):
            selected.append(phase_name)

    if len(selected) < 3:
        for fallback in _DEFAULT_PHASES:
            if fallback not in selected:
                selected.append(fallback)
            if len(selected) >= 3:
                break
    return selected


def _build_nodes(
    phases: list[str],
    project_start: date,
    project_end: date,
) -> list[ProjectNodeDraft]:
    total_days = (project_end - project_start).days + 1
    phase_count = max(1, len(phases))
    step = max(1, total_days // phase_count)
    nodes: list[ProjectNodeDraft] = []

    for phase_index, phase_name in enumerate(phases):
        node_start = project_start + timedelta(days=phase_index * step)
        if phase_index == phase_count - 1:
            node_end = project_end
        else:
            node_end = min(project_end, node_start + timedelta(days=step - 1))

        subtasks = _build_subtasks(
            phase_name=phase_name,
            node_start=node_start,
            node_end=node_end,
        )
        nodes.append(
            ProjectNodeDraft(
                name=phase_name,
                description=f"{phase_name}执行阶段",
                planned_start_date=node_start.isoformat(),
                planned_end_date=node_end.isoformat(),
                subtasks=subtasks,
            )
        )
    return nodes


def _build_subtasks(
    phase_name: str,
    node_start: date,
    node_end: date,
) -> list[SubTaskDraft]:
    task_names = _SUBTASK_TEMPLATES.get(phase_name, _build_generic_subtask_names(phase_name))
    subtask_count = len(task_names)
    node_days = (node_end - node_start).days + 1
    sub_step = max(1, node_days // max(1, subtask_count))

    subtasks: list[SubTaskDraft] = []
    for sub_index, name in enumerate(task_names):
        sub_start = node_start + timedelta(days=sub_index * sub_step)
        if sub_index == subtask_count - 1:
            sub_end = node_end
        else:
            sub_end = min(node_end, sub_start + timedelta(days=sub_step - 1))
        subtasks.append(
            SubTaskDraft(
                name=name,
                description=f"{phase_name}中的{name}",
                priority=2 if sub_index == 0 else 1,
                planned_start_date=sub_start.isoformat(),
                planned_end_date=sub_end.isoformat(),
            )
        )
    return subtasks


def _clamp_month(month: int) -> int:
    return max(1, min(12, month))


def infer_requirement_phases(requirement: str) -> list[str]:
    phase_text = ""
    for marker in ("包含", "包括", "覆盖", "分为", "分成", "分"):
        if marker in requirement:
            phase_text = requirement.split(marker, 1)[1]
            break

    if not phase_text:
        return []

    phase_text = phase_text.strip().strip("。；; ")
    raw_parts = re.split(r"[、,，]|(?:和|及|与)", phase_text)

    phases: list[str] = []
    for raw_part in raw_parts:
        phase = _clean_phase_name(raw_part)
        if not phase or phase in phases:
            continue
        phases.append(phase)

    return phases


def _clean_phase_name(value: str) -> str:
    phase = value.strip().strip("。；; ")
    phase = re.sub(r"(?:[一二三四五六七八九十0-9]+个)?阶段$", "", phase)
    phase = re.sub(r"(?:等|等内容|等能力)$", "", phase)
    return phase.strip()


def _build_generic_subtask_names(phase_name: str) -> list[str]:
    return [
        f"梳理{phase_name}范围",
        f"完成{phase_name}执行",
    ]
