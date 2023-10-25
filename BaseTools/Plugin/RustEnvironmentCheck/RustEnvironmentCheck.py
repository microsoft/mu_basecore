# @file RustEnvironmentCheck.py
# Plugin to confirm Rust tools are present needed to compile Rust code during
# firmare build.
#
# This provides faster, direct feedback to a developer about the changes they
# may need to make to successfully build Rust code. Otherwise, the build will
# fail much later during firmware code compilation when Rust tools are invoked
# with messages that are ambiguous or difficult to find.
#
# See the following documentation for more details:
# https://www.tianocore.org/edk2-pytool-extensions/features/rust_environment/
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
from edk2toolext.environment.rust import run
from edk2toolext.environment.plugintypes.uefi_build_plugin import IUefiBuildPlugin
from edk2toolext.environment.uefi_build import UefiBuilder


class RustEnvironmentCheck(IUefiBuildPlugin):
    """Checks that the system environment is ready to build Rust code."""

    def do_pre_build(self, _: UefiBuilder) -> int:
        """Rust environment checks during pre-build.

        Args:
            builder (UefiBuilder): A UEFI builder object for this build.

        Returns:
            int: The number of environment issues found. Zero indicates no
            action is needed.
        """
        return run()
