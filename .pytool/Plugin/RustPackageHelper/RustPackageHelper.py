# @file RustPackageHelper.py
# HelperFucntion used to share the RustPackage
# class to the rest of the build system.
##
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import io
import os
import toml
from typing import Union
from pathlib import Path
from edk2toolext.environment.plugintypes.uefi_helper_plugin import IUefiHelperPlugin
from edk2toollib.utility_functions import RunCmd


class RustPackage:
    def __init__(self, path: Path):
        self.path = path
        self.name = path.name


class RustWorkspace:
    def __init__(self, path: Union[Path, str]):
        self.path: Path = Path(path)
        self.toml: dict = {}
        self.members: list[RustPackage] = []

        self.__load_toml()
        self.__set_members()
    
    def __load_toml(self):
        """Loads the repositories Cargo.toml file as a dictionary."""
        try:
            self.toml = toml.load(self.path / "Cargo.toml")
        except Exception:
            raise Exception(f"Failed to load Cargo.toml from {self.path}")
        
    def __set_members(self):
        """Finds all members of the workspace."""
        workspace = self.toml.get("workspace")
        members = set()

        # Grab all members specifically specified in the workspace
        for member in workspace["members"]:
            members.add(RustPackage(self.path / member))

        # Build a dep list that only contains dependencies with a path. These are workspace
        # members.
        dep_list = workspace["dependencies"]
        dep_list = [dep_list[dep] for dep in dep_list if type(dep_list[dep]) != str and dep_list[dep].get("path")]
        
        for dep in dep_list:
            members.add(RustPackage(self.path / dep["path"]))
        
        self.members = list(members)

    def coverage(self, pkg_list = None, ignore_list = None, report_type: str = "html" ):
        """Runs coverage at the workspace level.
        
        Generates a single report that provides coverage information for all
        packages in the workspace.
        """ 
        if pkg_list is None:
            pkg_list = [pkg.name for pkg in self.members]

        # Set up the command
        command = "cargo"
        params = "make"
        if ignore_list:
            params += f' -e COV_FLAGS="--out {report_type} --exclude-files {",".join(ignore_list)}"'
        else:
            params += f' -e COV_FLAGS="--out {report_type}"'
        params += f" coverage {','.join(pkg_list)}"

        # Run the command
        output = io.StringIO()
        RunCmd(command, params, workingdir=self.path, outstream=output)
        output.seek(0)
        lines = output.readlines()
        
        result = {
            "pass": [],
            "fail": [],
            "coverage": {}
        }
        
        # Determine passed and failed tests
        for line in lines:
            line = line.strip().strip("\n")
            
            if line.startswith("test result:"):
                continue
            
            if line.startswith("test "):
                line = line.replace("test ", "")
                if line.endswith("... ok"):
                    result["pass"].append(line.replace(" ... ok", ""))
                else:
                    result["fail"].append(line.replace(" ... FAILED", "")) 
                continue
        
        if len(result["fail"]) > 0:
            return result
        
        # Determine coverage if all tests passed
        for line in lines:
            line = line.strip().strip("\n")
            if line.startswith("|| Tested/Total Lines"):
                continue

            if line == "||":
                continue

            if line.startswith("||"):
                line = line.replace("|| ", "")
                path, cov = line.split(":")
                cov = cov.split()[0]
                result["coverage"][path] = cov

        return result


class RustPackageHelper(IUefiHelperPlugin):
    def RegisterHelpers(self, obj):
        fp = os.path.abspath(__file__)
        obj.Register("RustPackage", RustPackage, fp)
        obj.Register("RustWorkspace", RustWorkspace, fp)
