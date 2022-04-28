/** @file
  This file defines the EFI Media Sanitize Protocol.

  Copyright (c) 2016, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  This Protocol is introduced in UEFI Specification x.x

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
/// EFI_MEDIA_SANITIZE_TOKEN
///
typedef struct {
  //
  // If Event is NULL, then blocking I/O is performed. If Event is not NULL and
  // non-blocking I/O is supported, then non-blocking I/O is performed, and
  // Event will be signaled when the erase request is completed.
  //
  EFI_EVENT     Event;
  //
  // Defines whether the signaled event encountered an error.
  //
  EFI_STATUS    TransactionStatus;
} EFI_MEDIA_SANITIZE_TOKEN;

///
/// EFI_MEDIA_SANITIZE_ACTION
///
typedef enum {
  EfiBlockSanitizeNoAction      = 0x00000001,
  EfiBlockSanitizeOverwrite     = 0x00000002,
  EfiBlockSanitizeBlockErase    = 0x00000004,
  EfiBlockSanitizeCryptoErase   = 0x00000008,
  EfiBlockSanitizeResetRequired = 0x00000010,
  EfiBlockSanitizeNoDeallocate  = 0x00000020
} EFI_BLOCK_SANITIZE_ACTION;

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
  EFI_BLOCK_SANITIZE_ACTION    SanitizeAction;

  EFI_BLOCK_MEDIA_CLEAR        MediaClear;
  EFI_BLOCK_MEDIA_PURGE        MediaPurge;
};

extern EFI_GUID  gEfiMediaSanitizeProtocolGuid;

#endif
