from ai_project_manager.heuristic_generator import generate_plan_from_requirement


def test_heuristic_generator_produces_three_level_plan():
    plan = generate_plan_from_requirement(
        "做一个公司官网改版项目，5月开始，6月中结束，包含设计、开发、测试上线阶段"
    )

    assert plan.project.name
    assert len(plan.nodes) >= 3
    assert all(node.subtasks for node in plan.nodes)


def test_heuristic_generator_without_explicit_months_uses_stable_default_window():
    plan = generate_plan_from_requirement(
        "做一个内部管理后台项目，分需求分析、开发实现、验收上线三个阶段"
    )

    assert plan.project.planned_start_date == "2026-01-01"
    assert plan.project.planned_end_date == "2026-02-28"


def test_heuristic_generator_uses_explicit_phase_list_from_requirement():
    plan = generate_plan_from_requirement(
        "规划一个智能设备配套平台升级项目，包含立项、方案设计、开发联调、验证、发布准备和交付支持"
    )

    assert plan.project.name == "智能设备配套平台升级项目"
    assert [node.name for node in plan.nodes] == [
        "立项",
        "方案设计",
        "开发联调",
        "验证",
        "发布准备",
        "交付支持",
    ]
    assert all(len(node.subtasks) >= 2 for node in plan.nodes)


def test_heuristic_generator_infers_short_requirement_name_and_two_phases():
    plan = generate_plan_from_requirement(
        "上线一个市场活动专题页，包含内容整理和页面发布"
    )

    assert plan.project.name == "市场活动专题页项目"
    assert [node.name for node in plan.nodes] == ["内容整理", "页面发布"]
