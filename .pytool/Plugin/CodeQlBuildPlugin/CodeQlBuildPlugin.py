# @file CodeQlBuildPlugin.py
#
# A build plugin that produces CodeQL results for the present build.
#
# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import os

from edk2toolext.environment.plugintypes.uefi_build_plugin import \
    IUefiBuildPlugin                                               # noqa: E402
from edk2toolext.environment.uefi_build import UefiBuilder         # noqa: E402
from edk2toollib.uefi.edk2.path_utilities import Edk2Path          # noqa: E402


class CodeQlBuildPlugin(IUefiBuildPlugin):

    def do_pre_build(self, builder: UefiBuilder) -> int:
        """CodeQL pre-build functionality.

        Args:
            builder (UefiBuilder): A UEFI builder object for this build.

        Returns:
            int: The number of debug macro errors found. Zero indicates the
            check either did not run or no errors were found.
        """
        pp = builder.pp.split(os.pathsep)
        edk2 = Edk2Path(builder.ws, pp)
        package = edk2.GetContainingPackage(
                            builder.mws.join(builder.ws,
                                             builder.env.GetValue(
                                                "ACTIVE_PLATFORM")))
        target = builder.env.GetValue("TARGET")

        codeql_db_dir_name = "codeql-db-" + package + "-" + target
        codeql_db_dir_name = codeql_db_dir_name.lower()

        codeql_db_path = os.path.join("Build", codeql_db_dir_name)
        builder.EnableCodeQl(codeql_db_path)

        return 0
