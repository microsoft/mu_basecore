## @file FlattenPdbs.py
# Plugin to support copying all PDBs to 1 directory.
# This makes symbol publishing easier and with the usage of
# ALT PDB PATH can shrink the size of each module.
#
##
# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# All rights reserved.
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##
###
from edk2toolext.environment.plugintypes.uefi_build_plugin import IUefiBuildPlugin
import logging
from pathlib import Path

# MU_CHANGE Entire File - Perf improvements
class FlattenPdbs(IUefiBuildPlugin):

    def do_post_build(self, thebuilder):
        #Path to Build output
        build_path = Path(thebuilder.env.GetValue("BUILD_OUTPUT_BASE"))
        #Path to where the PDBs will be stored
        pdb_path = Path(build_path, "PDB")

        try:
            if not pdb_path.is_dir():
                pdb_path.mkdir()
        except Exception:
            logging.critical("Error making PDB directory")

        logging.critical("Copying PDBs to flat directory")
        for file in Path(build_path).rglob("*.pdb"):
            # pdb exists in DEBUG and OUTPUT directory. Same file.
            pdb_out = Path(pdb_path / file.name)
            if file.parent.name != "OUTPUT":
                continue
            # If it exists and has the same file identifier, skip it.
            if pdb_out.exists() and file.stat().st_ino == pdb_out.stat().st_ino:
                continue
            if "vc1" in file.name.lower():
                continue
            # Hard link it, which is slightly faster, but mainly allows us to tell
            # if the file has changed (st_ino is different)
            pdb_out.unlink(missing_ok=True)
            pdb_out.hardlink_to(file)
        return 0
