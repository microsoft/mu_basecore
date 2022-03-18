/** @file
Helper functions for working with the declared test data.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "VariableRuntimeDxeUnitTest.h"

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

extern TEST_VARIABLE_HEADER  *mGlobalTestVarDb[];
extern UINT32                mGlobalTestVarDbCount;

STATIC
UINT8 *
DecodeBase64String (
  IN CONST  CHAR8   *Data,
  OUT       UINT32  *OutputSize
  )
{
  UINTN  SourceSize, DestSize;
  UINT8  *Result;

  ASSERT (Data != NULL);
  ASSERT (OutputSize != NULL);

  SourceSize = AsciiStrLen (Data);
  DestSize   = 0;
  Result     = NULL;

  if (EFI_ERROR (Base64Decode (Data, SourceSize, NULL, &DestSize))) {
    return NULL;
  }

  Result = AllocatePool (DestSize);

  if ((Result != NULL) && EFI_ERROR (Base64Decode (Data, SourceSize, Result, &DestSize))) {
    FreePool (Result);
    Result = NULL;
  } else {
    *OutputSize = (UINT32)DestSize;
  }

  return Result;
}

STATIC
UINT8 *
DecodeHexString (
  IN CONST  CHAR8   *Data,
  OUT       UINT32  *OutputSize
  )
{
  UINTN  SourceSize, DestSize;
  UINT8  *Result;

  ASSERT (Data != NULL);
  ASSERT (OutputSize != NULL);

  SourceSize = AsciiStrLen (Data);
  DestSize   = 0;
  Result     = NULL;

  ASSERT ((SourceSize & 0x1) == 0);
  DestSize = SourceSize >> 1;
  Result   = AllocatePool (DestSize);

  if ((Result != NULL) && EFI_ERROR (AsciiStrHexToBytes (Data, SourceSize, Result, DestSize))) {
    FreePool (Result);
    Result = NULL;
  } else {
    *OutputSize = (UINT32)DestSize;
  }

  return Result;
}

STATIC
UINT8 *
DecodeDataString (
  IN        UINT32  Encoding,
  IN CONST  CHAR8   *Data,
  OUT       UINT32  *OutputSize
  )
{
  UINT8  *Result;

  switch (Encoding) {
    case DATA_ENC_BASE64:
      Result = DecodeBase64String (Data, OutputSize);
      break;
    default:
      Result = DecodeHexString (Data, OutputSize);
      break;
  }

  return Result;
}

STATIC
BOOLEAN
ShouldHaveSigData (
  TEST_VARIABLE_MODEL  *Model
  )
{
  return (Model->VarType == VAR_TYPE_TIME_AUTH);
}

TEST_VARIABLE_MODEL *
LoadTestVariable (
  IN CONST    CHAR8  *TestName
  )
{
  UINTN                 Index;
  TEST_VARIABLE_HEADER  *FoundVariable;
  TEST_VARIABLE_AUTH    *FoundAuthVariable;
  TEST_VARIABLE_MODEL   *NewModel;

  FoundVariable     = NULL;
  FoundAuthVariable = NULL;
  NewModel          = NULL;

  for (Index = 0; Index < mGlobalTestVarDbCount; Index++) {
    if (AsciiStrCmp (TestName, mGlobalTestVarDb[Index]->TestName) == 0) {
      FoundVariable = mGlobalTestVarDb[Index];
      break;
    }
  }

  if (FoundVariable != NULL) {
    NewModel = AllocateZeroPool (sizeof (*NewModel));
    if (NewModel == NULL) {
      return NULL;
    }

    CopyMem (NewModel, FoundVariable, OFFSET_OF (TEST_VARIABLE_HEADER, Data));
    NewModel->Data = DecodeDataString (
                       FoundVariable->DataEnc,
                       FoundVariable->Data,
                       &NewModel->DataSize
                       );

    if (FoundVariable->VarType == VAR_TYPE_TIME_AUTH) {
      FoundAuthVariable = (TEST_VARIABLE_AUTH *)FoundVariable;
      CopyMem (&NewModel->Timestamp, &FoundAuthVariable->Timestamp, sizeof (EFI_TIME));
      NewModel->SigData = DecodeDataString (
                            FoundAuthVariable->SigDataEnc,
                            FoundAuthVariable->SigData,
                            &NewModel->SigDataSize
                            );
    }

    if ((NewModel->Data == NULL) ||
        (ShouldHaveSigData (NewModel) && (NewModel->SigData == NULL)))
    {
      FreeTestVariable (NewModel);
      NewModel = NULL;
    }
  }

  return NewModel;
}

VOID
FreeTestVariable (
  IN OUT      TEST_VARIABLE_MODEL  *VarModel
  )
{
  ASSERT (VarModel != NULL);
  if (VarModel->Data != NULL) {
    FreePool (VarModel->Data);
  }

  if (VarModel->SigData != NULL) {
    FreePool (VarModel->SigData);
  }

  FreePool (VarModel);
}
