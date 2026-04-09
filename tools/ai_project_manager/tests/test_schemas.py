import pytest

from ai_project_manager.schemas import GeneratedPlan, ProjectDraft, ProjectNodeDraft, SubTaskDraft


def test_subtask_defaults_and_optional_responsible_user_id():
    subtask = SubTaskDraft(
        name="首页视觉稿",
        planned_start_date="2026-05-01",
        planned_end_date="2026-05-07",
    )

    assert subtask.responsible_user_id is None
    assert subtask.priority == 2
    assert subtask.description == ""


def test_subtask_accepts_integer_responsible_user_id():
    subtask = SubTaskDraft(
        name="首页视觉稿",
        responsible_user_id=42,
        planned_start_date="2026-05-01",
        planned_end_date="2026-05-07",
    )

    assert subtask.responsible_user_id == 42


def test_description_defaults_to_empty_string_when_omitted():
    project = ProjectDraft(
        name="官网改版项目",
        planned_start_date="2026-05-01",
        planned_end_date="2026-06-15",
    )
    node = ProjectNodeDraft(
        name="设计阶段",
        planned_start_date="2026-05-01",
        planned_end_date="2026-05-10",
    )
    subtask = SubTaskDraft(
        name="首页视觉稿",
        planned_start_date="2026-05-01",
        planned_end_date="2026-05-05",
    )

    assert project.description == ""
    assert node.description == ""
    assert subtask.description == ""


def test_invalid_priority_raises():
    with pytest.raises(ValueError):
        SubTaskDraft(
            name="首页视觉稿",
            priority=5,
            planned_start_date="2026-05-01",
            planned_end_date="2026-05-07",
        )


def test_generated_plan_defaults():
    plan = GeneratedPlan(
        project=ProjectDraft(
            name="官网改版项目",
            planned_start_date="2026-05-01",
            planned_end_date="2026-06-15",
        )
    )

    assert plan.nodes == []
