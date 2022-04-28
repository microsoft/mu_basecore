/** @file
  Header file for EFI_MEDIA_SANITIZE_PROTOCOL interface.

Copyright (c) 2013 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_NVME_MEDIA_SANITIZE_H_
#define _EFI_NVME_MEDIA_SANITIZE_H_

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

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

 **/
EFI_STATUS
NvmExpressFormatNvm (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 NamespaceId,
  IN UINT32                 Ses,
  IN UINT32                 Flbas
  );

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

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

 **/
EFI_STATUS
NvmExpressSanitize (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 NamespaceId,
  IN UINT32                 SanitizeAction,
  IN UINT32                 NoDeallocAfterSanitize,
  IN UINT32                 OverwritePattern
  );

/**
  Clear Media utilizes transport native WRITE commands to write a fixed pattern
  of non-sensitive data to the media.

  NOTE: The caller shall send buffer of one sector/LBA size with overwrite data.
  NOTE: This operation is a blocking call.

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
  IN EFI_MEDIA_SANITIZE_PROTOCOL  *This,
  IN UINT32                       MediaId,
  IN UINT32                       PassCount,
  IN VOID                         *SectorOwBuffer
  );

/**
  Purge Media utilizes transport native Sanitize operations. Sanitize specific
  purge actions include: overwrite, block erase, or crypto erase.

  Functions are defined to erase and purge data at a block level from mass
  storage devices as well as to manage such devices in the EFI boot services
  environment. Sanitization refers to a process that renders access to target
  data on the media infeasible for a given level of effort.

  NOTE: This operation is a blocking call.

  @param  This             Indicates a pointer to the calling context.
  @param  MediaId          The media ID that the write request is for.
  @param  PurgeAction      The purage action (overwrite, crypto erase, block erase).
  @param  OverwritePattern 32-bit pattern to overwrite on media (for overwrite).

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
NvmExpressMediaPurge (
  IN EFI_MEDIA_SANITIZE_PROTOCOL  *This,
  IN UINT32                       MediaId,
  IN UINT32                       PurgeAction,
  IN UINT32                       OverwritePattern
  );

#endif
