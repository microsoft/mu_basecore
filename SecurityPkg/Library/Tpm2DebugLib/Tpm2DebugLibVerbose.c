/** @file -- Tpm2DebugLibVerbose.c
This file contains helper functions to perform a detailed debugging of
TPM transactions as they go to and from the TPM device.

Copyright (c) 2016, Microsoft Corporation

MS_CHANGE

All rights reserved.
Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>

#include <IndustryStandard/Tpm20.h>

#define MAX_TPM_BUFFER_DUMP       80      // If printing an entire buffer, only print up to MAX bytes.

#pragma pack(1)

// TPM2_PCR_COMMON_COMMAND - The common fields shared between the PCR Extend
//                           and PCR Event commands.
typedef struct {
  TPM2_COMMAND_HEADER       Header;
  TPMI_DH_PCR               PcrHandle;
  UINT32                    AuthorizationSize;
  TPMS_AUTH_COMMAND         AuthSessionPcr;
} TPM2_PCR_COMMON_COMMAND;

#pragma pack()

typedef struct {
  UINT32    Code;
  CHAR8     *Text;
} TPM2_CODE_STRING;

TPM2_CODE_STRING    CommandCodeStrings[] = {
  { TPM_CC_NV_UndefineSpaceSpecial, "TPM_CC_NV_UndefineSpaceSpecial" },
  { TPM_CC_EvictControl, "TPM_CC_EvictControl" },
  { TPM_CC_HierarchyControl, "TPM_CC_HierarchyControl" },
  { TPM_CC_NV_UndefineSpace, "TPM_CC_NV_UndefineSpace" },
  { TPM_CC_ChangeEPS, "TPM_CC_ChangeEPS" },
  { TPM_CC_ChangePPS, "TPM_CC_ChangePPS" },
  { TPM_CC_Clear, "TPM_CC_Clear" },
  { TPM_CC_ClearControl, "TPM_CC_ClearControl" },
  { TPM_CC_ClockSet, "TPM_CC_ClockSet" },
  { TPM_CC_HierarchyChangeAuth, "TPM_CC_HierarchyChangeAuth" },
  { TPM_CC_NV_DefineSpace, "TPM_CC_NV_DefineSpace" },
  { TPM_CC_PCR_Allocate, "TPM_CC_PCR_Allocate" },
  { TPM_CC_PCR_SetAuthPolicy, "TPM_CC_PCR_SetAuthPolicy" },
  { TPM_CC_PP_Commands, "TPM_CC_PP_Commands" },
  { TPM_CC_SetPrimaryPolicy, "TPM_CC_SetPrimaryPolicy" },
  { TPM_CC_FieldUpgradeStart, "TPM_CC_FieldUpgradeStart" },
  { TPM_CC_ClockRateAdjust, "TPM_CC_ClockRateAdjust" },
  { TPM_CC_CreatePrimary, "TPM_CC_CreatePrimary" },
  { TPM_CC_NV_GlobalWriteLock, "TPM_CC_NV_GlobalWriteLock" },
  { TPM_CC_PP_LAST, "TPM_CC_PP_LAST" },
  { TPM_CC_GetCommandAuditDigest, "TPM_CC_GetCommandAuditDigest" },
  { TPM_CC_NV_Increment, "TPM_CC_NV_Increment" },
  { TPM_CC_NV_SetBits, "TPM_CC_NV_SetBits" },
  { TPM_CC_NV_Extend, "TPM_CC_NV_Extend" },
  { TPM_CC_NV_Write, "TPM_CC_NV_Write" },
  { TPM_CC_NV_WriteLock, "TPM_CC_NV_WriteLock" },
  { TPM_CC_DictionaryAttackLockReset, "TPM_CC_DictionaryAttackLockReset" },
  { TPM_CC_DictionaryAttackParameters, "TPM_CC_DictionaryAttackParameters" },
  { TPM_CC_NV_ChangeAuth, "TPM_CC_NV_ChangeAuth" },
  { TPM_CC_PCR_Event, "TPM_CC_PCR_Event" },
  { TPM_CC_PCR_Reset, "TPM_CC_PCR_Reset" },
  { TPM_CC_SequenceComplete, "TPM_CC_SequenceComplete" },
  { TPM_CC_SetAlgorithmSet, "TPM_CC_SetAlgorithmSet" },
  { TPM_CC_SetCommandCodeAuditStatus, "TPM_CC_SetCommandCodeAuditStatus" },
  { TPM_CC_FieldUpgradeData, "TPM_CC_FieldUpgradeData" },
  { TPM_CC_IncrementalSelfTest, "TPM_CC_IncrementalSelfTest" },
  { TPM_CC_SelfTest, "TPM_CC_SelfTest" },
  { TPM_CC_Startup, "TPM_CC_Startup" },
  { TPM_CC_Shutdown, "TPM_CC_Shutdown" },
  { TPM_CC_StirRandom, "TPM_CC_StirRandom" },
  { TPM_CC_ActivateCredential, "TPM_CC_ActivateCredential" },
  { TPM_CC_Certify, "TPM_CC_Certify" },
  { TPM_CC_PolicyNV, "TPM_CC_PolicyNV" },
  { TPM_CC_CertifyCreation, "TPM_CC_CertifyCreation" },
  { TPM_CC_Duplicate, "TPM_CC_Duplicate" },
  { TPM_CC_GetTime, "TPM_CC_GetTime" },
  { TPM_CC_GetSessionAuditDigest, "TPM_CC_GetSessionAuditDigest" },
  { TPM_CC_NV_Read, "TPM_CC_NV_Read" },
  { TPM_CC_NV_ReadLock, "TPM_CC_NV_ReadLock" },
  { TPM_CC_ObjectChangeAuth, "TPM_CC_ObjectChangeAuth" },
  { TPM_CC_PolicySecret, "TPM_CC_PolicySecret" },
  { TPM_CC_Rewrap, "TPM_CC_Rewrap" },
  { TPM_CC_Create, "TPM_CC_Create" },
  { TPM_CC_ECDH_ZGen, "TPM_CC_ECDH_ZGen" },
  { TPM_CC_HMAC, "TPM_CC_HMAC" },
  { TPM_CC_Import, "TPM_CC_Import" },
  { TPM_CC_Load, "TPM_CC_Load" },
  { TPM_CC_Quote, "TPM_CC_Quote" },
  { TPM_CC_RSA_Decrypt, "TPM_CC_RSA_Decrypt" },
  { TPM_CC_HMAC_Start, "TPM_CC_HMAC_Start" },
  { TPM_CC_SequenceUpdate, "TPM_CC_SequenceUpdate" },
  { TPM_CC_Sign, "TPM_CC_Sign" },
  { TPM_CC_Unseal, "TPM_CC_Unseal" },
  { TPM_CC_PolicySigned, "TPM_CC_PolicySigned" },
  { TPM_CC_ContextLoad, "TPM_CC_ContextLoad" },
  { TPM_CC_ContextSave, "TPM_CC_ContextSave" },
  { TPM_CC_ECDH_KeyGen, "TPM_CC_ECDH_KeyGen" },
  { TPM_CC_EncryptDecrypt, "TPM_CC_EncryptDecrypt" },
  { TPM_CC_FlushContext, "TPM_CC_FlushContext" },
  { TPM_CC_LoadExternal, "TPM_CC_LoadExternal" },
  { TPM_CC_MakeCredential, "TPM_CC_MakeCredential" },
  { TPM_CC_NV_ReadPublic, "TPM_CC_NV_ReadPublic" },
  { TPM_CC_PolicyAuthorize, "TPM_CC_PolicyAuthorize" },
  { TPM_CC_PolicyAuthValue, "TPM_CC_PolicyAuthValue" },
  { TPM_CC_PolicyCommandCode, "TPM_CC_PolicyCommandCode" },
  { TPM_CC_PolicyCounterTimer, "TPM_CC_PolicyCounterTimer" },
  { TPM_CC_PolicyCpHash, "TPM_CC_PolicyCpHash" },
  { TPM_CC_PolicyLocality, "TPM_CC_PolicyLocality" },
  { TPM_CC_PolicyNameHash, "TPM_CC_PolicyNameHash" },
  { TPM_CC_PolicyOR, "TPM_CC_PolicyOR" },
  { TPM_CC_PolicyTicket, "TPM_CC_PolicyTicket" },
  { TPM_CC_ReadPublic, "TPM_CC_ReadPublic" },
  { TPM_CC_RSA_Encrypt, "TPM_CC_RSA_Encrypt" },
  { TPM_CC_StartAuthSession, "TPM_CC_StartAuthSession" },
  { TPM_CC_VerifySignature, "TPM_CC_VerifySignature" },
  { TPM_CC_ECC_Parameters, "TPM_CC_ECC_Parameters" },
  { TPM_CC_FirmwareRead, "TPM_CC_FirmwareRead" },
  { TPM_CC_GetCapability, "TPM_CC_GetCapability" },
  { TPM_CC_GetRandom, "TPM_CC_GetRandom" },
  { TPM_CC_GetTestResult, "TPM_CC_GetTestResult" },
  { TPM_CC_Hash, "TPM_CC_Hash" },
  { TPM_CC_PCR_Read, "TPM_CC_PCR_Read" },
  { TPM_CC_PolicyPCR, "TPM_CC_PolicyPCR" },
  { TPM_CC_PolicyRestart, "TPM_CC_PolicyRestart" },
  { TPM_CC_ReadClock, "TPM_CC_ReadClock" },
  { TPM_CC_PCR_Extend, "TPM_CC_PCR_Extend" },
  { TPM_CC_PCR_SetAuthValue, "TPM_CC_PCR_SetAuthValue" },
  { TPM_CC_NV_Certify, "TPM_CC_NV_Certify" },
  { TPM_CC_EventSequenceComplete, "TPM_CC_EventSequenceComplete" },
  { TPM_CC_HashSequenceStart, "TPM_CC_HashSequenceStart" },
  { TPM_CC_PolicyPhysicalPresence, "TPM_CC_PolicyPhysicalPresence" },
  { TPM_CC_PolicyDuplicationSelect, "TPM_CC_PolicyDuplicationSelect" },
  { TPM_CC_PolicyGetDigest, "TPM_CC_PolicyGetDigest" },
  { TPM_CC_TestParms, "TPM_CC_TestParms" },
  { TPM_CC_Commit, "TPM_CC_Commit" },
  { TPM_CC_PolicyPassword, "TPM_CC_PolicyPassword" },
  { TPM_CC_ZGen_2Phase, "TPM_CC_ZGen_2Phase" },
  { TPM_CC_EC_Ephemeral, "TPM_CC_EC_Ephemeral" },
};
UINTN   CommandCodeStringsCount = sizeof( CommandCodeStrings ) / sizeof( CommandCodeStrings[0] );

TPM2_CODE_STRING    ResponseCodeStrings[] = {
  { TPM_RC_SUCCESS, "TPM_RC_SUCCESS" },
  { TPM_RC_BAD_TAG, "TPM_RC_BAD_TAG" },
  { TPM_RC_INITIALIZE, "TPM_RC_INITIALIZE" },
  { TPM_RC_FAILURE, "TPM_RC_FAILURE" },
  { TPM_RC_SEQUENCE, "TPM_RC_SEQUENCE" },
  { TPM_RC_PRIVATE, "TPM_RC_PRIVATE" },
  { TPM_RC_HMAC, "TPM_RC_HMAC" },
  { TPM_RC_DISABLED, "TPM_RC_DISABLED" },
  { TPM_RC_EXCLUSIVE, "TPM_RC_EXCLUSIVE" },
  { TPM_RC_AUTH_TYPE, "TPM_RC_AUTH_TYPE" },
  { TPM_RC_AUTH_MISSING, "TPM_RC_AUTH_MISSING" },
  { TPM_RC_POLICY, "TPM_RC_POLICY" },
  { TPM_RC_PCR, "TPM_RC_PCR" },
  { TPM_RC_PCR_CHANGED, "TPM_RC_PCR_CHANGED" },
  { TPM_RC_UPGRADE, "TPM_RC_UPGRADE" },
  { TPM_RC_TOO_MANY_CONTEXTS, "TPM_RC_TOO_MANY_CONTEXTS" },
  { TPM_RC_AUTH_UNAVAILABLE, "TPM_RC_AUTH_UNAVAILABLE" },
  { TPM_RC_REBOOT, "TPM_RC_REBOOT" },
  { TPM_RC_UNBALANCED, "TPM_RC_UNBALANCED" },
  { TPM_RC_COMMAND_SIZE, "TPM_RC_COMMAND_SIZE" },
  { TPM_RC_COMMAND_CODE, "TPM_RC_COMMAND_CODE" },
  { TPM_RC_AUTHSIZE, "TPM_RC_AUTHSIZE" },
  { TPM_RC_AUTH_CONTEXT, "TPM_RC_AUTH_CONTEXT" },
  { TPM_RC_NV_RANGE, "TPM_RC_NV_RANGE" },
  { TPM_RC_NV_SIZE, "TPM_RC_NV_SIZE" },
  { TPM_RC_NV_LOCKED, "TPM_RC_NV_LOCKED" },
  { TPM_RC_NV_AUTHORIZATION, "TPM_RC_NV_AUTHORIZATION" },
  { TPM_RC_NV_UNINITIALIZED, "TPM_RC_NV_UNINITIALIZED" },
  { TPM_RC_NV_SPACE, "TPM_RC_NV_SPACE" },
  { TPM_RC_NV_DEFINED, "TPM_RC_NV_DEFINED" },
  { TPM_RC_BAD_CONTEXT, "TPM_RC_BAD_CONTEXT" },
  { TPM_RC_CPHASH, "TPM_RC_CPHASH" },
  { TPM_RC_PARENT, "TPM_RC_PARENT" },
  { TPM_RC_NEEDS_TEST, "TPM_RC_NEEDS_TEST" },
  { TPM_RC_NO_RESULT, "TPM_RC_NO_RESULT" },
  { TPM_RC_SENSITIVE, "TPM_RC_SENSITIVE" },
};
UINTN   ResponseCodeStringsCount = sizeof( ResponseCodeStrings ) / sizeof( ResponseCodeStrings[0] );


/**
  This simple function will dump up to MAX_TPM_BUFFER_DUMP bytes
  of a TPM data buffer and apppend '...' if buffer is larger.

  @param[in]  DebugLevel    The debugging level to use.
  @param[in]  Preamble      [Optional] A string to print before the buffer dump.
  @param[in]  BufferSize    The actual size of the provided buffer.
  @param[in]  Buffer        A pointer to the buffer in question.

**/
VOID
DumpTpmBuffer (
  IN UINTN          DebugLevel,
  IN CHAR8          *Preamble OPTIONAL,
  IN UINTN          BufferSize,
  IN CONST UINT8    *Buffer
  )
{
  UINTN                           DebugBufferCount, Index;

  // JBB TODO: Don't even evaluate if below required debugging level.
  // JBB TODO: Pass in max buffer size? Format nicely?

  // Determine max buffer size.
  DebugBufferCount = MIN( BufferSize, MAX_TPM_BUFFER_DUMP );

  // Print the preamble, if supplied.
  if (Preamble)
  {
    DEBUG(( DebugLevel, "%a", Preamble ));
  }

  // Dump them bytes.
  for (Index = 0; Index < DebugBufferCount; Index++)
  {
    DEBUG(( DebugLevel, "%02X ", Buffer[Index] ));
  }

  // FINISH HIM!!!
  if (DebugBufferCount != BufferSize)
  {
    DEBUG(( DebugLevel, "...\n" ));
  }
  else
  {
    DEBUG(( DebugLevel, "\n" ));
  }

  return;
} // DumpTpmBuffer()


/**
  This abstract function takes in a list of codes and strings and returns either
  a string matching the code or the supplied "default" string.

  @param[in]  Code            TPM2 Command or Response Code in little-endian format.
  @param[in]  List            Pointer to the start of a TPM2_CODE_STRING list.
  @param[in]  ListCount       Number of items in the given list.
  @param[in]  DefaultString   A string to be returned if no matching string is found.

  @retval     CHAR8* of a human-readable string for the code (as defined in the spec).

**/
STATIC
CHAR8*
GetTpmCodeString (
  IN UINT32             Code,
  IN TPM2_CODE_STRING   *List,
  IN UINTN              ListCount,
  IN CHAR8              *DefaultString
  )
{
  UINTN   Index;
  CHAR8   *Result = DefaultString;

  // Find the code in the string array.
  for (Index = 0; Index < ListCount; Index++)
  {
    if (List[Index].Code == Code)
    {
      Result = List[Index].Text;
      break;
    }
  }

  return Result;
}


/**
  Simple wrapper function to use GetTpmCodeString() to search for a command code.

  @param[in]  Code    TPM2 Command Code in marshalled (generally big-endian) format.

  @retval     CHAR8* of a human-readable string for the code (as defined in the spec).

**/
CHAR8*
GetTpm2CommandString (
  IN TPM_CC     Code
  )
{
  return GetTpmCodeString( SwapBytes32( Code ), CommandCodeStrings, CommandCodeStringsCount, "TPM_CC_UNKNOWN" );
}


/**
  Simple wrapper function to use GetTpmCodeString() to search for a response code.

  @param[in]  Code    TPM2 Response Code in marshalled (generally big-endian) format.

  @retval     CHAR8* of a human-readable string for the code (as defined in the spec).

**/
CHAR8*
GetTpm2ResponseString (
  IN TPM_RC     Code
  )
{
  return GetTpmCodeString( SwapBytes32( Code ), ResponseCodeStrings, ResponseCodeStringsCount, "TPM_RC_UNKNOWN" );
}


/**
  This function dumps additional information about TPM PCR Extend
  and Event operations.

  @param[in]  Command         Little-endian format of the command being issued.
  @param[in]  InputBlockSize  Size of the input buffer.
  @param[in]  InputBlock      Pointer to the input buffer itself.

**/
STATIC
VOID
DumpTpmPcrCommand (
  IN TPM_CC         Command,
  IN UINT32         InputBlockSize,
  IN CONST UINT8    *InputBlock
  )
{
  CONST TPM2_PCR_COMMON_COMMAND   *PcrHeader;
  TPMI_DH_PCR                     PcrHandle;

  // If this is an unrecognized command, we can't go much further.
  if (Command != TPM_CC_PCR_Extend && Command != TPM_CC_PCR_Event)
  {
    DEBUG(( DEBUG_WARN, __FUNCTION__" - Unrecognized command! 0x%X\n", Command ));
    return;
  }

  // Start the debugging by mapping some stuff.
  PcrHeader  = (TPM2_PCR_COMMON_COMMAND*) InputBlock;
  PcrHandle   = SwapBytes32( PcrHeader->PcrHandle );
  DEBUG(( DEBUG_INFO, "- PCR:    %d (0x%08X)\n", PcrHandle, PcrHandle ));

  // Now handle any command-specific debugging.
  if (Command == TPM_CC_PCR_Extend)
  {
    CONST TPML_DIGEST_VALUES        *DigestValues;
    CONST TPMT_HA                   *CurrentDigest;
    UINT32                          DigestCount;

    // Move the dump data to the start of the digest section.
    DigestValues = (TPML_DIGEST_VALUES*)(InputBlock + OFFSET_OF( TPM2_PCR_COMMON_COMMAND, AuthSessionPcr ) + SwapBytes32( PcrHeader->AuthorizationSize ));

    // Determine the digest count and locate the first digest.
    DigestCount   = SwapBytes32( DigestValues->count );
    CurrentDigest = &DigestValues->digests[0];

    // Print each digest.
    do
    {
      // Print the current digest.
      switch (SwapBytes16( CurrentDigest->hashAlg ))
      {
        case TPM_ALG_SHA1:
          DumpTpmBuffer( DEBUG_INFO, "- SHA1:   ", SHA1_DIGEST_SIZE, CurrentDigest->digest.sha1 );
          CurrentDigest = (TPMT_HA*)((UINT8*)CurrentDigest + OFFSET_OF( TPMT_HA, digest ) + SHA1_DIGEST_SIZE);
          DigestCount--;    // Account for this digest.
          break;

        case TPM_ALG_SHA256:
          DumpTpmBuffer( DEBUG_INFO, "- SHA256: ", SHA256_DIGEST_SIZE, CurrentDigest->digest.sha256 );
          CurrentDigest = (TPMT_HA*)((UINT8*)CurrentDigest + OFFSET_OF( TPMT_HA, digest ) + SHA256_DIGEST_SIZE);
          DigestCount--;    // Account for this digest.
          break;

        default:
          // This algorithm hasn't been programmed yet. We need to bail.
          DEBUG(( DEBUG_WARN, __FUNCTION__" - Unknown hash algorithm! 0x%04X\n", SwapBytes16( CurrentDigest->hashAlg ) ));
          // Zero the count so we can get out of here.
          DigestCount = 0;
          break;
      }
    } while (DigestCount > 0);
  }

  return;
} // DumpTpmPcrCommand()


/**
  This function dumps as much information as possible about
  a command being sent to the TPM for maximum user-readability.

  @param[in]  InputBlockSize  Size of the input buffer.
  @param[in]  InputBlock      Pointer to the input buffer itself.

**/
VOID
EFIAPI
DumpTpmInputBlock (
  IN UINT32         InputBlockSize,
  IN CONST UINT8    *InputBlock
  )
{
  CONST TPM2_COMMAND_HEADER       *CommHeader;
  TPM_ST                          NativeTag;
  UINT32                          NativeSize;
  TPM_CC                          NativeCode;

  DEBUG(( DEBUG_INFO, "\n=== BEGIN TPM COMMAND ===\n" ));
  DEBUG(( DEBUG_VERBOSE, "Size:     %d (0x%X), Address:  0x%X\n", InputBlockSize, InputBlockSize, InputBlock ));

  // Make sure we've got at least enough data for a valid header.
  if (InputBlockSize < sizeof( *CommHeader ))
  {
    DEBUG(( DEBUG_WARN, __FUNCTION__" - Invalid buffer size!\n" ));
    return;
  }

  // Start the debugging by mapping some stuff.
  CommHeader  = (TPM2_COMMAND_HEADER*) InputBlock;
  NativeTag   = SwapBytes16( CommHeader->tag );
  NativeSize  = SwapBytes32( CommHeader->paramSize );
  NativeCode  = SwapBytes32( CommHeader->commandCode );
  DEBUG(( DEBUG_INFO, "Command:  %a (0x%08X)\n", GetTpm2CommandString( CommHeader->commandCode ), NativeCode ));
  DEBUG(( DEBUG_INFO, "Tag:      0x%04X\n", NativeTag ));
  DEBUG(( DEBUG_INFO, "Size:     %d (0x%X)\n", NativeSize, NativeSize ));

  // Debug anything else.
  switch (NativeCode)
  {
    case TPM_CC_PCR_Event:
    case TPM_CC_PCR_Extend:
      DumpTpmPcrCommand( NativeCode, InputBlockSize, InputBlock );
      break;

    default:
      break;
  }

  // If verbose, dump all of the buffer contents for deeper analysis.
  DumpTpmBuffer( DEBUG_VERBOSE, "DATA:     ", InputBlockSize, InputBlock );

  return;
} // DumpTpmInputBlock()


/**
  This function dumps as much information as possible about
  a response from the TPM for maximum user-readability.

  @param[in]  OutputBlockSize  Size of the output buffer.
  @param[in]  OutputBlock      Pointer to the output buffer itself.

**/
VOID
EFIAPI
DumpTpmOutputBlock (
  IN UINT32         OutputBlockSize,
  IN CONST UINT8    *OutputBlock
  )
{
  CONST TPM2_RESPONSE_HEADER      *RespHeader;
  TPM_ST                          NativeTag;
  UINT32                          NativeSize;
  TPM_CC                          NativeCode;

  DEBUG(( DEBUG_VERBOSE, "Size:     %d (0x%X), Address:  0x%X\n", OutputBlockSize, OutputBlockSize, OutputBlock ));

  // Start the debugging by mapping some stuff.
  RespHeader  = (TPM2_RESPONSE_HEADER*) OutputBlock;
  NativeTag   = SwapBytes16( RespHeader->tag );
  NativeSize  = SwapBytes32( RespHeader->paramSize );
  NativeCode  = SwapBytes32( RespHeader->responseCode );
  DEBUG(( DEBUG_INFO, "Response: %a (0x%08X)\n", GetTpm2ResponseString( RespHeader->responseCode ), NativeCode ));
  DEBUG(( DEBUG_INFO, "Tag:      0x%04X\n", NativeTag ));
  DEBUG(( DEBUG_INFO, "Size:     %d (0x%X)\n", NativeSize, NativeSize ));

  // If verbose, dump all of the buffer contents for deeper analysis.
  DumpTpmBuffer( DEBUG_VERBOSE, "DATA:     ", OutputBlockSize, OutputBlock );

  DEBUG(( DEBUG_INFO, "=== END TPM COMMAND ===\n\n" ));

  return;
} // DumpTpmOutputBlock()
