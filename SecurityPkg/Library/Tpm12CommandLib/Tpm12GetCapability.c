/** @file
  Implement TPM1.2 Get Capabilities related commands.

Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/Tpm12CommandLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/Tpm12DeviceLib.h>

#pragma pack(1)

typedef struct {
  TPM_RQU_COMMAND_HDR  Hdr;
  UINT32               Capability;
  UINT32               CapabilityFlagSize;
  UINT32               CapabilityFlag;
} TPM_CMD_GET_CAPABILITY;

typedef struct {
  TPM_RSP_COMMAND_HDR  Hdr;
  UINT32               ResponseSize;
  TPM_PERMANENT_FLAGS  Flags;
} TPM_RSP_GET_CAPABILITY_PERMANENT_FLAGS;

typedef struct {
  TPM_RSP_COMMAND_HDR  Hdr;
  UINT32               ResponseSize;
  TPM_STCLEAR_FLAGS    Flags;
} TPM_RSP_GET_CAPABILITY_STCLEAR_FLAGS;

// MS_CHANGE [BEGIN] - Support retrieving raw capability info from TPM 1.2.
typedef struct {
  TPM_STRUCTURE_TAG                 tag;
  TPM_VERSION                       version;
  UINT16                            specLevel;
  UINT8                             errataRev;
  UINT8                             tpmVendorID[4];
  UINT16                            vendorSpecificSize;
  // The buffer must be provided immediately after this structure.
  // UINT8                             vendorSpecific[];
} TPM_RSP_CAP_VERSION_INFO;

typedef struct {
  TPM_RSP_COMMAND_HDR         Hdr;
  UINT32                      ResponseSize;
  TPM_RSP_CAP_VERSION_INFO    Info;
  // Allocate a buffer sufficiently large for most vendor data.
  UINT8                       VendorSpecificData[1024];
} TPM_RSP_GET_CAPABILITY_VERSION_INFO;
// MS_CHANGE [END]

#pragma pack()

/**
Get TPM capability permanent flags.

@param[out] TpmPermanentFlags   Pointer to the buffer for returned flag structure.

@retval EFI_SUCCESS           Operation completed successfully.
@retval EFI_TIMEOUT           The register can't run into the expected status in time.
@retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
@retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
EFIAPI
Tpm12GetCapabilityFlagPermanent (
  OUT TPM_PERMANENT_FLAGS  *TpmPermanentFlags
  )
{
  EFI_STATUS                              Status;
  TPM_CMD_GET_CAPABILITY                  Command;
  TPM_RSP_GET_CAPABILITY_PERMANENT_FLAGS  Response;
  UINT32                                  Length;

  //
  // send Tpm command TPM_ORD_GetCapability
  //
  Command.Hdr.tag            = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  Command.Hdr.paramSize      = SwapBytes32 (sizeof (Command));
  Command.Hdr.ordinal        = SwapBytes32 (TPM_ORD_GetCapability);
  Command.Capability         = SwapBytes32 (TPM_CAP_FLAG);
  Command.CapabilityFlagSize = SwapBytes32 (sizeof (TPM_CAP_FLAG_PERMANENT));
  Command.CapabilityFlag     = SwapBytes32 (TPM_CAP_FLAG_PERMANENT);
  Length = sizeof (Response);
  Status = Tpm12SubmitCommand (sizeof (Command), (UINT8 *)&Command, &Length, (UINT8 *)&Response);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (SwapBytes32 (Response.Hdr.returnCode) != TPM_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Tpm12GetCapabilityFlagPermanent: Response Code error! 0x%08x\r\n", SwapBytes32 (Response.Hdr.returnCode)));
    return EFI_DEVICE_ERROR;
  }

  ZeroMem (TpmPermanentFlags, sizeof (*TpmPermanentFlags));
  CopyMem (TpmPermanentFlags, &Response.Flags, MIN (sizeof (*TpmPermanentFlags), SwapBytes32(Response.ResponseSize)));

  return Status;
}

/**
Get TPM capability volatile flags.

@param[out] VolatileFlags   Pointer to the buffer for returned flag structure.

@retval EFI_SUCCESS      Operation completed successfully.
@retval EFI_DEVICE_ERROR The command was unsuccessful.

**/
EFI_STATUS
EFIAPI
Tpm12GetCapabilityFlagVolatile (
  OUT TPM_STCLEAR_FLAGS  *VolatileFlags
  )
{
  EFI_STATUS                            Status;
  TPM_CMD_GET_CAPABILITY                Command;
  TPM_RSP_GET_CAPABILITY_STCLEAR_FLAGS  Response;
  UINT32                                Length;

  //
  // send Tpm command TPM_ORD_GetCapability
  //
  Command.Hdr.tag            = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  Command.Hdr.paramSize      = SwapBytes32 (sizeof (Command));
  Command.Hdr.ordinal        = SwapBytes32 (TPM_ORD_GetCapability);
  Command.Capability         = SwapBytes32 (TPM_CAP_FLAG);
  Command.CapabilityFlagSize = SwapBytes32 (sizeof (TPM_CAP_FLAG_VOLATILE));
  Command.CapabilityFlag     = SwapBytes32 (TPM_CAP_FLAG_VOLATILE);
  Length = sizeof (Response);
  Status = Tpm12SubmitCommand (sizeof (Command), (UINT8 *)&Command, &Length, (UINT8 *)&Response);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (SwapBytes32 (Response.Hdr.returnCode) != TPM_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Tpm12GetCapabilityFlagVolatile: Response Code error! 0x%08x\r\n", SwapBytes32 (Response.Hdr.returnCode)));
    return EFI_DEVICE_ERROR;
  }

  ZeroMem (VolatileFlags, sizeof (*VolatileFlags));
  CopyMem (VolatileFlags, &Response.Flags, MIN (sizeof (*VolatileFlags), SwapBytes32(Response.ResponseSize)));

  return Status;
}

// MS_CHANGE [BEGIN] - Support retrieving raw capability info from TPM 1.2.
/**
Get TPM version from capability.

@param[out] TpmVersion    A pointer to a TPM_VERSION structure to hold the base version info.
@param[out] TpmMfg        [Optional] A UINT32 that will contain a big-endian encoding of the 4-byte mfg ID.
                          (eg. for "TPM ", TpmMfg[0] == 'T', TpmMfg[1] == 'P', etc)
@param[in,out]  VendorSpecificSize  [Optional] On input, the size of the buffer pointed to by VendorSpecificBuffer.
                                    On output, the number of bytes copied into the buffer.
@param[out]     VendorSpecificBuffer  [Optional] A caller-allocated buffer containing the vendor-specific data.

@retval EFI_SUCCESS      Operation completed successfully.
@retval EFI_DEVICE_ERROR The command was unsuccessful.

**/
EFI_STATUS
EFIAPI
Tpm12GetCapabilityFirmwareVersion (
  OUT     TPM_VERSION   *TpmVersion,
  OUT     UINT32        *TpmMfg OPTIONAL,
  IN OUT  UINT16        *VendorSpecificSize OPTIONAL,
  OUT     UINT8         *VendorSpecificBuffer OPTIONAL
  )
{
  EFI_STATUS                            Status;
  TPM_CMD_GET_CAPABILITY                Command;
  TPM_RSP_GET_CAPABILITY_VERSION_INFO   Response;   // This structure is 1KB+
  UINT32                                Length;

  //
  // send Tpm command TPM_ORD_GetCapability
  //
  Command.Hdr.tag            = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  Command.Hdr.paramSize      = SwapBytes32 (OFFSET_OF(TPM_CMD_GET_CAPABILITY, CapabilityFlag));
  Command.Hdr.ordinal        = SwapBytes32 (TPM_ORD_GetCapability);
  Command.Capability         = SwapBytes32 (TPM_CAP_VERSION_VAL);
  Command.CapabilityFlagSize = 0;                                   // We won't use the sub-capability.
  Length = sizeof (Response);
  Status = Tpm12SubmitCommand (sizeof (Command), (UINT8 *)&Command, &Length, (UINT8 *)&Response);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (SwapBytes32 (Response.Hdr.returnCode) != TPM_SUCCESS) {
    DEBUG ((DEBUG_VERBOSE, "Tpm12GetCapabilityFirmwareVersion: Response Code error! 0x%08x\r\n", SwapBytes32 (Response.Hdr.returnCode)));
    return EFI_DEVICE_ERROR;
  }

  // Now we know we've got good data.
  // Copy the version data.
  CopyMem (TpmVersion, &Response.Info.version, sizeof (*TpmVersion));
  if (TpmMfg != NULL) {
    CopyMem ((UINT8*)TpmMfg, &Response.Info.tpmVendorID[0], sizeof (*TpmMfg));
  }

  // Now we need to see whether we've got enough buffer to hold the vendor-specific data.
  if (VendorSpecificSize != NULL && VendorSpecificBuffer != NULL) {
    if (*VendorSpecificSize >= Response.Info.vendorSpecificSize) {
      // Yes, we can hold all of the data. Set the size and copy the data.
      *VendorSpecificSize = Response.Info.vendorSpecificSize;
      CopyMem (VendorSpecificBuffer, &Response.VendorSpecificData[0], *VendorSpecificSize);
    }
    else {
      // No, we cannot hold all of the data. Set the size and return the error.
      *VendorSpecificSize = Response.Info.vendorSpecificSize;
      Status = EFI_BUFFER_TOO_SMALL;
    }
  }

  return Status;
}
// MS_CHANGE [END]
