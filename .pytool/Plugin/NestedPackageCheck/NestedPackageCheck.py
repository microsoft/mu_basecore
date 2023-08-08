# @file NestedPackageCheck.py
#
# A CI Check that verifies there are no nested packages in the package being tested.
# Review the readme for a description of nested packages and why they are not allowed.
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
from edk2toolext.environment.plugintypes.ci_build_plugin import ICiBuildPlugin
from edk2toolext.environment.var_dict import VarDict
from pathlib import Path


class NestedPackageCheck(ICiBuildPlugin):
    """CiBuildPlugin that finds all nested packages in the workspace and fails if any are found in
    the target package.
    
    Configuration Options:
    "NestedPackageCheck": {
        "AuditOnly": False,
    }
    """
    def GetTestName(self, packagename: str, environment: VarDict) -> tuple:
        return (f"Check for nested packages in {packagename}", f"{packagename}.nestedpackagecheck")
    
    def RunBuildPlugin(self, packagename, edk2path, pkgconfig, env, PLM, PLMHelper, tc, output_stream=None) -> int:
        target_package = Path(edk2path.GetAbsolutePathOnThisSystemFromEdk2RelativePath(packagename))
        
        # Get all packages in the entire workspace
        package_path_packages = {}
        for package_path in edk2path.PackagePathList:
            package_path_packages[package_path] = \
                [p.parent for p in Path(package_path).glob('**/*.dec')]

        # Find any nested packages in the workspace
        nested_packages = []
        for package_path, packages_to_check in package_path_packages.items():
            for i, package in enumerate(packages_to_check):
                for j in range(i + 1, len(packages_to_check)):
                    comp_package = packages_to_check[j]
                    if package.is_relative_to(comp_package) or comp_package.is_relative_to(package):
                        nested_packages.append((package_path, package, comp_package))
        
        # Record only nested packages in the target package
        failed = 0
        for conflict in nested_packages:
            if target_package in conflict:
                tc.LogStdError(f"Nested package detected: {conflict[1]} and {conflict[2]}")
                failed += 1

        # Don't fail if in AuditOnly mode.  Just skip the test.
        if failed > 0:
            if pkgconfig.get("AuditOnly", False):
                tc.SetSkipped()
                return -1
            else:
                tc.SetFailed(f"Nested Packages Check {packagename} failed.  Errors: {failed}", "CHECK_FAILED")
                return failed
        
        tc.SetSuccess()
        return 0
