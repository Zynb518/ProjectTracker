from __future__ import annotations

import inspect
import json
import sys
from pathlib import Path

PACKAGE_ROOT = Path(__file__).resolve().parents[1]
SRC_DIR = PACKAGE_ROOT / "src"
if str(SRC_DIR) not in sys.path:
    sys.path.insert(0, str(SRC_DIR))

from datasets import load_dataset
from transformers import (
    AutoModelForSeq2SeqLM,
    AutoTokenizer,
    DataCollatorForSeq2Seq,
    Seq2SeqTrainer,
    Seq2SeqTrainingArguments,
)

from ai_project_manager.model_plan_format import serialize_plan_for_model
from ai_project_manager.schemas import GeneratedPlan
from ai_project_manager.validator import validate_generated_plan

MODEL_NAME = "google/mt5-small"
TRAIN_PATH = PACKAGE_ROOT / "data" / "processed" / "train.jsonl"
MODEL_OUTPUT_DIR = PACKAGE_ROOT / "models" / "mt5-small-offline"
CHECKPOINT_DIR = MODEL_OUTPUT_DIR / "checkpoints"
MAX_SOURCE_LENGTH = 512
MAX_TARGET_LENGTH = 1024


def build_training_arguments(output_dir: Path) -> Seq2SeqTrainingArguments:
    kwargs: dict[str, object] = {
        "output_dir": str(output_dir),
        "per_device_train_batch_size": 1,
        "learning_rate": 1e-4,
        "num_train_epochs": 64,
        "lr_scheduler_type": "constant",
        "save_strategy": "no",
        "logging_steps": 1,
        "report_to": [],
    }
    init_params = inspect.signature(Seq2SeqTrainingArguments.__init__).parameters
    if "eval_strategy" in init_params:
        kwargs["eval_strategy"] = "no"
    else:
        kwargs["evaluation_strategy"] = "no"
    return Seq2SeqTrainingArguments(**kwargs)


def build_trainer_kwargs(tokenizer: AutoTokenizer) -> dict[str, object]:
    kwargs: dict[str, object] = {}
    init_params = inspect.signature(Seq2SeqTrainer.__init__).parameters
    if "processing_class" in init_params:
        kwargs["processing_class"] = tokenizer
    else:
        kwargs["tokenizer"] = tokenizer
    return kwargs


def patch_accelerate_unwrap_model_compat() -> bool:
    """Patch accelerate<->transformers unwrap_model keyword drift at runtime."""
    try:
        from accelerate import Accelerator
    except ImportError:
        return False

    unwrap_model = getattr(Accelerator, "unwrap_model", None)
    if unwrap_model is None:
        return False
    if getattr(unwrap_model, "__ai_pm_compat_patch__", False):
        return False

    params = inspect.signature(unwrap_model).parameters
    if "keep_torch_compile" in params:
        return False
    if "keep_fp32_wrapper" not in params:
        return False

    def unwrap_model_compat(
        self,
        model: object,
        keep_fp32_wrapper: bool = True,
        keep_torch_compile: bool | None = None,
    ) -> object:
        if keep_torch_compile is not None:
            keep_fp32_wrapper = keep_torch_compile
        return unwrap_model(self, model, keep_fp32_wrapper=keep_fp32_wrapper)

    setattr(unwrap_model_compat, "__ai_pm_compat_patch__", True)
    Accelerator.unwrap_model = unwrap_model_compat
    return True


def build_training_target(output_text: str) -> str:
    payload = json.loads(output_text)
    plan = GeneratedPlan.model_validate(payload)
    errors = validate_generated_plan(plan)
    if errors:
        raise ValueError(f"training target failed plan validation: {', '.join(errors)}")
    return serialize_plan_for_model(plan)


def main() -> None:
    if not TRAIN_PATH.exists():
        raise FileNotFoundError(f"training dataset does not exist: {TRAIN_PATH}")

    dataset = load_dataset("json", data_files=str(TRAIN_PATH), split="train")
    tokenizer = AutoTokenizer.from_pretrained(MODEL_NAME, use_fast=False)
    model = AutoModelForSeq2SeqLM.from_pretrained(MODEL_NAME)

    def preprocess(batch: dict[str, list[str]]) -> dict[str, list[list[int]]]:
        model_inputs = tokenizer(
            batch["input"], max_length=MAX_SOURCE_LENGTH, truncation=True
        )
        full_targets = [build_training_target(text) for text in batch["output"]]
        labels = tokenizer(
            text_target=full_targets, max_length=MAX_TARGET_LENGTH, truncation=True
        )
        model_inputs["labels"] = labels["input_ids"]
        return model_inputs

    tokenized_dataset = dataset.map(
        preprocess, batched=True, remove_columns=dataset.column_names
    )

    training_args = build_training_arguments(CHECKPOINT_DIR)

    patch_accelerate_unwrap_model_compat()
    trainer_kwargs = build_trainer_kwargs(tokenizer)
    trainer = Seq2SeqTrainer(
        model=model,
        args=training_args,
        train_dataset=tokenized_dataset,
        data_collator=DataCollatorForSeq2Seq(tokenizer=tokenizer, model=model),
        **trainer_kwargs,
    )
    trainer.train()

    MODEL_OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    trainer.model.save_pretrained(MODEL_OUTPUT_DIR)
    tokenizer.save_pretrained(MODEL_OUTPUT_DIR)
    print(f"saved model and tokenizer to {MODEL_OUTPUT_DIR}")


if __name__ == "__main__":
    main()
