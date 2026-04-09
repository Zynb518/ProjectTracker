import os
import sys
from pathlib import Path
import subprocess

from ai_project_manager.cli import build_parser


def test_cli_registers_generate_command():
    parser = build_parser()
    args = parser.parse_args(["generate", "--requirement", "做一个官网改版项目"])
    assert args.command == "generate"
    assert args.requirement == "做一个官网改版项目"


def test_cli_entrypoint_runs_help():
    package_root = Path(__file__).resolve().parents[1]
    cli_path = Path(__file__).resolve().parents[1] / "src" / "ai_project_manager" / "cli.py"
    env = dict(os.environ, PYTHONPATH=str(package_root / "src"))
    result = subprocess.run(
        [sys.executable, str(cli_path), "--help"],
        check=True,
        capture_output=True,
        text=True,
        env=env,
    )
    assert "usage" in result.stdout.lower()
