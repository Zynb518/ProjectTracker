import importlib.util
import json
import sys
import types
from pathlib import Path

import pytest

import ai_project_manager.cli as cli_module
import ai_project_manager.model_generator as model_generator_module
from ai_project_manager.model_generator import load_local_generator
from ai_project_manager.model_plan_format import serialize_plan_for_model
from ai_project_manager.schemas import (
    GeneratedPlan,
    ProjectDraft,
    ProjectNodeDraft,
    SubTaskDraft,
)


def _load_train_model_module():
    script_path = Path(__file__).resolve().parents[1] / "scripts" / "train_model.py"
    spec = importlib.util.spec_from_file_location("train_model_script", script_path)
    if spec is None or spec.loader is None:
        raise AssertionError("failed to load train_model.py module spec")

    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_load_local_generator_rejects_missing_model_dir(tmp_path):
    missing = tmp_path / "missing-model"
    try:
        load_local_generator(missing)
    except FileNotFoundError as exc:
        assert "model directory does not exist" in str(exc)
    else:
        raise AssertionError("expected FileNotFoundError")


def test_train_model_script_builds_compatible_training_arguments():
    module = _load_train_model_module()
    training_args = module.build_training_arguments(Path("tmp/checkpoints"))
    assert isinstance(training_args, module.Seq2SeqTrainingArguments)
    assert training_args.num_train_epochs == 64


def test_train_model_script_builds_compatible_trainer_kwargs():
    module = _load_train_model_module()
    trainer_kwargs = module.build_trainer_kwargs(object())
    init_params = module.inspect.signature(module.Seq2SeqTrainer.__init__).parameters
    if "processing_class" in init_params:
        assert "processing_class" in trainer_kwargs
        assert "tokenizer" not in trainer_kwargs
    else:
        assert "tokenizer" in trainer_kwargs


def test_train_model_script_patches_accelerate_unwrap_model_signature(monkeypatch):
    module = _load_train_model_module()
    class FakeAccelerator:
        def __init__(self):
            self.calls: list[tuple[object, bool]] = []

        def unwrap_model(self, model: object, keep_fp32_wrapper: bool = True) -> object:
            self.calls.append((model, keep_fp32_wrapper))
            return model

    monkeypatch.setitem(
        sys.modules,
        "accelerate",
        types.SimpleNamespace(Accelerator=FakeAccelerator),
    )

    patched = module.patch_accelerate_unwrap_model_compat()
    assert patched is True

    accelerator = FakeAccelerator()
    accelerator.unwrap_model("demo-model", keep_torch_compile=False)
    assert accelerator.calls == [("demo-model", False)]


def test_train_model_script_preserves_full_plan_targets():
    module = _load_train_model_module()
    raw_output = json.dumps(
        {
            "project": {
                "name": "复杂项目",
                "planned_start_date": "2026-04-01",
                "planned_end_date": "2026-06-30",
            },
            "nodes": [
                {
                    "name": "阶段一",
                    "planned_start_date": "2026-04-01",
                    "planned_end_date": "2026-04-20",
                    "subtasks": [
                        {
                            "name": "任务一",
                            "planned_start_date": "2026-04-01",
                            "planned_end_date": "2026-04-05",
                        }
                    ],
                }
            ],
        },
        ensure_ascii=False,
    )

    target = module.build_training_target(raw_output)
    lines = target.splitlines()
    assert lines[0].startswith("PROJECT|")
    assert any(line.startswith("NODE|") for line in lines)
    assert any(line.startswith("TASK|") for line in lines)
    assert lines[-1] == "END"


def test_train_model_script_compacts_training_targets():
    module = _load_train_model_module()
    raw_output = json.dumps(
        {
            "project": {
                "name": "复杂项目",
                "planned_start_date": "2026-04-01",
                "planned_end_date": "2026-06-30",
            },
            "nodes": [
                {
                    "name": "阶段一",
                    "planned_start_date": "2026-04-01",
                    "planned_end_date": "2026-04-20",
                    "subtasks": [
                        {
                            "name": "任务一",
                            "planned_start_date": "2026-04-01",
                            "planned_end_date": "2026-04-05",
                        }
                    ],
                }
            ],
        },
        ensure_ascii=False,
    )

    target = module.build_training_target(raw_output)
    assert "responsible_user_id" not in target
    assert "{" not in target
    assert "\"project\"" not in target


def test_load_local_generator_builds_pipeline_with_slow_tokenizer(tmp_path, monkeypatch):
    model_dir = tmp_path / "local-model"
    model_dir.mkdir(parents=True)
    calls: dict[str, object] = {}

    class FakeAutoModelForSeq2SeqLM:
        @staticmethod
        def from_pretrained(path: str) -> str:
            calls["model_path"] = path
            return "fake-model"

    class FakeAutoTokenizer:
        @staticmethod
        def from_pretrained(path: str, *, use_fast: bool) -> str:
            calls["tokenizer_path"] = path
            calls["use_fast"] = use_fast
            return "fake-tokenizer"

    def fake_pipeline(*, task: str, model: object, tokenizer: object) -> object:
        calls["task"] = task
        calls["pipeline_model"] = model
        calls["pipeline_tokenizer"] = tokenizer
        return {"ok": True}

    fake_transformers = types.SimpleNamespace(
        AutoModelForSeq2SeqLM=FakeAutoModelForSeq2SeqLM,
        AutoTokenizer=FakeAutoTokenizer,
        pipeline=fake_pipeline,
    )
    monkeypatch.setitem(sys.modules, "transformers", fake_transformers)

    generator = load_local_generator(model_dir)

    assert generator == {"ok": True}
    assert calls["model_path"] == str(model_dir)
    assert calls["tokenizer_path"] == str(model_dir)
    assert calls["use_fast"] is False
    assert calls["task"] == "text2text-generation"
    assert calls["pipeline_model"] == "fake-model"
    assert calls["pipeline_tokenizer"] == "fake-tokenizer"


def test_generate_plan_from_local_model_formats_prompt_with_training_prefix(monkeypatch):
    captured: dict[str, object] = {}
    generated_plan = GeneratedPlan(
        project=ProjectDraft(
            name="模型计划",
            planned_start_date="2026-05-01",
            planned_end_date="2026-05-31",
        ),
        nodes=[
            ProjectNodeDraft(
                name="阶段一",
                planned_start_date="2026-05-01",
                planned_end_date="2026-05-31",
                subtasks=[
                    SubTaskDraft(
                        name="任务一",
                        planned_start_date="2026-05-01",
                        planned_end_date="2026-05-31",
                    )
                ],
            )
        ],
    )

    def fake_load_local_generator(_: Path):
        def fake_generator(
            prompt: str, **kwargs: object
        ) -> list[dict[str, str]]:
            captured["prompt"] = prompt
            captured["kwargs"] = kwargs
            return [{"generated_text": serialize_plan_for_model(generated_plan)}]

        return fake_generator

    monkeypatch.setattr(
        model_generator_module, "load_local_generator", fake_load_local_generator
    )
    plan = model_generator_module.generate_plan_from_local_model(
        Path("tools/ai_project_manager/models/mt5-small-offline"),
        "发布一个新版官网",
    )

    assert plan.project.name == "模型计划"
    assert captured["prompt"] == f"{model_generator_module.PREFIX}发布一个新版官网"
    kwargs = captured["kwargs"]
    assert isinstance(kwargs, dict)
    assert kwargs["max_new_tokens"] == 768
    assert kwargs["do_sample"] is False
    assert kwargs["no_repeat_ngram_size"] == 4


def test_run_inference_main_prints_validated_plan_json(monkeypatch, capsys, tmp_path):
    script_path = Path(__file__).resolve().parents[1] / "scripts" / "run_inference.py"
    spec = importlib.util.spec_from_file_location("run_inference_script", script_path)
    if spec is None or spec.loader is None:
        raise AssertionError("failed to load run_inference.py module spec")

    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)

    valid_plan = GeneratedPlan(
        project=ProjectDraft(
            name="脚本计划",
            planned_start_date="2026-05-01",
            planned_end_date="2026-05-31",
        ),
        nodes=[
            ProjectNodeDraft(
                name="阶段一",
                planned_start_date="2026-05-01",
                planned_end_date="2026-05-31",
                subtasks=[
                    SubTaskDraft(
                        name="任务一",
                        planned_start_date="2026-05-01",
                        planned_end_date="2026-05-31",
                    )
                ],
            )
        ],
    )
    captured: dict[str, object] = {}

    def fake_generate_plan_from_local_model(model_dir: Path, requirement: str) -> GeneratedPlan:
        captured["model_dir"] = model_dir
        captured["requirement"] = requirement
        return valid_plan

    monkeypatch.setattr(
        module, "generate_plan_from_local_model", fake_generate_plan_from_local_model
    )
    monkeypatch.setattr(
        module.sys,
        "argv",
        [
            "run_inference.py",
            "--model-dir",
            str(tmp_path / "model"),
            "--requirement",
            "发布一个新版官网",
        ],
    )

    module.main()
    out = capsys.readouterr().out

    assert captured["model_dir"] == tmp_path / "model"
    assert captured["requirement"] == "发布一个新版官网"
    payload = json.loads(out)
    assert payload["project"]["name"] == "脚本计划"


def test_cli_model_branch_uses_local_model_path(monkeypatch, capsys, tmp_path):
    valid_plan = GeneratedPlan(
        project=ProjectDraft(
            name="模型计划",
            planned_start_date="2026-05-01",
            planned_end_date="2026-05-31",
        ),
        nodes=[
            ProjectNodeDraft(
                name="阶段一",
                planned_start_date="2026-05-01",
                planned_end_date="2026-05-31",
                subtasks=[
                    SubTaskDraft(
                        name="任务一",
                        planned_start_date="2026-05-01",
                        planned_end_date="2026-05-31",
                    )
                ],
            )
        ],
    )
    captured: dict[str, object] = {}

    def fake_generate_plan_from_local_model(model_dir: Path, requirement: str) -> GeneratedPlan:
        captured["model_dir"] = model_dir
        captured["requirement"] = requirement
        return valid_plan

    monkeypatch.setattr(
        cli_module, "generate_plan_from_local_model", fake_generate_plan_from_local_model
    )

    model_dir = tmp_path / "offline-model"
    exit_code = cli_module.main(
        [
            "generate",
            "--generator",
            "model",
            "--model-dir",
            str(model_dir),
            "--requirement",
            "做一个公司官网改版项目",
        ]
    )
    out = capsys.readouterr().out

    assert exit_code == 0
    assert captured["model_dir"] == model_dir
    assert captured["requirement"] == "做一个公司官网改版项目"
    payload = json.loads(out)
    assert payload["project"]["name"] == "模型计划"


def test_generate_plan_from_local_model_rejects_invalid_json(monkeypatch):
    def fake_load_local_generator(_: Path):
        def fake_generator(
            prompt: str, **kwargs: object
        ) -> list[dict[str, str]]:
            return [{"generated_text": "not-json"}]

        return fake_generator

    monkeypatch.setattr(
        model_generator_module, "load_local_generator", fake_load_local_generator
    )

    with pytest.raises(ValueError, match="model output is not valid plan text"):
        model_generator_module.generate_plan_from_local_model(
            Path("tools/ai_project_manager/models/mt5-small-offline"),
            "做一个公司官网改版项目",
        )


def test_generate_plan_from_local_model_falls_back_when_model_plan_quality_is_low(
    monkeypatch,
):
    recovered_plan = GeneratedPlan(
        project=ProjectDraft(
            name="修复后计划",
            planned_start_date="2026-05-01",
            planned_end_date="2026-05-31",
        ),
        nodes=[
            ProjectNodeDraft(
                name="内容整理",
                planned_start_date="2026-05-01",
                planned_end_date="2026-05-15",
                subtasks=[
                    SubTaskDraft(
                        name="整理内容",
                        planned_start_date="2026-05-01",
                        planned_end_date="2026-05-07",
                    )
                ],
            ),
            ProjectNodeDraft(
                name="页面发布",
                planned_start_date="2026-05-16",
                planned_end_date="2026-05-31",
                subtasks=[
                    SubTaskDraft(
                        name="发布页面",
                        planned_start_date="2026-05-16",
                        planned_end_date="2026-05-20",
                    )
                ],
            ),
        ],
    )

    def fake_load_local_generator(_: Path):
        def fake_generator(
            prompt: str, **kwargs: object
        ) -> list[dict[str, str]]:
            return [
                {
                    "generated_text": (
                        "PROJECT|短项目|描述|2026-04-01|2026-04-12 "
                        "NODE|页面发布|2|2026-04-02 END"
                    )
                }
            ]

        return fake_generator

    monkeypatch.setattr(
        model_generator_module, "load_local_generator", fake_load_local_generator
    )
    monkeypatch.setattr(
        model_generator_module,
        "generate_plan_from_requirement",
        lambda requirement: recovered_plan,
    )

    plan = model_generator_module.generate_plan_from_local_model(
        Path("tools/ai_project_manager/models/mt5-small-offline"),
        "上线一个市场活动专题页，包含内容整理和页面发布",
    )

    assert plan == recovered_plan
