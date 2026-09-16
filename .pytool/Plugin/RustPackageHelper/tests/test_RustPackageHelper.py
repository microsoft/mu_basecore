# @file
# Unit tests for the RustPackageHelper plugin.
#
# An example of running these tests from the root of the workspace:
#   python -m unittest discover -s ./.pytool/Plugin/RustPackageHelper/tests -v
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import pathlib
import sys
import tempfile
import unittest

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
    """Tests for RustWorkspace member discovery."""

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


if __name__ == "__main__":
    unittest.main()
