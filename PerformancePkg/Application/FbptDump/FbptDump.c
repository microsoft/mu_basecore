/** @file -- FbptDump.c
 This user-facing application dumps FBPT into a file.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Guid/FirmwarePerformance.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Library/SafeIntLib.h>
#include <Protocol/Smbios.h>

#define MAX_STRING_SIZE  0x1000

/**

  Acquire the string associated with the Index from smbios structure and return it.
  The caller is responsible for free the string buffer.

  @param    OptionalStrStart  The start position to search the string
  @param    Index             The index of the string to extract
  @param    String            The string that is extracted

  @retval   EFI_SUCCESS       The function returns EFI_SUCCESS always.

**/
EFI_STATUS
GetOptionalStringByIndex (
  IN      CHAR8   *OptionalStrStart,
  IN      UINT8   Index,
  OUT     CHAR16  **String
  )
{
  UINTN  StrSize;

  if (Index == 0) {
    *String = AllocateZeroPool (sizeof (CHAR16));
    return EFI_SUCCESS;
  }

  StrSize = 0;
  do {
    Index--;
    OptionalStrStart += StrSize;
    StrSize           = AsciiStrSize (OptionalStrStart);
  } while (OptionalStrStart[StrSize] != 0 && Index != 0);

  if ((Index != 0) || (StrSize == 1)) {
    *String = AllocateZeroPool (sizeof (CHAR16));
  } else {
    *String = AllocatePool (StrSize * sizeof (CHAR16));
    AsciiStrToUnicodeStrS (OptionalStrStart, *String, StrSize);
  }

  return EFI_SUCCESS;
}

/**
 * @brief      Writes a buffer to file.
 *
 * @param      FileName     The name of the file being written to.
 * @param      Buffer       The buffer to write to file.
 * @param[in]  BufferSize   Size of the buffer.
 * @param[in]  WriteCount   Number to append to the end of the file.
 */
STATIC
VOID
WriteBufferToFile (
  IN CONST CHAR16  *FileName,
  IN       VOID    *Buffer,
  IN       UINTN   BufferSize
  )
{
  EFI_STATUS         Status;
  UINTN              StringSize = BufferSize;
  SHELL_FILE_HANDLE  FileHandle;
  CHAR16             FileNameAndExt[MAX_STRING_SIZE];

  // Calculate final file name.
  ZeroMem (FileNameAndExt, sizeof (CHAR16) * MAX_STRING_SIZE);
  UnicodeSPrint (FileNameAndExt, MAX_STRING_SIZE, L"%s.bin", FileName);

  // First, let's open the file if it exists so we can delete it...
  // This is the work around for truncation
  Status = ShellOpenFileByName (
             FileNameAndExt,
             &FileHandle,
             (EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE),
             0
             );
  if (!EFI_ERROR (Status)) {
    // If file handle above was opened it will be closed by the delete.
    Status = ShellDeleteFile (&FileHandle);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a failed to delete file %r\n", __FUNCTION__, Status));
    }
  }

  // Open the file and write buffer contents.
  Status = ShellOpenFileByName (
             FileNameAndExt,
             &FileHandle,
             (EFI_FILE_MODE_CREATE | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ),
             0
             );
  if (!EFI_ERROR (Status)) {
    ShellWriteFile (FileHandle, &StringSize, Buffer);
    ShellCloseFile (&FileHandle);

    ShellPrintEx (-1, -1, L"Wrote to file %s\n", FileNameAndExt);
  }
}

/**
  This function uses the ACPI SDT protocol to locate an ACPI table.
  It is really only useful for finding tables that only have a single instance,
  e.g. FADT, FACS, MADT, etc.  It is not good for locating SSDT, etc.

  @param[in] Signature           - Pointer to an ASCII string containing the OEM Table ID from the ACPI table header
  @param[in, out] Table          - Updated with a pointer to the table
  @param[in, out] Handle         - AcpiSupport protocol table handle for the table found
  @param[in, out] Version        - The version of the table desired

  @retval EFI_SUCCESS            - The function completed successfully.
**/
static
EFI_STATUS
LocateAcpiTableBySignature (
  IN      UINT32                       Signature,
  IN OUT  EFI_ACPI_DESCRIPTION_HEADER  **Table,
  IN OUT  UINTN                        *Handle
  )
{
  EFI_STATUS              Status;
  INTN                    Index;
  EFI_ACPI_TABLE_VERSION  Version;
  EFI_ACPI_SDT_PROTOCOL   *AcpiSdt = NULL;

  Status = gBS->LocateProtocol (&gEfiAcpiSdtProtocolGuid, NULL, (VOID **)&AcpiSdt);

  if (EFI_ERROR (Status) || (AcpiSdt == NULL)) {
    return EFI_NOT_FOUND;
  }

  //
  // Locate table with matching ID
  //
  Version = 0;
  Index   = 0;
  do {
    Status = AcpiSdt->GetAcpiTable (Index, (EFI_ACPI_SDT_HEADER **)Table, &Version, Handle);
    if (EFI_ERROR (Status)) {
      break;
    }

    Index++;
  } while ((*Table)->Signature != Signature);

  //
  // If we found the table, there will be no error.
  //
  return Status;
}

/**
  FbptDumpEntryPoint

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occured when executing this entry point.

**/
EFI_STATUS
EFIAPI
FbptDumpEntryPoint (
  IN     EFI_HANDLE        ImageHandle,
  IN     EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINTN                                        Handle;
  FIRMWARE_PERFORMANCE_TABLE                   *pFPDT;
  EFI_ACPI_5_0_FPDT_FIRMWARE_BASIC_BOOT_TABLE  *pFBPT;
  UINT32                                       FBPTLength;
  EFI_STATUS                                   Status;
  UINTN                                        FBPTAddress;
  EFI_SMBIOS_HANDLE                            SmbiosHandle;
  EFI_SMBIOS_PROTOCOL                          *Smbios;
  SMBIOS_TABLE_TYPE0                           *Type0Record;
  SMBIOS_TABLE_TYPE1                           *Type1Record;
  UINT8                                        StrIndex;
  EFI_SMBIOS_TABLE_HEADER                      *Record;
  CHAR16                                       *UefiVersion;
  CHAR16                                       *Model;
  CHAR16                                       FileName[MAX_STRING_SIZE];

  UefiVersion = NULL;
  Model       = NULL;

  //
  // We'll use SMBIOS protocol to locate SMBIOS strings that have UEFI ver. and model name
  //
  Status = gBS->LocateProtocol (
                  &gEfiSmbiosProtocolGuid,
                  NULL,
                  (VOID **)&Smbios
                  );

  if (!EFI_ERROR (Status)) {
    SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
    Status       = Smbios->GetNext (Smbios, &SmbiosHandle, NULL, &Record, NULL);
    while (!EFI_ERROR (Status)) {
      if (Record->Type == SMBIOS_TYPE_BIOS_INFORMATION) {
        Type0Record = (SMBIOS_TABLE_TYPE0 *)Record;
        StrIndex    = Type0Record->BiosVersion;
        GetOptionalStringByIndex ((CHAR8 *)((UINT8 *)Type0Record + Type0Record->Hdr.Length), StrIndex, &UefiVersion);
      }

      if (Record->Type == SMBIOS_TYPE_SYSTEM_INFORMATION) {
        Type1Record = (SMBIOS_TABLE_TYPE1 *)Record;
        StrIndex    = Type1Record->ProductName;
        GetOptionalStringByIndex ((CHAR8 *)((UINT8 *)Type1Record + Type1Record->Hdr.Length), StrIndex, &Model);
      }

      Status = Smbios->GetNext (Smbios, &SmbiosHandle, NULL, &Record, NULL);
    }
  }

  //
  // Locate the FPDT.
  //
  Handle = 0;
  Status = LocateAcpiTableBySignature (
             EFI_ACPI_5_0_FIRMWARE_PERFORMANCE_DATA_TABLE_SIGNATURE,
             (EFI_ACPI_DESCRIPTION_HEADER **)&pFPDT,
             &Handle
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a Unable to locate FPDT\n", __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  //
  // Navigate to FBPT
  //
  Status = SafeUint64ToUintn (pFPDT->BootPointerRecord.BootPerformanceTablePointer, &FBPTAddress);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a unsafe truncation of BootPerformanceTablePointer\n", __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  pFBPT      = (EFI_ACPI_5_0_FPDT_FIRMWARE_BASIC_BOOT_TABLE *)FBPTAddress;
  FBPTLength = pFBPT->Header.Length;

  //
  // Compose the filename: FBPT_<Model>_<UefiVer>
  //
  if ((UefiVersion != NULL) && (Model != NULL)) {
    ZeroMem (FileName, sizeof (CHAR16) * MAX_STRING_SIZE);
    UnicodeSPrint (FileName, MAX_STRING_SIZE, L"FBPT_%s_%s", Model, UefiVersion);
  } else {
    UnicodeSPrint (FileName, MAX_STRING_SIZE, L"FBPT");
  }

  //
  // Save FBPT into a file
  //
  WriteBufferToFile (FileName, (VOID *)pFBPT, (UINTN)FBPTLength);

  if (UefiVersion != NULL) {
    FreePool (UefiVersion);
  }

  if (Model != NULL) {
    FreePool (Model);
  }

  return EFI_SUCCESS;
} // FbptDumpEntryPoint()
