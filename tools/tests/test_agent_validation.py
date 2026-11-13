"""Deterministic tests for Inox local agent validation without building Inox."""
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest import mock

SOURCE = Path(__file__).resolve().parents[1] / "agent_validation.py"
spec = importlib.util.spec_from_file_location("agent_validation", SOURCE)
validation = importlib.util.module_from_spec(spec)
spec.loader.exec_module(validation)


def run(repo, *cmd):
    subprocess.run(cmd, cwd=repo, check=True, capture_output=True)


class AgentValidationTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.repo = Path(self.temp.name)
        run(self.repo, "git", "init", "-b", "main")
        run(self.repo, "git", "config", "user.email", "test@example.com")
        run(self.repo, "git", "config", "user.name", "Test")
        (self.repo / "src").mkdir()
        (self.repo / "src" / "unit.cpp").write_text("int f(){return 1;}\n", encoding="utf-8")
        run(self.repo, "git", "add", "src/unit.cpp")
        run(self.repo, "git", "commit", "-m", "base")
        run(self.repo, "git", "switch", "-c", "feature")

    def test_committed_edits_are_detected_after_git_commit(self):
        (self.repo / "src" / "unit.cpp").write_text("int f(){return 2;}\n", encoding="utf-8")
        run(self.repo, "git", "add", ".")
        run(self.repo, "git", "commit", "-m", "change")
        self.assertIn("src/unit.cpp", validation.changed_files(self.repo))

    def test_staged_unstaged_untracked_are_detected(self):
        (self.repo / "src" / "unit.cpp").write_text("int f(){return 3;}\n", encoding="utf-8")
        (self.repo / "src" / "new.cpp").write_text("int g(){return 1;}\n", encoding="utf-8")
        self.assertEqual({"src/unit.cpp", "src/new.cpp"}, validation.changed_files(self.repo))
        run(self.repo, "git", "add", "src/unit.cpp")
        self.assertEqual({"src/unit.cpp", "src/new.cpp"}, validation.changed_files(self.repo))

    def test_machine_missing_and_placeholder_are_not_faked(self):
        self.assertIsNone(validation.machine(self.repo))
        (self.repo / ".inox-machine").write_text("id = CHANGE-ME\nrole = home-dev\nbuild_preset = a\ntest_command = b\n")
        self.assertIsNone(validation.machine(self.repo))

    def test_fingerprint_invalidates_on_edit(self):
        settings = {"id": "test", "role": "home-dev", "build_preset": "demo", "test_command": "true"}
        path = self.repo / "src" / "unit.cpp"
        before = validation.fingerprint(self.repo, {"src/unit.cpp"}, settings, "quick")
        path.write_text("int f(){return 6;}\n", encoding="utf-8")
        after = validation.fingerprint(self.repo, {"src/unit.cpp"}, settings, "quick")
        self.assertNotEqual(before, after)

    def test_quick_validation_is_cached_and_keeps_full_log(self):
        (self.repo / ".inox-machine").write_text("id = test\nrole = home-dev\nbuild_preset = demo\ntest_command = echo tests\n")
        (self.repo / "src" / "unit.cpp").write_text("int f(){return 4;}\n")
        with mock.patch.object(validation, "run_step", side_effect=[("PASS", 0), ("PASS", 0)]) as runner:
            self.assertEqual(0, validation.validate(self.repo, "quick"))
            self.assertEqual(0, validation.validate(self.repo, "quick"))
            self.assertEqual(2, runner.call_count)
        records = list((self.repo / "build" / "agent-validation").glob("*.json"))
        self.assertEqual(1, len(records))
        self.assertIn('"ci_e19_complete": false', records[0].read_text().lower())

    def test_missing_machine_is_not_success_in_explicit_verify(self):
        (self.repo / "src" / "unit.cpp").write_text("int f(){return 9;}\n")
        self.assertEqual(1, validation.validate(self.repo, "quick"))


if __name__ == "__main__":
    unittest.main()
