#
#  Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
#  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
#  Portions copyright (c) 2011 - 2014, ARM Ltd. All rights reserved.<BR>
#  Copyright (c) 2015, Hewlett-Packard Development Company, L.P.<BR>
#  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
#  Copyright (c) 2017, Microsoft Corporation
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

#
#Increase this version tag any time you want user to get warning about updating this file in the Conf dir.  By default it does not do update existing conf dirs. 
#
#
# 1.06 - Updated VS2015 64 dlink flags to set to 4kb aligned for MAT support.  This matches changes needed for vs2013 made earlier
# 1.07 - Updated VS2015 & VS2013 Build flags to remove /GS- option for DEBUG builds, thus enabling StackCookies
#
# 1.08 - Updated Iasl and Link16.exe path
# 1.09 - ARM/ARM64 tools update
# 1.10 - VS2017 + Gw
# 1.13 - Updated SDK Ver with version currently installed from microsoft website
# 1.14 - Created VSLATESTx86 for supporting latest VS version
# 1.15 - Added ARM,ARM64 proper for prerelease of VS2017 tools.
# 1.16 - Changed /DEBUG to FULL mode for VS2017+
# 1.17 - Asl compilers should be on the path
# 1.18 - Remove VS2013 support
# 1.19 - RC.exe from Windows 10 Kit
# 1.20 - Added debug info to Release Builds

#!VERSION=1.21

IDENTIFIER = Default TOOL_CHAIN_CONF

# 
# VS2015
DEFINE VS2015x86_BIN    = ENV(VS140COMNTOOLS)\..\..\Vc\Bin
#//C:\Program Files (x86)\Microsoft Visual Studio 14.0\Vc\bin
DEFINE VS2015x86_DLL    = ENV(VS140COMNTOOLS)\..\IDE;DEF(VS2015x86_BIN)
#//C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\IDE;DEF(VS2015x86_BIN)
DEFINE VS2015x86_BINX64 = DEF(VS2015x86_BIN)\x86_amd64

#
# VS2017 - New model for VS2017 where there is potential for many versions of the tools.
# If a specific version is required then the user must set both env variables:
## VS150INSTALLPATH:  base install path on system to VC install dir.  Here you will find the Common7 folder, VC folder, etc
## VS150TOOLVER:      version number for the VC compiler tools
## VS2017x86_DLL - x86 tools dir must come first before common7/ide folder.

DEFINE VS2017x86_BIN     = ENV(VS150INSTALLPATH)\VC\Tools\MSVC\ENV(VS150TOOLVER)\bin\HostX86\x86
DEFINE VS2017x86_DLL     = DEF(VS2017x86_BIN);ENV(VS150INSTALLPATH)\Common7\IDE
DEFINE VS2017x86_BINX64  = ENV(VS150INSTALLPATH)\VC\Tools\MSVC\ENV(VS150TOOLVER)\bin\HostX86\x64
DEFINE VS2017x86_BINARM  = ENV(VS150INSTALLPATH)\VC\Tools\MSVC\ENV(VS150TOOLVER)\bin\HostX86\arm
DEFINE VS2017x86_BINA64  = ENV(VS150INSTALLPATH)\VC\Tools\MSVC\ENV(VS150TOOLVER)\bin\HostX86\arm64

DEFINE WINSDK_VERSION   = 10.0.17134.0
DEFINE WINSDK_BIN       = c:\Program Files (x86)\Windows Kits\10\bin\DEF(WINSDK_VERSION)\x64
DEFINE WINSDKx86_BIN    = c:\Program Files (x86)\Windows Kits\10\bin\DEF(WINSDK_VERSION)\x86

DEFINE WIN_IASL_BIN            = iasl.exe
DEFINE WIN_ASL_BIN             = asl.exe

DEFINE IASL_FLAGS              =
DEFINE IASL_OUTFLAGS           = -p
DEFINE MS_ASL_OUTFLAGS         = /Fo=
DEFINE MS_ASL_FLAGS            =

#Intel
DEFINE DEFAULT_WIN_ASL_BIN      = DEF(WIN_IASL_BIN)
DEFINE DEFAULT_WIN_ASL_FLAGS    = DEF(IASL_FLAGS)
DEFINE DEFAULT_WIN_ASL_OUTFLAGS = DEF(IASL_OUTFLAGS)
#DEFINE DEFAULT_WIN_ASL_BIN      = DEF(WIN_ASL_BIN)
#DEFINE DEFAULT_WIN_ASL_FLAGS    = DEF(MS_ASL_FLAGS)
#DEFINE DEFAULT_WIN_ASL_OUTFLAGS = DEF(MS_ASL_OUTFLAGS)

DEFINE MSFT_ASLPP_FLAGS        = /nologo /E /C /FIAutoGen.h
DEFINE MSFT_ASLCC_FLAGS        = /nologo /c /FIAutoGen.h /TC /Dmain=ReferenceAcpiTable
DEFINE MSFT_ASLDLINK_FLAGS     = /NODEFAULTLIB /ENTRY:ReferenceAcpiTable /SUBSYSTEM:CONSOLE


####################################################################################
#
# format: TARGET_TOOLCHAIN_ARCH_COMMANDTYPE_ATTRIBUTE = <string>
# priority:
#         TARGET_TOOLCHAIN_ARCH_COMMANDTYPE_ATTRIBUTE (Highest)
#         ******_TOOLCHAIN_ARCH_COMMANDTYPE_ATTRIBUTE
#         TARGET_*********_ARCH_COMMANDTYPE_ATTRIBUTE
#         ******_*********_ARCH_COMMANDTYPE_ATTRIBUTE
#         TARGET_TOOLCHAIN_****_COMMANDTYPE_ATTRIBUTE
#         ******_TOOLCHAIN_****_COMMANDTYPE_ATTRIBUTE
#         TARGET_*********_****_COMMANDTYPE_ATTRIBUTE
#         ******_*********_****_COMMANDTYPE_ATTRIBUTE
#         TARGET_TOOLCHAIN_ARCH_***********_ATTRIBUTE
#         ******_TOOLCHAIN_ARCH_***********_ATTRIBUTE
#         TARGET_*********_ARCH_***********_ATTRIBUTE
#         ******_*********_ARCH_***********_ATTRIBUTE
#         TARGET_TOOLCHAIN_****_***********_ATTRIBUTE
#         ******_TOOLCHAIN_****_***********_ATTRIBUTE
#         TARGET_*********_****_***********_ATTRIBUTE
#         ******_*********_****_***********_ATTRIBUTE (Lowest)
#
####################################################################################
####################################################################################
#
# Supported Tool Chains
# =====================
#   VS2015x86   -win64-  Requires:
#                             Microsoft Visual Studio 2015 (x86) Professional Edition
#                             Microsoft Windows Server 2003 Driver Development Kit (Microsoft WINDDK) version 3790.1830
#                        Optional:
#                             Required to build platforms or ACPI tables:
#                               Intel(r) ACPI Compiler (iasl.exe) from
#                               https://acpica.org/downloads
#
####################################################################################
####################################################################################
#
# Supported Tool Chain Family
# ===========================
#   MSFT           - Microsoft
#   GCC            - GNU GCC
#   INTEL          - INTEL
#   RVCT           - ARM RealView Toolchain
####################################################################################
####################################################################################
#
# Other Supported Tools
# =====================
#   NASM -- http://www.nasm.us/
#   - NASM 2.10 or later for use with the GCC toolchain family
#   - NASM 2.12.01 or later for use with all other toolchain families
#
####################################################################################
####################################################################################
#
# Intel EFI Byte Code Compiler (Template)
#
####################################################################################
# *_*_EBC_*_FAMILY                   = INTEL
#
# *_*_EBC_PP_PATH                    = C:\Program Files\Intel\EBC\Bin\iec.exe
# *_*_EBC_CC_PATH                    = C:\Program Files\Intel\EBC\Bin\iec.exe
# *_*_EBC_SLINK_PATH                 = C:\Program Files\Intel\EBC\Bin\link.exe
#
# *_*_EBC_SLINK_FLAGS                = /lib /NOLOGO /MACHINE:EBC
# *_*_EBC_PP_FLAGS                   = /nologo /E /TC /FI$(DEST_DIR_DEBUG)/AutoGen.h
# *_*_EBC_CC_FLAGS                   = /nologo /FAcs /c /W3 /WX /FI$(DEST_DIR_DEBUG)/AutoGen.h
# *_*_EBC_DLINK_FLAGS                = "C:\Program Files\Intel\EBC\Lib\EbcLib.lib" /NOLOGO /MACHINE:EBC /OPT:REF /NODEFAULTLIB /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /ALIGN:32 /DRIVER
#
####################################################################################

####################################################################################
#   VS2015x86       - Microsoft Visual Studio 2015 (x86) professional with Intel ASL
#   ASL  - Intel ACPI Source Language Compiler (iasl.exe)
####################################################################################
#   VS2015x86           - Microsoft Visual Studio 2015 (x86) professional Edition with Intel ASL
*_VS2015x86_*_*_FAMILY        = MSFT

*_VS2015x86_*_MAKE_PATH       = DEF(VS2015x86_BIN)\nmake.exe
*_VS2015x86_*_MAKE_FLAG       = /nologo
*_VS2015x86_*_RC_PATH         = DEF(WINSDKx86_BIN)\rc.exe

*_VS2015x86_*_MAKE_FLAGS      = /nologo
*_VS2015x86_*_SLINK_FLAGS     = /NOLOGO /LTCG
*_VS2015x86_*_APP_FLAGS       = /nologo /E /TC
*_VS2015x86_*_PP_FLAGS        = /nologo /E /TC /FIAutoGen.h
*_VS2015x86_*_VFRPP_FLAGS     = /nologo /E /TC /DVFRCOMPILE /FI$(MODULE_NAME)StrDefs.h

*_VS2015x86_*_ASM16_PATH      = DEF(VS2015x86_BIN)\ml.exe

##################
# ASL definitions
##################
*_VS2015x86_*_ASL_PATH        = DEF(WIN_IASL_BIN)
*_VS2015x86_*_ASL_FLAGS       = DEF(DEFAULT_WIN_ASL_FLAGS)
*_VS2015x86_*_ASL_OUTFLAGS    = DEF(DEFAULT_WIN_ASL_OUTFLAGS)
*_VS2015x86_*_ASLCC_FLAGS     = DEF(MSFT_ASLCC_FLAGS)
*_VS2015x86_*_ASLPP_FLAGS     = DEF(MSFT_ASLPP_FLAGS)
*_VS2015x86_*_ASLDLINK_FLAGS  = DEF(MSFT_ASLDLINK_FLAGS)

##################
# IA32 definitions
##################
*_VS2015x86_IA32_*_DLL        = DEF(VS2015x86_DLL)

*_VS2015x86_IA32_MAKE_PATH    = DEF(VS2015x86_BIN)\nmake.exe
*_VS2015x86_IA32_CC_PATH      = DEF(VS2015x86_BIN)\cl.exe
*_VS2015x86_IA32_VFRPP_PATH   = DEF(VS2015x86_BIN)\cl.exe
*_VS2015x86_IA32_ASLCC_PATH   = DEF(VS2015x86_BIN)\cl.exe
*_VS2015x86_IA32_ASLPP_PATH   = DEF(VS2015x86_BIN)\cl.exe
*_VS2015x86_IA32_SLINK_PATH   = DEF(VS2015x86_BIN)\lib.exe
*_VS2015x86_IA32_DLINK_PATH   = DEF(VS2015x86_BIN)\link.exe
*_VS2015x86_IA32_ASLDLINK_PATH= DEF(VS2015x86_BIN)\link.exe
*_VS2015x86_IA32_APP_PATH     = DEF(VS2015x86_BIN)\cl.exe
*_VS2015x86_IA32_PP_PATH      = DEF(VS2015x86_BIN)\cl.exe
*_VS2015x86_IA32_ASM_PATH     = DEF(VS2015x86_BIN)\ml.exe

      *_VS2015x86_IA32_MAKE_FLAGS  = /nologo
  DEBUG_VS2015x86_IA32_CC_FLAGS    = /nologo /arch:IA32 /c /WX /GS- /W4 /Gs32768 /D UNICODE /O1ib2 /GL /FIAutoGen.h /EHs-c- /GR- /GF /Gy /Zi /Gm /Gw
RELEASE_VS2015x86_IA32_CC_FLAGS    = /nologo /arch:IA32 /c /WX /GS- /W4 /Gs32768 /D UNICODE /O1ib2 /GL /FIAutoGen.h /EHs-c- /GR- /GF /Gw
NOOPT_VS2015x86_IA32_CC_FLAGS      = /nologo /arch:IA32 /c /WX /GS- /W4 /Gs32768 /D UNICODE /FIAutoGen.h /EHs-c- /GR- /GF /Gy /Zi /Gm /Od /Gw

  DEBUG_VS2015x86_IA32_ASM_FLAGS   = /nologo /c /WX /W3 /Cx /coff /Zd /Zi
RELEASE_VS2015x86_IA32_ASM_FLAGS   = /nologo /c /WX /W3 /Cx /coff /Zd
NOOPT_VS2015x86_IA32_ASM_FLAGS     = /nologo /c /WX /W3 /Cx /coff /Zd /Zi

  DEBUG_VS2015x86_IA32_NASM_FLAGS  = -Ox -f win32 -g
RELEASE_VS2015x86_IA32_NASM_FLAGS  = -Ox -f win32
NOOPT_VS2015x86_IA32_NASM_FLAGS    = -O0 -f win32 -g

  DEBUG_VS2015x86_IA32_DLINK_FLAGS = /NOLOGO /NODEFAULTLIB /IGNORE:4001 /OPT:REF /OPT:ICF=10 /MAP /ALIGN:32 /SECTION:.xdata,D /SECTION:.pdata,D /MACHINE:X86 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG
RELEASE_VS2015x86_IA32_DLINK_FLAGS = /NOLOGO /NODEFAULTLIB /IGNORE:4001 /IGNORE:4254 /OPT:REF /OPT:ICF=10 /MAP /ALIGN:32 /SECTION:.xdata,D /SECTION:.pdata,D /MACHINE:X86 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG /MERGE:.rdata=.data
NOOPT_VS2015x86_IA32_DLINK_FLAGS   = /NOLOGO /NODEFAULTLIB /IGNORE:4001 /OPT:REF /OPT:ICF=10 /MAP /ALIGN:32 /SECTION:.xdata,D /SECTION:.pdata,D /MACHINE:X86 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG

##################
# X64 definitions
##################
*_VS2015x86_X64_*_DLL         = DEF(VS2015x86_DLL)

*_VS2015x86_X64_CC_PATH       = DEF(VS2015x86_BINX64)\cl.exe
*_VS2015x86_X64_PP_PATH       = DEF(VS2015x86_BINX64)\cl.exe
*_VS2015x86_X64_APP_PATH      = DEF(VS2015x86_BINX64)\cl.exe
*_VS2015x86_X64_VFRPP_PATH    = DEF(VS2015x86_BINX64)\cl.exe
*_VS2015x86_X64_ASLCC_PATH    = DEF(VS2015x86_BINX64)\cl.exe
*_VS2015x86_X64_ASLPP_PATH    = DEF(VS2015x86_BINX64)\cl.exe
*_VS2015x86_X64_ASM_PATH      = DEF(VS2015x86_BINX64)\ml64.exe
*_VS2015x86_X64_SLINK_PATH    = DEF(VS2015x86_BINX64)\lib.exe
*_VS2015x86_X64_DLINK_PATH    = DEF(VS2015x86_BINX64)\link.exe
*_VS2015x86_X64_ASLDLINK_PATH = DEF(VS2015x86_BINX64)\link.exe

  DEBUG_VS2015x86_X64_CC_FLAGS     = /nologo /c /WX /W4 /Gs32768 /D UNICODE /O1ib2s /GL /Gy /FIAutoGen.h /EHs-c- /GR- /GF /Zi /Gm /Gw
RELEASE_VS2015x86_X64_CC_FLAGS     = /nologo /c /WX /GS- /W4 /Gs32768 /D UNICODE /O1ib2s /GL /Gy /FIAutoGen.h /EHs-c- /GR- /GF /Gw
NOOPT_VS2015x86_X64_CC_FLAGS       = /nologo /c /WX /GS- /W4 /Gs32768 /D UNICODE /Gy /FIAutoGen.h /EHs-c- /GR- /GF /Zi /Gm /Od /Gw

  DEBUG_VS2015x86_X64_ASM_FLAGS    = /nologo /c /WX /W3 /Cx /Zd /Zi
RELEASE_VS2015x86_X64_ASM_FLAGS    = /nologo /c /WX /W3 /Cx /Zd
NOOPT_VS2015x86_X64_ASM_FLAGS      = /nologo /c /WX /W3 /Cx /Zd /Zi

  DEBUG_VS2015x86_X64_NASM_FLAGS   = -Ox -f win64 -g
RELEASE_VS2015x86_X64_NASM_FLAGS   = -Ox -f win64
NOOPT_VS2015x86_X64_NASM_FLAGS     = -O0 -f win64 -g

  DEBUG_VS2015x86_X64_DLINK_FLAGS  = /NOLOGO /NODEFAULTLIB /IGNORE:4001 /OPT:REF /OPT:ICF=10 /MAP /ALIGN:4096 /SECTION:.xdata,D /SECTION:.pdata,D /Machine:X64 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG
RELEASE_VS2015x86_X64_DLINK_FLAGS  = /NOLOGO /NODEFAULTLIB /IGNORE:4001 /IGNORE:4254 /OPT:REF /OPT:ICF=10 /MAP /ALIGN:4096 /SECTION:.xdata,D /SECTION:.pdata,D /Machine:X64 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG
NOOPT_VS2015x86_X64_DLINK_FLAGS    = /NOLOGO /NODEFAULTLIB /IGNORE:4001 /OPT:REF /OPT:ICF=10 /MAP /ALIGN:4096 /SECTION:.xdata,D /SECTION:.pdata,D /Machine:X64 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG


####################################################################################
#   VS2017x86       - Microsoft Visual Studio 2017 (x86) professional with Intel ASL
#   ASL  - Intel ACPI Source Language Compiler (iasl.exe)
####################################################################################
#   VS2017x86           - Microsoft Visual Studio 2017 (x86) professional Edition with Intel ASL
*_VS2017x86_*_*_FAMILY        = MSFT

*_VS2017x86_*_MAKE_PATH       = DEF(VS2017x86_BIN)\nmake.exe
*_VS2017x86_*_MAKE_FLAG       = /nologo
*_VS2017x86_*_RC_PATH         = DEF(WINSDKx86_BIN)\rc.exe

*_VS2017x86_*_MAKE_FLAGS      = /nologo
*_VS2017x86_*_SLINK_FLAGS     = /NOLOGO /LTCG
*_VS2017x86_*_APP_FLAGS       = /nologo /E /TC
*_VS2017x86_*_PP_FLAGS        = /nologo /E /TC /FIAutoGen.h
*_VS2017x86_*_VFRPP_FLAGS     = /nologo /E /TC /DVFRCOMPILE /FI$(MODULE_NAME)StrDefs.h

*_VS2017x86_*_ASM16_PATH      = DEF(VS2017x86_BIN)\ml.exe

##################
# ASL definitions
##################
*_VS2017x86_*_ASL_PATH        = DEF(WIN_IASL_BIN)
*_VS2017x86_*_ASL_FLAGS       = DEF(DEFAULT_WIN_ASL_FLAGS)
*_VS2017x86_*_ASL_OUTFLAGS    = DEF(DEFAULT_WIN_ASL_OUTFLAGS)
*_VS2017x86_*_ASLCC_FLAGS     = DEF(MSFT_ASLCC_FLAGS)
*_VS2017x86_*_ASLPP_FLAGS     = DEF(MSFT_ASLPP_FLAGS)
*_VS2017x86_*_ASLDLINK_FLAGS  = DEF(MSFT_ASLDLINK_FLAGS)

##################
# IA32 definitions
##################
#*_VS2017x86_IA32_*_DLL        = DEF(VS2017x86_DLL)

*_VS2017x86_IA32_MAKE_PATH    = DEF(VS2017x86_BIN)\nmake.exe
*_VS2017x86_IA32_CC_PATH      = DEF(VS2017x86_BIN)\cl.exe
*_VS2017x86_IA32_VFRPP_PATH   = DEF(VS2017x86_BIN)\cl.exe
*_VS2017x86_IA32_ASLCC_PATH   = DEF(VS2017x86_BIN)\cl.exe
*_VS2017x86_IA32_ASLPP_PATH   = DEF(VS2017x86_BIN)\cl.exe
*_VS2017x86_IA32_SLINK_PATH   = DEF(VS2017x86_BIN)\lib.exe
*_VS2017x86_IA32_DLINK_PATH   = DEF(VS2017x86_BIN)\link.exe
*_VS2017x86_IA32_ASLDLINK_PATH= DEF(VS2017x86_BIN)\link.exe
*_VS2017x86_IA32_APP_PATH     = DEF(VS2017x86_BIN)\cl.exe
*_VS2017x86_IA32_PP_PATH      = DEF(VS2017x86_BIN)\cl.exe
*_VS2017x86_IA32_ASM_PATH     = DEF(VS2017x86_BIN)\ml.exe

      *_VS2017x86_IA32_MAKE_FLAGS  = /nologo
  DEBUG_VS2017x86_IA32_CC_FLAGS    = /nologo /arch:IA32 /c /WX /GS- /W4 /Gs32768 /D UNICODE /O1ib2 /GL /FIAutoGen.h /EHs-c- /GR- /GF /Gy /Zi /Gm /Gw
RELEASE_VS2017x86_IA32_CC_FLAGS    = /nologo /arch:IA32 /c /WX /GS- /W4 /Gs32768 /D UNICODE /O1ib2 /GL /FIAutoGen.h /EHs-c- /GR- /GF /Gw
NOOPT_VS2017x86_IA32_CC_FLAGS      = /nologo /arch:IA32 /c /WX /GS- /W4 /Gs32768 /D UNICODE /FIAutoGen.h /EHs-c- /GR- /GF /Gy /Zi /Gm /Od /Gw

  DEBUG_VS2017x86_IA32_ASM_FLAGS   = /nologo /c /WX /W3 /Cx /coff /Zd /Zi
RELEASE_VS2017x86_IA32_ASM_FLAGS   = /nologo /c /WX /W3 /Cx /coff /Zd
NOOPT_VS2017x86_IA32_ASM_FLAGS     = /nologo /c /WX /W3 /Cx /coff /Zd /Zi

  DEBUG_VS2017x86_IA32_NASM_FLAGS  = -Ox -f win32 -g
RELEASE_VS2017x86_IA32_NASM_FLAGS  = -Ox -f win32
NOOPT_VS2017x86_IA32_NASM_FLAGS    = -O0 -f win32 -g

  DEBUG_VS2017x86_IA32_DLINK_FLAGS = /NOLOGO /NODEFAULTLIB /IGNORE:4001 /OPT:REF /OPT:ICF=10 /MAP /ALIGN:32 /SECTION:.xdata,D /SECTION:.pdata,D /MACHINE:X86 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG:FULL
RELEASE_VS2017x86_IA32_DLINK_FLAGS = /NOLOGO /NODEFAULTLIB /IGNORE:4001 /IGNORE:4254 /OPT:REF /OPT:ICF=10 /MAP /ALIGN:32 /SECTION:.xdata,D /SECTION:.pdata,D /MACHINE:X86 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG:FULL /MERGE:.rdata=.data
NOOPT_VS2017x86_IA32_DLINK_FLAGS   = /NOLOGO /NODEFAULTLIB /IGNORE:4001 /OPT:REF /OPT:ICF=10 /MAP /ALIGN:32 /SECTION:.xdata,D /SECTION:.pdata,D /MACHINE:X86 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG:FULL

##################
# X64 definitions
##################
*_VS2017x86_X64_*_DLL         = DEF(VS2017x86_DLL)

*_VS2017x86_X64_CC_PATH       = DEF(VS2017x86_BINX64)\cl.exe
*_VS2017x86_X64_PP_PATH       = DEF(VS2017x86_BINX64)\cl.exe
*_VS2017x86_X64_APP_PATH      = DEF(VS2017x86_BINX64)\cl.exe
*_VS2017x86_X64_VFRPP_PATH    = DEF(VS2017x86_BINX64)\cl.exe
*_VS2017x86_X64_ASLCC_PATH    = DEF(VS2017x86_BINX64)\cl.exe
*_VS2017x86_X64_ASLPP_PATH    = DEF(VS2017x86_BINX64)\cl.exe
*_VS2017x86_X64_ASM_PATH      = DEF(VS2017x86_BINX64)\ml64.exe
*_VS2017x86_X64_SLINK_PATH    = DEF(VS2017x86_BINX64)\lib.exe
*_VS2017x86_X64_DLINK_PATH    = DEF(VS2017x86_BINX64)\link.exe
*_VS2017x86_X64_ASLDLINK_PATH = DEF(VS2017x86_BINX64)\link.exe

  DEBUG_VS2017x86_X64_CC_FLAGS     = /nologo /c /WX /W4 /Gs32768 /D UNICODE /O1ib2s /GL /Gy /FIAutoGen.h /EHs-c- /GR- /GF /Zi /Gm /Gw
RELEASE_VS2017x86_X64_CC_FLAGS     = /nologo /c /WX /GS- /W4 /Gs32768 /D UNICODE /O1ib2s /GL /Gy /FIAutoGen.h /EHs-c- /GR- /GF /Gw
NOOPT_VS2017x86_X64_CC_FLAGS       = /nologo /c /WX /GS- /W4 /Gs32768 /D UNICODE /Gy /FIAutoGen.h /EHs-c- /GR- /GF /Zi /Gm /Od /GL /Gw

  DEBUG_VS2017x86_X64_ASM_FLAGS    = /nologo /c /WX /W3 /Cx /Zd /Zi
RELEASE_VS2017x86_X64_ASM_FLAGS    = /nologo /c /WX /W3 /Cx /Zd
NOOPT_VS2017x86_X64_ASM_FLAGS      = /nologo /c /WX /W3 /Cx /Zd /Zi

  DEBUG_VS2017x86_X64_NASM_FLAGS   = -Ox -f win64 -g
RELEASE_VS2017x86_X64_NASM_FLAGS   = -Ox -f win64
NOOPT_VS2017x86_X64_NASM_FLAGS     = -O0 -f win64 -g

  DEBUG_VS2017x86_X64_DLINK_FLAGS  = /NOLOGO /NODEFAULTLIB /IGNORE:4001 /OPT:REF /OPT:ICF=10 /MAP /ALIGN:4096 /SECTION:.xdata,D /SECTION:.pdata,D /Machine:X64 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG:FULL
RELEASE_VS2017x86_X64_DLINK_FLAGS  = /NOLOGO /NODEFAULTLIB /IGNORE:4001 /IGNORE:4254 /OPT:REF /OPT:ICF=10 /MAP /ALIGN:4096 /SECTION:.xdata,D /SECTION:.pdata,D /Machine:X64 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER
NOOPT_VS2017x86_X64_DLINK_FLAGS    = /NOLOGO /NODEFAULTLIB /IGNORE:4001 /OPT:REF /OPT:ICF=10 /MAP /ALIGN:4096 /SECTION:.xdata,D /SECTION:.pdata,D /Machine:X64 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG:FULL

##################
# ARM definitions
##################
*_VS2017x86_ARM_*_DLL         = DEF(VS2017x86_DLL)

*_VS2017x86_ARM_MAKE_PATH     = DEF(VS2017x86_BIN)\nmake.exe
*_VS2017x86_ARM_CC_PATH       = DEF(VS2017x86_BINARM)\cl.exe
*_VS2017x86_ARM_PP_PATH       = DEF(VS2017x86_BINARM)\cl.exe
*_VS2017x86_ARM_APP_PATH      = DEF(VS2017x86_BINARM)\cl.exe
*_VS2017x86_ARM_VFRPP_PATH    = DEF(VS2017x86_BINARM)\cl.exe
*_VS2017x86_ARM_ASLCC_PATH    = DEF(VS2017x86_BINARM)\cl.exe
*_VS2017x86_ARM_ASLPP_PATH    = DEF(VS2017x86_BINARM)\cl.exe
*_VS2017x86_ARM_ASM_PATH      = DEF(VS2017x86_BINARM)\armasm.exe
*_VS2017x86_ARM_SLINK_PATH    = DEF(VS2017x86_BINARM)\link.exe
*_VS2017x86_ARM_DLINK_PATH    = DEF(VS2017x86_BINARM)\link.exe
*_VS2017x86_ARM_ASLDLINK_PATH = DEF(VS2017x86_BINARM)\link.exe

*_VS2017x86_ARM_SLINK_FLAGS   =  /LIB /machine:ARM /SUBSYSTEM:WINDOWSCE

      *_VS2017x86_ARM_MAKE_FLAGS  = /nologo
  DEBUG_VS2017x86_ARM_CC_FLAGS    = /X /nologo $(PLATFORM_FLAGS) /c /WX /GS- /W4 /Gs32768 /D UNICODE /O1ib2 /GL /FIAutoGen.h /EHs-c- /GR- /GF /Gy /Zi /Gm /wd4127 /wd4214 /wd4100 /wd4201 /wd4702
RELEASE_VS2017x86_ARM_CC_FLAGS    = /X /nologo $(PLATFORM_FLAGS) /c /WX /GS- /W4 /Gs32768 /D UNICODE /O1ib2 /GL /FIAutoGen.h /EHs-c- /GR- /GF /wd4127 /wd4214 /wd4100 /wd4201 /wd4702
NOOPT_VS2017x86_ARM_CC_FLAGS      = /X /nologo $(PLATFORM_FLAGS) /c /WX /GS- /W4 /Gs32768 /D UNICODE /FIAutoGen.h /EHs-c- /GR- /GF /Gy /Zi /Gm /Od /wd4127 /wd4214 /wd4100 /wd4201 /wd4702

  DEBUG_VS2017x86_ARM_ASM_FLAGS   = -nologo -g -32 -machine ARMCE
RELEASE_VS2017x86_ARM_ASM_FLAGS   = -nologo -32 -machine ARMCE
NOOPT_VS2017x86_ARM_ASM_FLAGS     = -nologo -g -32 -machine ARMCE

#
# for SEC file/section alignment and XIP, use file type buildoptions
#
# ex: [BuildOptions.common.EDKII.SEC]
#
  DEBUG_VS2017x86_ARM_DLINK_FLAGS = /NOLOGO /NODEFAULTLIB /IGNORE:4001 /OPT:REF /OPT:ICF=10 /MAP /SECTION:.xdata,D /SECTION:.pdata,D /MACHINE:ARM /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG:FULL
RELEASE_VS2017x86_ARM_DLINK_FLAGS = /NOLOGO /NODEFAULTLIB /IGNORE:4001 /IGNORE:4254 /OPT:REF /OPT:ICF=10 /MAP /SECTION:.xdata,D /SECTION:.pdata,D /MACHINE:ARM /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /MERGE:.rdata=.data
NOOPT_VS2017x86_ARM_DLINK_FLAGS   = /NOLOGO /NODEFAULTLIB /IGNORE:4001 /OPT:REF /OPT:ICF=10 /MAP /SECTION:.xdata,D /SECTION:.pdata,D /MACHINE:ARM   /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG:FULL


##################
# AARCH64 definitions
##################

*_VS2017x86_AARCH64_*_DLL         = DEF(VS2017x86_DLL)

*_VS2017x86_AARCH64_MAKE_PATH     = DEF(VS2017x86_BIN)\nmake.exe
*_VS2017x86_AARCH64_CC_PATH       = DEF(VS2017x86_BINA64)\cl.exe
*_VS2017x86_AARCH64_PP_PATH       = DEF(VS2017x86_BINA64)\cl.exe
*_VS2017x86_AARCH64_APP_PATH      = DEF(VS2017x86_BINA64)\cl.exe
*_VS2017x86_AARCH64_VFRPP_PATH    = DEF(VS2017x86_BINA64)\cl.exe
*_VS2017x86_AARCH64_ASLCC_PATH    = DEF(VS2017x86_BINA64)\cl.exe
*_VS2017x86_AARCH64_ASLPP_PATH    = DEF(VS2017x86_BINA64)\cl.exe
*_VS2017x86_AARCH64_ASM_PATH      = DEF(VS2017x86_BINA64)\armasm64.exe
*_VS2017x86_AARCH64_SLINK_PATH    = DEF(VS2017x86_BINA64)\lib.exe
*_VS2017x86_AARCH64_DLINK_PATH    = DEF(VS2017x86_BINA64)\link.exe
*_VS2017x86_AARCH64_ASLDLINK_PATH = DEF(VS2017x86_BINA64)\link.exe

  DEBUG_VS2017x86_AARCH64_CC_FLAGS     = /X /nologo $(PLATFORM_FLAGS) /c /WX /GS- /W4 /Gs32768 /D UNICODE /O1ib2s /GL /Gy /FIAutoGen.h /EHs-c- /GR- /GF /Zi /Gm /wd4214 /wd4127 /wd4100 /wd4312 /wd4702
RELEASE_VS2017x86_AARCH64_CC_FLAGS     = /X /nologo $(PLATFORM_FLAGS) /c /WX /GS- /W4 /Gs32768 /D UNICODE /O1ib2s /GL /Gy /FIAutoGen.h /EHs-c- /GR- /GF /wd4214 /wd4127 /wd4100 /wd4312 /wd4702
NOOPT_VS2017x86_AARCH64_CC_FLAGS       = /X /nologo $(PLATFORM_FLAGS) /c /WX /GS- /W4 /Gs32768 /D UNICODE /Gy /FIAutoGen.h /EHs-c- /GR- /GF /Zi /Gm /Od /wd4702

  DEBUG_VS2017x86_AARCH64_ASM_FLAGS    = -g
RELEASE_VS2017x86_AARCH64_ASM_FLAGS    =
NOOPT_VS2017x86_AARCH64_ASM_FLAGS      = -g

#
# for SEC file/section alignment and XIP, use file type buildoptions
#
# ex: [BuildOptions.common.EDKII.SEC]
#
  DEBUG_VS2017x86_AARCH64_DLINK_FLAGS  = /NOLOGO /NODEFAULTLIB /RunBelow4GB /IGNORE:4001 /IGNORE:4226 /OPT:REF /OPT:ICF=10 /MAP /SECTION:.xdata,D /SECTION:.pdata,D /Machine:ARM64 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG:FULL
RELEASE_VS2017x86_AARCH64_DLINK_FLAGS  = /NOLOGO /NODEFAULTLIB /RunBelow4GB /IGNORE:4001 /IGNORE:4254 /IGNORE:4226 /OPT:REF /OPT:ICF=10 /MAP /SECTION:.xdata,D /SECTION:.pdata,D /Machine:ARM64 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /MERGE:.rdata=.data
NOOPT_VS2017x86_AARCH64_DLINK_FLAGS    = /NOLOGO /NODEFAULTLIB /RunBelow4GB /IGNORE:4001 /OPT:REF /OPT:ICF=10 /MAP /SECTION:.xdata,D /SECTION:.pdata,D /Machine:ARM64 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG:FULL

####################################################################################
#   VSLATESTx86       - Microsoft Visual Studio Latest with Intel ASL  (always moving to latest tools)
#   ASL  - Intel ACPI Source Language Compiler (iasl.exe)
####################################################################################
*_VSLATESTx86_*_*_FAMILY        = MSFT

*_VSLATESTx86_*_MAKE_PATH       = DEF(VS2017x86_BIN)\nmake.exe
*_VSLATESTx86_*_MAKE_FLAG       = /nologo
*_VSLATESTx86_*_RC_PATH         = DEF(WINSDKx86_BIN)\rc.exe

*_VSLATESTx86_*_MAKE_FLAGS      = /nologo
*_VSLATESTx86_*_SLINK_FLAGS     = /NOLOGO /LTCG
*_VSLATESTx86_*_APP_FLAGS       = /nologo /E /TC
*_VSLATESTx86_*_PP_FLAGS        = /nologo /E /TC /FIAutoGen.h
*_VSLATESTx86_*_VFRPP_FLAGS     = /nologo /E /TC /DVFRCOMPILE /FI$(MODULE_NAME)StrDefs.h

*_VSLATESTx86_*_ASM16_PATH      = DEF(VS2017x86_BIN)\ml.exe

##################
# ASL definitions
##################
*_VSLATESTx86_*_ASL_PATH        = DEF(WIN_IASL_BIN)
*_VSLATESTx86_*_ASL_FLAGS       = DEF(DEFAULT_WIN_ASL_FLAGS)
*_VSLATESTx86_*_ASL_OUTFLAGS    = DEF(DEFAULT_WIN_ASL_OUTFLAGS)
*_VSLATESTx86_*_ASLCC_FLAGS     = DEF(MSFT_ASLCC_FLAGS)
*_VSLATESTx86_*_ASLPP_FLAGS     = DEF(MSFT_ASLPP_FLAGS)
*_VSLATESTx86_*_ASLDLINK_FLAGS  = DEF(MSFT_ASLDLINK_FLAGS)

##################
# IA32 definitions
##################
#*_VSLATESTx86_IA32_*_DLL        = DEF(VS2017x86_DLL)

*_VSLATESTx86_IA32_MAKE_PATH    = DEF(VS2017x86_BIN)\nmake.exe
*_VSLATESTx86_IA32_CC_PATH      = DEF(VS2017x86_BIN)\cl.exe
*_VSLATESTx86_IA32_VFRPP_PATH   = DEF(VS2017x86_BIN)\cl.exe
*_VSLATESTx86_IA32_ASLCC_PATH   = DEF(VS2017x86_BIN)\cl.exe
*_VSLATESTx86_IA32_ASLPP_PATH   = DEF(VS2017x86_BIN)\cl.exe
*_VSLATESTx86_IA32_SLINK_PATH   = DEF(VS2017x86_BIN)\lib.exe
*_VSLATESTx86_IA32_DLINK_PATH   = DEF(VS2017x86_BIN)\link.exe
*_VSLATESTx86_IA32_ASLDLINK_PATH= DEF(VS2017x86_BIN)\link.exe
*_VSLATESTx86_IA32_APP_PATH     = DEF(VS2017x86_BIN)\cl.exe
*_VSLATESTx86_IA32_PP_PATH      = DEF(VS2017x86_BIN)\cl.exe
*_VSLATESTx86_IA32_ASM_PATH     = DEF(VS2017x86_BIN)\ml.exe

      *_VSLATESTx86_IA32_MAKE_FLAGS  = /nologo
  DEBUG_VSLATESTx86_IA32_CC_FLAGS    = /nologo /arch:IA32 /c /WX /GS- /W4 /Gs32768 /D UNICODE /O1ib2 /GL /FIAutoGen.h /EHs-c- /GR- /GF /Gy /Zi /Gm /Gw
RELEASE_VSLATESTx86_IA32_CC_FLAGS    = /nologo /arch:IA32 /c /WX /GS- /W4 /Gs32768 /D UNICODE /O1ib2 /GL /FIAutoGen.h /EHs-c- /GR- /GF /Gw
NOOPT_VSLATESTx86_IA32_CC_FLAGS      = /nologo /arch:IA32 /c /WX /GS- /W4 /Gs32768 /D UNICODE /FIAutoGen.h /EHs-c- /GR- /GF /Gy /Zi /Gm /Od /Gw

  DEBUG_VSLATESTx86_IA32_ASM_FLAGS   = /nologo /c /WX /W3 /Cx /coff /Zd /Zi
RELEASE_VSLATESTx86_IA32_ASM_FLAGS   = /nologo /c /WX /W3 /Cx /coff /Zd
NOOPT_VSLATESTx86_IA32_ASM_FLAGS     = /nologo /c /WX /W3 /Cx /coff /Zd /Zi

  DEBUG_VSLATESTx86_IA32_NASM_FLAGS  = -Ox -f win32 -g
RELEASE_VSLATESTx86_IA32_NASM_FLAGS  = -Ox -f win32
NOOPT_VSLATESTx86_IA32_NASM_FLAGS    = -O0 -f win32 -g

  DEBUG_VSLATESTx86_IA32_DLINK_FLAGS = /NOLOGO /NODEFAULTLIB /IGNORE:4001 /OPT:REF /OPT:ICF=10 /MAP /ALIGN:32 /SECTION:.xdata,D /SECTION:.pdata,D /MACHINE:X86 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG:FULL
RELEASE_VSLATESTx86_IA32_DLINK_FLAGS = /NOLOGO /NODEFAULTLIB /IGNORE:4001 /IGNORE:4254 /OPT:REF /OPT:ICF=10 /MAP /ALIGN:32 /SECTION:.xdata,D /SECTION:.pdata,D /MACHINE:X86 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /MERGE:.rdata=.data /DEBUG:FULL
NOOPT_VSLATESTx86_IA32_DLINK_FLAGS   = /NOLOGO /NODEFAULTLIB /IGNORE:4001 /OPT:REF /OPT:ICF=10 /MAP /ALIGN:32 /SECTION:.xdata,D /SECTION:.pdata,D /MACHINE:X86 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG:FULL

##################
# X64 definitions
##################
*_VSLATESTx86_X64_*_DLL         = DEF(VS2017x86_DLL)

*_VSLATESTx86_X64_CC_PATH       = DEF(VS2017x86_BINX64)\cl.exe
*_VSLATESTx86_X64_PP_PATH       = DEF(VS2017x86_BINX64)\cl.exe
*_VSLATESTx86_X64_APP_PATH      = DEF(VS2017x86_BINX64)\cl.exe
*_VSLATESTx86_X64_VFRPP_PATH    = DEF(VS2017x86_BINX64)\cl.exe
*_VSLATESTx86_X64_ASLCC_PATH    = DEF(VS2017x86_BINX64)\cl.exe
*_VSLATESTx86_X64_ASLPP_PATH    = DEF(VS2017x86_BINX64)\cl.exe
*_VSLATESTx86_X64_ASM_PATH      = DEF(VS2017x86_BINX64)\ml64.exe
*_VSLATESTx86_X64_SLINK_PATH    = DEF(VS2017x86_BINX64)\lib.exe
*_VSLATESTx86_X64_DLINK_PATH    = DEF(VS2017x86_BINX64)\link.exe
*_VSLATESTx86_X64_ASLDLINK_PATH = DEF(VS2017x86_BINX64)\link.exe

  DEBUG_VSLATESTx86_X64_CC_FLAGS     = /nologo /c /WX /W4 /Gs32768 /D UNICODE /O1ib2s /GL /Gy /FIAutoGen.h /EHs-c- /GR- /GF /Zi /Gm /Gw
RELEASE_VSLATESTx86_X64_CC_FLAGS     = /nologo /c /WX /GS- /W4 /Gs32768 /D UNICODE /O1ib2s /GL /Gy /FIAutoGen.h /EHs-c- /GR- /GF /Gw
NOOPT_VSLATESTx86_X64_CC_FLAGS       = /nologo /c /WX /GS- /W4 /Gs32768 /D UNICODE /Gy /FIAutoGen.h /EHs-c- /GR- /GF /Zi /Gm /Od /GL /Gw

  DEBUG_VSLATESTx86_X64_ASM_FLAGS    = /nologo /c /WX /W3 /Cx /Zd /Zi
RELEASE_VSLATESTx86_X64_ASM_FLAGS    = /nologo /c /WX /W3 /Cx /Zd
NOOPT_VSLATESTx86_X64_ASM_FLAGS      = /nologo /c /WX /W3 /Cx /Zd /Zi

  DEBUG_VSLATESTx86_X64_NASM_FLAGS   = -Ox -f win64 -g
RELEASE_VSLATESTx86_X64_NASM_FLAGS   = -Ox -f win64
NOOPT_VSLATESTx86_X64_NASM_FLAGS     = -O0 -f win64 -g

  DEBUG_VSLATESTx86_X64_DLINK_FLAGS  = /NOLOGO /NODEFAULTLIB /IGNORE:4001 /OPT:REF /OPT:ICF=10 /MAP /ALIGN:4096 /SECTION:.xdata,D /SECTION:.pdata,D /Machine:X64 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG:FULL
RELEASE_VSLATESTx86_X64_DLINK_FLAGS  = /NOLOGO /NODEFAULTLIB /IGNORE:4001 /IGNORE:4254 /OPT:REF /OPT:ICF=10 /MAP /ALIGN:4096 /SECTION:.xdata,D /SECTION:.pdata,D /Machine:X64 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG:FULL
NOOPT_VSLATESTx86_X64_DLINK_FLAGS    = /NOLOGO /NODEFAULTLIB /IGNORE:4001 /OPT:REF /OPT:ICF=10 /MAP /ALIGN:4096 /SECTION:.xdata,D /SECTION:.pdata,D /Machine:X64 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG:FULL

##################
# ARM definitions
##################
*_VSLATESTx86_ARM_*_DLL         = DEF(VS2017x86_DLL)

*_VSLATESTx86_ARM_MAKE_PATH     = DEF(VS2017x86_BIN)\nmake.exe
*_VSLATESTx86_ARM_CC_PATH       = DEF(VS2017x86_BINARM)\cl.exe
*_VSLATESTx86_ARM_PP_PATH       = DEF(VS2017x86_BINARM)\cl.exe
*_VSLATESTx86_ARM_APP_PATH      = DEF(VS2017x86_BINARM)\cl.exe
*_VSLATESTx86_ARM_VFRPP_PATH    = DEF(VS2017x86_BINARM)\cl.exe
*_VSLATESTx86_ARM_ASLCC_PATH    = DEF(VS2017x86_BINARM)\cl.exe
*_VSLATESTx86_ARM_ASLPP_PATH    = DEF(VS2017x86_BINARM)\cl.exe
*_VSLATESTx86_ARM_ASM_PATH      = DEF(VS2017x86_BINARM)\armasm.exe
*_VSLATESTx86_ARM_SLINK_PATH    = DEF(VS2017x86_BINARM)\link.exe
*_VSLATESTx86_ARM_DLINK_PATH    = DEF(VS2017x86_BINARM)\link.exe
*_VSLATESTx86_ARM_ASLDLINK_PATH = DEF(VS2017x86_BINARM)\link.exe

*_VSLATESTx86_ARM_SLINK_FLAGS   =  /LIB /machine:ARM /SUBSYSTEM:WINDOWSCE

      *_VSLATESTx86_ARM_MAKE_FLAGS  = /nologo
  DEBUG_VSLATESTx86_ARM_CC_FLAGS    = /X /nologo $(PLATFORM_FLAGS) /c /WX /GS- /W4 /Gs32768 /D UNICODE /O1ib2 /GL /FIAutoGen.h /EHs-c- /GR- /GF /Gy /Zi /Gm /wd4127 /wd4214 /wd4100 /wd4201 /wd4702
RELEASE_VSLATESTx86_ARM_CC_FLAGS    = /X /nologo $(PLATFORM_FLAGS) /c /WX /GS- /W4 /Gs32768 /D UNICODE /O1ib2 /GL /FIAutoGen.h /EHs-c- /GR- /GF /wd4127 /wd4214 /wd4100 /wd4201 /wd4702
NOOPT_VSLATESTx86_ARM_CC_FLAGS      = /X /nologo $(PLATFORM_FLAGS) /c /WX /GS- /W4 /Gs32768 /D UNICODE /FIAutoGen.h /EHs-c- /GR- /GF /Gy /Zi /Gm /Od /wd4127 /wd4214 /wd4100 /wd4201 /wd4702

  DEBUG_VSLATESTx86_ARM_ASM_FLAGS   = -nologo -g -32 -machine ARMCE
RELEASE_VSLATESTx86_ARM_ASM_FLAGS   = -nologo -32 -machine ARMCE
NOOPT_VSLATESTx86_ARM_ASM_FLAGS     = -nologo -g -32 -machine ARMCE

#
# for SEC file/section alignment and XIP, use file type buildoptions
#
# ex: [BuildOptions.common.EDKII.SEC]
#
  DEBUG_VSLATESTx86_ARM_DLINK_FLAGS = /NOLOGO /NODEFAULTLIB /IGNORE:4001 /OPT:REF /OPT:ICF=10 /MAP /SECTION:.xdata,D /SECTION:.pdata,D /MACHINE:ARM /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG:FULL
RELEASE_VSLATESTx86_ARM_DLINK_FLAGS = /NOLOGO /NODEFAULTLIB /IGNORE:4001 /IGNORE:4254 /OPT:REF /OPT:ICF=10 /MAP /SECTION:.xdata,D /SECTION:.pdata,D /MACHINE:ARM /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /MERGE:.rdata=.data
NOOPT_VSLATESTx86_ARM_DLINK_FLAGS   = /NOLOGO /NODEFAULTLIB /IGNORE:4001 /OPT:REF /OPT:ICF=10 /MAP /SECTION:.xdata,D /SECTION:.pdata,D /MACHINE:ARM   /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG:FULL

##################
# AARCH64 definitions
##################

*_VSLATESTx86_AARCH64_*_DLL         = DEF(VS2017x86_DLL)

*_VSLATESTx86_AARCH64_MAKE_PATH     = DEF(VS2017x86_BIN)\nmake.exe
*_VSLATESTx86_AARCH64_CC_PATH       = DEF(VS2017x86_BINA64)\cl.exe
*_VSLATESTx86_AARCH64_PP_PATH       = DEF(VS2017x86_BINA64)\cl.exe
*_VSLATESTx86_AARCH64_APP_PATH      = DEF(VS2017x86_BINA64)\cl.exe
*_VSLATESTx86_AARCH64_VFRPP_PATH    = DEF(VS2017x86_BINA64)\cl.exe
*_VSLATESTx86_AARCH64_ASLCC_PATH    = DEF(VS2017x86_BINA64)\cl.exe
*_VSLATESTx86_AARCH64_ASLPP_PATH    = DEF(VS2017x86_BINA64)\cl.exe
*_VSLATESTx86_AARCH64_ASM_PATH      = DEF(VS2017x86_BINA64)\armasm64.exe
*_VSLATESTx86_AARCH64_SLINK_PATH    = DEF(VS2017x86_BINA64)\lib.exe
*_VSLATESTx86_AARCH64_DLINK_PATH    = DEF(VS2017x86_BINA64)\link.exe
*_VSLATESTx86_AARCH64_ASLDLINK_PATH = DEF(VS2017x86_BINA64)\link.exe

  DEBUG_VSLATESTx86_AARCH64_CC_FLAGS     = /X /nologo $(PLATFORM_FLAGS) /c /WX /GS- /W4 /Gs32768 /D UNICODE /O1ib2s /GL /Gy /FIAutoGen.h /EHs-c- /GR- /GF /Zi /Gm /wd4214 /wd4127 /wd4100 /wd4312 /wd4702
RELEASE_VSLATESTx86_AARCH64_CC_FLAGS     = /X /nologo $(PLATFORM_FLAGS) /c /WX /GS- /W4 /Gs32768 /D UNICODE /O1ib2s /GL /Gy /FIAutoGen.h /EHs-c- /GR- /GF /wd4214 /wd4127 /wd4100 /wd4312 /wd4702
NOOPT_VSLATESTx86_AARCH64_CC_FLAGS       = /X /nologo $(PLATFORM_FLAGS) /c /WX /GS- /W4 /Gs32768 /D UNICODE /Gy /FIAutoGen.h /EHs-c- /GR- /GF /Zi /Gm /Od /wd4702

  DEBUG_VSLATESTx86_AARCH64_ASM_FLAGS    = -g
RELEASE_VSLATESTx86_AARCH64_ASM_FLAGS    =
NOOPT_VSLATESTx86_AARCH64_ASM_FLAGS      = -g

#
# for SEC file/section alignment and XIP, use file type buildoptions
#
# ex: [BuildOptions.common.EDKII.SEC]
#
  DEBUG_VSLATESTx86_AARCH64_DLINK_FLAGS  = /NOLOGO /NODEFAULTLIB /RunBelow4GB /IGNORE:4001 /IGNORE:4226 /OPT:REF /OPT:ICF=10 /MAP /SECTION:.xdata,D /SECTION:.pdata,D /Machine:ARM64 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG:FULL
RELEASE_VSLATESTx86_AARCH64_DLINK_FLAGS  = /NOLOGO /NODEFAULTLIB /RunBelow4GB /IGNORE:4001 /IGNORE:4254 /IGNORE:4226 /OPT:REF /OPT:ICF=10 /MAP /SECTION:.xdata,D /SECTION:.pdata,D /Machine:ARM64 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /MERGE:.rdata=.data
NOOPT_VSLATESTx86_AARCH64_DLINK_FLAGS    = /NOLOGO /NODEFAULTLIB /RunBelow4GB /IGNORE:4001 /OPT:REF /OPT:ICF=10 /MAP /SECTION:.xdata,D /SECTION:.pdata,D /Machine:ARM64 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG:FULL





#################
# ASM 16 linker defintions
#################
*_*_*_ASMLINK_PATH                 = link16.exe
*_*_*_ASMLINK_FLAGS                = /nologo /tiny

##################
# VfrCompiler definitions
##################
*_*_*_VFR_PATH                      = VfrCompile
*_*_*_VFR_FLAGS                     = -l -n

##################
# OptionRom tool definitions
##################
*_*_*_OPTROM_PATH                   = EfiRom
*_*_*_OPTROM_FLAGS                  = -e

##################
# GenFw tool definitions
##################
*_*_*_GENFW_PATH                   = GenFw
*_*_*_GENFW_FLAGS                  =

##################
# Asl Compiler definitions
##################
*_*_*_ASLCC_FLAGS                  = /nologo /c /FIAutoGen.h /TC /Dmain=ReferenceAcpiTable
*_*_*_ASLDLINK_FLAGS               = /NODEFAULTLIB /ENTRY:ReferenceAcpiTable /SUBSYSTEM:CONSOLE
*_*_*_ASLPP_FLAGS                  = /nologo /EP /C
*_*_*_ASL_FLAGS                    =

##################
# GenCrc32 tool definitions
##################
*_*_*_CRC32_PATH          = GenCrc32
*_*_*_CRC32_GUID          = FC1BCDB0-7D31-49AA-936A-A4600D9DD083

##################
# Rsa2048Sha256Sign tool definitions
#
# Notes: This tool defintion uses a test signing key for development purposes only.
#        The tool Rsa2048Sha256GenerateKeys can be used to generate a new private/public key
#        and the gEfiSecurityPkgTokenSpaceGuid.PcdRsa2048Sha256PublicKeyBuffer PCD value.
#        A custom tool/script can be implemented using the new private/public key with
#        the Rsa2048Sha256Sign tool and this tool defintiion can be updated to use a
#        custom tool/script.
#
#   Generate new private/public key and gEfiSecurityPkgTokenSpaceGuid.PcdRsa2048Sha256PublicKeyBuffer PCD value
#
#       Rsa2048Sha256GenerateKeys.py -o MyKey.pem --public-key-hash-c MyKey.pcd
#
#   Custom script example (MyRsa2048Sha256Sign.cmd):
#
#       Rsa2048Sha256Sign --private-key MyKey.pem %1 %2 %3 %4 %5 %6 %7 %8 %9
#
#   WARNING: Vendors that uses private keys are responsible for proper management and protection 
#            of private keys.  Vendors may choose to use infrastructure such as signing servers 
#            or signing portals to support the management and protection of private keys.
#
##################
*_*_*_RSA2048SHA256SIGN_PATH = Rsa2048Sha256Sign
*_*_*_RSA2048SHA256SIGN_GUID = A7717414-C616-4977-9420-844712A735BF

##################
# BrotliCompress tool definitions
##################
*_*_*_BROTLI_PATH        = BrotliCompress
*_*_*_BROTLI_GUID        = 3D532050-5CDA-4FD0-879E-0F7F630D5AFB

##################
# LzmaCompress tool definitions
##################
*_*_*_LZMA_PATH          = LzmaCompress
*_*_*_LZMA_GUID          = EE4E5898-3914-4259-9D6E-DC7BD79403CF

##################
# LzmaF86Compress tool definitions with converter for x86 code.
# It can improve the compression ratio if the input file is IA32 or X64 PE image.
# Notes: If X64 PE image is built based on GCC44, it may not get the better compression.
##################
*_*_*_LZMAF86_PATH         = LzmaF86Compress
*_*_*_LZMAF86_GUID         = D42AE6BD-1352-4bfb-909A-CA72A6EAE889

##################
# TianoCompress tool definitions
##################
*_*_*_TIANO_PATH         = TianoCompress
*_*_*_TIANO_GUID         = A31280AD-481E-41B6-95E8-127F4C984779

##################
# BPDG tool definitions
##################
*_*_*_VPDTOOL_PATH         = BPDG
*_*_*_VPDTOOL_GUID         = 8C3D856A-9BE6-468E-850A-24F7A8D38E08

##################
# SignFv tool definitions  -- INTEL TOOL IN BDW
##################
*_*_*_SIGNFV_PATH         = SignFvPlatform
*_*_*_SIGNFV_GUID         = 0f9d89e8-9259-4f76-a5af-0c89e34023df

##################
# NASM tool definitions
##################
*_*_*_NASM_PATH                = ENV(NASM_PREFIX)\nasm
# NASMB uses NASM produce a .bin from a .nasmb NASM source file
*_*_*_NASMB_FLAGS              = -f bin

##################
# RUSTC definitions
##################
*_*_*_RUSTC_PATH            = rustc
DEBUG_*_X64_RUSTC_FLAGS     = --edition=2018 --target x86_64-pc-windows-msvc -C debuginfo=2 -C panic=abort --crate-type staticlib --extern r_efi=ENV(WORKSPACE)\RustPkg\External\r-efi\target\debug\libr_efi.rlib
RELEASE_*_X64_RUSTC_FLAGS   = --edition=2018 --target x86_64-pc-windows-msvc -C opt-level=3 -C panic=abort --crate-type staticlib --extern r_efi=ENV(WORKSPACE)\RustPkg\External\r-efi\target\release\libr_efi.rlib

##################
# CARGO definitions
##################
*_*_*_CARGO_PATH            = cargo
DEBUG_*_X64_CARGO_FLAGS     = --target x86_64-pc-windows-msvc
RELEASE_*_X64_CARGO_FLAGS   = --target x86_64-pc-windows-msvc --release
DEBUG_*_IA32_CARGO_FLAGS    = --target i686-pc-windows-msvc
RELEASE_*_IA32_CARGO_FLAGS  = --target i686-pc-windows-msvc   --release

#################
# Build rule order
#################
*_*_*_*_BUILDRULEORDER = nasm asm Asm ASM S s
