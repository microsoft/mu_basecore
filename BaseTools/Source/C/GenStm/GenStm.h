/** @file
  STM tool generate STM binary

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  # MU_CHANGE [WHOLE FILE] - Add the GenStm application

**/

#ifndef _GEN_STM_H_
#define _GEN_STM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "EfiUtilityMsgs.h"
#include "CommonLib.h"
#include "ParseInf.h"

#include <Common/PiFirmwareFile.h>

//
// Data structure needed to pass build
//
#include <IndustryStandard/PeImage.h>

#define  SIZE_2KB    0x00000800

#include <Register/Intel/StmApi.h>

//
// The EFI memory allocation functions work in units of EFI_PAGEs that are
// 4K. This should in no way be confused with the page size of the processor.
// An EFI_PAGE is just the quanta of memory in EFI.
//
#define STM_PAGE_SIZE             0x1000
#define STM_PAGE_MASK             0xFFF
#define STM_PAGE_SHIFT            12

#define STM_SIZE_TO_PAGES(a)  (((a) >> STM_PAGE_SHIFT) + (((a) & STM_PAGE_MASK) ? 1 : 0))

#define STM_PAGES_TO_SIZE(a)   ( (a) << STM_PAGE_SHIFT)

//
// Utility Name
//
#define UTILITY_NAME  "GenStm"

//
// Utility version information
//
#define UTILITY_MAJOR_VERSION 0
#define UTILITY_MINOR_VERSION 1
#define UTILITY_DATE          __DATE__

//
// STM data structure
//
#define STM_DATA_OFFSET           0x1000
#define STM_GDT_OFFSET            STM_DATA_OFFSET
#define STM_CODE_OFFSET           STM_DATA_OFFSET + 0x1000

#endif
