/** @file

A UEFI shell application for testing TPM 2.0 Physical Presence Interface
operations, including querying and configuring PCR banks.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Protocol/Tcg2Protocol.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>

typedef struct {
  UINT32         HashMask;
  CONST CHAR8    *Name;
} HASH_ALG_INFO;

STATIC CONST HASH_ALG_INFO  mHashAlgTable[] = {
  { EFI_TCG2_BOOT_HASH_ALG_SHA1,    "SHA1"    },
  { EFI_TCG2_BOOT_HASH_ALG_SHA256,  "SHA256"  },
  { EFI_TCG2_BOOT_HASH_ALG_SHA384,  "SHA384"  },
  { EFI_TCG2_BOOT_HASH_ALG_SHA512,  "SHA512"  },
  { EFI_TCG2_BOOT_HASH_ALG_SM3_256, "SM3_256" },
};

/**
  Print a bitmask of hash algorithms as human-readable names.

  @param[in] AlgBitmap  Bitmask of HASH_ALG_* values.
**/
STATIC
VOID
PrintAlgorithms (
  IN UINT32  AlgBitmap
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mHashAlgTable); Index++) {
    if ((AlgBitmap & mHashAlgTable[Index].HashMask) != 0) {
      DEBUG ((DEBUG_INFO, " * %a\n", mHashAlgTable[Index].Name));
    }
  }
}

/**
  Query and display TPM PCR bank information via TCG2 Protocol.

  Retrieves the supported and currently active PCR banks from the
  TCG2 Protocol capability structure.

  @param[in] Tcg2Protocol  Pointer to the TCG2 Protocol instance.

  @retval EFI_SUCCESS           Information retrieved and displayed.
  @retval Others                Error querying the protocol.
**/
STATIC
EFI_STATUS
ShowPcrBanks (
  IN EFI_TCG2_PROTOCOL  *Tcg2Protocol
  )
{
  EFI_STATUS                        Status;
  EFI_TCG2_BOOT_SERVICE_CAPABILITY  Capability;

  Capability.Size = sizeof (Capability);
  Status          = Tcg2Protocol->GetCapability (Tcg2Protocol, &Capability);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a GetCapability failed - %r\n", __func__, Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "Supported PCR banks: %x\n", Capability.HashAlgorithmBitmap));
  PrintAlgorithms (Capability.HashAlgorithmBitmap);
  DEBUG ((DEBUG_INFO, "\n"));

  DEBUG ((DEBUG_INFO, "Active PCR banks: %x\n", Capability.ActivePcrBanks));
  PrintAlgorithms (Capability.ActivePcrBanks);

  return EFI_SUCCESS;
}

/**
  Submit a request to set specific PCR banks via TCG2 Protocol.

  This calls TCG2 Protocol SetActivePcrBanks which submits a Physical
  Presence request internally. The change will take effect on the next
  reboot.

  @param[in] Tcg2Protocol  Pointer to the TCG2 Protocol instance.
  @param[in] DesiredBanks  Bitmask of desired PCR banks
                           (EFI_TCG2_BOOT_HASH_ALG_* values).

  @retval EFI_SUCCESS       Request submitted successfully.
  @retval Others            Error submitting the request.
**/
STATIC
EFI_STATUS
RequestSetPcrBanks (
  IN EFI_TCG2_PROTOCOL  *Tcg2Protocol,
  IN UINT32             DesiredBanks
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "Submitting SetActivePcrBanks with parameter %x\n", DesiredBanks));

  Status = Tcg2Protocol->SetActivePcrBanks (Tcg2Protocol, DesiredBanks);

  DEBUG ((DEBUG_INFO, "Status: %r\n", Status));

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SetActivePcrBanks failed.\n"));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "Request submitted. Changes will take effect after reboot.\n"));
  return EFI_SUCCESS;
}

/**
  Submit a request to enable all supported PCR banks via TCG2 Protocol.

  @param[in] Tcg2Protocol  Pointer to the TCG2 Protocol instance.

  @retval EFI_SUCCESS       Request submitted successfully.
  @retval Others            Error submitting the request.
**/
STATIC
EFI_STATUS
RequestLogAllDigests (
  IN EFI_TCG2_PROTOCOL  *Tcg2Protocol
  )
{
  EFI_STATUS                        Status;
  EFI_TCG2_BOOT_SERVICE_CAPABILITY  Capability;

  Capability.Size = sizeof (Capability);
  Status          = Tcg2Protocol->GetCapability (Tcg2Protocol, &Capability);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a GetCapability failed - %r\n", __func__, Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "Requesting all supported PCR banks: %x\n", Capability.HashAlgorithmBitmap));

  Status = Tcg2Protocol->SetActivePcrBanks (Tcg2Protocol, Capability.HashAlgorithmBitmap);

  DEBUG ((DEBUG_INFO, "Status: %r\n", Status));

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SetActivePcrBanks failed.\n"));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "Request submitted. All supported PCR banks will be enabled after reboot.\n"));
  return EFI_SUCCESS;
}

/**
  Print usage information.
**/
STATIC
VOID
PrintUsage (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "TpmTestApp - TPM 2.0 Physical Presence Test Utility\n"));
  DEBUG ((DEBUG_INFO, "\n"));
  DEBUG ((DEBUG_INFO, "Usage:\n"));
  DEBUG ((DEBUG_INFO, "  TpmTestApp help             - Show usage\n"));
  DEBUG ((DEBUG_INFO, "  TpmTestApp info             - Show PCR banks\n"));
  DEBUG ((DEBUG_INFO, "  TpmTestApp setpcr <mask>    - Request PCR bank change (hex bitmask)\n"));
  DEBUG ((DEBUG_INFO, "  TpmTestApp logall           - Enable all supported PCR banks\n"));
  DEBUG ((DEBUG_INFO, "  TpmTestApp lastresponse     - Show last SetActivePcrBanks result\n"));
  DEBUG ((DEBUG_INFO, "\n"));
  DEBUG ((DEBUG_INFO, "PCR bank bitmask values:\n"));
  DEBUG ((DEBUG_INFO, "  0x00000001 = SHA1\n"));
  DEBUG ((DEBUG_INFO, "  0x00000002 = SHA256\n"));
  DEBUG ((DEBUG_INFO, "  0x00000004 = SHA384\n"));
  DEBUG ((DEBUG_INFO, "  0x00000008 = SHA512\n"));
  DEBUG ((DEBUG_INFO, "  0x00000010 = SM3_256\n"));
  DEBUG ((DEBUG_INFO, "\n"));
  DEBUG ((DEBUG_INFO, "Example: TpmTestApp setpcr 0x2   (enable SHA256 only)\n"));
  DEBUG ((DEBUG_INFO, "Example: TpmTestApp setpcr 0x6   (enable SHA256 + SHA384)\n"));
}

/**
  Entry point for the TPM Test UEFI Shell Application.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     Application executed successfully.
  @retval Others          An error occurred.
**/
EFI_STATUS
EFIAPI
TpmTestAppEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS         Status;
  UINTN              Argc;
  LIST_ENTRY         *ParamPackage;
  CONST CHAR16       *Command;
  CONST CHAR16       *PcrMaskStr;
  UINT64             PcrMaskVal;
  EFI_TCG2_PROTOCOL  *Tcg2Protocol;
  UINT32             OperationPresent;
  UINT32             Response;

  // Initialize the Shell library.
  Status = ShellInitialize ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a ShellInitialize failed - %r\n", __func__, Status));
    return Status;
  }

  // Parse command line.
  Status = ShellCommandLineParse (EmptyParamList, &ParamPackage, NULL, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Validate number of input parameters.
  Argc = ShellCommandLineGetCount (ParamPackage);
  if (Argc < 2) {
    DEBUG ((DEBUG_ERROR, "\n[Invalid Usage]\n"));
    DEBUG ((DEBUG_INFO, "  Use 'TpmTestApp help' for usage.\n"));
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  // Acquire the command value.
  Command = ShellCommandLineGetRawValue (ParamPackage, 1);

  // Locate TCG2 Protocol it is required for this test app to function
  Status = gBS->LocateProtocol (&gEfiTcg2ProtocolGuid, NULL, (VOID **)&Tcg2Protocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "TCG2 Protocol not found - %r\n", Status));
    DEBUG ((DEBUG_ERROR, "  TPM 2.0 may not be enabled on this platform.\n"));
    goto Exit;
  }

  // Dispatch based on command.
  if (StrCmp (Command, L"help") == 0) {
    DEBUG ((DEBUG_INFO, "\n[TPM 2.0 Help]\n\n"));
    PrintUsage ();
    goto Exit;
  } else if (StrCmp (Command, L"info") == 0) {
    DEBUG ((DEBUG_INFO, "\n[TPM 2.0 Information]\n\n"));
    ShowPcrBanks (Tcg2Protocol);
  } else if (StrCmp (Command, L"setpcr") == 0) {
    DEBUG ((DEBUG_INFO, "\n[Set PCR Banks]\n\n"));
    if (Argc < 3) {
      DEBUG ((DEBUG_ERROR, "setpcr requires a hex bitmask parameter.\n"));
      DEBUG ((DEBUG_ERROR, "  Example: TpmTestApp setpcr 0x2\n"));
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }

    PcrMaskStr = ShellCommandLineGetRawValue (ParamPackage, 2);
    Status     = ShellConvertStringToUint64 (
                   PcrMaskStr,
                   &PcrMaskVal,
                   TRUE, // Hex
                   FALSE // No prefix required
                   );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Invalid hex value.\n"));
      goto Exit;
    }

    Status = RequestSetPcrBanks (Tcg2Protocol, (UINT32)PcrMaskVal);
  } else if (StrCmp (Command, L"logall") == 0) {
    DEBUG ((DEBUG_INFO, "\n[Enable All PCR Banks]\n\n"));
    Status = RequestLogAllDigests (Tcg2Protocol);
  } else if (StrCmp (Command, L"lastresponse") == 0) {
    DEBUG ((DEBUG_INFO, "\n[Last SetActivePcrBanks Result]\n\n"));
    Status = Tcg2Protocol->GetResultOfSetActivePcrBanks (Tcg2Protocol, &OperationPresent, &Response);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "  GetResultOfSetActivePcrBanks failed - %r\n", Status));
      goto Exit;
    }

    DEBUG ((DEBUG_INFO, "  Operation present: %a\n", OperationPresent ? "YES" : "NO"));
    DEBUG ((DEBUG_INFO, "  Response code:     %u\n", Response));
  } else {
    DEBUG ((DEBUG_INFO, "\n[Invalid Input Command]\n\n"));
    DEBUG ((DEBUG_ERROR, "  Unknown command '%s'\n", Command));
    DEBUG ((DEBUG_INFO, "  Use 'TpmTestApp help' for usage.\n"));
    Status = EFI_INVALID_PARAMETER;
  }

Exit:
  DEBUG ((DEBUG_INFO, "\n"));
  ShellCommandLineFreeVarList (ParamPackage);
  return Status;
}
