# .ver file specification

## 1 Introduction

This document describes file format for .ver files. The end goal is to add a
VERSIONINFO resource to a UEFI driver or application. The .ver file provides
a convenient method to declare the version information. The build system first
converts the .VER file to a resource-definition file (.ARC), which is then
compiled into a binary resource file and finally linked to the module.

The .ver file is referred to as the version info file. VERSIONINFO refers to
the format that is specified by the resource compiler.

### 1.1 Related Information

The following publications and sources of information may be useful to you or
are referred to by this specification:

- [_Unified Extensible Firmware Interface Specifications_](http://uefi.org/specifications)
- [EDK2 Documents](http://www.tianocore.org/docs/EDK_II_Documents.html)
  - _EDK II Build Specification,_ Intel, 2016.
  - _EDK II DEC File Specification_, Intel, 2016.
  - _EDK II INF File Specification_, Intel, 2016.
  - _EDK II DSC File Specification_, Intel, 2016.
  - _EDK II FDF File Specification_, Intel, 2016.
  - _EDK II Expression Syntax Specification_, Intel, 2015.
  - _EDK II C Coding Standards Specification_, Intel, 2015.

Copyright (c) Microsoft

### 1.2 Terms

The following terms are used throughout this document:

#### EFI

Generic term that refers to one of the versions of the EFI specification: EFI
1.02, EFI 1.10, or UEFI 2.0.

#### Module

A module is either an executable image or a library instance. For a list of
module types supported by this package, see module type.

#### UEFI Application

An application that follows the UEFI specification. The only difference between
a UEFI application and a UEFI driver is that an application is unloaded from
memory when it exits regardless of return status, while a driver that returns a
successful return status is not unloaded when its entry point exits.

#### UEFI Driver

A driver that follows the UEFI specification.

## 2 Version Info File Format

The format is JSON files with the extension .ver that encodes the VERSIONINFO
metadata according to the
[PE spec here](https://docs.microsoft.com/en-us/windows/win32/menurc/versioninfo-resource).

## 2.1 Version Info file examples

When specifying a version info file (.ver), there are two modes: minimal and full.

An example of the minimal file would be:

```json
{
        "FileVersion": "1.0.0.0",
        "CompanyName": "Example Company",
        "OriginalFilename": "ExampleApp.efi",
}
```

or equivalently:

```json
{
        "Minimal": true,
        "FileVersion": "1.0.0.0",
        "CompanyName": "Example Company",
        "OriginalFilename": "ExampleApp.efi",
}
```

The `Minimal` attribute if not defined is considered to be true.
Other values are inferred using the defaults specified in this document.
However, if `Minimal` is set to false, you must define more attributes.
However, many more additional attributes can be included. Details are
listed on the [VERSIONINFO spec here](https://docs.microsoft.com/en-us/windows/win32/menurc/versioninfo-resource).

A full example follows:

```json
{
        "Minimal": false,
        "FileVersion": "1.0.0.0",
        "ProductVersion": "1.0.0.0",
        "FileFlagsMask": "VS_FFI_FILEFLAGSMASK",
        "FileFlags": "0",
        "FileOS": "VOS_NT",
        "FileType": "VFT_DRV",
        "FileSubtype": "VFT2_DRV_SYSTEM",
        "StringFileInfo": {
            "CompanyName": "Example Company",
            "OriginalFilename": "ExampleApp.efi",
            "FileVersion": "1.0.0.0",
        },
        "VarFileInfo": {
            "Translation": "0x0409 0x04b0"
        }
 }
```

These three examples produce the same results as the values used in the above
example are the default values used by versioninfo_tool.

StringFileInfo and VarFileInfo can both have many more entries.
More information is
[available here](https://docs.microsoft.com/en-us/windows/win32/menurc/versioninfo-resource).

### 2.2 VERSIONINFO data format

The attributes of VERSIONINFO are listed below:

| Attribute      | Description |
|----------------|-------------|
| FileVersion    | Binary version number for the file. The version consists of four 16-bit integers |
| ProductVersion | Binary version number for the product with which the file is distributed. The version parameter is four 16-bit integers |
| FileFlagsMask  | Indicates which bits in the FILEFLAGS statement are valid. For 16-bit Windows, this value is 0x3f. |
| FileFlags      | Attributes of the file. See the spec |
| FileOS         | Operating system for which this file was designed. The fileos parameter can be one of the operating system values given in the spec |
| FileType       | General type of file. The filetype parameter can be one of the file type values listed in the spec. |
| FileSubtype    | Function of the file. The subtype parameter is zero unless the filetype parameter in the FILETYPE statement is VFT_DRV, VFT_FONT, or VFT_VXD. For a list of file subtype values, see the spec.  |
| StringFileInfo | Contains details around the file information such as CompanyName or OriginalFilename |
| VarFileInfo    | Contains data around the language and character set in the file |

The spec referred to above is the
[VERSIONINFO spec](https://docs.microsoft.com/en-us/windows/win32/menurc/versioninfo-resource).

For `VarFileInfo` in the .ver file, it has a single member `Translation`, which
is two hexadecimal numbers separated by a space.
The two number represent, the `langId` and `charsetId` respectively.

Currently the recommend value for `Translation` is _"0x0409 0x04b0"_,
which corresponds to English and Unicode. More information can
[be found here](https://docs.microsoft.com/en-us/windows/win32/menurc/varfileinfo-block).

## 3 Build System Support

The EDK2 build system supports adding version info files (.ver) to UEFI
applications, modules, and drivers. By adding a .ver file to the sources
of the inf, the build system will process it.

An example follows:

```ini
##
#  Sample UEFI Application Reference EDKII Module.
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  INF_VERSION                    = 0x00010005
  BASE_NAME                      = HelloWorld
  MODULE_UNI_FILE                = HelloWorld.uni
  FILE_GUID                      = 6987936E-ED34-44db-AE97-1FA5E4ED2116
  MODULE_TYPE                    = UEFI_APPLICATION
  VERSION_STRING                 = 1.0
  ENTRY_POINT                    = UefiMain

[Sources]
  HelloWorld.c
  HelloWorldStr.uni
  HelloWorld.ver
```

### 3.1 A Detailed Flow

1. First, versioninfo_tool is invoked on the .ver file.
More information [is available here:](https://github.com/tianocore/edk2-pytool-extensions/blob/master/docs/usability/using_versioninfo.md)
or at the edk2-pytool-extensions repo. The versioninfo_tool creates a
temporary file ending in .arc, which is the metadata encoded in the proper
resource compiler format.
2. The .arc file is processed by the Microsoft Resource Compiler
(x86_64-w64-mingw32-windres on Linux and rc.exe on windows) to produce an
object file.
3. This file is then linked in to the resulting binary, the .rsrc section
being present in the final PE formatted image.

### 3.2 Current Constraints

Currently, Linux is not supported. You can track the progress of this work with
[this GitHub issue](https://github.com/microsoft/mu/issues/105).
