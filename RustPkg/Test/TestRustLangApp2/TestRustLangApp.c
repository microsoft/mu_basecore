/** @file
  A shell application that triggers capsule update process.

  Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestRustLangApp.h"

EFI_STATUS
EFIAPI
TestIntegerOverflow (
  IN UINTN  BufferSize,
  IN UINT32 Width,
  IN UINT32 Height
  );

VOID
EFIAPI
ExternInit (
  OUT UINTN  *Data
  )
{
  *Data = 2;
}

UINTN
EFIAPI
TestUninitializedVariable (
  IN UINTN  Index
  );

UINTN
EFIAPI
TestArrayOutOfRange (
  IN UINTN  Index
  );

typedef struct {
  UINT32  Type;
  UINT32  Length;
  UINT8   Value[0];
} TEST_TABLE;

VOID
EFIAPI
TestBufferOverflow (
  IN OUT UINT8  *Buffer,
  IN UINTN      BufferSize,
  IN TEST_TABLE *Table,
  IN UINTN      TableSize
  );

typedef struct {
  UINT32  Type;
  UINT32  Length;
  UINT8   Value[64];
} TEST_TABLE_FIXED;

VOID
EFIAPI
TestBufferOverflowFixed (
  IN OUT UINT8  Buffer[32],
  IN TEST_TABLE_FIXED *Table
  );

VOID
EFIAPI
TestBufferDrop (
  VOID
  );

VOID
EFIAPI
TestBufferBorrow (
  IN OUT TEST_TABLE_FIXED *Table
  );

VOID
EFIAPI
TestBufferAlloc (
  VOID
  );

TEST_TABLE_FIXED *
EFIAPI
TestBoxAlloc (
  IN UINT32 Type
  );

VOID
EFIAPI
TestBoxFree (
  IN TEST_TABLE_FIXED *Buffer
  );

VOID *
EFIAPI
TestBoxAllocFail (
  VOID
  );


/**
  Update Capsule image.

  @param[in]  ImageHandle     The image handle.
  @param[in]  SystemTable     The system table.

  @retval EFI_SUCCESS            Command completed successfully.
  @retval EFI_UNSUPPORTED        Command usage unsupported.
  @retval EFI_INVALID_PARAMETER  Command usage invalid.
  @retval EFI_NOT_FOUND          The input file can't be found.
**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINT8 Buffer[32];
  TEST_TABLE_FIXED Table;
  TEST_TABLE_FIXED *TableFixed;

  DEBUG ((DEBUG_INFO, "TestIntegerOverflow\n"));
  if (0) TestIntegerOverflow (0x10000, 0xFFFFFFFF, 0xFFFFFFFF); // panic
  DEBUG ((DEBUG_INFO, "TestUninitializedVariable\n"));
  TestUninitializedVariable (0);
  DEBUG ((DEBUG_INFO, "TestArrayOutOfRange\n"));
  if (0) TestArrayOutOfRange (10); // panic

  Table.Length = 64;
  DEBUG ((DEBUG_INFO, "TestBufferOverflow\n"));
  if (0) TestBufferOverflow (Buffer, 8, (TEST_TABLE *)&Table, 64); // panic
  DEBUG ((DEBUG_INFO, "TestBufferOverflowFixed\n"));
  if (0) TestBufferOverflowFixed (Buffer, &Table); // panic

  DEBUG ((DEBUG_INFO, "TestBufferDrop\n"));
  TestBufferDrop ();
  DEBUG ((DEBUG_INFO, "TestBufferBorrow\n"));
  TestBufferBorrow (&Table);
  DEBUG ((DEBUG_INFO, "TestBufferAlloc\n"));
  TestBufferAlloc ();

  DEBUG ((DEBUG_INFO, "TestBoxAlloc\n"));
  TableFixed = TestBoxAlloc (0xAFAF);
  DEBUG ((DEBUG_INFO, "TableFixed - 0x%x\n", TableFixed->Type));
  
  DEBUG ((DEBUG_INFO, "TestBoxFree\n"));
  TestBoxFree (TableFixed);

  DEBUG ((DEBUG_INFO, "TestBoxAllocFail\n"));
  if (0) TableFixed = TestBoxAllocFail (); // alloc_error_handler

  return EFI_UNSUPPORTED;
}
