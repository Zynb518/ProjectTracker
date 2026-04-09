import json

import pytest

from ai_project_manager.cli import main
from ai_project_manager.heuristic_generator import generate_plan_from_requirement
from ai_project_manager.normalizer import plan_to_json
from ai_project_manager.schemas import GeneratedPlan, ProjectDraft, ProjectNodeDraft


def test_normalizer_fills_default_priority_and_sorts_dates():
    plan = generate_plan_from_requirement(
        "做一个内部管理后台项目，分需求分析、开发实现、验收上线三个阶段"
    )

    first_task = plan.nodes[0].subtasks[0]
    assert first_task.priority in {1, 2, 3}
    assert plan.project.planned_start_date <= plan.project.planned_end_date


def test_plan_to_json_returns_pretty_validated_json():
    plan = generate_plan_from_requirement(
        "做一个公司官网改版项目，5月开始，6月中结束，包含设计、开发、测试上线阶段"
    )

    output = plan_to_json(plan)
    payload = json.loads(output)

    assert output.startswith("{\n  ")
    assert "project" in payload
    assert "nodes" in payload
    assert payload["nodes"][0]["subtasks"]


def test_plan_to_json_raises_value_error_for_invalid_plan():
    invalid = GeneratedPlan(
        project=ProjectDraft(
            name="坏计划",
            planned_start_date="2026-05-01",
            planned_end_date="2026-05-31",
        ),
        nodes=[
            ProjectNodeDraft(
                name="越界阶段",
                planned_start_date="2026-06-01",
                planned_end_date="2026-06-10",
            )
        ],
    )

    with pytest.raises(ValueError, match="node\\[0\\] date range must stay within project range"):
        plan_to_json(invalid)


def test_cli_generate_heuristic_prints_json_and_returns_zero(capsys: pytest.CaptureFixture[str]):
    exit_code = main(
        [
            "generate",
            "--generator",
            "heuristic",
            "--requirement",
            "做一个公司官网改版项目，5月开始，6月中结束，包含设计、开发、测试上线阶段",
        ]
    )
    captured = capsys.readouterr()

    assert exit_code == 0
    payload = json.loads(captured.out)
    assert payload["project"]["name"]
    assert payload["nodes"]
    assert payload["nodes"][0]["subtasks"]
