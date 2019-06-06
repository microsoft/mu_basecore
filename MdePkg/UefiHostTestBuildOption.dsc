## @file UefiHostTestBuildOption.dsc
# 
# Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[BuildOptions]
  GCC:*_*_IA32_CC_FLAGS == -m32 -g -fshort-wchar -fno-strict-aliasing -Wall -malign-double -idirafter/usr/include -c -include $(DEST_DIR_DEBUG)/AutoGen.h
  GCC:*_*_IA32_PP_FLAGS == -m32 -E -x assembler-with-cpp -include $(DEST_DIR_DEBUG)/AutoGen.h
  GCC:*_*_IA32_ASM_FLAGS == -m32 -c -x assembler -imacros $(DEST_DIR_DEBUG)/AutoGen.h

  GCC:*_*_X64_CC_FLAGS == -m64 -g -fshort-wchar -fno-strict-aliasing -Wall -malign-double -idirafter/usr/include -c -include $(DEST_DIR_DEBUG)/AutoGen.h
  GCC:*_GCC49_X64_CC_FLAGS = "-DEFIAPI=__attribute__((ms_abi))"
  GCC:*_GCC5_X64_CC_FLAGS = "-DEFIAPI=__attribute__((ms_abi))" -DUSING_LTO -Os
  GCC:*_*_X64_PP_FLAGS == -m64 -E -x assembler-with-cpp -include $(DEST_DIR_DEBUG)/AutoGen.h
  GCC:*_*_X64_ASM_FLAGS == -m64 -c -x assembler -imacros $(DEST_DIR_DEBUG)/AutoGen.h

  GCC:*_GCC5_*_CC_FLAGS = --coverage
  GCC:*_GCC5_*_DLINK_FLAGS = --coverage

  GCC:*_GCC5_X64_CC_FLAGS = "-DNO_MSABI_VA_FUNCS=TRUE"

[BuildOptions.common.EDKII.USER_DEFINED]
  MSFT:*_*_IA32_DLINK_FLAGS == /out:"$(BIN_DIR)\$(BASE_NAME).exe" /base:0x10000000 /pdb:"$(BIN_DIR)\$(BASE_NAME).pdb" /LIBPATH:"$(VCINSTALLDIR)\Lib" /LIBPATH:"$(VCINSTALLDIR)\PlatformSdk\Lib" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x86" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x86" /NOLOGO /SUBSYSTEM:CONSOLE /IGNORE:4086 /MAP /OPT:REF /DEBUG /MACHINE:I386 /LTCG Kernel32.lib MSVCRTD.lib Gdi32.lib User32.lib Winmm.lib Advapi32.lib
  MSFT:*_VS2015_IA32_DLINK_FLAGS == /out:"$(BIN_DIR)\$(BASE_NAME).exe" /base:0x10000000 /pdb:"$(BIN_DIR)\$(BASE_NAME).pdb" /LIBPATH:"$(VCINSTALLDIR)\Lib" /LIBPATH:"$(VCINSTALLDIR)\PlatformSdk\Lib" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x86" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x86" /NOLOGO /SUBSYSTEM:CONSOLE /IGNORE:4086 /MAP /OPT:REF /DEBUG /MACHINE:I386 /LTCG Kernel32.lib MSVCRTD.lib vcruntimed.lib ucrtd.lib Gdi32.lib User32.lib Winmm.lib Advapi32.lib
  MSFT:*_VS2015x86_IA32_DLINK_FLAGS == /out:"$(BIN_DIR)\$(BASE_NAME).exe" /base:0x10000000 /pdb:"$(BIN_DIR)\$(BASE_NAME).pdb" /LIBPATH:"$(VCINSTALLDIR)\Lib" /LIBPATH:"$(VCINSTALLDIR)\PlatformSdk\Lib" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x86" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x86" /NOLOGO /SUBSYSTEM:CONSOLE /IGNORE:4086 /MAP /OPT:REF /DEBUG /MACHINE:I386 /LTCG Kernel32.lib MSVCRTD.lib vcruntimed.lib ucrtd.lib Gdi32.lib User32.lib Winmm.lib Advapi32.lib
  MSFT:*_VS2017_IA32_DLINK_FLAGS == /out:"$(BIN_DIR)\$(BASE_NAME).exe" /base:0x10000000 /pdb:"$(BIN_DIR)\$(BASE_NAME).pdb" /LIBPATH:"%VCToolsInstallDir%lib\x86" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x86" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x86" /NOLOGO /SUBSYSTEM:CONSOLE /IGNORE:4086 /MAP /OPT:REF /DEBUG /MACHINE:I386 /LTCG Kernel32.lib MSVCRTD.lib vcruntimed.lib ucrtd.lib Gdi32.lib User32.lib Winmm.lib Advapi32.lib
  MSFT:*_*_IA32_CC_FLAGS == /nologo /W4 /WX /Gy /c /D UNICODE /Od /FIAutoGen.h /EHs-c- /GF /Gs8192 /Zi /Gm /D _CRT_SECURE_NO_WARNINGS /D _CRT_SECURE_NO_DEPRECATE
  MSFT:*_*_IA32_PP_FLAGS == /nologo /E /TC /FIAutoGen.h
  MSFT:*_*_IA32_ASM_FLAGS == /nologo /W3 /WX /c /coff /Cx /Zd /W0 /Zi
  MSFT:*_*_IA32_ASMLINK_FLAGS       == /link /nologo /tiny

  MSFT:*_*_X64_DLINK_FLAGS == /out:"$(BIN_DIR)\$(BASE_NAME).exe" /base:0x10000000 /pdb:"$(BIN_DIR)\$(BASE_NAME).pdb" /LIBPATH:"$(VCINSTALLDIR)\Lib\AMD64" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x64" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x64" /NOLOGO /SUBSYSTEM:CONSOLE /IGNORE:4086 /MAP /OPT:REF /DEBUG /MACHINE:AMD64 /LTCG Kernel32.lib MSVCRTD.lib Gdi32.lib User32.lib Winmm.lib Advapi32.lib
  MSFT:*_VS2015_X64_DLINK_FLAGS == /out:"$(BIN_DIR)\$(BASE_NAME).exe" /base:0x10000000 /pdb:"$(BIN_DIR)\$(BASE_NAME).pdb" /LIBPATH:"$(VCINSTALLDIR)\Lib\AMD64" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x64" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x64" /NOLOGO /SUBSYSTEM:CONSOLE /IGNORE:4086 /MAP /OPT:REF /DEBUG /MACHINE:AMD64 /LTCG Kernel32.lib MSVCRTD.lib vcruntimed.lib ucrtd.lib Gdi32.lib User32.lib Winmm.lib Advapi32.lib
  MSFT:*_VS2015x86_X64_DLINK_FLAGS == /out:"$(BIN_DIR)\$(BASE_NAME).exe" /base:0x10000000 /pdb:"$(BIN_DIR)\$(BASE_NAME).pdb" /LIBPATH:"$(VCINSTALLDIR)\Lib\AMD64" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x64" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x64" /NOLOGO /SUBSYSTEM:CONSOLE /IGNORE:4086 /MAP /OPT:REF /DEBUG /MACHINE:AMD64 /LTCG Kernel32.lib MSVCRTD.lib vcruntimed.lib ucrtd.lib Gdi32.lib User32.lib Winmm.lib Advapi32.lib
  MSFT:*_VS2017_X64_DLINK_FLAGS == /out:"$(BIN_DIR)\$(BASE_NAME).exe" /base:0x10000000 /pdb:"$(BIN_DIR)\$(BASE_NAME).pdb" /LIBPATH:"%VCToolsInstallDir%lib\x64" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x64" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x64" /NOLOGO /SUBSYSTEM:CONSOLE /IGNORE:4086 /MAP /OPT:REF /DEBUG /MACHINE:AMD64 /LTCG Kernel32.lib MSVCRTD.lib vcruntimed.lib ucrtd.lib Gdi32.lib User32.lib Winmm.lib Advapi32.lib
  MSFT:*_*_X64_CC_FLAGS == /nologo /W4 /WX /Gy /c /D UNICODE /Od /FIAutoGen.h /EHs-c- /GF /Gs8192 /Zi /Gm /D _CRT_SECURE_NO_WARNINGS /D _CRT_SECURE_NO_DEPRECATE
  MSFT:*_*_X64_PP_FLAGS == /nologo /E /TC /FIAutoGen.h
  MSFT:*_*_X64_ASM_FLAGS == /nologo /W3 /WX /c /Cx /Zd /W0 /Zi
  MSFT:*_*_X64_ASMLINK_FLAGS       == /link /nologo

  GCC:*_*_IA32_DLINK_FLAGS == -o $(BIN_DIR)/$(BASE_NAME) -m32 -L/usr/X11R6/lib
  GCC:*_*_IA32_CC_FLAGS == -m32 -g -fshort-wchar -fno-strict-aliasing -Wall -malign-double -idirafter/usr/include -c -include $(DEST_DIR_DEBUG)/AutoGen.h
  GCC:*_*_IA32_PP_FLAGS == -m32 -E -x assembler-with-cpp -include $(DEST_DIR_DEBUG)/AutoGen.h
  GCC:*_*_IA32_ASM_FLAGS == -m32 -c -x assembler -imacros $(DEST_DIR_DEBUG)/AutoGen.h
  GCC:*_*_IA32_DLINK2_FLAGS == -Wno-error -no-pie

  GCC:*_*_X64_DLINK_FLAGS == -o $(BIN_DIR)/$(BASE_NAME) -m64 -L/usr/X11R6/lib
  GCC:*_GCC5_X64_DLINK_FLAGS == -o $(BIN_DIR)/$(BASE_NAME) -m64 -L/usr/X11R6/lib
  GCC:*_*_X64_CC_FLAGS == -m64 -g -fshort-wchar -fno-strict-aliasing -Wall -malign-double -idirafter/usr/include -c -include $(DEST_DIR_DEBUG)/AutoGen.h
  GCC:*_GCC49_X64_CC_FLAGS = "-DEFIAPI=__attribute__((ms_abi))"
  GCC:*_GCC5_X64_CC_FLAGS = "-DEFIAPI=__attribute__((ms_abi))" -DUSING_LTO -Os
  GCC:*_*_X64_PP_FLAGS == -m64 -E -x assembler-with-cpp -include $(DEST_DIR_DEBUG)/AutoGen.h
  GCC:*_*_X64_ASM_FLAGS == -m64 -c -x assembler -imacros $(DEST_DIR_DEBUG)/AutoGen.h
  GCC:*_*_X64_DLINK2_FLAGS == -Wno-error -no-pie

  GCC:*_GCC5_*_CC_FLAGS = -fstack-protector -fstack-protector-strong -fstack-protector-all

  GCC:*_GCC5_*_CC_FLAGS = --coverage
  GCC:*_GCC5_*_DLINK_FLAGS = --coverage

  GCC:*_GCC5_X64_CC_FLAGS = "-DNO_MSABI_VA_FUNCS=TRUE"

