# @file R-EfiPreBuildPlugin.py
# Plugin to support check that the dependency for Rust is compiled
##
# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
###

import logging
import os
try:
    from edk2toolext.environment.plugintypes.uefi_build_plugin import IUefiBuildPlugin
    from edk2toollib.utility_functions import RunCmd
except Exception:
    pass

class REfiPreBuildPlugin(IUefiBuildPlugin):

    def __init__(self):
        self.logger = logging.getLogger(__name__)

    def _do_cargo_build(self, directory, state_file):
        commands = ["xbuild --release --target x86_64-unknown-uefi", "xbuild --target x86_64-unknown-uefi", "xbuild --release --target i686-unknown-uefi", "xbuild --target i686-unknown-uefi"]
        for command in commands:
            ret = RunCmd("cargo", command, workingdir=directory)
            if ret != 0:
                self.logger.error("Failed to compile r-efi package. Is rust installed?")
                return ret
        self.logger.info("Compiled r-efi rust package")

        # get current git hash? Get current version? How to do versioning?
        f = open(state_file, "w")
        f.write("Now the file has more content!")
        f.close()
        return 0

    def do_pre_build(self, thebuilder):
        r_efi_folder = os.path.dirname(__file__)
        target_folder = os.path.join(r_efi_folder, "target")
        state_file = os.path.join(target_folder, "indep_state.yaml")

        if not os.path.exists(target_folder) or not os.path.exists(state_file):
            return self._do_cargo_build(r_efi_folder, state_file)

        # read in the state file and make sure we are in sync
        self.logger.info("No need to rebuild the r-efi rust package")
        return 0