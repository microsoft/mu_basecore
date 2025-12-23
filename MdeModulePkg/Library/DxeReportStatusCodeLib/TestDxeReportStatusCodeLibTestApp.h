/** @file
  UEFI based application for unit testing the DxeReportStatusCodeLib.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TEST_DXE_REPORT_STATUS_CODE_LIB_H_
#define TEST_DXE_REPORT_STATUS_CODE_LIB_H_

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Library/ReportStatusCodeLib.h>

UNIT_TEST_STATUS
EFIAPI
TestReportStatusCodeDoesNotInvertTpl (
  IN UNIT_TEST_CONTEXT  Context
  );

#endif
