from ai_project_manager.schemas import GeneratedPlan, ProjectDraft, ProjectNodeDraft, SubTaskDraft
from ai_project_manager.validator import validate_generated_plan


def _project(**overrides) -> ProjectDraft:
    data = {
        "name": "官网改版项目",
        "planned_start_date": "2026-05-01",
        "planned_end_date": "2026-06-15",
    }
    data.update(overrides)
    return ProjectDraft(**data)


def _node(**overrides) -> ProjectNodeDraft:
    data = {
        "name": "设计阶段",
        "planned_start_date": "2026-05-01",
        "planned_end_date": "2026-05-15",
    }
    data.update(overrides)
    return ProjectNodeDraft(**data)


def _subtask(**overrides) -> SubTaskDraft:
    data = {
        "name": "首页视觉稿",
        "planned_start_date": "2026-05-01",
        "planned_end_date": "2026-05-07",
    }
    data.update(overrides)
    return SubTaskDraft(**data)


def test_generated_plan_accepts_optional_responsible_user_id():
    plan = GeneratedPlan(
        project=_project(),
        nodes=[_node(subtasks=[_subtask(responsible_user_id=7)])],
    )

    errors = validate_generated_plan(plan)
    assert errors == []


def test_generated_plan_rejects_node_dates_outside_project_range():
    plan = GeneratedPlan(
        project=_project(),
        nodes=[_node(planned_start_date="2026-04-29")],
    )

    errors = validate_generated_plan(plan)
    assert "node[0] date range must stay within project range" in errors


def test_invalid_project_date_range():
    plan = GeneratedPlan(
        project=_project(planned_end_date="2026-04-15"),
    )

    errors = validate_generated_plan(plan)
    assert errors == ["project date range is invalid"]


def test_reversed_node_range_reports_error():
    plan = GeneratedPlan(
        project=_project(),
        nodes=[_node(planned_start_date="2026-05-15", planned_end_date="2026-05-01")],
    )

    errors = validate_generated_plan(plan)
    assert "node[0] date range must stay within project range" in errors


def test_invalid_subtask_date_range_reports_error():
    plan = GeneratedPlan(
        project=_project(),
        nodes=[_node(subtasks=[_subtask(planned_start_date="2026-04-20")])],
    )

    errors = validate_generated_plan(plan)
    assert "node[0].subtasks[0] date range must stay within node range" in errors


def test_reversed_subtask_range_reports_error():
    plan = GeneratedPlan(
        project=_project(),
        nodes=[
            _node(
                subtasks=[
                    _subtask(planned_start_date="2026-05-10", planned_end_date="2026-05-05")
                ]
            )
        ],
    )

    errors = validate_generated_plan(plan)
    assert "node[0].subtasks[0] date range must stay within node range" in errors
