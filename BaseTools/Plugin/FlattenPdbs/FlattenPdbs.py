# @file FlattenPdbs.py
# Plugin to support copying all PDBs to 1 directory.
# This makes symbol publishing easier and with the usage of
# ALT PDB PATH can shrink the size of each module.
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
###
from edk2toolext.environment.plugintypes.uefi_build_plugin import IUefiBuildPlugin
import logging
from pathlib import Path


class FlattenPdbs(IUefiBuildPlugin):

    def do_post_build(self, thebuilder):
        # Path to Build output
        build_path = Path(thebuilder.env.GetValue("BUILD_OUTPUT_BASE"))
        # Path to where the PDBs will be stored
        pdb_path = Path(build_path, "PDB")

        try:
            if not pdb_path.is_dir():
                pdb_path.mkdir()
        except Exception:
            logging.critical("Error making PDB directory")

        logging.critical("Copying PDBs to flat directory")
        for file in Path(build_path).rglob("*.pdb"):
            # PDB exists in DEBUG and OUTPUT directory. Same file.
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
