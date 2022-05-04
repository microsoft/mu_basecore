/** @file -- NvmExpressMediaSanitize.c
  This driver will implement sanitize operations on all NVMe mass storage devices
  based on NIST purge and clear operations. These operations will then be mapped to
  one of two NVMe admin commands:

  -Format NVM
  -Sanitize

  Implementation based off NVMe spec revision 1.4c.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "NvmExpress.h"

/**
  Send NVM Express FormatNVM Admin Command

  The Format NVM command is used to low level format the NVM media. This command is used by
  the host to change the LBA data size and/or metadata size.

  A low level format may destroy all data and metadata associated with all namespaces or only
  the specific namespace associated with the command (refer to the Format NVM Attributes field
  in the Identify Controller data structure).

  After the Format NVM command successfully completes, the controller shall not return any user
  data that was previously contained in an affected namespace.

  @param[in] This            Indicates a pointer to the calling context (Block IO Protocol)
  @param[in] NamespaceId     The NVM Express namespace ID  for which a device path node is to be
                             allocated and built. Caller must set the NamespaceId to zero if the
                             device path node will contain a valid UUID.
  @param[in] Ses             Secure Erase Setting (SES) value
                               - 000b: No secure erase operation requested
                               - 001b: User Data Erase
                               - 010b: Cryptographic Erase
                               - 011b to 111b: Reserved
  @param[in] Flbas           Current LBA Format size Index (bits 3:0) in NamespaceData

  @retval EFI_SUCCESS           The device formatted correctly.
  @retval EFI_WRITE_PROTECTED   The device can not be formatted due to write protection.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the format.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_INVALID_PARAMETER The format request contains parameters that are not valid.

 **/
EFI_STATUS
NvmExpressFormatNvm (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 NamespaceId,
  IN UINT32                 Ses,
  IN UINT32                 Flbas
  )
{
  NVME_DEVICE_PRIVATE_DATA                  *Device;
  EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET  CommandPacket;
  EFI_NVM_EXPRESS_COMMAND                   Command;
  EFI_NVM_EXPRESS_COMPLETION                Completion;
  NVME_ADMIN_FORMAT_NVM                     FormatNvmCdw10;
  UINT32                                    LbaFormat = 0;
  EFI_STATUS                                Status    = EFI_NOT_STARTED;

  Device = NVME_DEVICE_PRIVATE_DATA_FROM_BLOCK_IO (This);

  ZeroMem (&CommandPacket, sizeof (EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof (EFI_NVM_EXPRESS_COMMAND));
  ZeroMem (&Completion, sizeof (EFI_NVM_EXPRESS_COMPLETION));
  ZeroMem (&FormatNvmCdw10, sizeof (NVME_ADMIN_FORMAT_NVM));

  CommandPacket.NvmeCmd        = &Command;
  CommandPacket.NvmeCompletion = &Completion;
  CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
  CommandPacket.QueueType      = NVME_ADMIN_QUEUE;
  Command.Cdw0.Opcode          = NVME_ADMIN_FORMAT_NVM_CMD;
  Command.Nsid                 = NamespaceId;

  DEBUG ((
    DEBUG_VERBOSE,
    "NvmExpressFormatNvm: NSID = 0x%x, SES = 0x%x, FLBAS = 0x%x\n",
    NamespaceId,
    Ses,
    Flbas
    ));

  //
  // SES (Secure Erase Settings)
  //
  FormatNvmCdw10.Ses = Ses;

  //
  // Change LBA size/format if LbaFormat param != NULL, otherwise keep same LBA format.
  // Current supported LBA format size in Identify Namespace LBA Format Table, indexed by
  // FLBAS (bits 3:0).
  //
  LbaFormat           = (!LbaFormat ? Device->NamespaceData.Flbas : Flbas);
  FormatNvmCdw10.Lbaf = LbaFormat & NVME_LBA_FORMATNVM_LBAF_MASK;
  CopyMem (&CommandPacket.NvmeCmd->Cdw10, &FormatNvmCdw10, sizeof (NVME_ADMIN_FORMAT_NVM));

  //
  // Send Format NVM command via passthru and wait for completion
  //
  // If LBA size changed successfully, then update private data structures and Block IO
  // and Media protocols to reflect new LBA size.
  //
  Status = Device->Controller->Passthru.PassThru (
                                          &(Device->Controller->Passthru),
                                          NamespaceId,
                                          &CommandPacket,
                                          NULL
                                          );

  if (EFI_ERROR (Status)) {
    UINT16  StatusField = 0;
    UINT16  Sct         = 0;
    UINT16  Sc          = 0;

    StatusField = (UINT16)((CommandPacket.NvmeCompletion->DW3 & NVME_CQE_STATUS_FIELD_MASK) >>
                           NVME_CQE_STATUS_FIELD_OFFSET);

    Sc  = (StatusField & NVME_CQE_STATUS_FIELD_SC_MASK) >> NVME_CQE_STATUS_FIELD_SC_OFFSET;
    Sct = (StatusField & NVME_CQE_STATUS_FIELD_SCT_MASK) >> NVME_CQE_STATUS_FIELD_SCT_OFFSET;

    DEBUG ((DEBUG_ERROR, "ERROR %a - %r\n", __FUNCTION__, Status));
    DEBUG ((DEBUG_ERROR, "SCT = 0x%x, SC = 0x%x\n", Sct, Sc));
  } else {
    //
    // Update Block IO and Media Protocols only if Flbas parameter was not NULL.
    // Call Identify Namespace again and update all protocols fields and local
    // cached copies of fields related to block size.
    //
    if (Flbas) {
      NVME_ADMIN_NAMESPACE_DATA  *NewNamespaceData;
      UINT32                     Lbads;
      UINT32                     NewFlbas;
      UINT32                     LbaFmtIdx;

      NewNamespaceData = AllocateZeroPool (sizeof (NVME_ADMIN_NAMESPACE_DATA));
      if (NewNamespaceData == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
      } else {
        Status = NvmeIdentifyNamespace (
                   Device->Controller,
                   NamespaceId,
                   (VOID *)NewNamespaceData
                   );

        if (!EFI_ERROR (Status)) {
          //
          // Update all fields related to LBA size, allocation, and alignment
          //
          NewFlbas                = NewNamespaceData->Flbas;
          LbaFmtIdx               = NewFlbas & NVME_LBA_FORMATNVM_LBAF_MASK;
          Lbads                   = NewNamespaceData->LbaFormat[LbaFmtIdx].Lbads;
          Device->Media.BlockSize = (UINT32)1 << Lbads;
          Device->Media.LastBlock = NewNamespaceData->Nsze - 1;

          CopyMem (&Device->NamespaceData, NewNamespaceData, sizeof (NVME_ADMIN_NAMESPACE_DATA));
        }
      }
    }
  }

  return Status;
}

/**
  Send NVM Express Sanitize Admin Command

  The Sanitize command is used to start a sanitize operation or to recover from a previously
  failed sanitize operation. The sanitize operation types that may be supported are Block
  Erase, Crypto Erase, and Overwrite.

  All sanitize operations are processed in the background (i.e., completion of the Sanitize
  command does not indicate completion of the sanitize operation).

  @param[in] This                Indicates a pointer to the calling context (Block IO Protocol)
  @param[in] NamespaceId         The NVM Express namespace ID  for which a device path node is to be
                                 allocated and built. Caller must set the NamespaceId to zero if the
                                 device path node will contain a valid UUID.
  @param[in] SanitizeAction      Sanitize action
  @param[in] NoDeallocAfterSani  No deallocate after sanitize option
  @param[in] OverwritePatter     Pattern to overwrite old user data

  @retval EFI_SUCCESS           The media was sanitized successfully on the device.
  @retval EFI_WRITE_PROTECTED   The device can not be sanitized due to write protection.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the sanitize.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not match the current device.
  @retval EFI_INVALID_PARAMETER The sanitize request contains parameters that are not valid.

 **/
EFI_STATUS
NvmExpressSanitize (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 NamespaceId,
  IN UINT32                 SanitizeAction,
  IN UINT32                 NoDeallocAfterSanitize,
  IN UINT32                 OverwritePattern
  )
{
  NVME_DEVICE_PRIVATE_DATA                  *Device;
  EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET  CommandPacket;
  EFI_NVM_EXPRESS_COMMAND                   Command;
  EFI_NVM_EXPRESS_COMPLETION                Completion;
  NVME_ADMIN_SANITIZE                       SanitizeCdw10Cdw11;
  EFI_STATUS                                Status;

  Device = NVME_DEVICE_PRIVATE_DATA_FROM_BLOCK_IO (This);

  ZeroMem (&CommandPacket, sizeof (EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof (EFI_NVM_EXPRESS_COMMAND));
  ZeroMem (&Completion, sizeof (EFI_NVM_EXPRESS_COMPLETION));
  ZeroMem (&SanitizeCdw10Cdw11, sizeof (NVME_ADMIN_SANITIZE));

  CommandPacket.NvmeCmd        = &Command;
  CommandPacket.NvmeCompletion = &Completion;
  CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
  CommandPacket.QueueType      = NVME_ADMIN_QUEUE;
  Command.Cdw0.Opcode          = NVME_ADMIN_SANITIZE_CMD;
  Command.Nsid                 = NamespaceId;

  DEBUG ((
    DEBUG_VERBOSE,
    "NvmExpressSanitize NSID = 0x%x, SANACT = 0x%x, NDAS = 0x%x\n",
    NamespaceId,
    SanitizeAction,
    NoDeallocAfterSanitize
    ));

  SanitizeCdw10Cdw11.Ndas   = NoDeallocAfterSanitize;
  SanitizeCdw10Cdw11.Sanac  = SanitizeAction;
  SanitizeCdw10Cdw11.Ovrpat = OverwritePattern;
  CopyMem (&CommandPacket.NvmeCmd->Cdw10, &SanitizeCdw10Cdw11, sizeof (NVME_ADMIN_SANITIZE));

  //
  // Send Format NVM command via passthru and wait for completion
  //
  Status = Device->Controller->Passthru.PassThru (
                                          &(Device->Controller->Passthru),
                                          NamespaceId,
                                          &CommandPacket,
                                          NULL
                                          );

  if (EFI_ERROR (Status)) {
    UINT16  StatusField = 0;
    UINT16  Sct         = 0;
    UINT16  Sc          = 0;

    StatusField = (UINT16)((CommandPacket.NvmeCompletion->DW3 & NVME_CQE_STATUS_FIELD_MASK) >>
                           NVME_CQE_STATUS_FIELD_OFFSET);

    Sc  = (StatusField & NVME_CQE_STATUS_FIELD_SC_MASK) >> NVME_CQE_STATUS_FIELD_SC_OFFSET;
    Sct = (StatusField & NVME_CQE_STATUS_FIELD_SCT_MASK) >> NVME_CQE_STATUS_FIELD_SCT_OFFSET;

    DEBUG ((DEBUG_ERROR, "ERROR %a - %r\n", __FUNCTION__, Status));
    DEBUG ((DEBUG_ERROR, "SCT = 0x%x, SC = 0x%x\n", Sct, Sc));

    //
    // Check for an error status code of "Invalid Command Opcode" in case
    // the NVM Express controller does not support Sanitize. If the NVM
    // Exress Controller does not support Sanitize, then send a Format NVM
    // admin command instead to perform the Purge operation.
    if ((Sct == NVME_CQE_SCT_GENERIC_CMD_STATUS) &&
        (Sc == NVME_CQE_SC_INVALID_CMD_OPCODE))
    {
      UINT32  FnvmSes = 0;

      switch (SanitizeCdw10Cdw11.Sanac) {
        case SANITIZE_ACTION_BLOCK_ERASE:
          FnvmSes = SES_USER_DATA_ERASE; // User Data Erase (LBAs indeterminate after)
          break;
        case SANITIZE_ACTION_CRYPTO_ERASE:
          FnvmSes = SES_CRYPTO_ERASE; // Crypto Erase
          break;
        case SANITIZE_ACTION_OVERWRITE:
        case SANITIZE_ACTION_EXIT_FAILURE_MODE:
        default:
          //
          // Cannot perform an equivalent FormatNVM action/operation
          //
          FnvmSes = SES_NO_SECURE_ERASE;
          break;
      }

      if ((FnvmSes == SES_USER_DATA_ERASE) || (FnvmSes == SES_CRYPTO_ERASE)) {
        Status = NvmExpressFormatNvm (
                   This,
                   NVME_ALL_NAMESPACES,
                   FnvmSes,
                   Device->NamespaceData.Flbas
                   );
      }
    }
  }

  return Status;
}

/**
  Clear Media utilizes transport native WRITE commands to write a fixed pattern
  of non-sensitive data to the media.

  NOTE: The caller shall send buffer of one sector/LBA size with overwrite data.
  NOTE: This operation is a blocking call.
  NOTE: This function must be called from TPL aaplication or callback.

  Functions are defined to erase and purge data at a block level from mass
  storage devices as well as to manage such devices in the EFI boot services
  environment.

  @param  This             Indicates a pointer to the calling context.
  @param  MediaId          The media ID that the write request is for.
  @param  PassCount        The number of passes to write over media.
  @param  SectorOwBuffer   A pointer to the overwrite buffer.

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
NvmExpressMediaClear (
  IN MEDIA_SANITIZE_PROTOCOL  *This,
  IN UINT32                   MediaId,
  IN UINT32                   PassCount,
  IN VOID                     *SectorOwBuffer
  )
{
  NVME_DEVICE_PRIVATE_DATA  *Device;
  EFI_BLOCK_IO_MEDIA        *Media;
  EFI_LBA                   SectorOffset;
  UINT32                    TotalPassCount;
  EFI_STATUS                Status;

  //
  // Check parameters.
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Device       = NVME_DEVICE_PRIVATE_DATA_FROM_MEDIA_SANITIZE (This);
  Media        = &Device->Media;
  SectorOffset = 0;

  if ((MediaId != Media->MediaId) || (!Media->MediaPresent)) {
    return EFI_MEDIA_CHANGED;
  }

  //
  // If an invalid buffer or buffer size is sent, the Media Clear operation
  // cannot be performed as it requires a native WRITE command. The overwrite
  // buffer must have granularity of a namespace block size.
  //
  if (SectorOwBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Per NIST 800-88r1, one or more pass of writes may be alteratively used.
  //
  for (TotalPassCount = 0; TotalPassCount < PassCount; TotalPassCount++) {
    for (SectorOffset = 0; SectorOffset < Media->LastBlock; SectorOffset++ ) {
      Status = Device->BlockIo.WriteBlocks (
                                 &Device->BlockIo,
                                 MediaId,
                                 SectorOffset,  // Sector/LBA offset (increment each pass)
                                 1,             // Write one sector per write
                                 SectorOwBuffer // overwrite buffer
                                 );
    }

    //
    // Reset SectorOffset back to zero if another pass on namespace is needed
    //
    SectorOffset = 0;
  }

  return Status;
}

/**
  Purge Media utilizes transport native Sanitize operations. Sanitize specific
  purge actions include: overwrite, block erase, or crypto erase.

  Functions are defined to erase and purge data at a block level from mass
  storage devices as well as to manage such devices in the EFI boot services
  environment. Sanitization refers to a process that renders access to target
  data on the media infeasible for a given level of effort.

  NOTE: This operation is a blocking call.
  NOTE: This function must be called from TPL aaplication or callback.

  @param  This             Indicates a pointer to the calling context.
  @param  MediaId          The media ID that the write request is for.
  @param  PurgeAction      The purage action (overwrite, crypto erase, block erase).
  @param  OverwritePattern 32-bit pattern to overwrite on media (for overwrite).

  @retval EFI_SUCCESS           The media was purged successfully on the device.
  @retval EFI_WRITE_PROTECTED   The device can not be purged due to write protection.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the purge.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not match the current device.
  @retval EFI_INVALID_PARAMETER The purge request contains parameters that are not valid.

**/
EFI_STATUS
EFIAPI
NvmExpressMediaPurge (
  IN MEDIA_SANITIZE_PROTOCOL  *This,
  IN UINT32                   MediaId,
  IN UINT32                   PurgeAction,
  IN UINT32                   OverwritePattern
  )
{
  NVME_DEVICE_PRIVATE_DATA  *Device;
  EFI_BLOCK_IO_MEDIA        *Media;
  NVME_SANICAP              SaniCap;
  UINT32                    SanitizeAction;
  UINT32                    NoDeallocate;
  UINT32                    NamespaceId;
  EFI_STATUS                Status;

  //
  // Check parameters.
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Device      = NVME_DEVICE_PRIVATE_DATA_FROM_MEDIA_SANITIZE (This);
  NamespaceId = Device->NamespaceId;
  Media       = &Device->Media;
  SaniCap     = Device->Controller->ControllerData->Sanicap;

  if ((MediaId != Media->MediaId) || (!Media->MediaPresent)) {
    return EFI_MEDIA_CHANGED;
  }

  //
  // Purge action will directly map to sanitize action. If no valid purge
  // action is selected, then default to no action and let the NVMe SSD handle
  // the no-op sanitize action (as there may be other contingencies).
  //
  if ((PurgeAction & PURGE_ACTION_OVERWRITE) && (SaniCap.Ows)) {
    SanitizeAction = SANITIZE_ACTION_OVERWRITE;
  } else if ((PurgeAction & PURGE_ACTION_BLOCK_ERASE) && (SaniCap.Bes)) {
    SanitizeAction = SANITIZE_ACTION_BLOCK_ERASE;
  } else if ((PurgeAction & PURGE_ACTION_CRYPTO_ERASE) && (SaniCap.Ces)) {
    SanitizeAction = SANITIZE_ACTION_CRYPTO_ERASE;
  } else {
    SanitizeAction = SANITIZE_ACTION_NO_ACTION;
  }

  if (PurgeAction & PURGE_ACTION_NO_DEALLOCATE) {
    NoDeallocate = NVME_NO_DEALLOCATE_AFTER_SANITZE;
  }

  //
  // Call NVM Express Admin command Sanitize (blocking call).
  //
  Status = NvmExpressSanitize (
             &Device->BlockIo,
             NamespaceId,
             SanitizeAction,
             NoDeallocate,
             OverwritePattern
             );

  return Status;
}
