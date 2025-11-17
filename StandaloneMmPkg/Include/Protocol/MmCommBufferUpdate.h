/** @file
  MM communication buffer updated protocol is an indicator of the MM communcation
  buffer is done and will carry needed updated buffer location.

  Copyright (c), Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MM_COMMUNICATION_BUFFER_UPDATE_H_
#define MM_COMMUNICATION_BUFFER_UPDATE_H_

#include <Guid/MmCommBuffer.h>

#define MM_COMMUNICATE_BUFFER_UPDATE_PROTOCOL_GUID \
  { \
    0x2a22e38f, 0x9d1c, 0x49d0, { 0xbd, 0xce, 0x7d, 0xda, 0xc1, 0x6d, 0xa4, 0x5d } \
  }

//
// Forward reference
//
typedef struct _MM_COMM_BUFFER_UPDATE_PROTOCOL MM_COMM_BUFFER_UPDATE_PROTOCOL;

#define MM_COMMUNICATE_BUFFER_UPDATE_PROTOCOL_VERSION  0x00000001

#pragma pack(1)

struct _MM_COMM_BUFFER_UPDATE_PROTOCOL {
  UINT64            Version;                    // Version of this structure
  MM_COMM_BUFFER    UpdatedCommBuffer;          // MM communication buffer information
};

#pragma pack()

STATIC_ASSERT (sizeof (MM_COMM_BUFFER_UPDATE_PROTOCOL) == (sizeof (MM_COMM_BUFFER) + sizeof (UINT64)), "MM_COMM_BUFFER_UPDATE_PROTOCOL size mismatch!");

extern EFI_GUID  gMmCommBufferUpdateProtocolGuid;

#endif // MM_COMMUNICATION_BUFFER_UPDATE_H_
