# @file ImageValidation.py
# Plugin to validate any PE images against a set of requirements
##
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import os
import re
from pathlib import Path
from pefile import PE
from edk2toolext.environment.plugintypes.uefi_build_plugin import IUefiBuildPlugin
from edk2toolext.image_validation import (
    Result, TestManager, TestInterface, TestWriteExecuteFlags,
    TestSectionAlignment, MACHINE_TYPE
)
from edk2toollib.uefi.edk2.parsers.fdf_parser import FdfParser
from edk2toollib.uefi.edk2.parsers.dsc_parser import DscParser
from edk2toollib.uefi.edk2.parsers.inf_parser import InfParser
from edk2toollib.gitignore_parser import parse_gitignore_lines
import yaml
from typing import List
import logging
from datetime import datetime

DEFAULT_CONFIG_FILE_PATH = Path(__file__).parent.resolve() / "image_validation.cfg"

class TestImageBase(TestInterface):
    """Image base verification test.

    Checks the image base of the binary by accessing the optional
    header, then the image base. This value must be the same value
    as specified in the config file.

    Output:
        @Success: Image base matches the expected value
        @Skip: Image base requirement not set in the config file
        @Warn: Image Alignment value is not found in the Optional Header
        @Fail: Image base does not match the expected value
    """
    def name(self) -> str:
        """Returns the name of the test."""
        return 'Image Base verification'

    def execute(self, pe: PE, config_data: dict) -> Result:
        """Executes the test on the pefile.

        Arguments:
            pe (PE): a parsed PE/COFF image file
            config_data (dict): the configuration data for the test

        Returns:
            (Result): SKIP, WARN, FAIL, PASS
        """
        target_requirements = config_data["TARGET_REQUIREMENTS"]

        required_base = target_requirements.get("IMAGE_BASE")
        if required_base is None or required_base == -1:
            return Result.SKIP

        try:
            image_base = pe.OPTIONAL_HEADER.ImageBase
        except Exception:
            logging.warning("Image Base not found in Optional Header")
            return Result.WARN

        if image_base != required_base:
            logging.error(
                f'[{Result.FAIL}]: Image Base address Expected: {hex(required_base)}, Found: {hex(image_base)}'
            )
            return Result.FAIL
        return Result.PASS


class ImageValidation(IUefiBuildPlugin):
    def __init__(self):
        self.test_manager = TestManager()

        # Default tests provided by edk2toolext.image_validation
        self.test_manager.add_test(TestWriteExecuteFlags())
        self.test_manager.add_test(TestSectionAlignment())
        self.test_manager.add_test(TestImageBase())
        # Add additional Tests here

    def do_post_build(self, thebuilder):

        starttime = datetime.now()
        logging.info(
            "---------------------------------------------------------")
        logging.info(
            "-----------Postbuild Image Validation Starting-----------")
        logging.info(
            "---------------------------------------------------------")

        # Load Configuration Data
        config_path = thebuilder.env.GetValue("PE_VALIDATION_PATH", None)
        if config_path is None:
            logging.info("PE_VALIDATION_PATH not set, Using default configuration")
            logging.info("Review ImageValidation/Readme.md for configuration options.")
        elif not os.path.isfile(config_path):
            logging.error("Invalid PE_VALIDATION_PATH. File not Found")
            return 1

        # Use the default configuration. If a configuration file is provided, merge the two
        # At the top level entries, with the provided configuration taking precedence.
        if not DEFAULT_CONFIG_FILE_PATH.is_file():
            logging.error("Default configuration file not found.")
            return 1
        try:
            with open(DEFAULT_CONFIG_FILE_PATH) as f:
                config_data = yaml.safe_load(f)
        except Exception as e:
            logging.error(f"Error parsing {DEFAULT_CONFIG_FILE_PATH}: [{e}]")
            return 1

        try:
            if config_path:
                with open(config_path) as f:
                    config_data = ImageValidation.merge_config(
                        config_data, yaml.safe_load(f))

        except Exception as e:
            logging.error(f"Error parsing {config_path}: [{e}]")
            return 1

        self.test_manager.config_data = config_data
        self.config_data = config_data
        self.ignore = parse_gitignore_lines(config_data.get("IGNORE_LIST", []), os.path.join(
            thebuilder.ws, "nofile.txt"), thebuilder.ws)

        self.arch_dict = config_data["TARGET_ARCH"]

        count = 0

        # Start Pre-Compiled Image Verification
        fdf_parser = FdfParser()
        dsc_parser = DscParser()

        edk2 = thebuilder.edk2path

        ActiveDsc = edk2.GetAbsolutePathOnThisSystemFromEdk2RelativePath(
            thebuilder.env.GetValue("ACTIVE_PLATFORM"))
        ActiveFdf = edk2.GetAbsolutePathOnThisSystemFromEdk2RelativePath(
            thebuilder.env.GetValue("FLASH_DEFINITION"))

        if ActiveFdf is None:
            logging.info("No FDF found - PE Image Validation skipped")
            return 0

        # parse the DSC and the FDF
        env_vars = thebuilder.env.GetAllBuildKeyValues()
        dsc_parser.SetEdk2Path(edk2)
        dsc_parser.SetInputVars(env_vars).ParseFile(ActiveDsc)

        env_vars.update(dsc_parser.LocalVars)
        fdf_parser.SetEdk2Path(edk2)
        fdf_parser.SetInputVars(env_vars).ParseFile(ActiveFdf)

        # Test all pre-compiled efis described in the fdf
        result = Result.PASS
        for FV_name in fdf_parser.FVs:  # Get all Firmware volumes
            FV_files = fdf_parser.FVs[FV_name]["Files"]
            for fv_file_name in FV_files:  # Iterate over each file in the firmware volume
                fv_file = FV_files[fv_file_name]
                if "PE32" in fv_file:  # Any PE32 section in the FV contains a path to the efi
                    # could have multiple PE32 sections
                    for efi_path in fv_file["PE32"]:
                        efi_path = self._resolve_vars(thebuilder, efi_path)
                        efi_path = edk2.GetAbsolutePathOnThisSystemFromEdk2RelativePath(
                            efi_path)
                        if efi_path is None:
                            logging.warning(
                                "Unable to parse the path to the pre-compiled efi")
                            continue
                        if self.ignore(efi_path):
                            continue
                        logging.debug(
                            f'Performing Image Verification ... {os.path.basename(efi_path)}')
                        if self._validate_image(efi_path, fv_file["type"]) == Result.FAIL:
                            logging.error(f'{os.path.basename(efi_path)} Failed Image Validation.')
                            result = Result.FAIL
                        count += 1
        # End Pre-Compiled Image Verification

        # Start Build Time Compiled Image Verification
        result = Result.PASS
        for arch in thebuilder.env.GetValue("TARGET_ARCH").split():
            efi_path_list = self._walk_directory_for_extension(
                ['.efi'], f'{os.path.join(thebuilder.env.GetValue("BUILD_OUTPUT_BASE"), arch)}')

            for efi_path in efi_path_list:
                if self.ignore(efi_path):
                    continue

                # Perform Image Verification on any output efi's
                # Grab profile from makefile
                if "OUTPUT" in efi_path:
                    efi_inf = Path(efi_path).with_suffix('.inf')
                    if not efi_inf.is_file():
                        logging.warning(
                            f"Cannot find {os.path.basename(efi_inf)} for {os.path.basename(efi_path)}, Skipping...")
                        continue
                    infp = InfParser().SetEdk2Path(thebuilder.edk2path)
                    infp.ParseFile(efi_inf)
                    profile = infp.Dict.get("MODULE_TYPE", "DEFAULT")
                    if profile == "DEFAULT":
                        logging.debug("Failed to parse MODULE_TYPE from INF, using DEFAULT")

                    logging.debug(
                        f'Performing Image Verification ... {os.path.basename(efi_path)}')
                    if self._validate_image(efi_path, profile) == Result.FAIL:
                        logging.error(f'{os.path.basename(efi_path)} Failed Image Validation.')
                        result = Result.FAIL
                    count += 1
        # End Built Time Compiled Image Verification

        endtime = datetime.now()
        delta = endtime - starttime
        logging.info(
            "---------------------------------------------------------")
        logging.info(
            "-----------Postbuild Image Validation Finished-----------")
        logging.info(
            "------------------{:04d} Images Verified-------------------".format(count))
        logging.info(
            "-------------- Running Time (mm:ss): {0[0]:02}:{0[1]:02} --------------".format(divmod(delta.seconds, 60)))
        logging.info(
            "---------------------------------------------------------")

        if result == Result.FAIL:
            return 1
        else:
            return 0

    # Executes run_tests() on the efi
    def _validate_image(self, efi_path, profile="DEFAULT"):
        try:
            pe = PE(efi_path, fast_load=True)
        except Exception:
            logging.error(f'Failed to parse {os.path.basename(efi_path)}')
            return Result.FAIL

        target_config = self.config_data[MACHINE_TYPE[pe.FILE_HEADER.Machine]].get(
            profile)
        if target_config == {}:  # The target_config is present, but empty, therefore, override to default
            profile = "DEFAULT"

        return self.test_manager.run_tests(pe, profile)

    # Resolves variable names matching the $(...) pattern.
    def _resolve_vars(self, thebuilder, s):
        var_pattern = re.compile(r'\$\([^)]*\)')  # Detect $(...) pattern
        env = thebuilder.env
        rs = s
        for match in var_pattern.findall(s):
            var_name = match[2:-1]
            env_var = env.GetValue(var_name) if env.GetValue(
                var_name) is not None else env.GetBuildValue(var_name)
            if env_var is None:
                pass
            rs = rs.replace(match, env_var)
        return rs

    def _walk_directory_for_extension(self, extensionlist: List[str], directory: os.PathLike,
                                      ignorelist: List[str] = None) -> List[os.PathLike]:
        ''' Walks a file directory recursively for all items ending in certain extension
            @extensionlist: List[str] list of file extensions
            @directory: Path - absolute path to directory to start looking
            @ignorelist: List[str] or None.  optional - default is None: a list of case insensitive filenames to ignore
            @returns a List of file paths to matching files
        '''
        if not isinstance(extensionlist, list):
            logging.critical("Expected list but got " +
                             str(type(extensionlist)))
            raise TypeError("extensionlist must be a list")

        if directory is None:
            logging.critical("No directory given")
            raise TypeError("directory is None")

        if not os.path.isabs(directory):
            logging.critical(f"Directory {directory} not an abs path")
            raise ValueError(f"directory {directory} is not an absolute path")

        if not os.path.isdir(directory):
            logging.critical(f"Directory {directory} is invalid")
            raise ValueError(f"directory {directory} is not a valid directory path")

        if ignorelist is not None:
            if not isinstance(ignorelist, list):
                logging.critical("Expected list but got " +
                                 str(type(ignorelist)))
                raise TypeError("ignorelist must be a list")

            ignorelist_lower = list()
            for item in ignorelist:
                ignorelist_lower.append(item.lower())

        extensionlist_lower = list()
        for item in extensionlist:
            extensionlist_lower.append(item.lower())

        returnlist = list()
        for Root, Dirs, Files in os.walk(directory):
            for File in Files:
                for Extension in extensionlist_lower:
                    if File.lower().endswith(Extension):
                        ignoreIt = False
                        if(ignorelist is not None):
                            for c in ignorelist_lower:
                                if(File.lower().startswith(c)):
                                    ignoreIt = True
                                    break
                        if not ignoreIt:
                            returnlist.append(os.path.join(Root, File))

        return returnlist

    # Merged two configuration dictionaries, with the provided configuration taking precedence
    # config = { **default, **provided } is shallow and merged only top level entries. We want
    # to be able to replace individual profiles per architecture.
    def merge_config(default: dict, provided: dict) -> dict:

        ret_dict = {}

        # Take these top level entries from the provided configuration if available
        ret_dict["TARGET_ARCH"] = provided.get("TARGET_ARCH", default["TARGET_ARCH"])
        ret_dict["IGNORE_LIST"] = provided.get("IGNORE_LIST", default["IGNORE_LIST"])

        # Take all configuration profiles for each architecture, from the default but allow
        # for overrides per profile (DEFAULT, SEC, DEX_DRIVER, etc.)
        ret_dict["IMAGE_FILE_MACHINE_AMD64"] = default["IMAGE_FILE_MACHINE_AMD64"]
        ret_dict["IMAGE_FILE_MACHINE_ARM64"] = default["IMAGE_FILE_MACHINE_ARM64"]
        ret_dict["IMAGE_FILE_MACHINE_I386"] = default["IMAGE_FILE_MACHINE_I386"]
        ret_dict["IMAGE_FILE_MACHINE_ARM"] = default["IMAGE_FILE_MACHINE_ARM"]

        # Update the default configuration with the provided configuration
        ret_dict["IMAGE_FILE_MACHINE_AMD64"].update(
            provided.get("IMAGE_FILE_MACHINE_AMD64", provided.get("X64", {}))
        )

        ret_dict["IMAGE_FILE_MACHINE_ARM64"].update(
            provided.get("IMAGE_FILE_MACHINE_ARM64", provided.get("AARCH64", {}))
        )

        ret_dict["IMAGE_FILE_MACHINE_I386"].update(
            provided.get("IMAGE_FILE_MACHINE_I386", provided.get("IA32", {}))
        )

        ret_dict["IMAGE_FILE_MACHINE_ARM"].update(
            provided.get("IMAGE_FILE_MACHINE_ARM", provided.get("ARM", {}))
        )

        return ret_dict
