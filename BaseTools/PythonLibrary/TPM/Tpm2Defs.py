## @file Tpm2Defs.py
# This file contains utility classes to help interpret definitions from the
# Tpm20.h header file in TianoCore.
#
##
# Copyright (c) 2017, Microsoft Corporation
#
# All rights reserved.
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##
# INCLUDES CONTENTS FROM TianoCore Tpm20.h HEADER FILE!!
#
# TPM2.0 Specification data structures
#   (Trusted Platform Module Library Specification, Family "2.0", Level 00, Revision 00.96,
#   @http://www.trustedcomputinggroup.org/resources/tpm_library_specification)
#
#   Check http://trustedcomputinggroup.org for latest specification updates.
#
# Copyright (c) 2013 - 2015, Intel Corporation. All rights reserved. <BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
##


## Table 7 - TPM_ALG_ID Constants
TPM_ALG_Size                      = 2
TPM_ALG_Pack                      = "H"
TPM_ALG_ERROR                     = 0x0000
TPM_ALG_FIRST                     = 0x0001
TPM_ALG_RSA                       = 0x0001
TPM_ALG_SHA                       = 0x0004
TPM_ALG_SHA1                      = 0x0004
TPM_ALG_HMAC                      = 0x0005
TPM_ALG_AES                       = 0x0006
TPM_ALG_MGF1                      = 0x0007
TPM_ALG_KEYEDHASH                 = 0x0008
TPM_ALG_XOR                       = 0x000A
TPM_ALG_SHA256                    = 0x000B
TPM_ALG_SHA384                    = 0x000C
TPM_ALG_SHA512                    = 0x000D
TPM_ALG_NULL                      = 0x0010
TPM_ALG_SM3_256                   = 0x0012
TPM_ALG_SM4                       = 0x0013
TPM_ALG_RSASSA                    = 0x0014
TPM_ALG_RSAES                     = 0x0015
TPM_ALG_RSAPSS                    = 0x0016
TPM_ALG_OAEP                      = 0x0017
TPM_ALG_ECDSA                     = 0x0018
TPM_ALG_ECDH                      = 0x0019
TPM_ALG_ECDAA                     = 0x001A
TPM_ALG_SM2                       = 0x001B
TPM_ALG_ECSCHNORR                 = 0x001C
TPM_ALG_ECMQV                     = 0x001D
TPM_ALG_KDF1_SP800_56a            = 0x0020
TPM_ALG_KDF2                      = 0x0021
TPM_ALG_KDF1_SP800_108            = 0x0022
TPM_ALG_ECC                       = 0x0023
TPM_ALG_SYMCIPHER                 = 0x0025
TPM_ALG_CTR                       = 0x0040
TPM_ALG_OFB                       = 0x0041
TPM_ALG_CBC                       = 0x0042
TPM_ALG_CFB                       = 0x0043
TPM_ALG_ECB                       = 0x0044
TPM_ALG_LAST                      = 0x0044

## Table 11 - TPM_CC Constants (Numeric Order)
TPM_CC_Size                       = 4
TPM_CC_Pack                       = "L"
TPM_CC_FIRST                      = 0x0000011F
TPM_CC_PP_FIRST                   = 0x0000011F
TPM_CC_NV_UndefineSpaceSpecial    = 0x0000011F
TPM_CC_EvictControl               = 0x00000120
TPM_CC_HierarchyControl           = 0x00000121
TPM_CC_NV_UndefineSpace           = 0x00000122
TPM_CC_ChangeEPS                  = 0x00000124
TPM_CC_ChangePPS                  = 0x00000125
TPM_CC_Clear                      = 0x00000126
TPM_CC_ClearControl               = 0x00000127
TPM_CC_ClockSet                   = 0x00000128
TPM_CC_HierarchyChangeAuth        = 0x00000129
TPM_CC_NV_DefineSpace             = 0x0000012A
TPM_CC_PCR_Allocate               = 0x0000012B
TPM_CC_PCR_SetAuthPolicy          = 0x0000012C
TPM_CC_PP_Commands                = 0x0000012D
TPM_CC_SetPrimaryPolicy           = 0x0000012E
TPM_CC_FieldUpgradeStart          = 0x0000012F
TPM_CC_ClockRateAdjust            = 0x00000130
TPM_CC_CreatePrimary              = 0x00000131
TPM_CC_NV_GlobalWriteLock         = 0x00000132
TPM_CC_PP_LAST                    = 0x00000132
TPM_CC_GetCommandAuditDigest      = 0x00000133
TPM_CC_NV_Increment               = 0x00000134
TPM_CC_NV_SetBits                 = 0x00000135
TPM_CC_NV_Extend                  = 0x00000136
TPM_CC_NV_Write                   = 0x00000137
TPM_CC_NV_WriteLock               = 0x00000138
TPM_CC_DictionaryAttackLockReset  = 0x00000139
TPM_CC_DictionaryAttackParameters = 0x0000013A
TPM_CC_NV_ChangeAuth              = 0x0000013B
TPM_CC_PCR_Event                  = 0x0000013C
TPM_CC_PCR_Reset                  = 0x0000013D
TPM_CC_SequenceComplete           = 0x0000013E
TPM_CC_SetAlgorithmSet            = 0x0000013F
TPM_CC_SetCommandCodeAuditStatus  = 0x00000140
TPM_CC_FieldUpgradeData           = 0x00000141
TPM_CC_IncrementalSelfTest        = 0x00000142
TPM_CC_SelfTest                   = 0x00000143
TPM_CC_Startup                    = 0x00000144
TPM_CC_Shutdown                   = 0x00000145
TPM_CC_StirRandom                 = 0x00000146
TPM_CC_ActivateCredential         = 0x00000147
TPM_CC_Certify                    = 0x00000148
TPM_CC_PolicyNV                   = 0x00000149
TPM_CC_CertifyCreation            = 0x0000014A
TPM_CC_Duplicate                  = 0x0000014B
TPM_CC_GetTime                    = 0x0000014C
TPM_CC_GetSessionAuditDigest      = 0x0000014D
TPM_CC_NV_Read                    = 0x0000014E
TPM_CC_NV_ReadLock                = 0x0000014F
TPM_CC_ObjectChangeAuth           = 0x00000150
TPM_CC_PolicySecret               = 0x00000151
TPM_CC_Rewrap                     = 0x00000152
TPM_CC_Create                     = 0x00000153
TPM_CC_ECDH_ZGen                  = 0x00000154
TPM_CC_HMAC                       = 0x00000155
TPM_CC_Import                     = 0x00000156
TPM_CC_Load                       = 0x00000157
TPM_CC_Quote                      = 0x00000158
TPM_CC_RSA_Decrypt                = 0x00000159
TPM_CC_HMAC_Start                 = 0x0000015B
TPM_CC_SequenceUpdate             = 0x0000015C
TPM_CC_Sign                       = 0x0000015D
TPM_CC_Unseal                     = 0x0000015E
TPM_CC_PolicySigned               = 0x00000160
TPM_CC_ContextLoad                = 0x00000161
TPM_CC_ContextSave                = 0x00000162
TPM_CC_ECDH_KeyGen                = 0x00000163
TPM_CC_EncryptDecrypt             = 0x00000164
TPM_CC_FlushContext               = 0x00000165
TPM_CC_LoadExternal               = 0x00000167
TPM_CC_MakeCredential             = 0x00000168
TPM_CC_NV_ReadPublic              = 0x00000169
TPM_CC_PolicyAuthorize            = 0x0000016A
TPM_CC_PolicyAuthValue            = 0x0000016B
TPM_CC_PolicyCommandCode          = 0x0000016C
TPM_CC_PolicyCounterTimer         = 0x0000016D
TPM_CC_PolicyCpHash               = 0x0000016E
TPM_CC_PolicyLocality             = 0x0000016F
TPM_CC_PolicyNameHash             = 0x00000170
TPM_CC_PolicyOR                   = 0x00000171
TPM_CC_PolicyTicket               = 0x00000172
TPM_CC_ReadPublic                 = 0x00000173
TPM_CC_RSA_Encrypt                = 0x00000174
TPM_CC_StartAuthSession           = 0x00000176
TPM_CC_VerifySignature            = 0x00000177
TPM_CC_ECC_Parameters             = 0x00000178
TPM_CC_FirmwareRead               = 0x00000179
TPM_CC_GetCapability              = 0x0000017A
TPM_CC_GetRandom                  = 0x0000017B
TPM_CC_GetTestResult              = 0x0000017C
TPM_CC_Hash                       = 0x0000017D
TPM_CC_PCR_Read                   = 0x0000017E
TPM_CC_PolicyPCR                  = 0x0000017F
TPM_CC_PolicyRestart              = 0x00000180
TPM_CC_ReadClock                  = 0x00000181
TPM_CC_PCR_Extend                 = 0x00000182
TPM_CC_PCR_SetAuthValue           = 0x00000183
TPM_CC_NV_Certify                 = 0x00000184
TPM_CC_EventSequenceComplete      = 0x00000185
TPM_CC_HashSequenceStart          = 0x00000186
TPM_CC_PolicyPhysicalPresence     = 0x00000187
TPM_CC_PolicyDuplicationSelect    = 0x00000188
TPM_CC_PolicyGetDigest            = 0x00000189
TPM_CC_TestParms                  = 0x0000018A
TPM_CC_Commit                     = 0x0000018B
TPM_CC_PolicyPassword             = 0x0000018C
TPM_CC_ZGen_2Phase                = 0x0000018D
TPM_CC_EC_Ephemeral               = 0x0000018E
TPM_CC_LAST                       = 0x0000018E

## Table 18 - TPM_ST Constants
TPM_ST_Size                       = 2
TPM_ST_Pack                       = "H"
TPM_ST_RSP_COMMAND                = 0x00C4
TPM_ST_NULL                       = 0X8000
TPM_ST_NO_SESSIONS                = 0x8001
TPM_ST_SESSIONS                   = 0x8002
TPM_ST_ATTEST_NV                  = 0x8014
TPM_ST_ATTEST_COMMAND_AUDIT       = 0x8015
TPM_ST_ATTEST_SESSION_AUDIT       = 0x8016
TPM_ST_ATTEST_CERTIFY             = 0x8017
TPM_ST_ATTEST_QUOTE               = 0x8018
TPM_ST_ATTEST_TIME                = 0x8019
TPM_ST_ATTEST_CREATION            = 0x801A
TPM_ST_CREATION                   = 0x8021
TPM_ST_VERIFIED                   = 0x8022
TPM_ST_AUTH_SECRET                = 0x8023
TPM_ST_HASHCHECK                  = 0x8024
TPM_ST_AUTH_SIGNED                = 0x8025
TPM_ST_FU_MANIFEST                = 0x8029

## Table 19 - TPM_SU Constants
TPM_SU_Size                       = 2
TPM_SU_Pack                       = "H"
TPM_SU_CLEAR                      = 0x0000
TPM_SU_STATE                      = 0x0001

## Table 20 - TPM_SE Constants
TPM_SE_Size                       = 1
TPM_SE_Pack                       = "B"
TPM_SE_HMAC                       = 0x00
TPM_SE_POLICY                     = 0x01
TPM_SE_TRIAL                      = 0x03

## Table 27 - TPM_RH Constants
TPM_RH_Size                       = 4
TPM_RH_Pack                       = "L"
TPM_RH_FIRST                      = 0x40000000
TPM_RH_SRK                        = 0x40000000
TPM_RH_OWNER                      = 0x40000001
TPM_RH_REVOKE                     = 0x40000002
TPM_RH_TRANSPORT                  = 0x40000003
TPM_RH_OPERATOR                   = 0x40000004
TPM_RH_ADMIN                      = 0x40000005
TPM_RH_EK                         = 0x40000006
TPM_RH_NULL                       = 0x40000007
TPM_RH_UNASSIGNED                 = 0x40000008
TPM_RS_PW                         = 0x40000009
TPM_RH_LOCKOUT                    = 0x4000000A
TPM_RH_ENDORSEMENT                = 0x4000000B
TPM_RH_PLATFORM                   = 0x4000000C
TPM_RH_PLATFORM_NV                = 0x4000000D
TPM_RH_AUTH_00                    = 0x40000010
TPM_RH_AUTH_FF                    = 0x4000010F
TPM_RH_LAST                       = 0x4000010F


class CommandCode(object):
  @staticmethod
  def get_code(cc_string):
    return {
      "TPM_CC_NV_UndefineSpaceSpecial": 0x0000011F,
      "TPM_CC_EvictControl": 0x00000120,
      "TPM_CC_HierarchyControl": 0x00000121,
      "TPM_CC_NV_UndefineSpace": 0x00000122,
      "TPM_CC_ChangeEPS": 0x00000124,
      "TPM_CC_ChangePPS": 0x00000125,
      "TPM_CC_Clear": 0x00000126,
      "TPM_CC_ClearControl": 0x00000127,
      "TPM_CC_ClockSet": 0x00000128,
      "TPM_CC_HierarchyChangeAuth": 0x00000129,
      "TPM_CC_NV_DefineSpace": 0x0000012A,
      "TPM_CC_PCR_Allocate": 0x0000012B,
      "TPM_CC_PCR_SetAuthPolicy": 0x0000012C,
      "TPM_CC_PP_Commands": 0x0000012D,
      "TPM_CC_SetPrimaryPolicy": 0x0000012E,
      "TPM_CC_FieldUpgradeStart": 0x0000012F,
      "TPM_CC_ClockRateAdjust": 0x00000130,
      "TPM_CC_CreatePrimary": 0x00000131,
      "TPM_CC_NV_GlobalWriteLock": 0x00000132,
      "TPM_CC_GetCommandAuditDigest": 0x00000133,
      "TPM_CC_NV_Increment": 0x00000134,
      "TPM_CC_NV_SetBits": 0x00000135,
      "TPM_CC_NV_Extend": 0x00000136,
      "TPM_CC_NV_Write": 0x00000137,
      "TPM_CC_NV_WriteLock": 0x00000138,
      "TPM_CC_DictionaryAttackLockReset": 0x00000139,
      "TPM_CC_DictionaryAttackParameters": 0x0000013A,
      "TPM_CC_NV_ChangeAuth": 0x0000013B,
      "TPM_CC_PCR_Event": 0x0000013C,
      "TPM_CC_PCR_Reset": 0x0000013D,
      "TPM_CC_SequenceComplete": 0x0000013E,
      "TPM_CC_SetAlgorithmSet": 0x0000013F,
      "TPM_CC_SetCommandCodeAuditStatus": 0x00000140,
      "TPM_CC_FieldUpgradeData": 0x00000141,
      "TPM_CC_IncrementalSelfTest": 0x00000142,
      "TPM_CC_SelfTest": 0x00000143,
      "TPM_CC_Startup": 0x00000144,
      "TPM_CC_Shutdown": 0x00000145,
      "TPM_CC_StirRandom": 0x00000146,
      "TPM_CC_ActivateCredential": 0x00000147,
      "TPM_CC_Certify": 0x00000148,
      "TPM_CC_PolicyNV": 0x00000149,
      "TPM_CC_CertifyCreation": 0x0000014A,
      "TPM_CC_Duplicate": 0x0000014B,
      "TPM_CC_GetTime": 0x0000014C,
      "TPM_CC_GetSessionAuditDigest": 0x0000014D,
      "TPM_CC_NV_Read": 0x0000014E,
      "TPM_CC_NV_ReadLock": 0x0000014F,
      "TPM_CC_ObjectChangeAuth": 0x00000150,
      "TPM_CC_PolicySecret": 0x00000151,
      "TPM_CC_Rewrap": 0x00000152,
      "TPM_CC_Create": 0x00000153,
      "TPM_CC_ECDH_ZGen": 0x00000154,
      "TPM_CC_HMAC": 0x00000155,
      "TPM_CC_Import": 0x00000156,
      "TPM_CC_Load": 0x00000157,
      "TPM_CC_Quote": 0x00000158,
      "TPM_CC_RSA_Decrypt": 0x00000159,
      "TPM_CC_HMAC_Start": 0x0000015B,
      "TPM_CC_SequenceUpdate": 0x0000015C,
      "TPM_CC_Sign": 0x0000015D,
      "TPM_CC_Unseal": 0x0000015E,
      "TPM_CC_PolicySigned": 0x00000160,
      "TPM_CC_ContextLoad": 0x00000161,
      "TPM_CC_ContextSave": 0x00000162,
      "TPM_CC_ECDH_KeyGen": 0x00000163,
      "TPM_CC_EncryptDecrypt": 0x00000164,
      "TPM_CC_FlushContext": 0x00000165,
      "TPM_CC_LoadExternal": 0x00000167,
      "TPM_CC_MakeCredential": 0x00000168,
      "TPM_CC_NV_ReadPublic": 0x00000169,
      "TPM_CC_PolicyAuthorize": 0x0000016A,
      "TPM_CC_PolicyAuthValue": 0x0000016B,
      "TPM_CC_PolicyCommandCode": 0x0000016C,
      "TPM_CC_PolicyCounterTimer": 0x0000016D,
      "TPM_CC_PolicyCpHash": 0x0000016E,
      "TPM_CC_PolicyLocality": 0x0000016F,
      "TPM_CC_PolicyNameHash": 0x00000170,
      "TPM_CC_PolicyOR": 0x00000171,
      "TPM_CC_PolicyTicket": 0x00000172,
      "TPM_CC_ReadPublic": 0x00000173,
      "TPM_CC_RSA_Encrypt": 0x00000174,
      "TPM_CC_StartAuthSession": 0x00000176,
      "TPM_CC_VerifySignature": 0x00000177,
      "TPM_CC_ECC_Parameters": 0x00000178,
      "TPM_CC_FirmwareRead": 0x00000179,
      "TPM_CC_GetCapability": 0x0000017A,
      "TPM_CC_GetRandom": 0x0000017B,
      "TPM_CC_GetTestResult": 0x0000017C,
      "TPM_CC_Hash": 0x0000017D,
      "TPM_CC_PCR_Read": 0x0000017E,
      "TPM_CC_PolicyPCR": 0x0000017F,
      "TPM_CC_PolicyRestart": 0x00000180,
      "TPM_CC_ReadClock": 0x00000181,
      "TPM_CC_PCR_Extend": 0x00000182,
      "TPM_CC_PCR_SetAuthValue": 0x00000183,
      "TPM_CC_NV_Certify": 0x00000184,
      "TPM_CC_EventSequenceComplete": 0x00000185,
      "TPM_CC_HashSequenceStart": 0x00000186,
      "TPM_CC_PolicyPhysicalPresence": 0x00000187,
      "TPM_CC_PolicyDuplicationSelect": 0x00000188,
      "TPM_CC_PolicyGetDigest": 0x00000189,
      "TPM_CC_TestParms": 0x0000018A,
      "TPM_CC_Commit": 0x0000018B,
      "TPM_CC_PolicyPassword": 0x0000018C,
      "TPM_CC_ZGen_2Phase": 0x0000018D,
      "TPM_CC_EC_Ephemeral": 0x0000018E,
    }.get(cc_string, None)


  @staticmethod
  def get_string(cc_code):
    return {
      0x0000011F: "TPM_CC_NV_UndefineSpaceSpecial",
      0x00000120: "TPM_CC_EvictControl",
      0x00000121: "TPM_CC_HierarchyControl",
      0x00000122: "TPM_CC_NV_UndefineSpace",
      0x00000124: "TPM_CC_ChangeEPS",
      0x00000125: "TPM_CC_ChangePPS",
      0x00000126: "TPM_CC_Clear",
      0x00000127: "TPM_CC_ClearControl",
      0x00000128: "TPM_CC_ClockSet",
      0x00000129: "TPM_CC_HierarchyChangeAuth",
      0x0000012A: "TPM_CC_NV_DefineSpace",
      0x0000012B: "TPM_CC_PCR_Allocate",
      0x0000012C: "TPM_CC_PCR_SetAuthPolicy",
      0x0000012D: "TPM_CC_PP_Commands",
      0x0000012E: "TPM_CC_SetPrimaryPolicy",
      0x0000012F: "TPM_CC_FieldUpgradeStart",
      0x00000130: "TPM_CC_ClockRateAdjust",
      0x00000131: "TPM_CC_CreatePrimary",
      0x00000132: "TPM_CC_NV_GlobalWriteLock",
      0x00000133: "TPM_CC_GetCommandAuditDigest",
      0x00000134: "TPM_CC_NV_Increment",
      0x00000135: "TPM_CC_NV_SetBits",
      0x00000136: "TPM_CC_NV_Extend",
      0x00000137: "TPM_CC_NV_Write",
      0x00000138: "TPM_CC_NV_WriteLock",
      0x00000139: "TPM_CC_DictionaryAttackLockReset",
      0x0000013A: "TPM_CC_DictionaryAttackParameters",
      0x0000013B: "TPM_CC_NV_ChangeAuth",
      0x0000013C: "TPM_CC_PCR_Event",
      0x0000013D: "TPM_CC_PCR_Reset",
      0x0000013E: "TPM_CC_SequenceComplete",
      0x0000013F: "TPM_CC_SetAlgorithmSet",
      0x00000140: "TPM_CC_SetCommandCodeAuditStatus",
      0x00000141: "TPM_CC_FieldUpgradeData",
      0x00000142: "TPM_CC_IncrementalSelfTest",
      0x00000143: "TPM_CC_SelfTest",
      0x00000144: "TPM_CC_Startup",
      0x00000145: "TPM_CC_Shutdown",
      0x00000146: "TPM_CC_StirRandom",
      0x00000147: "TPM_CC_ActivateCredential",
      0x00000148: "TPM_CC_Certify",
      0x00000149: "TPM_CC_PolicyNV",
      0x0000014A: "TPM_CC_CertifyCreation",
      0x0000014B: "TPM_CC_Duplicate",
      0x0000014C: "TPM_CC_GetTime",
      0x0000014D: "TPM_CC_GetSessionAuditDigest",
      0x0000014E: "TPM_CC_NV_Read",
      0x0000014F: "TPM_CC_NV_ReadLock",
      0x00000150: "TPM_CC_ObjectChangeAuth",
      0x00000151: "TPM_CC_PolicySecret",
      0x00000152: "TPM_CC_Rewrap",
      0x00000153: "TPM_CC_Create",
      0x00000154: "TPM_CC_ECDH_ZGen",
      0x00000155: "TPM_CC_HMAC",
      0x00000156: "TPM_CC_Import",
      0x00000157: "TPM_CC_Load",
      0x00000158: "TPM_CC_Quote",
      0x00000159: "TPM_CC_RSA_Decrypt",
      0x0000015B: "TPM_CC_HMAC_Start",
      0x0000015C: "TPM_CC_SequenceUpdate",
      0x0000015D: "TPM_CC_Sign",
      0x0000015E: "TPM_CC_Unseal",
      0x00000160: "TPM_CC_PolicySigned",
      0x00000161: "TPM_CC_ContextLoad",
      0x00000162: "TPM_CC_ContextSave",
      0x00000163: "TPM_CC_ECDH_KeyGen",
      0x00000164: "TPM_CC_EncryptDecrypt",
      0x00000165: "TPM_CC_FlushContext",
      0x00000167: "TPM_CC_LoadExternal",
      0x00000168: "TPM_CC_MakeCredential",
      0x00000169: "TPM_CC_NV_ReadPublic",
      0x0000016A: "TPM_CC_PolicyAuthorize",
      0x0000016B: "TPM_CC_PolicyAuthValue",
      0x0000016C: "TPM_CC_PolicyCommandCode",
      0x0000016D: "TPM_CC_PolicyCounterTimer",
      0x0000016E: "TPM_CC_PolicyCpHash",
      0x0000016F: "TPM_CC_PolicyLocality",
      0x00000170: "TPM_CC_PolicyNameHash",
      0x00000171: "TPM_CC_PolicyOR",
      0x00000172: "TPM_CC_PolicyTicket",
      0x00000173: "TPM_CC_ReadPublic",
      0x00000174: "TPM_CC_RSA_Encrypt",
      0x00000176: "TPM_CC_StartAuthSession",
      0x00000177: "TPM_CC_VerifySignature",
      0x00000178: "TPM_CC_ECC_Parameters",
      0x00000179: "TPM_CC_FirmwareRead",
      0x0000017A: "TPM_CC_GetCapability",
      0x0000017B: "TPM_CC_GetRandom",
      0x0000017C: "TPM_CC_GetTestResult",
      0x0000017D: "TPM_CC_Hash",
      0x0000017E: "TPM_CC_PCR_Read",
      0x0000017F: "TPM_CC_PolicyPCR",
      0x00000180: "TPM_CC_PolicyRestart",
      0x00000181: "TPM_CC_ReadClock",
      0x00000182: "TPM_CC_PCR_Extend",
      0x00000183: "TPM_CC_PCR_SetAuthValue",
      0x00000184: "TPM_CC_NV_Certify",
      0x00000185: "TPM_CC_EventSequenceComplete",
      0x00000186: "TPM_CC_HashSequenceStart",
      0x00000187: "TPM_CC_PolicyPhysicalPresence",
      0x00000188: "TPM_CC_PolicyDuplicationSelect",
      0x00000189: "TPM_CC_PolicyGetDigest",
      0x0000018A: "TPM_CC_TestParms",
      0x0000018B: "TPM_CC_Commit",
      0x0000018C: "TPM_CC_PolicyPassword",
      0x0000018D: "TPM_CC_ZGen_2Phase",
      0x0000018E: "TPM_CC_EC_Ephemeral",
    }.get(cc_code, None)


class ResponseCode(object):
  @staticmethod
  def get_simple_string(rc_code):
    return {
      0x00000100: "TPM_RC_INITIALIZE",
      0x00000101: "TPM_RC_FAILURE",
      0x00000103: "TPM_RC_SEQUENCE",
      0x0000010B: "TPM_RC_PRIVATE",
      0x00000119: "TPM_RC_HMAC",
      0x00000120: "TPM_RC_DISABLED",
      0x00000121: "TPM_RC_EXCLUSIVE",
      0x00000124: "TPM_RC_AUTH_TYPE",
      0x00000125: "TPM_RC_AUTH_MISSING",
      0x00000126: "TPM_RC_POLICY",
      0x00000127: "TPM_RC_PCR",
      0x00000128: "TPM_RC_PCR_CHANGED",
      0x0000012D: "TPM_RC_UPGRADE",
      0x0000012E: "TPM_RC_TOO_MANY_CONTEXTS",
      0x0000012F: "TPM_RC_AUTH_UNAVAILABLE",
      0x00000130: "TPM_RC_REBOOT",
      0x00000131: "TPM_RC_UNBALANCED",
      0x00000142: "TPM_RC_COMMAND_SIZE",
      0x00000143: "TPM_RC_COMMAND_CODE",
      0x00000144: "TPM_RC_AUTHSIZE",
      0x00000145: "TPM_RC_AUTH_CONTEXT",
      0x00000146: "TPM_RC_NV_RANGE",
      0x00000147: "TPM_RC_NV_SIZE",
      0x00000148: "TPM_RC_NV_LOCKED",
      0x00000149: "TPM_RC_NV_AUTHORIZATION",
      0x0000014A: "TPM_RC_NV_UNINITIALIZED",
      0x0000014B: "TPM_RC_NV_SPACE",
      0x0000014C: "TPM_RC_NV_DEFINED",
      0x00000150: "TPM_RC_BAD_CONTEXT",
      0x00000151: "TPM_RC_CPHASH",
      0x00000152: "TPM_RC_PARENT",
      0x00000153: "TPM_RC_NEEDS_TEST",
      0x00000154: "TPM_RC_NO_RESULT",
      0x00000155: "TPM_RC_SENSITIVE",
    }.get(rc_code, None)

  @staticmethod
  def parse_code(rc_code):
    generic_errors = {
      0x000: 'TPM_RC_INITIALIZE',
      0x001: 'TPM_RC_FAILURE',
      0x003: 'TPM_RC_SEQUENCE',
      0x00B: 'TPM_RC_PRIVATE',
      0x019: 'TPM_RC_HMAC',
      0x020: 'TPM_RC_DISABLED',
      0x021: 'TPM_RC_EXCLUSIVE',
      0x024: 'TPM_RC_AUTH_TYPE',
      0x025: 'TPM_RC_AUTH_MISSING',
      0x026: 'TPM_RC_POLICY',
      0x027: 'TPM_RC_PCR',
      0x028: 'TPM_RC_PCR_CHANGED',
      0x02D: 'TPM_RC_UPGRADE',
      0x02E: 'TPM_RC_TOO_MANY_CONTEXTS',
      0x02F: 'TPM_RC_AUTH_UNAVAILABLE',
      0x030: 'TPM_RC_REBOOT',
      0x031: 'TPM_RC_UNBALANCED',
      0x042: 'TPM_RC_COMMAND_SIZE',
      0x043: 'TPM_RC_COMMAND_CODE',
      0x044: 'TPM_RC_AUTHSIZE',
      0x045: 'TPM_RC_AUTH_CONTEXT',
      0x046: 'TPM_RC_NV_RANGE',
      0x047: 'TPM_RC_NV_SIZE',
      0x048: 'TPM_RC_NV_LOCKED',
      0x049: 'TPM_RC_NV_AUTHORIZATION',
      0x04A: 'TPM_RC_NV_UNINITIALIZED',
      0x04B: 'TPM_RC_NV_SPACE',
      0x04C: 'TPM_RC_NV_DEFINED',
      0x050: 'TPM_RC_BAD_CONTEXT',
      0x051: 'TPM_RC_CPHASH',
      0x052: 'TPM_RC_PARENT',
      0x053: 'TPM_RC_NEEDS_TEST',
      0x054: 'TPM_RC_NO_RESULT',
      0x055: 'TPM_RC_SENSITIVE',
    }

    handle_errors = {
      0x001: 'TPM_RC_ASYMMETRIC',
      0x002: 'TPM_RC_ATTRIBUTES',
      0x003: 'TPM_RC_HASH',
      0x004: 'TPM_RC_VALUE',
      0x005: 'TPM_RC_HIERARCHY',
      0x007: 'TPM_RC_KEY_SIZE',
      0x008: 'TPM_RC_MGF',
      0x009: 'TPM_RC_MODE',
      0x00A: 'TPM_RC_TYPE',
      0x00B: 'TPM_RC_HANDLE',
      0x00C: 'TPM_RC_KDF',
      0x00D: 'TPM_RC_RANGE',
      0x00E: 'TPM_RC_AUTH_FAIL',
      0x00F: 'TPM_RC_NONCE',
      0x010: 'TPM_RC_PP',
      0x012: 'TPM_RC_SCHEME',
      0x015: 'TPM_RC_SIZE',
      0x016: 'TPM_RC_SYMMETRIC',
      0x017: 'TPM_RC_TAG',
      0x018: 'TPM_RC_SELECTOR',
      0x01A: 'TPM_RC_INSUFFICIENT',
      0x01B: 'TPM_RC_SIGNATURE',
      0x01C: 'TPM_RC_KEY',
      0x01D: 'TPM_RC_POLICY_FAIL',
      0x01F: 'TPM_RC_INTEGRITY',
      0x020: 'TPM_RC_TICKET',
      0x021: 'TPM_RC_RESERVED_BITS',
      0x022: 'TPM_RC_BAD_AUTH',
      0x023: 'TPM_RC_EXPIRED',
      0x024: 'TPM_RC_POLICY_CC',
      0x025: 'TPM_RC_BINDING',
      0x026: 'TPM_RC_CURVE',
      0x027: 'TPM_RC_ECC_POINT',
    }

    warnings = {
      0x001: "TPM_RC_CONTEXT_GAP",
      0x002: "TPM_RC_OBJECT_MEMORY",
      0x003: "TPM_RC_SESSION_MEMORY",
      0x004: "TPM_RC_MEMORY",
      0x005: "TPM_RC_SESSION_HANDLES",
      0x006: "TPM_RC_OBJECT_HANDLES",
      0x007: "TPM_RC_LOCALITY",
      0x008: "TPM_RC_YIELDED",
      0x009: "TPM_RC_CANCELED",
      0x00A: "TPM_RC_TESTING",
      0x010: "TPM_RC_REFERENCE_H0",
      0x011: "TPM_RC_REFERENCE_H1",
      0x012: "TPM_RC_REFERENCE_H2",
      0x013: "TPM_RC_REFERENCE_H3",
      0x014: "TPM_RC_REFERENCE_H4",
      0x015: "TPM_RC_REFERENCE_H5",
      0x016: "TPM_RC_REFERENCE_H6",
      0x018: "TPM_RC_REFERENCE_S0",
      0x019: "TPM_RC_REFERENCE_S1",
      0x01A: "TPM_RC_REFERENCE_S2",
      0x01B: "TPM_RC_REFERENCE_S3",
      0x01C: "TPM_RC_REFERENCE_S4",
      0x01D: "TPM_RC_REFERENCE_S5",
      0x01E: "TPM_RC_REFERENCE_S6",
      0x020: "TPM_RC_NV_RATE",
      0x021: "TPM_RC_LOCKOUT",
      0x022: "TPM_RC_RETRY",
      0x023: "TPM_RC_NV_UNAVAILABLE",
    }

    # Check for TPM_RC_SUCCESS.
    if rc_code == 0x00:
      return ('Success', 'None', 0, 'TPM_RC_SUCCESS', 'NA')

    # Check for TPM 1.2 response.
    if not (rc_code & (0b11 << 7)):
      return ('Tpm1.2 Response', 'None', 0, 0, 'NA')

    # Check bit 7.
    if not (rc_code & (1 << 7)):
      # Check bit 10.
      if (rc_code & (1 << 10)):
        return ('Vendor Defined Code', 'None', 0, 0, 'NA')

      # At this point the code will be in [6:0]...
      code = rc_code & 0b1111111

      # Check bit 11.
      if (rc_code & (1 << 11)):
        return ('Warning', 'None', 0, warnings[code], 'NA') # TODO: Complete this.
      else:
        return ('Error', 'None', 0, code, generic_errors[code])

    # At this point the code will always be in [5:0]...
    code = rc_code & 0b111111

    # Check bit 6.
    if (rc_code & (1 << 6)):
      number = (rc_code >> 8) & 0b1111
      return ('Error', 'Parameter', number, code, 'NA') # TODO: Complete this.

    # At this point the nubmer will always be in [10:8]...
    number = (rc_code >> 8) & 0b111

    # Check bit 11.
    if not (rc_code & (1 << 11)):
      return ('Error', 'Handle', number, code, handle_errors[code]) # TODO: Complete this.
    else:
      return ('Error', 'Session', number, code, 'NA') # TODO: Complete this.

    raise ValueError("Code '0x%x' could not be parsed!" % rc_code)
    return None
