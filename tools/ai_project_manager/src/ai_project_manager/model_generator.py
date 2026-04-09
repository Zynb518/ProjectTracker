from __future__ import annotations

from datetime import date
from pathlib import Path
from typing import Any

from ai_project_manager.dataset_builder import PREFIX
from ai_project_manager.heuristic_generator import (
    generate_plan_from_requirement,
    infer_requirement_phases,
)
from ai_project_manager.model_plan_format import parse_model_plan_text
from ai_project_manager.normalizer import normalize_generated_plan
from ai_project_manager.schemas import GeneratedPlan
from ai_project_manager.validator import validate_generated_plan


def load_local_generator(model_dir: Path) -> Any:
    """Load a local text2text generation pipeline from a saved model directory."""
    resolved_model_dir = Path(model_dir)
    if not resolved_model_dir.exists() or not resolved_model_dir.is_dir():
        raise FileNotFoundError(f"model directory does not exist: {resolved_model_dir}")

    from transformers import AutoModelForSeq2SeqLM, AutoTokenizer, pipeline

    model = AutoModelForSeq2SeqLM.from_pretrained(str(resolved_model_dir))
    tokenizer = AutoTokenizer.from_pretrained(str(resolved_model_dir), use_fast=False)

    return pipeline(
        task="text2text-generation",
        model=model,
        tokenizer=tokenizer,
    )


def format_requirement_prompt(requirement: str) -> str:
    return f"{PREFIX}{requirement}"


def generate_plan_from_local_model(model_dir: Path, requirement: str) -> GeneratedPlan:
    generator = load_local_generator(model_dir)
    prompt = format_requirement_prompt(requirement)
    outputs = generator(
        prompt,
        max_new_tokens=768,
        do_sample=False,
        num_beams=2,
        no_repeat_ngram_size=4,
    )
    generated_text = _extract_generated_text(outputs)
    plan = _parse_and_validate_generated_plan(generated_text)
    if _should_fallback_to_heuristic(plan, requirement):
        return generate_plan_from_requirement(requirement)
    return plan


def _extract_generated_text(outputs: Any) -> str:
    if not outputs or "generated_text" not in outputs[0]:
        raise RuntimeError("model pipeline returned an unexpected output payload")

    generated_text = outputs[0]["generated_text"]
    if not isinstance(generated_text, str):
        raise ValueError("model output generated_text must be a string")
    return generated_text


def _parse_and_validate_generated_plan(generated_text: str) -> GeneratedPlan:
    try:
        plan = parse_model_plan_text(generated_text)
    except ValueError as exc:
        raise ValueError(f"model output is not valid plan text: {exc}") from exc

    plan = normalize_generated_plan(plan)
    errors = validate_generated_plan(plan)
    if errors:
        raise ValueError(f"model output failed plan validation: {', '.join(errors)}")
    return plan


def _should_fallback_to_heuristic(plan: GeneratedPlan, requirement: str) -> bool:
    explicit_phases = infer_requirement_phases(requirement)
    if len(explicit_phases) >= 2 and len(plan.nodes) < len(explicit_phases):
        return True

    if any(node.description.strip().isdigit() for node in plan.nodes):
        return True

    project_span = _date_span_days(
        plan.project.planned_start_date,
        plan.project.planned_end_date,
    )
    if project_span <= max(1, len(plan.nodes) - 1):
        return True

    return False


def _date_span_days(start_date: str, end_date: str) -> int:
    try:
        start = date.fromisoformat(start_date)
        end = date.fromisoformat(end_date)
    except ValueError:
        return 0
    return max(0, (end - start).days)
