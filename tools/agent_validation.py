#!/usr/bin/env python3
"""Local Inox validation with bounded output; CI remains the merge authority."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent
SCOPE = ("src/", "stdlib/", "tests/", "cmake/", "tools/scripts/")
ROOT_FILES = {"CMakeLists.txt", "CMakePresets.json"}


def git(repo: Path, *args: str) -> bytes:
    command = subprocess.run(["git", "-C", str(repo), *args], capture_output=True, timeout=30)
    if command.returncode:
        raise RuntimeError(command.stderr.decode("utf-8", "replace").strip() or "git failed")
    return command.stdout


def changed_files(repo: Path) -> set[str]:
    """Committed, staged, unstaged and untracked edits, including after commits."""
    baseline = git(repo, "merge-base", "HEAD", "main").decode().strip()
    outputs = [
        git(repo, "diff", "--name-only", "-z", baseline, "HEAD"),
        git(repo, "diff", "--name-only", "-z", "HEAD"),
        git(repo, "ls-files", "--others", "--exclude-standard", "-z"),
    ]
    return {p.decode("utf-8", "surrogateescape") for output in outputs
            for p in output.split(b"\0") if p}


def is_code(path: str) -> bool:
    return path.startswith(SCOPE) or path in ROOT_FILES


def machine(repo: Path) -> dict[str, str] | None:
    path = repo / ".inox-machine"
    if not path.is_file():
        return None
    data = {}
    for line in path.read_text(encoding="utf-8-sig").splitlines():
        line = line.split("#", 1)[0].strip()
        if "=" in line:
            key, value = line.split("=", 1)
            data[key.strip()] = value.strip()
    required = ("id", "role", "build_preset", "test_command")
    if any(not data.get(k) or data[k] == "CHANGE-ME" for k in required):
        return None
    return data


def fingerprint(repo: Path, changed: set[str], settings: dict[str, str], profile: str) -> str:
    digest = hashlib.sha256(profile.encode())
    for key in ("id", "role", "build_preset", "test_command"):
        digest.update(key.encode() + b"\0" + settings[key].encode() + b"\0")
    for name in sorted(changed):
        path = repo / name
        digest.update(name.encode("utf-8", "surrogateescape") + b"\0")
        if path.is_file():
            digest.update(hashlib.sha256(path.read_bytes()).digest())
        else:
            digest.update(b"<deleted>")
    return digest.hexdigest()


def summarize_failure(log: Path, limit: int = 8) -> str:
    lines = log.read_text(encoding="utf-8", errors="replace").splitlines()
    errors = [line for line in lines if any(tag in line.lower()
              for tag in ("[fail]", "error:", "failed", "traceback", "fatal error"))]
    useful = errors[:limit] or lines[-limit:]
    return " | ".join(line.strip()[:220] for line in useful)[:1600]


def run_step(repo: Path, command: list[str] | str, log: Path, timeout: int = 540) -> tuple[str, int]:
    with log.open("a", encoding="utf-8") as output:
        output.write("\n=== " + (command if isinstance(command, str) else " ".join(command)) + " ===\n")
        output.flush()
        try:
            proc = subprocess.run(command, cwd=repo, shell=isinstance(command, str),
                                  stdout=output, stderr=subprocess.STDOUT,
                                  timeout=timeout, check=False)
            return ("PASS" if proc.returncode == 0 else "FAIL", proc.returncode)
        except (OSError, subprocess.TimeoutExpired) as exc:
            output.write(f"NOT_RUN/FAIL: {type(exc).__name__}: {exc}\n")
            return ("NOT_RUN", 1)


def validate(repo: Path, profile: str, stop: bool = False) -> int:
    try:
        changed = changed_files(repo)
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as exc:
        print(f"INOX_LOCAL NOT_RUN git-baseline: {str(exc)[:160]}", file=sys.stderr)
        return 0 if stop else 1
    relevant = {p for p in changed if is_code(p)}
    if not relevant:
        print("INOX_LOCAL SKIP no compiler/runtime/test/build changes")
        return 0
    config = machine(repo)
    if not config:
        print("INOX_LOCAL NOT_RUN .inox-machine absent or incomplete; CI required")
        return 0 if stop else 1
    key = fingerprint(repo, relevant, config, profile)
    directory = repo / "build" / "agent-validation"
    directory.mkdir(parents=True, exist_ok=True)
    log = directory / f"{key[:16]}-{profile}.log"
    record = directory / f"{key[:16]}-{profile}.json"
    if record.is_file():
        try:
            cached = json.loads(record.read_text(encoding="utf-8"))
            if cached.get("fingerprint") == key and cached.get("result") == "PASS":
                print(f"INOX_LOCAL PASS (cached) profile={profile} log={log}")
                return 0
        except (ValueError, OSError):
            pass
    log.write_text("Inox local validation; detailed output kept out of LLM context.\n", encoding="utf-8")
    commands: list[tuple[str, list[str] | str]] = [
        ("build", ["cmake", "--build", "--preset", config["build_preset"]]),
        ("suite", config["test_command"]),
    ]
    if profile == "full":
        commands.extend([
            ("constitution", [sys.executable, "tools/constitution_check.py", "check"]),
            ("grammar", [sys.executable, "tools/grammar_consistency.py"]),
            ("version", [sys.executable, "tools/version_check.py"]),
        ])
    statuses = []
    for label, cmd in commands:
        status, code = run_step(repo, cmd, log)
        statuses.append({"check": label, "status": status, "exit": code})
        if status != "PASS":
            break
    result = "PASS" if len(statuses) == len(commands) and all(s["status"] == "PASS" for s in statuses) else "FAIL"
    logtext = log.read_text(encoding="utf-8", errors="replace")
    skip = sum("[SKIP]" in line for line in logtext.splitlines())
    if skip and result == "PASS":
        result = "PARTIAL"
    record.write_text(json.dumps({"fingerprint": key, "result": result,
                                  "machine": config["id"], "profile": profile,
                                  "checks": statuses, "skipped_tests": skip,
                                  "ci_e19_complete": False}, indent=2) + "\n", encoding="utf-8")
    short = " ".join(f'{s["check"]}:{s["status"]}' for s in statuses)
    print(f"INOX_LOCAL {result} machine={config['id']} profile={profile} {short} skipped={skip} log={log}")
    if result == "FAIL":
        print("INOX_LOCAL_ERRORS " + summarize_failure(log), file=sys.stderr)
        return 2 if stop else 1
    return 0 if result == "PASS" else (0 if stop else 1)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("action", choices=("verify", "stop"))
    parser.add_argument("--profile", choices=("quick", "full"), default="quick")
    args = parser.parse_args()
    if args.action == "stop":
        try:
            payload = json.loads(sys.stdin.read() or "{}")
        except ValueError:
            payload = {}
        if payload.get("stop_hook_active"):
            return 0  # Never create an unrepairable Stop-hook continuation loop.
    return validate(ROOT, args.profile, stop=args.action == "stop")


if __name__ == "__main__":
    raise SystemExit(main())
