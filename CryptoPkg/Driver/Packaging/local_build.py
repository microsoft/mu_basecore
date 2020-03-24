##
# this script is for approximating what happens on the server and can be used to build shared_Crypto locally
# It assumes that stuart is setup and able to be invoked from the command line
# please see https://github.com/tianocore/edk2-pytool-extensions for more info
#
# Copyright (c) Microsoft Corporation
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import os
import sys
from edk2toollib.utility_functions import RunCmd
import logging
import shutil
import glob
from generate_cryptodriver import get_flavors


def map_path(path: str):
    path = path.lstrip(os.path.sep)
    path_parts = path.split(os.path.sep)
    if len(path_parts) > 1:
        path_parts[0] = path_parts[0].split("_")[0].upper()
        # if path_parts[0] not in ["DEBUG, RELEASE"]:
        #    print(f"Invalid { path_parts[0]}")
        #    return None
    else:
        return None
    if path.endswith(".depex"):
        path_parts = [path_parts[0], path_parts[-1]]
        # \DEBUG_VS2017\X64\CryptoPkg\Driver\85F7EA15-3A2B-474A-8875-180542CD6BF3\OUTPUT\CryptoSmm.depex
    elif len(path_parts) > 3:
        return None
    return os.path.sep.join(path_parts)


def move_with_mapping(full_path: str, rel_path: str, output_dir: str, verbose=False):
    mapped_path = map_path(rel_path)
    if mapped_path == None:
        if verbose:
            print(f"Skipping {rel_path}")
        return
    out_path = os.path.join(output_dir, mapped_path)
    if verbose:
        print(out_path)
    out_path_dir = os.path.dirname(out_path)
    if not os.path.exists(out_path_dir):
        os.makedirs(out_path_dir)
    if os.path.exists(out_path):
        if verbose:
            print(f"We've already got this one: {out_path}")
    else:
        shutil.copyfile(full_path, out_path)


if __name__ == "__main__":
    clean = True
    verbose = False
    setup = True
    script_dir = os.path.dirname(__file__)

    root_dir = os.path.abspath(os.path.join(script_dir, "..", "..", ".."))
    nuget_collection_dir = os.path.join(root_dir, "Build", "CryptoDriver_Nuget")
    pytools_config = os.path.join(root_dir, ".pytool", "CISettings.py")
    build_output = os.path.join(root_dir, "Build", "CryptoPkg")
    # check if we have the pytool config as a sanity check
    if not os.path.exists(pytools_config):
        print(f"Make sure the pytool config is correct: {pytools_config}")
        sys.exit(2)
    # clean out our collection dir
    if clean and os.path.exists(nuget_collection_dir):
        print(f"Clearing out {nuget_collection_dir}")
        shutil.rmtree(nuget_collection_dir, ignore_errors=True)
    if not os.path.exists(nuget_collection_dir):
        os.makedirs(nuget_collection_dir)

    if verbose:
        logging.getLogger().setLevel(logging.INFO)

    # make sure we're doing a clean build if requested
    if clean:
        shutil.rmtree(build_output, ignore_errors=True)

    commands = ["stuart_setup", "stuart_ci_setup",
                "stuart_update"] if setup else []
    # first we do the setup and update and whatnot
    for command in commands:
        print(command)
        ret = RunCmd(command, f"-c {pytools_config}", workingdir=root_dir)
        if ret != 0:
            print(f"{command} failed with code: {ret}")
            sys.exit(ret)

    # copy the files we care about to the folder
    files = ["License.txt", "../readme.md"]
    for file_path in files:
        file_name = os.path.basename(file_path)
        shutil.copyfile(os.path.join(script_dir, file_path),
                        os.path.join(nuget_collection_dir, file_name))

    # now we do the build
    flavors = list(get_flavors())
    flavors.append("ALL")  # the all flavor is implicitly defined
    build_command = "stuart_ci_build"
    for service in flavors:
        # First we need to clean out the previous builds Build\CryptoPkg\DEBUG_VS2017\X64\CryptoPkg\Driver
        if clean:
            old_build_folders = glob.iglob(os.path.join(build_output, "*", "*", "CryptoPkg", "Driver"))
            for old_build_folder in old_build_folders:
                shutil.rmtree(old_build_folder)
                if verbose:
                    print(f"Removing {old_build_folder}")

        nuget_output_dir = os.path.join(nuget_collection_dir, service)
        os.mkdir(nuget_output_dir)
        print(f"{build_command}: {service}")
        params = f"-p CryptoPkg BLD_*_CRYPTO_SERVICES={service} BUILDREPORTING=TRUE BUILDREPORT_TYPES=\"LIBRARY DEPEX PCD BUILD_FLAGS\" TOOL_CHAIN_TAG=VS2019"
        ret = RunCmd(build_command, f"-c {pytools_config} -t RELEASE,DEBUG {params}", workingdir=root_dir)
        if ret != 0:
            print(f"{build_command} failed with code: {ret}")
            sys.exit(ret)

        # find all the things we want to find
        build_reports = glob.iglob(os.path.join(build_output, "*", "Build_REPORT.TXT"), recursive=True)
        for build_report in build_reports:
            # remove the beginning off the found path
            build_report_rel_path = build_report[len(build_output):]
            move_with_mapping(build_report, build_report_rel_path, nuget_output_dir, verbose)

        efi_searches = [os.path.join(build_output, "**", "Crypto*.efi"), os.path.join(build_output, "**", "Crypto*.depex")]
        for efi_search in efi_searches:
            efi_files = glob.iglob(efi_search, recursive=True)
            for efi_file in efi_files:
                # remove the beginning off the found path
                efi_rel_path = efi_file[len(build_output):]
                move_with_mapping(efi_file, efi_rel_path,
                                  nuget_output_dir, verbose)

    print("All done")
    sys.exit(0)
