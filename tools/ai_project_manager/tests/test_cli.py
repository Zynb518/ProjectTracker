import subprocess

from ai_project_manager.cli import build_parser


def test_cli_registers_generate_command():
    parser = build_parser()
    args = parser.parse_args(["generate", "--requirement", "做一个官网改版项目"])
    assert args.command == "generate"
    assert args.requirement == "做一个官网改版项目"


def test_cli_entrypoint_runs_help():
    result = subprocess.run(
        ["ai-project-manager", "--help"], check=True, capture_output=True, text=True
    )
    assert "usage" in result.stdout.lower()
