##
# This file creates files for a platform to include
# - The CryptoDriverBin_.inf files (one for each flavor, phase, and target) that pulls in the right binary from the nuget ext_dep
# - The CryptoDriver FDF includes, which are included by a platform FDF (BOOTBLOCK and DXE)
# - The CryptoDrive.inc.dsc which is a file included by the platform DSC that setups a platform to use the binary pre-packaged version
#
# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import os
import sys
import glob
SCRIPT_DIR = os.path.dirname(__file__)
sys.path.append(os.path.join(SCRIPT_DIR, "..", "Packaging"))
try:
    from generate_cryptodriver import get_flavors
    from generate_cryptodriver import generate_file_replacement
except:
    print("Failed to import")
    sys.exit(1)


def delete_files_of_pattern(pattern):
    files = glob.iglob(os.path.join(SCRIPT_DIR, pattern))
    for file_path in files:
        os.remove(file_path)


def get_supported_module_types(phase):
    phase = phase.upper()
    if phase == "PEI":
        return ["PEIM", ]
    elif phase == "DXE":
        return ["DXE_DRIVER", "UEFI_DRIVER", "UEFI_APPLICATION"]
    elif phase == "SMM":
        return ["DXE_SMM_DRIVER", ]
    return ["", ]


def get_supported_library_types(phase):
    phase = phase.upper()
    if phase == "PEI":
        return ["PEIM", "PEI_CORE"]
    elif phase == "DXE":
        return ["DXE_DRIVER", "UEFI_DRIVER", "UEFI_APPLICATION", "DXE_CORE"]
    elif phase == "SMM":
        return ["DXE_SMM_DRIVER", "SMM_CORE"]
    return ["", ]


def main():
    class options():
        out_dir = SCRIPT_DIR
        in_dir = SCRIPT_DIR
        verbose = False
    flavors = get_flavors()
    phases = ["Pei", "Dxe", "Smm"]
    # Arm is currently disabled
    arches = ["X64", "AARCH64", "IA32", ]  # "ARM"
    targets = ["DEBUG", "RELEASE"]

    # first we need to generate the INF files
    inf_files = []
    for flavor in flavors:
        for phase in phases:
            for target in targets:
                for arch in arches:
                    if arch == "ARM":
                        continue
                    if arch == "AARCH64" and phase != "Dxe":
                        continue
                    inf_files.append((flavor, phase, target, arch))
    print(f"Generating {len(inf_files)} inf files")
    # first delete any files that we don't need
    inf_start = "CryptoDriverBin"
    delete_files_of_pattern(f"{inf_start}*.inf")
    dsc_ci_lines = []

    # generate the inf files that include the binary files from nuget
    for flavor, phase, target, arch in inf_files:
        inf_lines = []
        guid = flavors[flavor]["guid"]
        module_types = {
            "Dxe": "DXE_DRIVER",
            "Pei": "PEIM",
            "Smm": "DXE_SMM_DRIVER"
        }
        mod_type = module_types[phase]
        original_guid = guid
        if arch == "IA32":
            guid = guid[:-4] + "1000"
        elif arch == "X64":
            guid = guid[:-4] + "2000"
        elif arch == "ARM":
            guid = guid[:-4] + "3000"
        elif arch == "AARCH64":
            guid = guid[:-4] + "4000"

        if target == "RELEASE":
            guid = guid[:-3] + "E00"
        elif target == "DEBUG":
            guid = guid[:-3] + "D00"

        if phase == "Pei":
            guid = guid[:-2] + "10"
        elif phase == "Dxe":
            guid = guid[:-2] + "20"
        elif phase == "Smm":
            guid = guid[:-2] + "30"
        if len(guid) != len(original_guid):
            raise ValueError(f"{guid} is not long enough. {len(guid)} vs {len(original_guid)}")
        inf_lines.extend(["[Defines]",
                          "  INF_VERSION                    = 0x0001001B",
                          f"  BASE_NAME                      = BaseCryptoDriver{phase}{arch}",
                          "  MODULE_UNI_FILE                = Crypto.uni",
                          f"  FILE_GUID                      = {guid}",
                          f"  MODULE_TYPE                    = {mod_type}",
                          "  VERSION_STRING                 = 1.0",
                          f"  ENTRY_POINT                    = Crypto{phase}Entry"])
        if phase == "Smm":
            inf_lines.append("  PI_SPECIFICATION_VERSION       = 0x00010014")
        inf_lines.append(f"\n[Binaries.{arch}]")
        inf_lines.append(f"  PE32|edk2-basecrypto-driver-bin_extdep/{flavor}/{target}/{arch}/Crypto{phase}.efi|{target}")
        inf_lines.append(f"  {phase.upper()}_DEPEX|edk2-basecrypto-driver-bin_extdep/{flavor}/{target}/Crypto{phase}.depex|{target}")
        inf_lines.append("\n[Packages]")
        inf_lines.append("  CryptoPkg/CryptoPkg.dec")
        inf_lines.append("")
        inf_lines.append("[Depex]")
        inf_lines.append("  TRUE")
        inf_filename = f"{inf_start}_{flavor}_{phase}_{target}_{arch}.inf"
        # Add to the CI 
        dsc_ci_lines.append(f"[Components.{arch}]")
        dsc_ci_lines.append("  CryptoPkg/Driver/Bin/" + inf_filename)
        generate_file_replacement(inf_lines, None, inf_filename, options(), comment="#")
    
    # now we generate the CI DSC include     
    generate_file_replacement(dsc_ci_lines, None, "CryptoPkg.ci.inc.dsc", options(), comment="#")

    # now we generate the DSC include
    # start with making sure variables are defined
    dsc_lines = []
    dsc_lines.append("# this is to be included by a platform :)")
    dsc_lines.append("[Defines]")
    all_flavors = "ALL "+" ".join(list(flavors))
    for phase in phases:
        phase = phase.upper()
        dsc_lines.append(f"!ifndef {phase}_CRYPTO_SERVICES")
        dsc_lines.append(f" !error Please define {phase}_CRYPTO_SERVICES")
        dsc_lines.append("!endif")
        dsc_lines.append(f"!if $({phase}_CRYPTO_SERVICES) IN \"{all_flavors}\"")
        dsc_lines.append(" # we don't have a problem")
        dsc_lines.append("!else")
        dsc_lines.append(f" !error CRYPTO_SERVICES must be set to one of {all_flavors}.")
        dsc_lines.append("!endif")
        dsc_lines.append(f"!ifndef {phase}_CRYPTO_ARCH")        
        dsc_lines.append(f" !error Please define {phase}_CRYPTO_ARCH for your platform")
        dsc_lines.append("!endif")
        dsc_lines.append("")

    # generate the components to include in the DSC
    for flavor in flavors:
        for phase in phases:
            upper_phase = phase.upper()
            comp_types = get_supported_module_types(phase)

            dsc_lines.append(f"!if $({upper_phase}_CRYPTO_SERVICES) == {flavor}")
            for arch in arches:
                comp_str = ", ".join(map(lambda x: "Components."+arch+"."+x.upper(), comp_types))
                dsc_lines.append(f" !if $({upper_phase}_CRYPTO_ARCH) == {arch}")
                dsc_lines.append(f"  [{comp_str}]")
                dsc_lines.append(f"    CryptoPkg/Driver/Bin/{inf_start}_{flavor}_{phase}_$(TARGET)_{arch}.inf ")
                dsc_lines.append(" !endif")
            dsc_lines.append("")
            # Add the library as well
            comp_types = get_supported_module_types(phase)
            comp_str = ", ".join(map(lambda x: "Components."+x.upper(), comp_types))
            dsc_lines.append(f" [{comp_str}]")
            dsc_lines.append(f"   CryptoPkg/Library/BaseCryptLibOnProtocolPpi/{phase}CryptLib.inf " + "{")
            dsc_lines.append("     <PcdsFixedAtBuild>")
            dsc_lines.append(f"      !include CryptoPkg/Driver/Packaging/Crypto.pcd.{flavor}.inc.dsc")
            dsc_lines.append("    }")
            dsc_lines.append("!endif\n")
    dsc_lines.append("")
    # generate the library classes to include
    dsc_lines.append("# LibraryClasses for ")
    for phase in phases:
        comp_types = get_supported_library_types(phase)
        upper_phase = phase.upper()
        for arch in arches:
            dsc_lines.append(f"!if $({upper_phase}_CRYPTO_ARCH) == {arch}")
            lib_class_str = ", ".join(map(lambda x: ".".join(["LibraryClasses", arch, x.upper()]), comp_types))
            dsc_lines.append(f"  [{lib_class_str}]")
            dsc_lines.append(f"    BaseCryptLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/{phase}CryptLib.inf")
            dsc_lines.append(f"    TlsLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/{phase}CryptLib.inf")
            dsc_lines.append("!endif\n")

    generate_file_replacement(dsc_lines, None, "CryptoDriver.inc.dsc", options(), comment="#")

    # now we generate the FDF includes (there should be two, one for BOOTBLOCK and one for DXE)
    fdf_bb_lines = []
    fdf_bb_lines.append("# this is to be included a platform inside the BOOTBLOCK or other PEI FV")
    fdf_bb_lines.append("!ifndef PEI_CRYPTO_SERVICES")
    fdf_bb_lines.append("!error You need to define PEI_CRYPTO_SERVICES")
    fdf_bb_lines.append("!endif")
    for flavor in flavors:
        fdf_bb_lines.append(f"!if $(PEI_CRYPTO_SERVICES) == {flavor}")
        for target in targets:
            fdf_bb_lines.append(f" !if $(TARGET) == {target}")
            fdf_bb_lines.append(f"    INF  CryptoPkg/Driver/Bin/{inf_start}_{flavor}_Pei_{target}_$(PEI_CRYPTO_ARCH).inf")
            fdf_bb_lines.append("  !endif")
        fdf_bb_lines.append("!endif\n")
    generate_file_replacement(fdf_bb_lines, None, "CryptoDriver.BOOTBLOCK.inc.fdf", options(), comment="#")

    fdf_dxe_lines = []
    fdf_dxe_lines.append("# this is to be included a platform inside the BOOTBLOCK or other PEI FV")
    fdf_dxe_lines.append("!ifndef DXE_CRYPTO_SERVICES")
    fdf_dxe_lines.append(" !error You need to define in your platform DXE_CRYPTO_SERVICES")
    fdf_dxe_lines.append("!endif")
    fdf_dxe_lines.append("!ifndef SMM_CRYPTO_SERVICES")
    fdf_dxe_lines.append(" !error You need to define in your platform SMM_CRYPTO_SERVICES")
    fdf_dxe_lines.append("!endif")
    fdf_dxe_lines.append("")
    for target in targets:
        fdf_dxe_lines.append(f"!if $(TARGET) == {target}")
        fdf_dxe_lines.append(f"  INF  CryptoPkg/Driver/Bin/CryptoDriverBin_$(DXE_CRYPTO_SERVICES)_Dxe_{target}_$(DXE_CRYPTO_ARCH).inf")
        fdf_dxe_lines.append(f"  INF  CryptoPkg/Driver/Bin/CryptoDriverBin_$(SMM_CRYPTO_SERVICES)_Smm_{target}_$(SMM_CRYPTO_ARCH).inf")
        fdf_dxe_lines.append("!endif")
    generate_file_replacement(fdf_dxe_lines, None, "CryptoDriver.DXE.inc.fdf", options(), comment="#")


if __name__ == "__main__":
    main()
