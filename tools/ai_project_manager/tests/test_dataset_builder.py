import json
from pathlib import Path

import pytest

from ai_project_manager.dataset_builder import (
    PREFIX,
    build_training_records,
    write_training_records,
)
from ai_project_manager.schemas import GeneratedPlan


COMMON_PLAN = {
    "project": {
        "name": "官网改版项目",
        "description": "通过新版官网提升转化率",
        "planned_start_date": "2026-05-01",
        "planned_end_date": "2026-06-20",
    },
    "nodes": [
        {
            "name": "需求调研",
            "description": "访谈客户与销售分析",
            "planned_start_date": "2026-05-01",
            "planned_end_date": "2026-05-10",
            "subtasks": [
                {
                    "name": "用户访谈",
                    "description": "收集使用痛点",
                    "priority": 1,
                    "planned_start_date": "2026-05-01",
                    "planned_end_date": "2026-05-03",
                },
                {
                    "name": "竞品调研",
                    "description": "整理竞品 UI/流程",
                    "priority": 2,
                    "planned_start_date": "2026-05-04",
                    "planned_end_date": "2026-05-07",
                },
            ],
        },
        {
            "name": "界面设计",
            "description": "确定视觉与交互规范",
            "planned_start_date": "2026-05-11",
            "planned_end_date": "2026-05-20",
            "subtasks": [
                {
                    "name": "高保真原型",
                    "description": "覆盖核心路径",
                    "priority": 2,
                    "planned_start_date": "2026-05-11",
                    "planned_end_date": "2026-05-14",
                }
            ],
        },
    ],
}


def _write_seed(tmp_path: Path, *plans: dict[str, object]) -> Path:
    source = tmp_path / "seed.jsonl"
    lines = [
        json.dumps({"requirement": f"需求 {idx}", "plan": plan}, ensure_ascii=False)
        for idx, plan in enumerate(plans, start=1)
    ]
    # include blank line between entries to ensure skipping works
    source.write_text("\n\n".join(lines) + "\n", encoding="utf-8")
    return source


def test_build_training_records_outputs_expected_prompt_and_json(tmp_path: Path):
    source = _write_seed(tmp_path, COMMON_PLAN)
    records = build_training_records(source)
    assert len(records) == 1

    assert records[0]["input"] == f"{PREFIX}需求 1"

    plan = GeneratedPlan.model_validate(COMMON_PLAN)
    assert json.loads(records[0]["output"]) == plan.model_dump()


def test_build_training_records_skips_blank_lines(tmp_path: Path):
    source = _write_seed(tmp_path, COMMON_PLAN, COMMON_PLAN)
    records = build_training_records(source)
    assert len(records) == 2


def test_build_training_records_rejects_invalid_plan(tmp_path: Path):
    invalid_plan = {
        "project": {
            "name": "官网改版项目",
            "description": "计划结束早于开始",
            "planned_start_date": "2026-06-01",
            "planned_end_date": "2026-05-30",
        },
        "nodes": [],
    }
    source = _write_seed(tmp_path, invalid_plan)
    with pytest.raises(ValueError) as excinfo:
        build_training_records(source)
    assert "line 1" in str(excinfo.value)
    assert "project date range is invalid" in str(excinfo.value)


def test_write_training_records_generates_jsonl(tmp_path: Path):
    record = {
        "input": f"{PREFIX}需求 1",
        "output": json.dumps(GeneratedPlan.model_validate(COMMON_PLAN).model_dump(), ensure_ascii=False),
    }
    destination = tmp_path / "processed" / "train.jsonl"
    write_training_records([record], destination)

    lines = destination.read_text(encoding="utf-8").strip().splitlines()
    assert lines == [json.dumps(record, ensure_ascii=False)]


def test_repository_seed_samples_cover_short_and_long_projects():
    source = Path(__file__).resolve().parents[1] / "data" / "raw" / "seed_samples.jsonl"

    records = build_training_records(source)

    assert len(records) >= 12

    node_counts = [
        len(json.loads(record["output"])["nodes"])
        for record in records
    ]
    assert any(count <= 3 for count in node_counts)
    assert any(count >= 6 for count in node_counts)
