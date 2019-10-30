  # Build Report
[Go to table of contents](#table-of-contents)
=====
 [Go to Error List](#error-list)
=====
    Log Started: Wednesday, October 30, 2019 01:27PM
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
    Running Python version: sys.version_info(major=3, minor=7, micro=4, releaselevel='final', serial=0)
    Cmd to run is: c:\_uefi\mu_ci\ci_env\lib\site-packages\edk2toollib\bin\vswhere.exe -latest -nologo -all -property installationPath -products * -version 15.0,16.0
    Cmd Output Starting
    C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise
    Cmd Output Finished
    Running Time (mm:ss): 00:00
    Cmd to run is: nmake.exe
    Cmd Output Starting
    Microsoft (R) Program Maintenance Utility Version 14.16.27027.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    Microsoft (R) Program Maintenance Utility Version 14.16.27027.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    Build libraries
    Microsoft (R) Program Maintenance Utility Version 14.16.27027.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Common
    Build executables
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\GenCrc32
    Microsoft (R) Program Maintenance Utility Version 14.16.27027.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\GenCrc32.exe C:\_uefi\mu_ci\mu_basecore\BaseTools\Lib\Win32\Common.lib GenCrc32.obj
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\GenFv
    Microsoft (R) Program Maintenance Utility Version 14.16.27027.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\GenFv.exe C:\_uefi\mu_ci\mu_basecore\BaseTools\Lib\Win32\Common.lib RpcRT4.lib GenFv.obj GenFvInternalLib.obj
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\GenFfs
    Microsoft (R) Program Maintenance Utility Version 14.16.27027.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\GenFfs.exe C:\_uefi\mu_ci\mu_basecore\BaseTools\Lib\Win32\Common.lib GenFfs.obj
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\GenSec
    Microsoft (R) Program Maintenance Utility Version 14.16.27027.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\GenSec.exe C:\_uefi\mu_ci\mu_basecore\BaseTools\Lib\Win32\Common.lib GenSec.obj
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\BrotliCompress
    Microsoft (R) Program Maintenance Utility Version 14.16.27027.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\Brotli.exe  tools\brotli.obj common\dictionary.obj common\transform.obj dec\bit_reader.obj dec\decode.obj dec\huffman.obj dec\state.obj enc\backward_references.obj enc\backward_references_hq.obj enc\bit_cost.obj enc\block_splitter.obj enc\brotli_bit_stream.obj enc\cluster.obj enc\compress_fragment.obj enc\compress_fragment_two_pass.obj enc\dictionary_hash.obj enc\encode.obj enc\encoder_dict.obj enc\entropy_encode.obj enc\histogram.obj enc\literal_cost.obj enc\memory.obj enc\metablock.obj enc\static_dict.obj enc\utf8_util.obj
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\GenFw
    Microsoft (R) Program Maintenance Utility Version 14.16.27027.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\GenFw.exe C:\_uefi\mu_ci\mu_basecore\BaseTools\Lib\Win32\Common.lib GenFw.obj ElfConvert.obj Elf32Convert.obj Elf64Convert.obj
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\EfiRom
    Microsoft (R) Program Maintenance Utility Version 14.16.27027.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\EfiRom.exe C:\_uefi\mu_ci\mu_basecore\BaseTools\Lib\Win32\Common.lib EfiRom.obj
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VfrCompile
    Microsoft (R) Program Maintenance Utility Version 14.16.27027.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\VfrCompile.exe C:\_uefi\mu_ci\mu_basecore\BaseTools\Lib\Win32\Common.lib AParser.obj DLexerBase.obj ATokenBuffer.obj EfiVfrParser.obj VfrLexer.obj VfrSyntax.obj VfrFormPkg.obj VfrError.obj VfrUtilityLib.obj VfrCompiler.obj
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\LzmaCompress
    Microsoft (R) Program Maintenance Utility Version 14.16.27027.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\LzmaCompress.exe  LzmaCompress.obj Sdk\C\Alloc.obj Sdk\C\LzFind.obj Sdk\C\LzmaDec.obj Sdk\C\LzmaEnc.obj Sdk\C\7zFile.obj Sdk\C\7zStream.obj Sdk\C\Bra86.obj Sdk\C\LzFindMt.obj Sdk\C\Threads.obj
    	copy LzmaF86Compress.bat C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\LzmaF86Compress.bat /Y
    1 file(s) copied.
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\Split
    Microsoft (R) Program Maintenance Utility Version 14.16.27027.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\Split.exe C:\_uefi\mu_ci\mu_basecore\BaseTools\Lib\Win32\Common.lib Split.obj
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\DevicePath
    Microsoft (R) Program Maintenance Utility Version 14.16.27027.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\DevicePath.exe C:\_uefi\mu_ci\mu_basecore\BaseTools\Lib\Win32\Common.lib DevicePath.obj UefiDevicePathLib.obj DevicePathFromText.obj DevicePathUtilities.obj
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\TianoCompress
    Microsoft (R) Program Maintenance Utility Version 14.16.27027.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\TianoCompress.exe C:\_uefi\mu_ci\mu_basecore\BaseTools\Lib\Win32\Common.lib TianoCompress.obj
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C\VolInfo
    Microsoft (R) Program Maintenance Utility Version 14.16.27027.1
    Copyright (C) Microsoft Corporation.  All rights reserved.
    	link.exe /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32\VolInfo.exe C:\_uefi\mu_ci\mu_basecore\BaseTools\Lib\Win32\Common.lib VolInfo.obj
    Install to C:\_uefi\mu_ci\mu_basecore\BaseTools\Lib\Win32
    Install to C:\_uefi\mu_ci\mu_basecore\BaseTools\Bin\Win32
    execute command "nmake all" in directory C:\_uefi\mu_ci\mu_basecore\BaseTools\Source\C
    Cmd Output Finished
    Running Time (mm:ss): 00:05
## Summary
    Success
## Table of Contents
+ [Init SDE](#init-sde)
+ [Loading Plugins](#loading-plugins)
+ [Start Invocable Tool](#start-invocable-tool)
+ [Summary](#summary)
## Error List
   No errors found