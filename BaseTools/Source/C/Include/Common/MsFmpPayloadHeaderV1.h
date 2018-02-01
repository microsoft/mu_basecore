/** @file
  data structure used for the FMP payload header structure in MS FMP capsules

  Copyright (c) 2014, Microsoft Corporation. All rights reserved.<BR>

**/


#ifndef _MS_FMP_PAYLOAD_H__
#define _MS_FMP_PAYLOAD_H__


//
// identifier is used to make sure the data in the header is
// of this structure and version.  If the structure changes update
// the last digit.   
//
#define MS_PAYLOAD_HEADER_IDENTIFIER SIGNATURE_32 ('M', 'S', 'S', '1')


#pragma pack(1)

typedef struct {
  UINT32 Identifier;
  UINT32 HeaderSize;
  UINT32 FwVersion;
  UINT32 LowestSupportedVersion;
  // FW_DEPENDENCY DependencyList[];
} MS_FMP_PAYLOAD_HEADER;

typedef struct {
  EFI_GUID FmpInstance;
  UINT32   MiniumVersionInSystem;
  UINT8    ImageIndex;  //matches the descriptor index
  UINT8    Reserved;
  UINT16   Flags;
} FW_DEPENDENCY;

#pragma pack()

// Flags to describe the expected dependency behaviour 

//dependency must be in system.  Default is only if FMP instance present in system. 
#define MS_FW_DEPENDENCY_FLAG_REQUIRED					0x0001
//version must match exactly.  Default is greater than or equal.
#define MS_FW_DEPENDENCY_FLAG_MATCH_EXACT_VERSION		0x0002

#endif
