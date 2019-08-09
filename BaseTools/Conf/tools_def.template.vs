#
#  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
#  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
#  Portions copyright (c) 2011 - 2014, ARM Ltd. All rights reserved.<BR>
#  Copyright (c) 2015, Hewlett-Packard Development Company, L.P.<BR>
#  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
#  Copyright (c) Microsoft Corporation
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
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
# 1.14 - Created VSLATEST for supporting latest VS version
# 1.15 - Added ARM,ARM64 proper for prerelease of VS2017 tools.
# 1.16 - Changed /DEBUG to FULL mode for VS2017+
# 1.17 - Asl compilers should be on the path
# 1.18 - Remove VS2013 support
# 1.19 - RC.exe from Windows 10 Kit
# 1.20 - Added debug info to Release Builds
# 1.21 - Update File and License to align with TC 1905.
#        Drop VS2015 and VSLATESTx86.
#        Change VS2017x86 to VS2017
#        Add VS2019. Continue to support asl.exe (ms).
#        Require Env for WIN_SDK_RC_EXE_BASE_PATH
#        Remove VS support for ARM.  Keep AARCH64.
#        Add new common flag functionality to make it easier to manage flags
#        Rename from *.ms to *.vs since this is Visual Studio specific
# 1.22 - Remove common flag for /GS mgmt and do it per arch and build type. Currently don't support /GS for anything other than x64 debug builds

#!VERSION=1.22

IDENTIFIER = Default TOOL_CHAIN_CONF



#
# VS2017 - New model for VS2017 where there is potential for many versions of the tools.
# If a specific version is required then the user must set both env variables:
## VS150INSTALLPATH:  base install path on system to VC install dir.  Here you will find the Common7 folder, VC folder, etc
## VS150TOOLVER:      version number for the VC compiler tools
DEFINE VS2017_BIN_IA32     = ENV(VS150INSTALLPATH)\VC\Tools\MSVC\ENV(VS150TOOLVER)\bin\HostX86\x86
DEFINE VS2017_BIN_X64      = ENV(VS150INSTALLPATH)\VC\Tools\MSVC\ENV(VS150TOOLVER)\bin\HostX86\x64
DEFINE VS2017_BIN_AARCH64  = ENV(VS150INSTALLPATH)\VC\Tools\MSVC\ENV(VS150TOOLVER)\bin\HostX86\arm64
DEFINE VS2017_BIN_HOST     = DEF(VS2017_BIN_IA32)

#
# VS2019 - Follow VS2019 where there is potential for many versions of the tools.
# If a specific version is required then the user must set both env variables:
## VS160INSTALLPATH:  base install path on system to VC install dir.  Here you will find the Common7 folder, VC folder, etc
## VS160TOOLVER:      version number for the VC compiler tools
DEFINE VS2019_BIN_IA32     = ENV(VS160INSTALLPATH)\VC\Tools\MSVC\ENV(VS160TOOLVER)\bin\HostX86\x86
DEFINE VS2019_BIN_X64      = ENV(VS160INSTALLPATH)\VC\Tools\MSVC\ENV(VS160TOOLVER)\bin\HostX86\x64
DEFINE VS2019_BIN_AARCH64  = ENV(VS160INSTALLPATH)\VC\Tools\MSVC\ENV(VS160TOOLVER)\bin\HostX86\arm64
DEFINE VS2019_BIN_HOST     = DEF(VS2019_BIN_IA32)


#
# Resource compiler
#
DEFINE RC_PATH    = ENV(WINSDK_PATH_FOR_RC_EXE)\rc.exe

#
# iasl ASL compiler
# Version is handled by the  build system as for which version is added to path
#
DEFINE WIN_IASL_BIN            = iasl.exe
DEFINE IASL_FLAGS              =
DEFINE IASL_OUTFLAGS           = -p

#
# Microsoft ASL compiler
#
DEFINE WIN_MS_ASL_BIN          = asl.exe
DEFINE MS_ASL_OUTFLAGS         = /Fo=
DEFINE MS_ASL_FLAGS            =

## Why is this different 
DEFINE MSFT_ASLPP_FLAGS        = /nologo /E /C /FIAutoGen.h


#
# VS Compiler flags
# https://docs.microsoft.com/en-us/cpp/build/reference/compiler-options-listed-alphabetically?view=vs-2019
#
DEFINE _VSCOMMON_CC_FLAGS        = /nologo /X /c /WX /W4 /Gs32768 /D UNICODE /O1b2 /Oi- /MP /GL /FIAutoGen.h /EHs-c- /GR- /GF /Gy /Zi /Zo /Gw
DEFINE DEBUG_VSCOMMON_CC_FLAGS   = DEF(_VSCOMMON_CC_FLAGS)

## remove stack checks beceause we don't have good "production" solution
DEFINE RELEASE_VSCOMMON_CC_FLAGS = DEF(_VSCOMMON_CC_FLAGS)

## remove stack checks and optimizations
DEFINE NOOPT_VSCOMMON_CC_FLAGS   = DEF(_VSCOMMON_CC_FLAGS) /Od

#
# VS Linker flags
# https://docs.microsoft.com/en-us/cpp/build/reference/linker-options?view=vs-2019
#
DEFINE _VSCOMMON_DLINK_FLAGS        = /NOLOGO /NODEFAULTLIB /IGNORE:4001 /IGNORE:4281 /OPT:REF /OPT:ICF=10 /MAP /SECTION:.xdata,D /SECTION:.pdata,D /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG:FULL

## 
DEFINE DEBUG_VSCOMMON_DLINK_FLAGS   = DEF(_VSCOMMON_DLINK_FLAGS)
DEFINE RELEASE_VSCOMMON_DLINK_FLAGS = DEF(_VSCOMMON_DLINK_FLAGS) /IGNORE:4254
#https://docs.microsoft.com/en-us/cpp/error-messages/tool-errors/linker-tools-warning-lnk4254?view=vs-2019
##TODO review /MERGE:.rdata=.data on release
DEFINE NOOPT_VSCOMMON_DLINK_FLAGS   = DEF(_VSCOMMON_DLINK_FLAGS)


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

####################################################################################
#   VS2017       - Microsoft Visual Studio 2017 with Intel ASL
####################################################################################
#   VS2017           - Microsoft Visual Studio 2017 with Intel ASL
*_VS2017_*_*_FAMILY        = MSFT
*_VS2017_*_*_DLL           = DEF(VS2017_BIN_HOST)

*_VS2017_*_MAKE_PATH       = DEF(VS2017_BIN_HOST)\nmake.exe
*_VS2017_*_MAKE_FLAG       = /nologo
*_VS2017_*_RC_PATH         = DEF(RC_PATH)

*_VS2017_*_MAKE_FLAGS      = /nologo
*_VS2017_*_SLINK_FLAGS     = /NOLOGO /LTCG
*_VS2017_*_APP_FLAGS       = /nologo /E /TC
*_VS2017_*_PP_FLAGS        = /nologo /E /TC /FIAutoGen.h
*_VS2017_*_VFRPP_FLAGS     = /nologo /E /TC /DVFRCOMPILE /FI$(MODULE_NAME)StrDefs.h
*_VS2017_*_DLINK2_FLAGS    = /WHOLEARCHIVE
*_VS2017_*_ASM16_PATH      = DEF(VS2017_BIN_IA32)\ml.exe

##################
# ASL definitions
##################
*_VS2017_*_ASL_PATH        = DEF(WIN_IASL_BIN)
*_VS2017_*_ASL_FLAGS       = DEF(IASL_FLAGS)
*_VS2017_*_ASL_OUTFLAGS    = DEF(IASL_OUTFLAGS)
#*_VS2017_*_ASLCC_FLAGS     = DEF(MSFT_ASLCC_FLAGS)
*_VS2017_*_ASLPP_FLAGS     = DEF(MSFT_ASLPP_FLAGS)
#*_VS2017_*_ASLDLINK_FLAGS  = DEF(MSFT_ASLDLINK_FLAGS)

##################
# IA32 definitions
##################
*_VS2017_IA32_CC_PATH      = DEF(VS2017_BIN_IA32)\cl.exe
*_VS2017_IA32_VFRPP_PATH   = DEF(VS2017_BIN_IA32)\cl.exe
*_VS2017_IA32_ASLCC_PATH   = DEF(VS2017_BIN_IA32)\cl.exe
*_VS2017_IA32_ASLPP_PATH   = DEF(VS2017_BIN_IA32)\cl.exe
*_VS2017_IA32_SLINK_PATH   = DEF(VS2017_BIN_IA32)\lib.exe
*_VS2017_IA32_DLINK_PATH   = DEF(VS2017_BIN_IA32)\link.exe
*_VS2017_IA32_ASLDLINK_PATH= DEF(VS2017_BIN_IA32)\link.exe
*_VS2017_IA32_APP_PATH     = DEF(VS2017_BIN_IA32)\cl.exe
*_VS2017_IA32_PP_PATH      = DEF(VS2017_BIN_IA32)\cl.exe
*_VS2017_IA32_ASM_PATH     = DEF(VS2017_BIN_IA32)\ml.exe

  DEBUG_VS2017_IA32_CC_FLAGS    = DEF(DEBUG_VSCOMMON_CC_FLAGS) /arch:IA32 /GS-
RELEASE_VS2017_IA32_CC_FLAGS    = DEF(RELEASE_VSCOMMON_CC_FLAGS) /arch:IA32 /GS-
NOOPT_VS2017_IA32_CC_FLAGS      = DEF(NOOPT_VSCOMMON_CC_FLAGS) /arch:IA32 /GS-

  DEBUG_VS2017_IA32_ASM_FLAGS   = /nologo /c /WX /W3 /Cx /coff /Zd /Zi
RELEASE_VS2017_IA32_ASM_FLAGS   = /nologo /c /WX /W3 /Cx /coff /Zd
NOOPT_VS2017_IA32_ASM_FLAGS     = /nologo /c /WX /W3 /Cx /coff /Zd /Zi

  DEBUG_VS2017_IA32_NASM_FLAGS  = -Ox -f win32 -g
RELEASE_VS2017_IA32_NASM_FLAGS  = -Ox -f win32
NOOPT_VS2017_IA32_NASM_FLAGS    = -O0 -f win32 -g

  DEBUG_VS2017_IA32_DLINK_FLAGS =  DEF(DEBUG_VSCOMMON_DLINK_FLAGS) /ALIGN:32 /MACHINE:X86
RELEASE_VS2017_IA32_DLINK_FLAGS =  DEF(RELEASE_VSCOMMON_DLINK_FLAGS) /ALIGN:32 /MACHINE:X86
NOOPT_VS2017_IA32_DLINK_FLAGS   =  DEF(NOOPT_VSCOMMON_DLINK_FLAGS) /ALIGN:32  /MACHINE:X86

##################
# X64 definitions
##################
*_VS2017_X64_CC_PATH       = DEF(VS2017_BIN_X64)\cl.exe
*_VS2017_X64_PP_PATH       = DEF(VS2017_BIN_X64)\cl.exe
*_VS2017_X64_APP_PATH      = DEF(VS2017_BIN_X64)\cl.exe
*_VS2017_X64_VFRPP_PATH    = DEF(VS2017_BIN_X64)\cl.exe
*_VS2017_X64_ASLCC_PATH    = DEF(VS2017_BIN_X64)\cl.exe
*_VS2017_X64_ASLPP_PATH    = DEF(VS2017_BIN_X64)\cl.exe
*_VS2017_X64_ASM_PATH      = DEF(VS2017_BIN_X64)\ml64.exe
*_VS2017_X64_SLINK_PATH    = DEF(VS2017_BIN_X64)\lib.exe
*_VS2017_X64_DLINK_PATH    = DEF(VS2017_BIN_X64)\link.exe
*_VS2017_X64_ASLDLINK_PATH = DEF(VS2017_BIN_X64)\link.exe

  DEBUG_VS2017_X64_CC_FLAGS    = DEF(DEBUG_VSCOMMON_CC_FLAGS)
RELEASE_VS2017_X64_CC_FLAGS    = DEF(RELEASE_VSCOMMON_CC_FLAGS) /GS-
NOOPT_VS2017_X64_CC_FLAGS      = DEF(NOOPT_VSCOMMON_CC_FLAGS) /GS-

  DEBUG_VS2017_X64_ASM_FLAGS    = /nologo /c /WX /W3 /Cx /Zd /Zi
RELEASE_VS2017_X64_ASM_FLAGS    = /nologo /c /WX /W3 /Cx /Zd
NOOPT_VS2017_X64_ASM_FLAGS      = /nologo /c /WX /W3 /Cx /Zd /Zi

  DEBUG_VS2017_X64_NASM_FLAGS   = -Ox -f win64 -g
RELEASE_VS2017_X64_NASM_FLAGS   = -Ox -f win64
NOOPT_VS2017_X64_NASM_FLAGS     = -O0 -f win64 -g

  DEBUG_VS2017_X64_DLINK_FLAGS  =  DEF(DEBUG_VSCOMMON_DLINK_FLAGS) /ALIGN:4096 /Machine:X64
RELEASE_VS2017_X64_DLINK_FLAGS  =  DEF(RELEASE_VSCOMMON_DLINK_FLAGS) /ALIGN:4096 /Machine:X64
NOOPT_VS2017_X64_DLINK_FLAGS    =  DEF(NOOPT_VSCOMMON_DLINK_FLAGS) /ALIGN:4096  /Machine:X64

#####################
# AARCH64 definitions
#####################
*_VS2017_AARCH64_CC_PATH           = DEF(VS2017_BIN_AARCH64)\cl.exe
*_VS2017_AARCH64_VFRPP_PATH        = DEF(VS2017_BIN_AARCH64)\cl.exe
*_VS2017_AARCH64_SLINK_PATH        = DEF(VS2017_BIN_AARCH64)\lib.exe
*_VS2017_AARCH64_DLINK_PATH        = DEF(VS2017_BIN_AARCH64)\link.exe
*_VS2017_AARCH64_APP_PATH          = DEF(VS2017_BIN_AARCH64)\cl.exe
*_VS2017_AARCH64_PP_PATH           = DEF(VS2017_BIN_AARCH64)\cl.exe
*_VS2017_AARCH64_ASM_PATH          = DEF(VS2017_BIN_AARCH64)\armasm64.exe
*_VS2017_AARCH64_ASLCC_PATH        = DEF(VS2017_BIN_AARCH64)\cl.exe
*_VS2017_AARCH64_ASLPP_PATH        = DEF(VS2017_BIN_AARCH64)\cl.exe
*_VS2017_AARCH64_ASLDLINK_PATH     = DEF(VS2017_BIN_AARCH64)\link.exe


  DEBUG_VS2017_AARCH64_CC_FLAGS    = DEF(DEBUG_VSCOMMON_CC_FLAGS) /GS- /wd4214 /wd4127 /wd4100 /wd4312 /wd4702
RELEASE_VS2017_AARCH64_CC_FLAGS    = DEF(RELEASE_VSCOMMON_CC_FLAGS) /GS- /wd4214 /wd4127 /wd4100 /wd4312 /wd4702
NOOPT_VS2017_AARCH64_CC_FLAGS      = DEF(NOOPT_VSCOMMON_CC_FLAGS) /GS- /wd4702

  DEBUG_VS2017_AARCH64_ASM_FLAGS   = /nologo /g
RELEASE_VS2017_AARCH64_ASM_FLAGS   = /nologo
NOOPT_VS2017_AARCH64_ASM_FLAGS     = /nologo

  DEBUG_VS2017_AARCH64_DLINK_FLAGS  = DEF(DEBUG_VSCOMMON_DLINK_FLAGS) /RunBelow4GB /Machine:ARM64
RELEASE_VS2017_AARCH64_DLINK_FLAGS  = DEF(RELEASE_VSCOMMON_DLINK_FLAGS) /RunBelow4GB /IGNORE:4226 /Machine:ARM64
NOOPT_VS2017_AARCH64_DLINK_FLAGS    = DEF(NOOPT_VSCOMMON_DLINK_FLAGS) /RunBelow4GB /Machine:ARM64

####################################################################################
#   VS2019       - Microsoft Visual Studio 2019 with Intel ASL
####################################################################################
#   VS2019           - Microsoft Visual Studio 2019 with Intel ASL
*_VS2019_*_*_FAMILY        = MSFT
*_VS2019_*_*_DLL           = DEF(VS2019_BIN_HOST)

*_VS2019_*_MAKE_PATH       = DEF(VS2019_BIN_HOST)\nmake.exe
*_VS2019_*_MAKE_FLAG       = /nologo
*_VS2019_*_RC_PATH         = DEF(RC_PATH)

*_VS2019_*_MAKE_FLAGS      = /nologo
*_VS2019_*_SLINK_FLAGS     = /NOLOGO /LTCG
*_VS2019_*_APP_FLAGS       = /nologo /E /TC
*_VS2019_*_PP_FLAGS        = /nologo /E /TC /FIAutoGen.h
*_VS2019_*_VFRPP_FLAGS     = /nologo /E /TC /DVFRCOMPILE /FI$(MODULE_NAME)StrDefs.h
*_VS2019_*_DLINK2_FLAGS    = /WHOLEARCHIVE
*_VS2019_*_ASM16_PATH      = DEF(VS2019_BIN_IA32)\ml.exe

##################
# ASL definitions
##################
*_VS2019_*_ASL_PATH        = DEF(WIN_IASL_BIN)
*_VS2019_*_ASL_FLAGS       = DEF(IASL_FLAGS)
*_VS2019_*_ASL_OUTFLAGS    = DEF(IASL_OUTFLAGS)
#*_VS2019_*_ASLCC_FLAGS     = DEF(MSFT_ASLCC_FLAGS)
*_VS2019_*_ASLPP_FLAGS     = DEF(MSFT_ASLPP_FLAGS)
#*_VS2019_*_ASLDLINK_FLAGS  = DEF(MSFT_ASLDLINK_FLAGS)

##################
# IA32 definitions
##################
*_VS2019_IA32_CC_PATH      = DEF(VS2019_BIN_IA32)\cl.exe
*_VS2019_IA32_VFRPP_PATH   = DEF(VS2019_BIN_IA32)\cl.exe
*_VS2019_IA32_ASLCC_PATH   = DEF(VS2019_BIN_IA32)\cl.exe
*_VS2019_IA32_ASLPP_PATH   = DEF(VS2019_BIN_IA32)\cl.exe
*_VS2019_IA32_SLINK_PATH   = DEF(VS2019_BIN_IA32)\lib.exe
*_VS2019_IA32_DLINK_PATH   = DEF(VS2019_BIN_IA32)\link.exe
*_VS2019_IA32_ASLDLINK_PATH= DEF(VS2019_BIN_IA32)\link.exe
*_VS2019_IA32_APP_PATH     = DEF(VS2019_BIN_IA32)\cl.exe
*_VS2019_IA32_PP_PATH      = DEF(VS2019_BIN_IA32)\cl.exe
*_VS2019_IA32_ASM_PATH     = DEF(VS2019_BIN_IA32)\ml.exe

  DEBUG_VS2019_IA32_CC_FLAGS    = DEF(DEBUG_VSCOMMON_CC_FLAGS) /arch:IA32 /GS-
RELEASE_VS2019_IA32_CC_FLAGS    = DEF(RELEASE_VSCOMMON_CC_FLAGS) /arch:IA32 /GS-
NOOPT_VS2019_IA32_CC_FLAGS      = DEF(NOOPT_VSCOMMON_CC_FLAGS) /arch:IA32 /GS-

  DEBUG_VS2019_IA32_ASM_FLAGS   = /nologo /c /WX /W3 /Cx /coff /Zd /Zi
RELEASE_VS2019_IA32_ASM_FLAGS   = /nologo /c /WX /W3 /Cx /coff /Zd
NOOPT_VS2019_IA32_ASM_FLAGS     = /nologo /c /WX /W3 /Cx /coff /Zd /Zi

  DEBUG_VS2019_IA32_NASM_FLAGS  = -Ox -f win32 -g
RELEASE_VS2019_IA32_NASM_FLAGS  = -Ox -f win32
NOOPT_VS2019_IA32_NASM_FLAGS    = -O0 -f win32 -g

  DEBUG_VS2019_IA32_DLINK_FLAGS =  DEF(DEBUG_VSCOMMON_DLINK_FLAGS) /ALIGN:32 /MACHINE:X86
RELEASE_VS2019_IA32_DLINK_FLAGS =  DEF(RELEASE_VSCOMMON_DLINK_FLAGS) /ALIGN:32 /MACHINE:X86
NOOPT_VS2019_IA32_DLINK_FLAGS   =  DEF(NOOPT_VSCOMMON_DLINK_FLAGS) /ALIGN:32  /MACHINE:X86

##################
# X64 definitions
##################
*_VS2019_X64_CC_PATH       = DEF(VS2019_BIN_X64)\cl.exe
*_VS2019_X64_PP_PATH       = DEF(VS2019_BIN_X64)\cl.exe
*_VS2019_X64_APP_PATH      = DEF(VS2019_BIN_X64)\cl.exe
*_VS2019_X64_VFRPP_PATH    = DEF(VS2019_BIN_X64)\cl.exe
*_VS2019_X64_ASLCC_PATH    = DEF(VS2019_BIN_X64)\cl.exe
*_VS2019_X64_ASLPP_PATH    = DEF(VS2019_BIN_X64)\cl.exe
*_VS2019_X64_ASM_PATH      = DEF(VS2019_BIN_X64)\ml64.exe
*_VS2019_X64_SLINK_PATH    = DEF(VS2019_BIN_X64)\lib.exe
*_VS2019_X64_DLINK_PATH    = DEF(VS2019_BIN_X64)\link.exe
*_VS2019_X64_ASLDLINK_PATH = DEF(VS2019_BIN_X64)\link.exe

  DEBUG_VS2019_X64_CC_FLAGS    = DEF(DEBUG_VSCOMMON_CC_FLAGS)
RELEASE_VS2019_X64_CC_FLAGS    = DEF(RELEASE_VSCOMMON_CC_FLAGS) /GS-
NOOPT_VS2019_X64_CC_FLAGS      = DEF(NOOPT_VSCOMMON_CC_FLAGS) /GS-

  DEBUG_VS2019_X64_ASM_FLAGS    = /nologo /c /WX /W3 /Cx /Zd /Zi
RELEASE_VS2019_X64_ASM_FLAGS    = /nologo /c /WX /W3 /Cx /Zd
NOOPT_VS2019_X64_ASM_FLAGS      = /nologo /c /WX /W3 /Cx /Zd /Zi

  DEBUG_VS2019_X64_NASM_FLAGS   = -Ox -f win64 -g
RELEASE_VS2019_X64_NASM_FLAGS   = -Ox -f win64
NOOPT_VS2019_X64_NASM_FLAGS     = -O0 -f win64 -g

  DEBUG_VS2019_X64_DLINK_FLAGS  =  DEF(DEBUG_VSCOMMON_DLINK_FLAGS) /ALIGN:4096 /Machine:X64
RELEASE_VS2019_X64_DLINK_FLAGS  =  DEF(RELEASE_VSCOMMON_DLINK_FLAGS) /ALIGN:4096 /Machine:X64
NOOPT_VS2019_X64_DLINK_FLAGS    =  DEF(NOOPT_VSCOMMON_DLINK_FLAGS) /ALIGN:4096  /Machine:X64

#####################
# AARCH64 definitions
#####################
*_VS2019_AARCH64_CC_PATH           = DEF(VS2019_BIN_AARCH64)\cl.exe
*_VS2019_AARCH64_VFRPP_PATH        = DEF(VS2019_BIN_AARCH64)\cl.exe
*_VS2019_AARCH64_SLINK_PATH        = DEF(VS2019_BIN_AARCH64)\lib.exe
*_VS2019_AARCH64_DLINK_PATH        = DEF(VS2019_BIN_AARCH64)\link.exe
*_VS2019_AARCH64_APP_PATH          = DEF(VS2019_BIN_AARCH64)\cl.exe
*_VS2019_AARCH64_PP_PATH           = DEF(VS2019_BIN_AARCH64)\cl.exe
*_VS2019_AARCH64_ASM_PATH          = DEF(VS2019_BIN_AARCH64)\armasm64.exe
*_VS2019_AARCH64_ASLCC_PATH        = DEF(VS2019_BIN_AARCH64)\cl.exe
*_VS2019_AARCH64_ASLPP_PATH        = DEF(VS2019_BIN_AARCH64)\cl.exe
*_VS2019_AARCH64_ASLDLINK_PATH     = DEF(VS2019_BIN_AARCH64)\link.exe


  DEBUG_VS2019_AARCH64_CC_FLAGS    = DEF(DEBUG_VSCOMMON_CC_FLAGS) /GS- /wd4214 /wd4127 /wd4100 /wd4312 /wd4702
RELEASE_VS2019_AARCH64_CC_FLAGS    = DEF(RELEASE_VSCOMMON_CC_FLAGS) /GS- /wd4214 /wd4127 /wd4100 /wd4312 /wd4702
NOOPT_VS2019_AARCH64_CC_FLAGS      = DEF(NOOPT_VSCOMMON_CC_FLAGS) /GS- /wd4702

  DEBUG_VS2019_AARCH64_ASM_FLAGS   = /nologo /g
RELEASE_VS2019_AARCH64_ASM_FLAGS   = /nologo
NOOPT_VS2019_AARCH64_ASM_FLAGS     = /nologo

  DEBUG_VS2019_AARCH64_DLINK_FLAGS  = DEF(DEBUG_VSCOMMON_DLINK_FLAGS) /RunBelow4GB /Machine:ARM64
RELEASE_VS2019_AARCH64_DLINK_FLAGS  = DEF(RELEASE_VSCOMMON_DLINK_FLAGS) /RunBelow4GB /IGNORE:4226 /Machine:ARM64
NOOPT_VS2019_AARCH64_DLINK_FLAGS    = DEF(NOOPT_VSCOMMON_DLINK_FLAGS) /RunBelow4GB /Machine:ARM64


#################
# ASM 16 linker definitions
#################
# require it on the path. 
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
# Notes: This tool definition uses a test signing key for development purposes only.
#        The tool Rsa2048Sha256GenerateKeys can be used to generate a new private/public key
#        and the gEfiSecurityPkgTokenSpaceGuid.PcdRsa2048Sha256PublicKeyBuffer PCD value.
#        A custom tool/script can be implemented using the new private/public key with
#        the Rsa2048Sha256Sign tool and this tool definition can be updated to use a
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
# Pkcs7Sign tool definitions
##################
*_*_*_PKCS7SIGN_PATH       = Pkcs7Sign
*_*_*_PKCS7SIGN_GUID       = 4AAFD29D-68DF-49EE-8AA9-347D375665A7

##################
# NASM tool definitions
##################
*_*_*_NASM_PATH                = ENV(NASM_PREFIX)\nasm
# NASMB uses NASM produce a .bin from a .nasmb NASM source file
*_*_*_NASMB_FLAGS              = -f bin

#################
# Build rule order
#################
*_*_*_*_BUILDRULEORDER = nasm asm Asm ASM S s nasmb asm16
