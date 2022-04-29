/** @file
  This file defines the EFI Media Sanitize Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_MEDIA_SANITIZE_PROTOCOL_H__
#define __EFI_MEDIA_SANITIZE_PROTOCOL_H__

#define EFI_MEDIA_SANITIZE_PROTOCOL_GUID \
  { \
    0x0d799a99, 0x25af, 0x429e, { 0x92, 0x72, 0xd0, 0xb2, 0x7d, 0x6d, 0x5f, 0x14 } \
  }

typedef struct _EFI_MEDIA_SANITIZE_PROTOCOL EFI_MEDIA_SANITIZE_PROTOCOL;

#define EFI_MEDIA_SANITIZE_PROTOCOL_REVISION  0x00010000

///
/// Sanitize actions for purge operation.
///
/// NOTE: First four actions (no action, overwrite, block erase, crypto erase) cannot
/// be overlapped. All other fields may be overlapped as they apply. 
///
#define PURGE_ACTION_NO_ACTION                        0x00000000 // No purge action requested
#define PURGE_ACTION_OVERWRITE                        0x00000001 // Overwrite with 32-bit pattern
#define PURGE_ACTION_BLOCK_ERASE                      0x00000002 // Erase Blocks with indeterminate pattern
#define PURGE_ACTION_CRYPTO_ERASE                     0x00000004 // Delete encryption keys only
#define PURGE_ACTION_RESET_REQUIRED                   0x00000008 // Reset required after purge
#define PURGE_ACTION_NO_DEALLOCATE                    0x00000010 // Do no deallocate (trim) flash medai after sanitize
#define PURGE_ACTION_INVERT_OW_PATTERN                0x00000020 // Invert overwrite pattern between passes
#define PURGE_ACTION_ALLOW_UNRESTRICTED_SANITIZE_EXIT 0x00000040 // Allow exit without restrictions

/**
  Clear Media utilizes transport native WRITE commands to write a fixed pattern
  of non-sensitive data. The size of the overwrite buffer shall be equal to the
  one sector/LBA (in bytes).

  @param[in]       This           Indicates a pointer to the calling context.
  @param[in]       MediaId        The media ID that the clear request is for.
  @param[in]       PassCount      Number of passes to write over the media.
  @param[in]       SectorOwBuffer Pointer to overwrite pattern buffer.

  @retval EFI_SUCCESS             The sanitize request was queued if Event is
                                  not NULL. The data was erased correctly to the
                                  device if the Event is NULL.to the device.
  @retval EFI_WRITE_PROTECTED     The device can't be sanitized due to write
                                  protection.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting
                                  to perform the sanitize operation.
  @retval EFI_INVALID_PARAMETER   The clear request contains parameters that
                                  are not valid.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_MEDIA_CHANGED       The MediaId is not for the current media.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_BLOCK_MEDIA_CLEAR)(
  IN     EFI_MEDIA_SANITIZE_PROTOCOL   *This,
  IN     UINT32                        MediaId,
  IN     UINT32                        PassCount,
  IN     VOID                          *SectorOwBuffer
  );

/**
  Purge Media utilizes native Sanitize operations. Transport specific
  overwrite, block erase, or crypto erase functions shall be invoked based
  on transport.

  @param[in] This             Indicates a pointer to the calling context.
  @param[in] MediaId          The media ID that the clear request is for.
  @param[in] PurgeAction      Sanitize action: overwrite, crypto or block erase.
  @param[in] OverwritePattern 32-bit pattern to overwrite on media.

  @retval EFI_SUCCESS             The sanitize request was queued if Event is
                                  not NULL. The data was erased correctly to the
                                  device if the Event is NULL.to the device.
  @retval EFI_WRITE_PROTECTED     The device can't be sanitized due to write
                                  protection.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting
                                  to perform the sanitize operation.
  @retval EFI_INVALID_PARAMETER   The clear request contains parameters that
                                  are not valid.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_MEDIA_CHANGED       The MediaId is not for the current media.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_BLOCK_MEDIA_PURGE)(
  IN     EFI_MEDIA_SANITIZE_PROTOCOL   *This,
  IN     UINT32                        MediaId,
  IN     UINT32                        PurgeAction,
  IN     UINT32                        OverwritePattern
  );

///
/// The EFI Media Sanitize Protocol provides the ability for a device to expose
/// sanitize functionality. This optional protocol is installed on the same handle
/// as the EFI_BLOCK_IO_PROTOCOL or EFI_BLOCK_IO2_PROTOCOL.
///
struct _EFI_MEDIA_SANITIZE_PROTOCOL {
  ///
  /// The revision to which the EFI_MEDIA_SANITIZE_PROTOCOL adheres. All future
  /// revisions must be backwards compatible. If a future version is not
  /// backwards compatible, it is not the same GUID.
  ///
  UINT64                       Revision;

  ///
  /// A pointer to the EFI_BLOCK_IO_MEDIA data for this device.
  /// Type EFI_BLOCK_IO_MEDIA is defined in BlockIo.h.
  ///
  EFI_BLOCK_IO_MEDIA           *Media;

  ///
  /// Sanitize action shall be specific to the sanitize operation, such as
  /// deallocate after sanitize, number of passes, reset required, etc.
  ///
  //UINT32                       SanitizeAction;
  // TBD: Look into Sanitize Capabilites on this block/media for purge action
  // Get rid of Sanitize Action as a param

  EFI_BLOCK_MEDIA_CLEAR        MediaClear;
  EFI_BLOCK_MEDIA_PURGE        MediaPurge;
};

extern EFI_GUID  gEfiMediaSanitizeProtocolGuid;

#endif
