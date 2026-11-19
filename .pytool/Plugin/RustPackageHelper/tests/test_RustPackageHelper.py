# @file
# Unit tests for the RustPackageHelper plugin.
#
# An example of running these tests from the root of the workspace:
#   python -m unittest discover -s ./.pytool/Plugin/RustPackageHelper/tests -v
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import json
import pathlib
import sys
import tempfile
import unittest
from unittest.mock import ANY, patch

test_file = pathlib.Path(__file__)
sys.path.append(str(test_file.parent.parent))

from RustPackageHelper import RustWorkspace  # noqa: E402

_DUPLICATE_MEMBER_TOML = """[workspace]
members = ["shared_crate"]

[workspace.dependencies]
shared_crate = { path = "shared_crate" }
"""

_FILTERED_MEMBERS_TOML = """[workspace]
members = ["explicit_crate"]

[workspace.dependencies]
path_crate = { path = "path_crate" }
registry_crate = "1.0.0"
git_crate = { git = "https://example.com/git_crate" }
"""

_EMPTY_WORKSPACE_TOML = """[workspace]
members = []

[workspace.dependencies]
"""


class TestRustWorkspace(unittest.TestCase):
    """Tests for RustWorkspace member discovery and test execution."""

    @staticmethod
    def _create_workspace(
        workspace_path: pathlib.Path,
        cargo_toml: str,
    ) -> RustWorkspace:
        """Create a RustWorkspace from a Cargo.toml fixture."""
        (workspace_path / "Cargo.toml").write_text(
            cargo_toml,
            encoding="utf-8",
        )
        return RustWorkspace(workspace_path)

    def test_set_members_deduplicates_member_path(self) -> None:
        """Verify duplicate member paths produce one RustPackage."""
        with tempfile.TemporaryDirectory() as temp_dir:
            workspace_path = pathlib.Path(temp_dir)
            # A crate listed through both Cargo workspace mechanisms should only be added once.
            workspace = self._create_workspace(
                workspace_path,
                _DUPLICATE_MEMBER_TOML,
            )

            self.assertEqual(len(workspace.members), 1)
            self.assertEqual(workspace.members[0].path, workspace_path / "shared_crate")

    def test_set_members_includes_only_workspace_paths(self) -> None:
        """Verify explicit members and path dependencies are discovered."""
        with tempfile.TemporaryDirectory() as temp_dir:
            workspace_path = pathlib.Path(temp_dir)
            workspace = self._create_workspace(
                workspace_path,
                _FILTERED_MEMBERS_TOML,
            )

            self.assertCountEqual(
                [member.path for member in workspace.members],
                [workspace_path / "explicit_crate", workspace_path / "path_crate"],
            )

    def test_set_members_handles_empty_workspace(self) -> None:
        """Verify an empty workspace produces no RustPackages."""
        with tempfile.TemporaryDirectory() as temp_dir:
            workspace_path = pathlib.Path(temp_dir)
            workspace = self._create_workspace(
                workspace_path,
                _EMPTY_WORKSPACE_TOML,
            )

            self.assertEqual(workspace.members, [])

    def test_set_members_retains_explicit_members_under_exclusions(self) -> None:
        """Explicit members take precedence over exact and parent exclusions."""
        for excluded_path in ("Pkg/crate", "Pkg"):
            with self.subTest(excluded_path=excluded_path), tempfile.TemporaryDirectory() as temp_dir:
                workspace_path = pathlib.Path(temp_dir)
                workspace = self._create_workspace(
                    workspace_path,
                    '[workspace]\nmembers = ["Pkg/crate"]\n'
                    f'exclude = ["{excluded_path}"]\n'
                    "[workspace.dependencies]\n"
                    'crate = { path = "Pkg/crate" }\n',
                )

                self.assertEqual([member.path for member in workspace.members], [workspace_path / "Pkg/crate"])

    def test_set_members_matches_cargo_exclusion_paths(self) -> None:
        """Match normalized dependencies against literal directory exclusions."""
        for excluded_path, dependency_path, is_excluded in (
            ("Pkg/excluded_crate", "./Pkg/excluded_crate/", True),
            ("Pkg", "Pkg/excluded_crate", True),
            ("Pkg/excluded_crate", "Pkg/child/../excluded_crate", True),
            ("Pkg/child/../excluded_crate", "Pkg/excluded_crate", False),
            ("Pkg", "Pkg/../OtherPkg/kept_crate", False),
        ):
            with self.subTest(excluded_path=excluded_path, dependency_path=dependency_path), \
                    tempfile.TemporaryDirectory() as temp_dir:
                workspace_path = pathlib.Path(temp_dir)
                workspace = self._create_workspace(
                    workspace_path,
                    '[workspace]\nmembers = ["explicit_crate"]\n'
                    f'exclude = ["{excluded_path}"]\n'
                    "[workspace.dependencies]\n"
                    f'excluded_crate = {{ path = "{dependency_path}" }}\n'
                    'retained_crate = { path = "Pkg_extra/retained_crate" }\n',
                )

                expected_members = [workspace_path / "explicit_crate", workspace_path / "Pkg_extra/retained_crate"]
                if not is_excluded:
                    expected_members.append(workspace_path / dependency_path)

                self.assertCountEqual([member.path for member in workspace.members], expected_members)

    def test_coverage_ci_returns_filtered_line_counts(self) -> None:
        """Use coverage-ci and return LLVM line counts for non-ignored files."""
        with tempfile.TemporaryDirectory() as temp_dir:
            workspace_path = pathlib.Path(temp_dir)
            workspace = self._create_workspace(workspace_path, _DUPLICATE_MEMBER_TOML)
            report_path = workspace_path / "target" / "coverage.json"
            report_path.parent.mkdir()
            files = [
                ("shared_crate/src/lib.rs", 8, 10),
                ("shared_crate/src/empty.rs", 0, 0),
                ("shared_crate/tests/nested/test.rs", 1, 2),
                ("shared_crate/generated/lib.rs", 1, 2),
                ("Submodule/src/lib.rs", 2, 4),
                ("OtherPkg/src/lib.rs", 3, 6),
            ]
            report_path.write_text(
                json.dumps({
                    "data": [{
                        "files": [
                            {
                                "filename": str(workspace_path / name),
                                "summary": {"lines": {"covered": covered, "count": total}},
                            }
                            for name, covered, total in files
                        ],
                    }],
                }),
                encoding="utf-8",
            )

            def run_tests(command, params, *, workingdir, outstream):
                outstream.write(
                    "test tests::passes ... ok\n"
                    "test tests::ignored ... ignored\n"
                    "test result: ok. 1 passed; 0 failed; 1 ignored\n"
                )
                return 0

            with patch("RustPackageHelper.RunCmd", side_effect=run_tests) as run_cmd:
                result = workspace.test(
                    ignore_list=[
                        "**/tests/*",
                        "Submodule/**/*",
                        r"OtherPkg\**\*",
                        str(workspace_path / "shared_crate" / "generated" / "*"),
                    ],
                )

            self.assertEqual(result, {
                "pass": ["tests::passes"],
                "fail": [],
                "coverage": {"shared_crate/src/lib.rs": "8/10"},
            })
            run_cmd.assert_called_once_with(
                "cargo",
                "make coverage-ci shared_crate",
                workingdir=workspace_path,
                outstream=ANY,
            )

    def test_without_coverage_runs_test_task(self) -> None:
        """Run the test task without requiring a coverage report."""
        with tempfile.TemporaryDirectory() as temp_dir:
            workspace_path = pathlib.Path(temp_dir)
            workspace = self._create_workspace(workspace_path, _DUPLICATE_MEMBER_TOML)
            with patch("RustPackageHelper.RunCmd", return_value=0) as run_cmd:
                result = workspace.test(["first_crate", "second_crate"], coverage=False)

            self.assertEqual(result, {"pass": [], "fail": [], "coverage": {}})
            run_cmd.assert_called_once_with(
                "cargo",
                "make test first_crate,second_crate",
                workingdir=workspace_path,
                outstream=ANY,
            )

    def test_failed_tests_do_not_require_coverage_report(self) -> None:
        """Return test failures even when LLVM did not write coverage JSON."""
        with tempfile.TemporaryDirectory() as temp_dir:
            workspace_path = pathlib.Path(temp_dir)
            workspace = self._create_workspace(workspace_path, _DUPLICATE_MEMBER_TOML)

            def run_tests(command, params, *, workingdir, outstream):
                outstream.write("test tests::fails ... FAILED\n")
                return 1

            with patch("RustPackageHelper.RunCmd", side_effect=run_tests):
                result = workspace.test()

            self.assertEqual(result, {"pass": [], "fail": ["tests::fails"], "coverage": {}})

    def test_command_failure_without_failed_tests_raises(self) -> None:
        """Do not treat a failed coverage command as a passing test run."""
        with tempfile.TemporaryDirectory() as temp_dir:
            workspace_path = pathlib.Path(temp_dir)
            workspace = self._create_workspace(workspace_path, _DUPLICATE_MEMBER_TOML)
            with patch("RustPackageHelper.RunCmd", return_value=1):
                with self.assertRaisesRegex(RuntimeError, "Failed to compile tests or run command"):
                    workspace.test()

    def test_missing_or_invalid_coverage_report_raises(self) -> None:
        """Surface report errors rather than silently bypassing CI thresholds."""
        for contents in (None, "{", '{"data": [{"files": [{}]}]}'):
            with self.subTest(contents=contents), tempfile.TemporaryDirectory() as temp_dir:
                workspace_path = pathlib.Path(temp_dir)
                workspace = self._create_workspace(workspace_path, _DUPLICATE_MEMBER_TOML)
                if contents is not None:
                    report_path = workspace_path / "target" / "coverage.json"
                    report_path.parent.mkdir()
                    report_path.write_text(contents, encoding="utf-8")

                with patch("RustPackageHelper.RunCmd", return_value=0):
                    with self.assertRaisesRegex(RuntimeError, "Failed to read LLVM coverage report"):
                        workspace.test()


if __name__ == "__main__":
    unittest.main()
