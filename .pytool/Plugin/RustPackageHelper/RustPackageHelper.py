# @file
# Helper plugin used to share the RustPackage class to the rest of the build
# system.
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import io
import json
import os
from fnmatch import fnmatch
from pathlib import Path
from typing import Optional, TypedDict, Union

import toml
from edk2toolext.environment.plugintypes.uefi_helper_plugin import HelperFunctions, IUefiHelperPlugin
from edk2toollib.utility_functions import RunCmd

RustTestResult = TypedDict(
    "RustTestResult",
    {"pass": list[str], "fail": list[str], "coverage": dict[str, str]},
)


class RustPackage:
    def __init__(self, path: Path) -> None:
        self.path = path
        self.name = path.name


class RustWorkspace:
    def __init__(self, path: Union[Path, str]) -> None:
        self.path: Path = Path(path)
        self.toml: dict = {}
        self.members: list[RustPackage] = []

        self.__load_toml()
        self.__set_members()

    def __load_toml(self) -> None:
        """Load the repositories Cargo.toml file as a dictionary."""
        try:
            self.toml = toml.load(self.path / "Cargo.toml")
        except Exception:
            raise Exception(f"Failed to load Cargo.toml from {self.path}")

    def __set_members(self) -> None:
        """Find all members of the workspace."""
        workspace = self.toml.get("workspace")
        if workspace is None:
            return

        member_paths = set()

        # Grab all members specifically specified in the workspace
        for member in workspace["members"]:
            member_paths.add(self.path / member)

        # Build a dep list that only contains dependencies with a path. These are workspace
        # members.
        dep_list = workspace["dependencies"]
        dep_list = [
            dep_list[dep] for dep in dep_list if not isinstance(dep_list[dep], str) and dep_list[dep].get("path")
        ]

        for dep in dep_list:
            member_paths.add(self.path / dep["path"])

        self.members = [RustPackage(path) for path in member_paths]

    def test(
        self,
        pkg_list: Optional[list[str]] = None,
        ignore_list: Optional[list[str]] = None,
        coverage: bool = True,
    ) -> RustTestResult:
        """Run tests on a list of rust packages / crates.

        Will additionally calculate code coverage if coverage is set to True.
        """
        if pkg_list is None:
            pkg_list = [pkg.name for pkg in self.members]

        # Set up the command
        command = "cargo"
        task = "coverage-ci" if coverage else "test"
        params = f"make {task} {','.join(pkg_list)}"

        # Run the command
        output = io.StringIO()
        return_value = RunCmd(command, params, workingdir=self.path, outstream=output)
        output.seek(0)
        lines = output.readlines()

        result: RustTestResult = {"pass": [], "fail": [], "coverage": {}}

        # Determine passed and failed tests
        for line in lines:
            line = line.strip().strip("\n")

            if line.startswith("test result:"):
                continue

            if line.startswith("test "):
                line = line.replace("test ", "")
                if line.endswith("... ok"):
                    result["pass"].append(line.replace(" ... ok", ""))
                elif line.endswith("... ignored"):
                    continue
                else:
                    result["fail"].append(line.replace(" ... FAILED", ""))

        # Command failed, but we didn't parse any failed tests
        if return_value != 0 and len(result["fail"]) == 0:
            raise RuntimeError("Failed to compile tests or run command.")

        if len(result["fail"]) > 0:
            return result

        if not coverage:
            return result

        report_path = self.path / "target" / "coverage.json"
        try:
            with report_path.open(encoding="utf-8") as report:
                coverage_report = json.load(report)

            workspace_path = self.path.resolve()
            for data in coverage_report["data"]:
                for file in data["files"]:
                    path = workspace_path / file["filename"]
                    relative_path = path.relative_to(workspace_path) if path.is_relative_to(workspace_path) else path
                    if ignore_list and any(
                        fnmatch(relative_path.as_posix(), pattern.replace("\\", "/"))
                        or fnmatch(path.as_posix(), pattern.replace("\\", "/"))
                        for pattern in ignore_list
                    ):
                        continue

                    line_coverage = file["summary"]["lines"]
                    if line_coverage["count"] > 0:
                        result["coverage"][relative_path.as_posix()] = (
                            f'{line_coverage["covered"]}/{line_coverage["count"]}'
                        )
        except (OSError, ValueError, KeyError, TypeError) as error:
            raise RuntimeError(f"Failed to read LLVM coverage report {report_path}: {error}") from error

        return result


class RustPackageHelper(IUefiHelperPlugin):
    def RegisterHelpers(self, obj: HelperFunctions) -> None:
        fp = os.path.abspath(__file__)
        obj.Register("RustPackage", RustPackage, fp)
        obj.Register("RustWorkspace", RustWorkspace, fp)
