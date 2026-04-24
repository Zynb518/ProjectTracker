import importlib.util
import io
import json
from pathlib import Path

from ai_project_manager.schemas import GeneratedPlan, ProjectDraft, ProjectNodeDraft, SubTaskDraft


def _load_generate_worker_module():
    script_path = Path(__file__).resolve().parents[1] / "scripts" / "generate_worker.py"
    spec = importlib.util.spec_from_file_location("generate_worker_script", script_path)
    if spec is None or spec.loader is None:
        raise AssertionError("failed to load generate_worker.py module spec")

    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_generate_worker_script_loads_generator_once_and_returns_jsonl(tmp_path, monkeypatch):
    module = _load_generate_worker_module()
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

    def fake_load_local_generator(model_dir: Path):
        captured["model_dir"] = model_dir
        captured["load_calls"] = captured.get("load_calls", 0) + 1
        return "fake-generator"

    def fake_generate_plan_with_local_generator(generator: object, requirement: str) -> GeneratedPlan:
        captured["generator"] = generator
        captured["requirement"] = requirement
        captured["generate_calls"] = captured.get("generate_calls", 0) + 1
        return expected_plan

    monkeypatch.setattr(module, "load_local_generator", fake_load_local_generator)
    monkeypatch.setattr(
        module,
        "generate_plan_with_local_generator",
        fake_generate_plan_with_local_generator,
    )

    request_line = json.dumps(
        {
            "request_id": "req-1",
            "prompt": "做一个官网改版项目",
        },
        ensure_ascii=False,
    )
    fake_stdin = io.StringIO(f"{request_line}\n")
    fake_stdout = io.StringIO()
    monkeypatch.setattr(module.sys, "stdin", fake_stdin)
    monkeypatch.setattr(module.sys, "stdout", fake_stdout)

    model_dir = tmp_path / "model"
    model_dir.mkdir()

    exit_code = module.main(
        [
            "--model-dir",
            str(model_dir),
        ]
    )

    output_lines = fake_stdout.getvalue().strip().splitlines()
    assert exit_code == 0
    assert captured["model_dir"] == model_dir
    assert captured["load_calls"] == 1
    assert captured["generator"] == "fake-generator"
    assert captured["generate_calls"] == 1
    assert captured["requirement"] == "做一个官网改版项目"
    assert len(output_lines) == 1

    payload = json.loads(output_lines[0])
    assert payload["request_id"] == "req-1"
    assert payload["ok"] is True
    assert payload["draft"]["project"]["name"] == "官网改版项目"
    assert payload["draft"]["nodes"][0]["name"] == "设计阶段"
