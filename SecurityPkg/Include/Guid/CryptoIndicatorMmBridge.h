/** @file
  EFI Crypto Indicator Table (ECIT) MM->DXE bridge definitions.

  The ECIT collector and its registration protocol are DXE constructs, but some
  feature owners perform their cryptographic work in Standalone MM (for example
  authenticated-variable servicing). This header defines the MMI handler GUID
  and comm-buffer layout used to drain the MM side's accumulated records into
  the DXE collector before the table is sealed.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef CRYPTO_INDICATOR_MM_BRIDGE_H_
#define CRYPTO_INDICATOR_MM_BRIDGE_H_

//
// MMI handler GUID the DXE bridge communicates to in order to fetch the MM
// collector's accumulated ECIT records.
// {7b1e2c9a-4f83-4a16-9d5c-3e77210b64a8}
//
#define ECIT_MM_BRIDGE_HANDLER_GUID \
  { 0x7b1e2c9a, 0x4f83, 0x4a16, { 0x9d, 0x5c, 0x3e, 0x77, 0x21, 0x0b, 0x64, 0xa8 } }

#define ECIT_MM_BRIDGE_COMM_SIGNATURE  SIGNATURE_32 ('E', 'C', 'M', 'B')
#define ECIT_MM_BRIDGE_COMM_REVISION   0x00010000

//
// Comm-buffer body, placed at EFI_MM_COMMUNICATE_HEADER.Data. On entry the DXE
// bridge fills Signature/Revision. On return the MM handler fills ReturnStatus,
// EntryCount, and EntriesSize, followed by EntriesSize bytes of packed entries.
//
#pragma pack (1)
typedef struct {
  UINT32    Signature;       ///< ECIT_MM_BRIDGE_COMM_SIGNATURE.
  UINT32    Revision;        ///< ECIT_MM_BRIDGE_COMM_REVISION.
  UINT64    ReturnStatus;    ///< EFI_STATUS from the MM handler (as UINT64).
  UINT32    EntryCount;      ///< Number of packed records that follow.
  UINT32    EntriesSize;     ///< Total bytes of the packed records that follow.
  // Followed by EntryCount packed ECIT_MM_BRIDGE_ENTRY records.
} ECIT_MM_BRIDGE_COMM;

//
// One serialized record: fixed header immediately followed by DataSize payload
// bytes, then the next record (packed, no alignment padding).
//
typedef struct {
  EFI_GUID    FeatureIdentifier;   ///< ECIT feature GUID.
  UINT32      DataSize;            ///< Payload size in bytes (payload follows).
} ECIT_MM_BRIDGE_ENTRY;
#pragma pack ()

extern EFI_GUID  gEcitMmBridgeHandlerGuid;

#endif // CRYPTO_INDICATOR_MM_BRIDGE_H_
