import importlib.util
import json
from pathlib import Path

from ai_project_manager.schemas import GeneratedPlan, ProjectDraft, ProjectNodeDraft, SubTaskDraft


def _load_generate_once_module():
    script_path = Path(__file__).resolve().parents[1] / "scripts" / "generate_once.py"
    spec = importlib.util.spec_from_file_location("generate_once_script", script_path)
    if spec is None or spec.loader is None:
        raise AssertionError("failed to load generate_once.py module spec")

    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_generate_once_script_reads_input_file_and_prints_json(tmp_path, monkeypatch, capsys):
    module = _load_generate_once_module()
    input_path = tmp_path / "request.json"
    input_path.write_text(
        json.dumps({"prompt": "做一个官网改版项目"}, ensure_ascii=False),
        encoding="utf-8",
    )

    expected_plan = GeneratedPlan(
        project=ProjectDraft(
            name="官网改版项目",
            description="用于品牌升级",
            planned_start_date="2026-04-10",
            planned_end_date="2026-05-20",
        ),
        nodes=[
            ProjectNodeDraft(
                name="设计阶段",
                planned_start_date="2026-04-10",
                planned_end_date="2026-04-20",
                subtasks=[
                    SubTaskDraft(
                        name="首页视觉稿",
                        planned_start_date="2026-04-10",
                        planned_end_date="2026-04-12",
                    )
                ],
            )
        ],
    )

    captured: dict[str, object] = {}

    def fake_generate_plan(model_dir: Path, requirement: str) -> GeneratedPlan:
        captured["model_dir"] = model_dir
        captured["requirement"] = requirement
        return expected_plan

    monkeypatch.setattr(module, "generate_plan_from_local_model", fake_generate_plan)

    model_dir = tmp_path / "model"
    model_dir.mkdir()

    module.main(
        [
            "--model-dir",
            str(model_dir),
            "--input-file",
            str(input_path),
        ]
    )

    output = capsys.readouterr().out
    payload = json.loads(output)

    assert captured["model_dir"] == model_dir
    assert captured["requirement"] == "做一个官网改版项目"
    assert payload["project"]["name"] == "官网改版项目"
    assert payload["nodes"][0]["name"] == "设计阶段"
    assert payload["nodes"][0]["subtasks"][0]["name"] == "首页视觉稿"
