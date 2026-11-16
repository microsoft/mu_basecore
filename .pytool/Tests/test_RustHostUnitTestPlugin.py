# @file
# Unit tests for the RustHostUnitTestPlugin coverage integration.
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

"""Verify the Rust CI plugin against the RustWorkspace test result contract."""

import importlib.util
import io
import json
import tempfile
import unittest
from contextlib import ExitStack
from pathlib import Path
from unittest.mock import ANY, Mock, patch

plugin_directory = Path(__file__).resolve().parents[1] / "Plugin"
plugin_path = plugin_directory / "RustHostUnitTestPlugin" / "RustHostUnitTestPlugin.py"
spec = importlib.util.spec_from_file_location("RustHostUnitTestPlugin", plugin_path)
if spec is None or spec.loader is None:
    raise ImportError(f"Cannot load RustHostUnitTestPlugin from {plugin_path}")
rust_plugin = importlib.util.module_from_spec(spec)
spec.loader.exec_module(rust_plugin)

helper_path = plugin_directory / "RustPackageHelper" / "RustPackageHelper.py"
spec = importlib.util.spec_from_file_location("RustPackageHelper", helper_path)
if spec is None or spec.loader is None:
    raise ImportError(f"Cannot load RustPackageHelper from {helper_path}")
rust_helper = importlib.util.module_from_spec(spec)
spec.loader.exec_module(rust_helper)


class TestRustHostUnitTestPlugin(unittest.TestCase):
    """Run the plugin and real helper with mocked Cargo execution."""

    def setUp(self) -> None:
        """Create an isolated workspace with selected and excluded crates."""
        temporary_directory = tempfile.TemporaryDirectory()
        self.addCleanup(temporary_directory.cleanup)
        self.root = Path(temporary_directory.name)
        (self.root / "Cargo.toml").write_text(
            '[workspace]\nmembers = ["TestPkg/test_crate", "OtherPkg/other_crate"]\n'
            "[workspace.dependencies]\n",
            encoding="utf-8",
        )
        self.report = self.root / "target" / "cobertura.xml"
        self.report.parent.mkdir()
        self.destination = self.root / "Build" / "coverage.xml"
        self.path_obj = Mock(WorkspacePath=str(self.root))
        self.path_obj.GetAbsolutePathOnThisSystemFromEdk2RelativePath.return_value = str(self.root / "TestPkg")
        self.tc = Mock()
        self.plugin = rust_plugin.RustHostUnitTestPlugin()
        contexts = ExitStack()
        self.addCleanup(contexts.close)
        contexts.enter_context(patch.object(rust_plugin.platform, "system", return_value="Linux"))
        contexts.enter_context(patch.object(rust_plugin, "repo_details", return_value={"Submodules": ["Submodule"]}))
        self.run_cmd = contexts.enter_context(patch.object(rust_helper, "RunCmd", side_effect=self._run_tests))

    @staticmethod
    def _run_tests(command, params, *, workingdir, outstream) -> int:
        outstream.write("test tests::passes ... ok\n")
        return 0

    def _write_reports(self) -> None:
        files = [
            ("TestPkg/test_crate/src/lib.rs", 6, 8),
            ("TestPkg/test_crate/src/other.rs", 2, 2),
            ("TestPkg/test_crate/src/empty.rs", 0, 0),
            ("TestPkg/test_crate/tests/test.rs", 0, 100),
            ("OtherPkg/other_crate/src/lib.rs", 0, 100),
            ("Submodule/src/lib.rs", 0, 100),
        ]
        (self.root / "target" / "coverage.json").write_text(
            json.dumps({
                "data": [{
                    "files": [
                        {
                            "filename": str(self.root / name),
                            "summary": {"lines": {"covered": covered, "count": total}},
                        }
                        for name, covered, total in files
                    ],
                }],
            }),
            encoding="utf-8",
        )
        self.report.write_text(
            f"<coverage><sources><source>{self.root.as_posix()}</source></sources></coverage>",
            encoding="utf-8",
        )

    def _run_plugin(self, config: dict) -> int:
        return self.plugin.RunBuildPlugin(
            "TestPkg", self.path_obj, config, None, None, rust_helper, self.tc, io.StringIO()
        )

    def test_coverage_ci_publishes_report(self) -> None:
        """Pass at the aggregated 80% threshold and publish XML into a new Build directory."""
        self._write_reports()

        self.assertEqual(self._run_plugin({"CoverageOverrides": {"test_crate": 0.80}}), 0)

        self.run_cmd.assert_called_once_with(
            "cargo", "make coverage-ci test_crate", workingdir=self.root, outstream=ANY
        )
        self.tc.LogStdOut.assert_any_call("tests::passes ... PASS")
        self.tc.SetSuccess.assert_called_once_with()
        self.tc.SetFailed.assert_not_called()
        self.assertEqual(
            self.destination.read_text(encoding="utf-8"),
            "<coverage><sources><source>.</source></sources></coverage>",
        )
        self.assertFalse(self.report.exists())

    def test_coverage_ci_replaces_previous_report(self) -> None:
        """Replace an existing report using an independently initialized workspace."""
        self._write_reports()
        self.destination.parent.mkdir()
        self.destination.write_text("previous report", encoding="utf-8")

        self.assertEqual(self._run_plugin({}), 0)

        self.tc.SetSuccess.assert_called_once_with()
        self.tc.SetFailed.assert_not_called()
        self.assertEqual(
            self.destination.read_text(encoding="utf-8"),
            "<coverage><sources><source>.</source></sources></coverage>",
        )
        self.assertFalse(self.report.exists())

    def test_coverage_override_failure_still_publishes_report(self) -> None:
        """Fail at an 81% threshold for 80% coverage while still publishing XML."""
        self._write_reports()
        self.destination.parent.mkdir()

        with self.assertLogs(level="WARNING"):
            result = self._run_plugin({"CoverageOverrides": {"test_crate": 0.81}})

        self.assertEqual(result, 1)
        self.tc.SetFailed.assert_called_once_with("Coverage requirements not met. Failures 1", "CHECK_FAILED")
        self.tc.SetSuccess.assert_not_called()
        self.assertTrue(self.destination.exists())

    def test_missing_cobertura_report_preserves_coverage_result(self) -> None:
        """Skip a missing report without masking coverage-threshold failures."""
        for required_coverage, expected_failures in ((0.80, 0), (0.81, 1)):
            with self.subTest(required_coverage=required_coverage):
                self._write_reports()
                self.report.unlink()
                self.tc.reset_mock()

                with self.assertLogs(level="WARNING"):
                    result = self._run_plugin({"CoverageOverrides": {"test_crate": required_coverage}})

                self.assertEqual(result, expected_failures)
                if expected_failures:
                    self.tc.SetFailed.assert_called_once_with(
                        "Coverage requirements not met. Failures 1", "CHECK_FAILED"
                    )
                    self.tc.SetSuccess.assert_not_called()
                else:
                    self.tc.SetSuccess.assert_called_once_with()
                    self.tc.SetFailed.assert_not_called()
                    self.tc.LogStdError.assert_not_called()
                self.assertFalse(self.destination.exists())

    def test_report_publication_error_fails_ci(self) -> None:
        """Surface filesystem errors through the plugin's CI failure channel."""
        self._write_reports()
        self.destination.parent.write_text("not a directory", encoding="utf-8")

        with self.assertLogs(level="WARNING"):
            result = self._run_plugin({})

        self.assertEqual(result, 1)
        self.tc.SetFailed.assert_called_once_with(ANY, "CHECK_FAILED")
        self.tc.SetSuccess.assert_not_called()
        self.assertTrue(self.report.exists())

    def test_coverage_disabled_does_not_require_reports(self) -> None:
        """Keep ordinary test runs and the Windows ARM64 coverage exception."""
        for system, machine, config in (
            ("Linux", "x86_64", {"CalculateCoverage": False}),
            ("Windows", "ARM64", {}),
        ):
            with self.subTest(system=system), \
                    patch.object(rust_plugin.platform, "system", return_value=system), \
                    patch.object(rust_plugin.platform, "machine", return_value=machine):
                self.run_cmd.reset_mock()
                self.tc.reset_mock()

                self.assertEqual(self._run_plugin(config), 0)

                self.run_cmd.assert_called_once_with(
                    "cargo", "make test test_crate", workingdir=self.root, outstream=ANY
                )
                self.tc.SetSuccess.assert_called_once_with()
                self.assertFalse(self.destination.exists())

    def test_helper_report_error_fails_ci(self) -> None:
        """Propagate a missing LLVM JSON report as a failed CI check."""
        with self.assertLogs(level="WARNING"):
            result = self._run_plugin({})

        self.assertEqual(result, 1)
        self.tc.SetFailed.assert_called_once_with(ANY, "CHECK_FAILED")
        self.assertIn("Failed to read LLVM coverage report", self.tc.SetFailed.call_args.args[0])
        self.tc.SetSuccess.assert_not_called()


if __name__ == "__main__":
    unittest.main()
