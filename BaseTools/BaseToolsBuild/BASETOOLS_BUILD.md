  # Build Report
[Go to table of contents](#table-of-contents)
=====
 [Go to Error List](#error-list)
=====
    Log Started: Wednesday, October 30, 2019 05:20PM
## Init SDE
    Computing path for mu_nasm located at C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\mu_nasm_extdep on Host(os='Windows', arch='x86', bit='64')
    C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\mu_nasm_extdep\Windows-x86-64 was found!
  _ WARNING: version_aggregator: This mu_nasm:2.14.02 key/value pair was already registered_
    Computing path for mu_nasm located at C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\mu_nasm_extdep on Host(os='Windows', arch='x86', bit='64')
    C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\mu_nasm_extdep\Windows-x86-64 was found!
  _ WARNING: version_aggregator: This mu_nasm:2.14.02 key/value pair was already registered_
## Loading Plugins
    PLUGIN DESCRIPTOR:Edk2Tool Helper Functions
    PLUGIN DESCRIPTOR:Windows Capsule Support Helper Functions
## Start Invocable Tool
    Running Python version: sys.version_info(major=3, minor=7, micro=0, releaselevel='final', serial=0)
    Cmd to run is: c:\_uefi\mu_ci\ci_env\lib\site-packages\edk2toollib\bin\vswhere.exe -latest -nologo -all -property installationPath -products * -version 15.0,16.0
    Cmd Output Starting
    C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools
    Cmd Output Finished
    Running Time (mm:ss): 00:00
    Cmd to run is: nmake.exe
    Cmd Output Starting
    Microsoft (R) Program Maintenance Utility Version 14.16.27032.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    Microsoft (R) Program Maintenance Utility Version 14.16.27032.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    Build libraries
    Microsoft (R) Program Maintenance Utility Version 14.16.27032.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  BasePeCoff.c -FoBasePeCoff.obj
    BasePeCoff.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  BinderFuncs.c -FoBinderFuncs.obj
    BinderFuncs.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  CommonLib.c -FoCommonLib.obj
    CommonLib.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  Crc32.c -FoCrc32.obj
    Crc32.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  Decompress.c -FoDecompress.obj
    Decompress.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  EfiCompress.c -FoEfiCompress.obj
    EfiCompress.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  EfiUtilityMsgs.c -FoEfiUtilityMsgs.obj
    EfiUtilityMsgs.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  FirmwareVolumeBuffer.c -FoFirmwareVolumeBuffer.obj
    FirmwareVolumeBuffer.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  FvLib.c -FoFvLib.obj
    FvLib.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  MemoryFile.c -FoMemoryFile.obj
    MemoryFile.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  MyAlloc.c -FoMyAlloc.obj
    MyAlloc.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  OsPath.c -FoOsPath.obj
    OsPath.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  ParseGuidedSectionTools.c -FoParseGuidedSectionTools.obj
    ParseGuidedSectionTools.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  ParseInf.c -FoParseInf.obj
    ParseInf.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  PeCoffLoaderEx.c -FoPeCoffLoaderEx.obj
    PeCoffLoaderEx.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  SimpleFileParsing.c -FoSimpleFileParsing.obj
    SimpleFileParsing.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  StringFuncs.c -FoStringFuncs.obj
    StringFuncs.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  TianoCompress.c -FoTianoCompress.obj
    TianoCompress.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  PcdValueCommon.c -FoPcdValueCommon.obj
    PcdValueCommon.c
    	lib.exe /nologo /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Lib\Win32\Common.lib BasePeCoff.obj BinderFuncs.obj CommonLib.obj Crc32.obj Decompress.obj EfiCompress.obj EfiUtilityMsgs.obj FirmwareVolumeBuffer.obj FvLib.obj MemoryFile.obj MyAlloc.obj OsPath.obj ParseGuidedSectionTools.obj ParseInf.obj PeCoffLoaderEx.obj SimpleFileParsing.obj StringFuncs.obj TianoCompress.obj PcdValueCommon.obj
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common
    Build executables
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\GenCrc32
    Microsoft (R) Program Maintenance Utility Version 14.16.27032.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  GenCrc32.c -FoGenCrc32.obj
    GenCrc32.c
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\GenCrc32.exe C:\_uefi\mu_ci\mu_basecore\BaseTools\Lib\Win32\Common.lib GenCrc32.obj
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\EfiRom
    Microsoft (R) Program Maintenance Utility Version 14.16.27032.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  EfiRom.c -FoEfiRom.obj
    EfiRom.c
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\EfiRom.exe C:\_uefi\mu_ci\mu_basecore\BaseTools\Lib\Win32\Common.lib EfiRom.obj
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\GenSec
    Microsoft (R) Program Maintenance Utility Version 14.16.27032.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  GenSec.c -FoGenSec.obj
    GenSec.c
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\GenSec.exe C:\_uefi\mu_ci\mu_basecore\BaseTools\Lib\Win32\Common.lib GenSec.obj
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\GenFfs
    Microsoft (R) Program Maintenance Utility Version 14.16.27032.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  GenFfs.c -FoGenFfs.obj
    GenFfs.c
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\GenFfs.exe C:\_uefi\mu_ci\mu_basecore\BaseTools\Lib\Win32\Common.lib GenFfs.obj
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\GenFv
    Microsoft (R) Program Maintenance Utility Version 14.16.27032.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  GenFv.c -FoGenFv.obj
    GenFv.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  GenFvInternalLib.c -FoGenFvInternalLib.obj
    GenFvInternalLib.c
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\GenFv.exe C:\_uefi\mu_ci\mu_basecore\BaseTools\Lib\Win32\Common.lib RpcRT4.lib GenFv.obj GenFvInternalLib.obj
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Split
    Microsoft (R) Program Maintenance Utility Version 14.16.27032.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  Split.c -FoSplit.obj
    Split.c
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\Split.exe C:\_uefi\mu_ci\mu_basecore\BaseTools\Lib\Win32\Common.lib Split.obj
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\TianoCompress
    Microsoft (R) Program Maintenance Utility Version 14.16.27032.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  TianoCompress.c -FoTianoCompress.obj
    TianoCompress.c
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\TianoCompress.exe C:\_uefi\mu_ci\mu_basecore\BaseTools\Lib\Win32\Common.lib TianoCompress.obj
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VolInfo
    Microsoft (R) Program Maintenance Utility Version 14.16.27032.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  VolInfo.c -FoVolInfo.obj
    VolInfo.c
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\VolInfo.exe C:\_uefi\mu_ci\mu_basecore\BaseTools\Lib\Win32\Common.lib VolInfo.obj
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\DevicePath
    Microsoft (R) Program Maintenance Utility Version 14.16.27032.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  DevicePath.c -FoDevicePath.obj
    DevicePath.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  UefiDevicePathLib.c -FoUefiDevicePathLib.obj
    UefiDevicePathLib.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  DevicePathFromText.c -FoDevicePathFromText.obj
    DevicePathFromText.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  DevicePathUtilities.c -FoDevicePathUtilities.obj
    DevicePathUtilities.c
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\DevicePath.exe C:\_uefi\mu_ci\mu_basecore\BaseTools\Lib\Win32\Common.lib DevicePath.obj UefiDevicePathLib.obj DevicePathFromText.obj DevicePathUtilities.obj
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\GenFw
    Microsoft (R) Program Maintenance Utility Version 14.16.27032.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  GenFw.c -FoGenFw.obj
    GenFw.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  ElfConvert.c -FoElfConvert.obj
    ElfConvert.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  Elf32Convert.c -FoElf32Convert.obj
    Elf32Convert.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  Elf64Convert.c -FoElf64Convert.obj
    Elf64Convert.c
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\GenFw.exe C:\_uefi\mu_ci\mu_basecore\BaseTools\Lib\Win32\Common.lib GenFw.obj ElfConvert.obj Elf32Convert.obj Elf64Convert.obj
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\LzmaCompress
    Microsoft (R) Program Maintenance Utility Version 14.16.27032.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  LzmaCompress.c -FoLzmaCompress.obj
    LzmaCompress.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  Sdk\C\Alloc.c -FoSdk\C\Alloc.obj
    Alloc.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  Sdk\C\LzFind.c -FoSdk\C\LzFind.obj
    LzFind.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  Sdk\C\LzmaDec.c -FoSdk\C\LzmaDec.obj
    LzmaDec.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  Sdk\C\LzmaEnc.c -FoSdk\C\LzmaEnc.obj
    LzmaEnc.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  Sdk\C\7zFile.c -FoSdk\C\7zFile.obj
    7zFile.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  Sdk\C\7zStream.c -FoSdk\C\7zStream.obj
    7zStream.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  Sdk\C\Bra86.c -FoSdk\C\Bra86.obj
    Bra86.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  Sdk\C\LzFindMt.c -FoSdk\C\LzFindMt.obj
    LzFindMt.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  Sdk\C\Threads.c -FoSdk\C\Threads.obj
    Threads.c
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\LzmaCompress.exe  LzmaCompress.obj Sdk\C\Alloc.obj Sdk\C\LzFind.obj Sdk\C\LzmaDec.obj Sdk\C\LzmaEnc.obj Sdk\C\7zFile.obj Sdk\C\7zStream.obj Sdk\C\Bra86.obj Sdk\C\LzFindMt.obj Sdk\C\Threads.obj
    	copy LzmaF86Compress.bat C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\LzmaF86Compress.bat /Y
    1 file(s) copied.
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\BrotliCompress
    Microsoft (R) Program Maintenance Utility Version 14.16.27032.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /W2 -I .\include -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  tools\brotli.c -Fotools\brotli.obj
    cl : Command line warning D9025 : overriding '/W4' with '/W2'
    brotli.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /W2 -I .\include -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  common\dictionary.c -Focommon\dictionary.obj
    cl : Command line warning D9025 : overriding '/W4' with '/W2'
    dictionary.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /W2 -I .\include -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  common\transform.c -Focommon\transform.obj
    cl : Command line warning D9025 : overriding '/W4' with '/W2'
    transform.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /W2 -I .\include -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  dec\bit_reader.c -Fodec\bit_reader.obj
    cl : Command line warning D9025 : overriding '/W4' with '/W2'
    bit_reader.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /W2 -I .\include -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  dec\decode.c -Fodec\decode.obj
    cl : Command line warning D9025 : overriding '/W4' with '/W2'
    decode.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /W2 -I .\include -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  dec\huffman.c -Fodec\huffman.obj
    cl : Command line warning D9025 : overriding '/W4' with '/W2'
    huffman.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /W2 -I .\include -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  dec\state.c -Fodec\state.obj
    cl : Command line warning D9025 : overriding '/W4' with '/W2'
    state.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /W2 -I .\include -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  enc\backward_references.c -Foenc\backward_references.obj
    cl : Command line warning D9025 : overriding '/W4' with '/W2'
    backward_references.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /W2 -I .\include -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  enc\backward_references_hq.c -Foenc\backward_references_hq.obj
    cl : Command line warning D9025 : overriding '/W4' with '/W2'
    backward_references_hq.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /W2 -I .\include -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  enc\bit_cost.c -Foenc\bit_cost.obj
    cl : Command line warning D9025 : overriding '/W4' with '/W2'
    bit_cost.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /W2 -I .\include -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  enc\block_splitter.c -Foenc\block_splitter.obj
    cl : Command line warning D9025 : overriding '/W4' with '/W2'
    block_splitter.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /W2 -I .\include -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  enc\brotli_bit_stream.c -Foenc\brotli_bit_stream.obj
    cl : Command line warning D9025 : overriding '/W4' with '/W2'
    brotli_bit_stream.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /W2 -I .\include -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  enc\cluster.c -Foenc\cluster.obj
    cl : Command line warning D9025 : overriding '/W4' with '/W2'
    cluster.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /W2 -I .\include -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  enc\compress_fragment.c -Foenc\compress_fragment.obj
    cl : Command line warning D9025 : overriding '/W4' with '/W2'
    compress_fragment.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /W2 -I .\include -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  enc\compress_fragment_two_pass.c -Foenc\compress_fragment_two_pass.obj
    cl : Command line warning D9025 : overriding '/W4' with '/W2'
    compress_fragment_two_pass.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /W2 -I .\include -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  enc\dictionary_hash.c -Foenc\dictionary_hash.obj
    cl : Command line warning D9025 : overriding '/W4' with '/W2'
    dictionary_hash.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /W2 -I .\include -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  enc\encode.c -Foenc\encode.obj
    cl : Command line warning D9025 : overriding '/W4' with '/W2'
    encode.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /W2 -I .\include -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  enc\encoder_dict.c -Foenc\encoder_dict.obj
    cl : Command line warning D9025 : overriding '/W4' with '/W2'
    encoder_dict.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /W2 -I .\include -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  enc\entropy_encode.c -Foenc\entropy_encode.obj
    cl : Command line warning D9025 : overriding '/W4' with '/W2'
    entropy_encode.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /W2 -I .\include -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  enc\histogram.c -Foenc\histogram.obj
    cl : Command line warning D9025 : overriding '/W4' with '/W2'
    histogram.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /W2 -I .\include -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  enc\literal_cost.c -Foenc\literal_cost.obj
    cl : Command line warning D9025 : overriding '/W4' with '/W2'
    literal_cost.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /W2 -I .\include -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  enc\memory.c -Foenc\memory.obj
    cl : Command line warning D9025 : overriding '/W4' with '/W2'
    memory.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /W2 -I .\include -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  enc\metablock.c -Foenc\metablock.obj
    cl : Command line warning D9025 : overriding '/W4' with '/W2'
    metablock.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /W2 -I .\include -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  enc\static_dict.c -Foenc\static_dict.obj
    cl : Command line warning D9025 : overriding '/W4' with '/W2'
    static_dict.c
    	cl.exe -c  /nologo /Zi /c /O2 /MT /W4 /WX /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /W2 -I .\include -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  enc\utf8_util.c -Foenc\utf8_util.obj
    cl : Command line warning D9025 : overriding '/W4' with '/W2'
    utf8_util.c
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\Brotli.exe  tools\brotli.obj common\dictionary.obj common\transform.obj dec\bit_reader.obj dec\decode.obj dec\huffman.obj dec\state.obj enc\backward_references.obj enc\backward_references_hq.obj enc\bit_cost.obj enc\block_splitter.obj enc\brotli_bit_stream.obj enc\cluster.obj enc\compress_fragment.obj enc\compress_fragment_two_pass.obj enc\dictionary_hash.obj enc\encode.obj enc\encoder_dict.obj enc\entropy_encode.obj enc\histogram.obj enc\literal_cost.obj enc\memory.obj enc\metablock.obj enc\static_dict.obj enc\utf8_util.obj
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile
    Microsoft (R) Program Maintenance Utility Version 14.16.27032.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	cl.exe -c  /EHsc /nologo /Zi /c /O2 /MT /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /WX /D PCCTS_USE_NAMESPACE_STD -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Common\BuildVersion.h Pccts\h\AParser.cpp -FoAParser.obj
    cl : Command line warning D9024 : unrecognized source file type 'C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Common\BuildVersion.h', object file assumed
    cl : Command line warning D9027 : source file 'C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Common\BuildVersion.h' ignored
    AParser.cpp
    	cl.exe -c  /EHsc /nologo /Zi /c /O2 /MT /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /WX /D PCCTS_USE_NAMESPACE_STD -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Common\BuildVersion.h Pccts\h\DLexerBase.cpp -FoDLexerBase.obj
    cl : Command line warning D9024 : unrecognized source file type 'C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Common\BuildVersion.h', object file assumed
    cl : Command line warning D9027 : source file 'C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Common\BuildVersion.h' ignored
    DLexerBase.cpp
    	cl.exe -c  /EHsc /nologo /Zi /c /O2 /MT /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /WX /D PCCTS_USE_NAMESPACE_STD -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Common\BuildVersion.h Pccts\h\ATokenBuffer.cpp -FoATokenBuffer.obj
    cl : Command line warning D9024 : unrecognized source file type 'C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Common\BuildVersion.h', object file assumed
    cl : Command line warning D9027 : source file 'C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Common\BuildVersion.h' ignored
    ATokenBuffer.cpp
    	pushd . & cd Pccts & nmake -nologo & popd
    	pushd . & cd antlr & "C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Tools\MSVC\14.16.27023\bin\HostX86\x86\nmake.exe" /nologo /f AntlrMS.mak & popd
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /Zi /W3 -D__USE_PROTOS /wd4700  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\antlr\antlr.c
    antlr.c
    C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\antlr\antlr.c(44): warning C4068: unknown pragma
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /Zi /W3 -D__USE_PROTOS /wd4700  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\antlr\scan.c
    scan.c
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /Zi /W3 -D__USE_PROTOS /wd4700  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\antlr\err.c
    err.c
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /Zi /W3 -D__USE_PROTOS /wd4700  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\antlr\bits.c
    bits.c
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /Zi /W3 -D__USE_PROTOS /wd4700  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\antlr\build.c
    build.c
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /Zi /W3 -D__USE_PROTOS /wd4700  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\antlr\fset2.c
    fset2.c
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /Zi /W3 -D__USE_PROTOS /wd4700  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\antlr\fset.c
    fset.c
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /Zi /W3 -D__USE_PROTOS /wd4700  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\antlr\gen.c
    gen.c
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /Zi /W3 -D__USE_PROTOS /wd4700  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\antlr\globals.c
    globals.c
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /Zi /W3 -D__USE_PROTOS /wd4700  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\antlr\hash.c
    hash.c
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /Zi /W3 -D__USE_PROTOS /wd4700  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\antlr\lex.c
    lex.c
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /Zi /W3 -D__USE_PROTOS /wd4700  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\antlr\main.c
    main.c
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /Zi /W3 -D__USE_PROTOS /wd4700  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\antlr\misc.c
    misc.c
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /Zi /W3 -D__USE_PROTOS /wd4700  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\antlr\pred.c
    pred.c
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /Zi /W3 -D__USE_PROTOS /wd4700  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\antlr\egman.c
    egman.c
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /Zi /W3 -D__USE_PROTOS /wd4700  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\antlr\mrhoist.c
    mrhoist.c
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /Zi /W3 -D__USE_PROTOS /wd4700  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\antlr\fcache.c
    fcache.c
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /Zi /W3 -D__USE_PROTOS /wd4700  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set\set.c
    set.c
    	cl /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /Zi /W3 -D__USE_PROTOS /wd4700  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -Feantlr.exe antlr.obj scan.obj err.obj bits.obj build.obj fset2.obj  fset.obj gen.obj globals.obj hash.obj lex.obj main.obj  misc.obj pred.obj egman.obj mrhoist.obj fcache.obj set.obj
    	copy antlr.exe C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32
    1 file(s) copied.
    	pushd . & cd dlg & "C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Tools\MSVC\14.16.27023\bin\HostX86\x86\nmake.exe" /nologo /f DlgMS.mak & popd
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /W3 /Zi  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\dlg\dlg_p.c
    dlg_p.c
    C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\dlg\dlg_p.c(42): warning C4068: unknown pragma
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /W3 /Zi  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\dlg\dlg_a.c
    dlg_a.c
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /W3 /Zi  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\dlg\main.c
    main.c
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /W3 /Zi  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\dlg\err.c
    err.c
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /W3 /Zi  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\dlg\support.c
    support.c
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /W3 /Zi  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\dlg\output.c
    output.c
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /W3 /Zi  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\dlg\relabel.c
    relabel.c
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /W3 /Zi  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\dlg\automata.c
    automata.c
    	cl -c /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /W3 /Zi  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set\set.c
    set.c
    	cl /nologo -I "." -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h" -I "C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\support\set" -D "USER_ZZSYN" -D "PC"  -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /W3 /Zi  /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE -Fedlg.exe dlg_p.obj dlg_a.obj main.obj err.obj support.obj  output.obj relabel.obj automata.obj set.obj
    	copy dlg.exe C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32
    1 file(s) copied.
    	antlr -CC -e3 -ck 3 -k 2 -fl VfrParser.dlg -ft VfrTokens.h -o . VfrSyntax.g
    Antlr parser generator   Version 1.33MR33   1989-2001
    VfrSyntax.g(3508) : warning: alts 1 and 2 of {..} ambiguous upon ( ";" RefreshGuid GuidOp Locked Image EndIf InconsistentIf DisableIf SuppressIf Default GrayOutIf )
    VfrSyntax.g(3517) : warning: alts 1 and 2 of {..} ambiguous upon ( ";" RefreshGuid GuidOp Locked Image EndIf InconsistentIf DisableIf SuppressIf Default GrayOutIf )
    VfrSyntax.g(3526) : warning: alts 1 and 2 of {..} ambiguous upon ( ";" RefreshGuid GuidOp Locked Image EndIf InconsistentIf DisableIf SuppressIf Default GrayOutIf )
    VfrSyntax.g(3536) : warning: alts 1 and 2 of {..} ambiguous upon ( ";" RefreshGuid GuidOp Locked Image EndIf InconsistentIf DisableIf SuppressIf Default GrayOutIf )
    VfrSyntax.g(3566) : warning: alts 1 and 2 of {..} ambiguous upon ( ";" RefreshGuid GuidOp Locked Image EndIf InconsistentIf DisableIf SuppressIf Default GrayOutIf )
    VfrSyntax.g(3575) : warning: alts 1 and 2 of {..} ambiguous upon ( ";" RefreshGuid GuidOp Locked Image EndIf InconsistentIf DisableIf SuppressIf Default GrayOutIf )
    	cl.exe -c  /EHsc /nologo /Zi /c /O2 /MT /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /WX /D PCCTS_USE_NAMESPACE_STD -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h EfiVfrParser.cpp -FoEfiVfrParser.obj
    EfiVfrParser.cpp
    	dlg -C2 -i -CC -cl VfrLexer -o . VfrParser.dlg
    dlg  Version 1.33MR33   1989-2001
    	cl.exe -c  /EHsc /nologo /Zi /c /O2 /MT /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /WX /D PCCTS_USE_NAMESPACE_STD -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h VfrLexer.cpp -FoVfrLexer.obj
    VfrLexer.cpp
    	cl.exe -c  /EHsc /nologo /Zi /c /O2 /MT /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /WX /D PCCTS_USE_NAMESPACE_STD -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h VfrSyntax.cpp -FoVfrSyntax.obj
    VfrSyntax.cpp
    	cl.exe -c  /EHsc /nologo /Zi /c /O2 /MT /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /WX /D PCCTS_USE_NAMESPACE_STD -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h VfrFormPkg.cpp -FoVfrFormPkg.obj
    VfrFormPkg.cpp
    	cl.exe -c  /EHsc /nologo /Zi /c /O2 /MT /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /WX /D PCCTS_USE_NAMESPACE_STD -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h VfrError.cpp -FoVfrError.obj
    VfrError.cpp
    	cl.exe -c  /EHsc /nologo /Zi /c /O2 /MT /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /WX /D PCCTS_USE_NAMESPACE_STD -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h VfrUtilityLib.cpp -FoVfrUtilityLib.obj
    VfrUtilityLib.cpp
    	cl.exe -c  /EHsc /nologo /Zi /c /O2 /MT /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /WX /D PCCTS_USE_NAMESPACE_STD -I . -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Include\Ia32 -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common  -I C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile\Pccts\h VfrCompiler.cpp -FoVfrCompiler.obj
    VfrCompiler.cpp
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\VfrCompile.exe C:\_uefi\mu_ci\mu_basecore\BaseTools\Lib\Win32\Common.lib AParser.obj DLexerBase.obj ATokenBuffer.obj EfiVfrParser.obj VfrLexer.obj VfrSyntax.obj VfrFormPkg.obj VfrError.obj VfrUtilityLib.obj VfrCompiler.obj
    Install to C:\_uefi\mu_ci\mu_basecore\BaseTools\Lib\Win32
    Install to C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C
    Cmd Output Finished
    Running Time (mm:ss): 00:43
## Summary
    Success
## Table of Contents
+ [Init SDE](#init-sde)
+ [Loading Plugins](#loading-plugins)
+ [Start Invocable Tool](#start-invocable-tool)
+ [Summary](#summary)
## Error List
   No errors found