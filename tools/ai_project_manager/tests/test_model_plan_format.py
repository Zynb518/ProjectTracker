from ai_project_manager.model_plan_format import (
    parse_model_plan_text,
    serialize_plan_for_model,
)
from ai_project_manager.schemas import (
    GeneratedPlan,
    ProjectDraft,
    ProjectNodeDraft,
    SubTaskDraft,
)


def _build_plan() -> GeneratedPlan:
    return GeneratedPlan(
        project=ProjectDraft(
            name="智能设备平台项目",
            description="阶段结构验证",
            planned_start_date="2026-05-01",
            planned_end_date="2026-06-20",
        ),
        nodes=[
            ProjectNodeDraft(
                name="方案设计",
                description="完成总体方案设计",
                planned_start_date="2026-05-01",
                planned_end_date="2026-05-10",
                subtasks=[
                    SubTaskDraft(
                        name="需求梳理",
                        description="确认需求范围",
                        priority=2,
                        planned_start_date="2026-05-01",
                        planned_end_date="2026-05-03",
                    ),
                    SubTaskDraft(
                        name="方案评审",
                        description="完成方案评审",
                        priority=1,
                        planned_start_date="2026-05-04",
                        planned_end_date="2026-05-06",
                    ),
                ],
            ),
            ProjectNodeDraft(
                name="交付准备",
                description="准备发布与交付材料",
                planned_start_date="2026-05-11",
                planned_end_date="2026-05-20",
                subtasks=[
                    SubTaskDraft(
                        name="整理手册",
                        description="输出操作手册",
                        priority=2,
                        planned_start_date="2026-05-11",
                        planned_end_date="2026-05-14",
                    )
                ],
            ),
        ],
    )


def test_serialize_plan_for_model_emits_line_based_structure():
    text = serialize_plan_for_model(_build_plan())
    lines = text.splitlines()

    assert lines[0].startswith("PROJECT|")
    assert any(line.startswith("NODE|") for line in lines)
    assert any(line.startswith("TASK|") for line in lines)
    assert lines[-1] == "END"


def test_model_plan_format_round_trips_generated_plan():
    plan = _build_plan()

    text = serialize_plan_for_model(plan)
    parsed = parse_model_plan_text(text)

    assert parsed.model_dump(exclude_none=True) == plan.model_dump(exclude_none=True)


def test_parse_model_plan_text_stops_after_end_marker():
    text = "\n".join(
        [
            "PROJECT|短项目|验证END|2026-04-01|2026-04-10",
            "NODE|执行阶段|执行|2026-04-01|2026-04-10",
            "TASK|任务一|执行任务|2|2026-04-01|2026-04-05",
            "END",
            "TASK|噪音|不应被读取|1|2026-04-06|2026-04-07",
        ]
    )

    parsed = parse_model_plan_text(text)

    assert parsed.project.name == "短项目"
    assert len(parsed.nodes) == 1
    assert len(parsed.nodes[0].subtasks) == 1


def test_parse_model_plan_text_recovers_inline_records_and_pending_tasks():
    text = (
        "PROJECT|短项目|用于验证解析恢复|2026-04-01|2026-04-10 "
        "TASK|任务一|执行任务|2026-04-02 "
        "NODE|执行阶段|阶段描述|2026-04-01|2026-04-10 "
        "END"
    )

    parsed = parse_model_plan_text(text)

    assert parsed.project.name == "短项目"
    assert len(parsed.nodes) == 1
    assert parsed.nodes[0].name == "执行阶段"
    assert len(parsed.nodes[0].subtasks) == 1
    assert parsed.nodes[0].subtasks[0].name == "任务一"
    assert parsed.nodes[0].subtasks[0].priority == 2
    assert parsed.nodes[0].subtasks[0].planned_end_date == "2026-04-02"


def test_parse_model_plan_text_recovers_extra_fields_and_missing_task_priority():
    text = "\n".join(
        [
            "PROJECT|智能设备配套平台升级项目|升级项目|面向设备平台升级|2026-07-01|2026-07-30",
            "NODE|开发联调|完成设备和平台联调|2026-07-10|2026-07-20",
            "TASK|接口联调|验证设备接口|补充说明|2026-07-12|2026-07-15",
            "END",
        ]
    )

    parsed = parse_model_plan_text(text)

    assert parsed.project.description == "升级项目|面向设备平台升级"
    assert len(parsed.nodes) == 1
    assert len(parsed.nodes[0].subtasks) == 1
    assert parsed.nodes[0].subtasks[0].description == "验证设备接口|补充说明"
    assert parsed.nodes[0].subtasks[0].priority == 2
