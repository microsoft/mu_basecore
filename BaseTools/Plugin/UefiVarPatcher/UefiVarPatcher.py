# @file UefiVarPatcher.py
# Plugin used to updated UEFI variables in a ROM image
#
# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
import logging
import os

import edk2toollib.uefi.edk2.variablestore_manulipulations as VarStore
import edk2toollib.uefi.edk2.variable_format as VF

from edk2toolext.environment.plugintypes.uefi_build_plugin import IUefiBuildPlugin
from edk2toolext.environment.uefi_build import UefiBuilder
from edk2toolext.uefi.patch_var_store import (
    load_variable_xml,
    patch_variables,
    create_variables,
)

PLUGIN_NAME = "UefiVarPatcher"
CONFIG_FILE_NAME = "BuiltInVars.xml"


class UefiVarPatcher(IUefiBuildPlugin):
    def do_post_build(self, builder: UefiBuilder) -> int:
        """UEFI variable patching post-build functionality.

        Args:
            builder (UefiBuilder): A UEFI builder object for this build.

        Returns:
            int: Returrn code. 0 for success, -1 for failure.
        """
        logging.info(f"PLUGIN {PLUGIN_NAME}: Patching initial NvStore contents")

        # Not an error; no VAR_PATCH_CONFIG_DIR defined, so don't try to apply built-in vars.
        if not builder.env.GetValue("VAR_PATCH_CONFIG_DIR"):
            logging.debug(
                f"{PLUGIN_NAME} Post Build is not active due to VAR_PATCH_CONFIG_DIR not defined."
            )
            return 0

        built_in_vars_xml_path = (
            builder.edk2path.GetAbsolutePathOnThisSystemFromEdk2RelativePath(
                builder.env.GetValue("VAR_PATCH_CONFIG_DIR"),
                CONFIG_FILE_NAME,
                log_errors=False,
            )
        )

        # Not an error; if `CONFIG_FILE_NAME` does not exist, platform has no variables defined.
        if not built_in_vars_xml_path:
            logging.debug(
                f"{PLUGIN_NAME} Post Build is not active - {CONFIG_FILE_NAME} found."
            )
            return 0

        logging.info(
            f"{PLUGIN_NAME} File found in platform directory. Updating ROM image var store."
        )

        # If FLASH_SIZE not defined, error.
        if not builder.env.GetValue("FLASH_SIZE"):
            logging.error(
                f"{PLUGIN_NAME} Post Build failed due to FLASH_SIZE not being specified."
            )
            return -1

        # If FLASH_NVSTORAGE_OFFSET not defined, error.
        if not builder.env.GetValue("FLASH_REGION_NVSTORAGE_OFFSET"):
            logging.error(
                f"{PLUGIN_NAME}: Post Build failed due to FLASH_REGION_NVSTORAGE_OFFSET not being specified."
            )
            return -1

        # If FLASH_NVSTORAGE_SIZE not defined, error.
        if not builder.env.GetValue("FLASH_REGION_NVSTORAGE_SIZE"):
            logging.error(
                f"{PLUGIN_NAME}: Post Build failed due to FLASH_REGION_NVSTORAGE_SIZE not being specified."
            )
            return -1

        # If FLASH_NVSTORAGE_SIZE not defined, error.
        if not builder.env.GetValue("OUTPUT_ROM"):
            logging.error(
                f"{PLUGIN_NAME}: Post Build failed due to OUTPUT_ROM not being specified."
            )
            return -1

        # OUTPUT_ROM can be a semicolon(;)-delimited list of output ROMs that must be patched.
        out_roms = builder.env.GetValue("OUTPUT_ROM").split(";")
        for out_rom in out_roms:
            # If OUTPUT_ROM is not defined, but XML exists, implies attempt to patch a non-existent ROM. Error.
            if not os.path.isfile(out_rom):
                logging.error(
                    f"{PLUGIN_NAME} Post Build failed due to ROM file '{out_rom}' not found!"
                )
                return -1

            # Calculate the offset of the varstore within the file itself.
            flash_size = int(builder.env.GetValue("FLASH_SIZE"), 16)
            rom_size = os.path.getsize(out_rom)
            var_store_rom_offset = (
                rom_size
                - flash_size
                + int(builder.env.GetValue("FLASH_REGION_NVSTORAGE_OFFSET"), 16)
            )

        # Load the variable store from the file.
        var_store = VarStore.VariableStore(
            out_rom,
            store_base=var_store_rom_offset,
            store_size=int(builder.env.GetValue("FLASH_REGION_NVSTORAGE_SIZE"), 16),
        )

        # Print information about the current variables.
        for var in var_store.variables:
            if var.State == VF.VAR_ADDED:
                print(f"Var Found: '{var.VendorGuid}:{var.Name}'")

        # Attempt to load the set script file.
        set_vars = load_variable_xml(built_in_vars_xml_path)

        # Attempt to patch existing variables in the var store.
        create_vars = patch_variables(set_vars, var_store)

        # If we had variables we were unable to update, let's create them now.
        create_variables(create_vars, var_store)

        var_store.flush_to_file()

        return 0
