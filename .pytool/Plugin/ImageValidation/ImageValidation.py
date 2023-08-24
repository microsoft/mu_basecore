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
from edk2toolext.image_validation import *
from edk2toollib.uefi.edk2.path_utilities import Edk2Path
from edk2toollib.uefi.edk2.parsers.inf_parser import InfParser
from edk2toollib.uefi.edk2.parsers.fdf_parser import FdfParser
from edk2toollib.uefi.edk2.parsers.dsc_parser import DscParser
from edk2toollib.uefi.edk2.parsers.dsc_parser import *
import json
from typing import List
import logging
from datetime import datetime


class ImageValidation(IUefiBuildPlugin):
    def __init__(self):
        self.test_manager = TestManager()

        # Default tests provided by edk2toolext.image_validation
        self.test_manager.add_test(TestWriteExecuteFlags())
        self.test_manager.add_test(TestSectionAlignment())
        self.test_manager.add_test(TestSubsystemValue())
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
        tool_chain_tag = thebuilder.env.GetValue("TOOL_CHAIN_TAG")
        if config_path is None:
            logging.info(
                "PE_VALIDATION_PATH not set, PE Image Validation Skipped")
            return 0  # Path not set, Plugin skipped

        if not os.path.isfile(config_path):
            logging.error("Invalid PE_VALIDATION_PATH. File not Found")
            return 1

        with open(config_path) as jsonfile:
            config_data = json.load(jsonfile)

        self.test_manager.config_data = config_data
        self.config_data = config_data
        self.ignore_list = config_data["IGNORE_LIST"]
        self.arch_dict = config_data["TARGET_ARCH"]

        count = 0

        # Start Pre-Compiled Image Verification
        fdf_parser = FdfParser()
        dsc_parser = DscParser()

        ws = thebuilder.ws
        pp = thebuilder.pp.split(os.pathsep)
        edk2 = Edk2Path(ws, pp)

        ActiveDsc = edk2.GetAbsolutePathOnThisSystemFromEdk2RelativePath(
            thebuilder.env.GetValue("ACTIVE_PLATFORM"))
        ActiveFdf = edk2.GetAbsolutePathOnThisSystemFromEdk2RelativePath(
            thebuilder.env.GetValue("FLASH_DEFINITION"))

        if ActiveFdf is None:
            logging.info("No FDF found - PE Image Validation skipped")
            return 0

        # parse the DSC and the FDF
        dsc_parser.SetEdk2Path(edk2)
        dsc_parser.SetInputVars(thebuilder.env.GetAllBuildKeyValues()).ParseFile(
            ActiveDsc)  # parse the DSC for build vars
        fdf_parser.SetEdk2Path(edk2)
        fdf_parser.SetInputVars(dsc_parser.LocalVars).ParseFile(
            ActiveFdf)  # give FDF parser the vars from DSC

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
                        if efi_path == None:
                            logging.warning(
                                "Unable to parse the path to the pre-compiled efi")
                            continue
                        if os.path.basename(efi_path) in self.ignore_list:
                            continue
                        logging.info(
                            f'Performing Image Verification ... {os.path.basename(efi_path)}')
                        if self._validate_image(efi_path, fv_file["type"]) == Result.FAIL:
                            result = Result.FAIL
                        count += 1
        # End Pre-Compiled Image Verification

        # Start Build Time Compiled Image Verification
        result = Result.PASS
        for arch in thebuilder.env.GetValue("TARGET_ARCH").split():
            efi_path_list = self._walk_directory_for_extension(
                ['.efi'], f'{thebuilder.env.GetValue("BUILD_OUTPUT_BASE")}/{arch}')

            for efi_path in efi_path_list:
                if os.path.basename(efi_path) in self.ignore_list:
                    continue

                # Perform Image Verification on any output efi's
                # Grab profile from makefile
                if efi_path.__contains__("OUTPUT"):
                    try:
                        if tool_chain_tag.__contains__("VS"):
                            profile = self._get_profile_from_makefile(
                                f'{Path(efi_path).parent.parent}/Makefile')

                        elif tool_chain_tag.__contains__("GCC"):
                            profile = self._get_profile_from_makefile(
                                f'{Path(efi_path).parent.parent}/GNUmakefile')

                        elif tool_chain_tag.__contains__("CLANG"):
                            profile = self._get_profile_from_makefile(
                                f'{Path(efi_path).parent.parent}/GNUmakefile')
                        else:
                            logging.warning("Unexpected TOOL_CHAIN_TAG... Cannot parse makefile. Using DEFAULT profile.")
                            profile = "DEFAULT"
                    except:
                        logging.warning(f'Failed to parse makefile at [{Path(efi_path).parent.parent}/GNUmakefile]')
                        logging.warning(f'Using DEFAULT profile')
                        profile = "DEFAULT"

                    logging.info(
                        f'Performing Image Verification ... {os.path.basename(efi_path)}')
                    if self._validate_image(efi_path, profile) == Result.FAIL:
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
        pe = PE(efi_path)

        target_config = self.config_data[MACHINE_TYPE[pe.FILE_HEADER.Machine]].get(
            profile)
        if target_config == {}:  # The target_config is present, but empty, therefore, override to default
            profile = "DEFAULT"

        return self.test_manager.run_tests(pe, profile)

    # Reads the Makefile of an efi, if present, to determine profile
    def _get_profile_from_makefile(self, makefile):
        with open(makefile) as file:
            for line in file.readlines():
                if line.__contains__('MODULE_TYPE'):
                    line = line.split('=')
                    module_type = line[1]
                    module_type = module_type.strip()
                    return module_type
        return "DEFAULT"

    # Attempts to convert shorthand arch such as X64 to the
    # Fully describe architecture. Additional support for
    # Fallback architectures can be added here
    def _try_convert_full_arch(self, arch):
        full_arch = self.arch_dict.get(arch)
        if full_arch == None:
            if arch.__contains__("ARM"):
                full_arch = "IMAGE_FILE_MACHINE_ARM"
            # Add other Arches
        return full_arch

    # Resolves variable names matching the $(...) pattern.
    def _resolve_vars(self, thebuilder, s):
        var_pattern = re.compile(r'\$\([^)]*\)')  # Detect $(...) pattern
        env = thebuilder.env
        rs = s
        for match in var_pattern.findall(s):
            var_name = match[2:-1]
            env_var = env.GetValue(var_name) if env.GetValue(
                var_name) != None else env.GetBuildValue(var_name)
            if env_var == None:
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
            logging.critical("Directory not abs path")
            raise ValueError("directory is not an absolute path")

        if not os.path.isdir(directory):
            logging.critical("Invalid find directory to walk")
            raise ValueError("directory is not a valid directory path")

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
