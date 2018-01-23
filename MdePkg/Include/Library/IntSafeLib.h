/**

Copyright (c) 2017, Microsoft Corporation

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

This library provides helper functions to prevent integer overflow during
type conversion, addition, subtraction, and multiplication.

**/
#ifndef __INT_SAFE_LIB_H__
#define __INT_SAFE_LIB_H__

//
// It is common for -1 to be used as an error value
//
#define INT8_ERROR    ((INT8) -1)
#define UINT8_ERROR   MAX_UINT8
#define INT16_ERROR   ((INT16) -1)
#define UINT16_ERROR  MAX_UINT16
#define CHAR16_ERROR  MAX_UINT16
#define INT32_ERROR   ((INT32) -1)
#define UINT32_ERROR  MAX_UINT32
#define INT64_ERROR   ((INT64) -1)
#define UINT64_ERROR  MAX_UINT64
#define INTN_ERROR    ((INTN) -1)
#define UINTN_ERROR   MAX_UINTN

//
// CHAR16 is defined to be the same as UINT16, so for CHAR16
// operations redirect to the UINT16 ones:
//
#define SafeInt8ToChar16    SafeInt8ToUInt16
#define SafeUInt64ToChar16  SafeUInt64ToUInt16
#define SafeInt64ToChar16   SafeInt64ToUInt16
#define SafeUIntNToChar16   SafeUIntNToUInt16
#define SafeIntNToChar16    SafeIntNToUInt16
#define SafeUInt32ToChar16  SafeUInt32ToUInt16
#define SafeChar16ToChar8   SafeUInt16ToChar8
#define SafeChar16ToInt8    SafeUInt16ToInt8
#define SafeChar16ToUInt8   SafeUInt16ToUInt8
#define SafeChar16ToInt16   SafeUInt16ToInt16
#define SafeInt16ToChar16   SafeInt16ToUInt16
#define SafeInt32ToChar16   SafeInt32ToUInt16
#define SafeChar16Mult      SafeUInt16Mult
#define SafeChar16Sub       SafeUInt16Sub
#define SafeChar16Add       SafeUInt16Add

//
// Conversion functions
//
// There are three reasons for having conversion functions:
//
// 1. We are converting from a signed type to an unsigned type of the same
//    size, or vice-versa.
//
// 2. We are converting to a smaller type, and we could therefore possibly
//    overflow.
//
// 3. We are converting to a bigger type, and we are signed and the type we are
//    converting to is unsigned.
//

/**
  INT8 -> UINT8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt8ToUInt8 (
    IN INT8 Operand,
    OUT UINT8* Result
    );

/**
  INT8 -> UINT16 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt8ToUInt16 (
    IN INT8 Operand,
    OUT UINT16* Result
    );

/**
  INT8 -> UINT32 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt8ToUInt32 (
    IN INT8 Operand,
    OUT UINT32* Result
    );

/**
  INT8 -> UINTN conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt8ToUIntN (
    IN INT8 Operand,
    OUT UINTN* Result
    );

/**
  INT8 -> UINT64 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt8ToUInt64 (
    IN INT8 Operand,
    OUT UINT64* Result
    );

/**
  UINT8 -> INT8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt8ToInt8 (
    IN UINT8 Operand,
    OUT INT8* Result
    );

/**
  UINT8 -> CHAR8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt8ToChar8 (
    IN UINT8 Operand,
    OUT CHAR8* Result
    );

/**
  INT16 -> INT8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt16ToInt8 (
    IN INT16 Operand,
    OUT INT8* Result
    );

/**
  INT16 -> CHAR8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt16ToChar8 (
    IN INT16 Operand,
    OUT CHAR8* Result
    );

/**
  INT16 -> UINT8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt16ToUInt8 (
    IN INT16 Operand,
    OUT UINT8* pui8Result
    );

/**
  INT16 -> UINT16 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt16ToUInt16 (
    IN INT16 Operand,
    OUT UINT16* Result
    );

/**
  INT16 -> UINT32 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt16ToUInt32 (
    IN INT16 Operand,
    OUT UINT32* Result
    );

/**
  INT16 -> UINTN conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt16ToUIntN (
    IN INT16 Operand,
    OUT UINTN* Result
    );

/**
  INT16 -> UINT64 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt16ToUInt64 (
    IN INT16 Operand,
    OUT UINT64* Result
    );

/**
  UINT16 -> INT8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt16ToInt8 (
    IN UINT16 Operand,
    OUT INT8* Result
    );

/**
  UINT16 -> CHAR8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt16ToChar8 (
    IN UINT16 Operand,
    OUT CHAR8* Result
    );

/**
  UINT16 -> UINT8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt16ToUInt8 (
    IN UINT16 Operand,
    OUT UINT8* pui8Result
    );

/**
  UINT16 -> INT16 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt16ToInt16 (
    IN UINT16 Operand,
    OUT INT16* Result
    );

/**
  INT32 -> INT8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt32ToInt8 (
    IN INT32 Operand,
    OUT INT8* Result
    );

/**
  INT32 -> CHAR8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt32ToChar8 (
    IN INT32 Operand,
    OUT CHAR8* Result
    );

/**
  INT32 -> UINT8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt32ToUInt8 (
    IN INT32 Operand,
    OUT UINT8* pui8Result
    );

/**
  INT32 -> INT16 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt32ToInt16 (
    IN INT32 Operand,
    OUT INT16* Result
    );

/**
  INT32 -> UINT16 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt32ToUInt16 (
    IN INT32 Operand,
    OUT UINT16* Result
    );


/**
  INT32 -> UINT32 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt32ToUInt32 (
    IN INT32 Operand,
    OUT UINT32* Result
    );

/**
  INT32 -> UINTN conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt32ToUIntN (
    IN INT32 Operand,
    OUT UINTN* Result
    );

/**
  INT32 -> UINT64 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt32ToUInt64 (
    IN INT32 Operand,
    OUT UINT64* Result
    );

/**
  UINT32 -> INT8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt32ToInt8 (
    IN UINT32 Operand,
    OUT INT8* Result
    );

/**
  UINT32 -> CHAR8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt32ToChar8 (
    IN UINT32 Operand,
    OUT CHAR8* Result
    );

/**
  UINT32 -> UINT8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt32ToUInt8 (
    IN UINT32 Operand,
    OUT UINT8* pui8Result
    );

/**
  UINT32 -> INT16 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt32ToInt16 (
    IN UINT32 Operand,
    OUT INT16* Result
    );

/**
  UINT32 -> UINT16 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt32ToUInt16 (
    IN UINT32 Operand,
    OUT UINT16* Result
    );

/**
  UINT32 -> INT32 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt32ToInt32 (
    IN UINT32 Operand,
    OUT INT32* Result
    );

/**
  UINT32 -> INTN conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt32ToIntN (
    IN UINT32 Operand,
    OUT INTN* Result
    );

/**
  INTN -> INT8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeIntNToInt8 (
    IN INTN Operand,
    OUT INT8* Result
    );

/**
  INTN -> CHAR8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeIntNToChar8 (
    IN INTN Operand,
    OUT CHAR8* Result
    );

/**
  INTN -> UINT8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeIntNToUInt8 (
    IN INTN Operand,
    OUT UINT8* pui8Result
    );

/**
  INTN -> INT16 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeIntNToInt16 (
    IN INTN Operand,
    OUT INT16* Result
    );

/**
  INTN -> UINT16 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeIntNToUInt16 (
    IN INTN Operand,
    OUT UINT16* Result
    );

/**
  INTN -> INT32 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeIntNToInt32 (
    IN INTN Operand,
    OUT INT32* Result
    );

/**
  INTN -> UINT32 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeIntNToUInt32 (
    IN INTN Operand,
    OUT UINT32* Result
    );

/**
  INTN -> UINTN conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeIntNToUIntN (
    IN INTN Operand,
    OUT UINTN* Result
    );

/**
  INTN -> UINT64 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeIntNToUInt64 (
    IN INTN Operand,
    OUT UINT64* Result
    );

/**
  UINTN -> INT8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUIntNToInt8 (
    IN UINTN Operand,
    OUT INT8* Result
    );

/**
  UINTN -> CHAR8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUIntNToChar8 (
    IN UINTN Operand,
    OUT CHAR8* Result
    );

/**
  UINTN -> UINT8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUIntNToUInt8 (
    IN UINTN Operand,
    OUT UINT8* pui8Result
    );

/**
  UINTN -> INT16 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUIntNToInt16 (
    IN UINTN Operand,
    OUT INT16* Result
    );

/**
  UINTN -> UINT16 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUIntNToUInt16 (
    IN UINTN Operand,
    OUT UINT16* Result
    );

/**
  UINTN -> INT32 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUIntNToInt32 (
    IN UINTN Operand,
    OUT INT32* Result
    );

/**
  UINTN -> UINT32 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUIntNToUInt32 (
    IN UINTN Operand,
    OUT UINT32* Result
    );

/**
  UINTN -> INTN conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUIntNToIntN (
    IN UINTN Operand,
    OUT INTN* Result
    );

/**
  UINTN -> INT64 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUIntNToInt64 (
    IN UINTN Operand,
    OUT INT64* Result
    );

/**
  INT64 -> INT8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt64ToInt8 (
    IN INT64 Operand,
    OUT INT8* Result
    );

/**
  INT64 -> CHAR8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt64ToChar8 (
    IN INT64 Operand,
    OUT CHAR8* Result
    );

/**
  INT64 -> UINT8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt64ToUInt8 (
    IN INT64 Operand,
    OUT UINT8* Result
    );

/**
  INT64 -> INT16 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt64ToInt16 (
    IN INT64 Operand,
    OUT INT16* Result
    );

/**
  INT64 -> UINT16 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt64ToUInt16 (
    IN INT64 Operand,
    OUT UINT16* Result
    );

/**
  INT64 -> INT32 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt64ToInt32 (
    IN INT64 Operand,
    OUT INT32* Result
    );

/**
  INT64 -> UINT32 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt64ToUInt32 (
    IN INT64 Operand,
    OUT UINT32* Result
    );

/**
  INT64 -> INTN conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt64ToIntN (
    IN INT64 Operand,
    OUT INTN* Result
    );

/**
  INT64 -> UINTN conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt64ToUIntN (
    IN INT64 Operand,
    OUT UINTN* Result
    );

/**
  INT64 -> UINT64 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt64ToUInt64 (
    IN INT64 Operand,
    OUT UINT64* Result
    );

/**
  UINT64 -> INT8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt64ToInt8 (
    IN UINT64 Operand,
    OUT INT8* Result
    );

/**
  UINT64 -> CHAR8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt64ToChar8 (
    IN UINT64 Operand,
    OUT CHAR8* Result
    );

/**
  UINT64 -> UINT8 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt64ToUInt8 (
    IN UINT64 Operand,
    OUT UINT8* Result
    );

/**
  UINT64 -> INT16 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt64ToInt16 (
    IN UINT64 Operand,
    OUT INT16* Result
    );

/**
  UINT64 -> UINT16 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt64ToUInt16 (
    IN UINT64 Operand,
    OUT UINT16* Result
    );

/**
  UINT64 -> INT32 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt64ToInt32 (
    IN UINT64 Operand,
    OUT INT32* Result
    );

/**
  UINT64 -> UINT32 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt64ToUInt32 (
    IN UINT64 Operand,
    OUT UINT32* Result
    );

/**
  UINT64 -> INTN conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt64ToIntN (
    IN UINT64 Operand,
    OUT INTN* Result
    );

/**
  UINT64 -> UINTN conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt64ToUIntN (
    IN UINT64 Operand,
    OUT UINTN* Result
    );

/**
  UINT64 -> INT64 conversion

  @param[in]  Operand - Operand to be converted to new type
  @param[out] Result  - Pointer to the result of conversion

  @retval RETURN_SUCCESS          - Successful conversion
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt64ToInt64 (
    IN UINT64 Operand,
    OUT INT64* Result
    );

//
// Addition functions
//

/**
  UINT8 addition

  @param[in]  Augend - A number to which addend will be added
  @param[in]  Addend - A number to be added to another
  @param[out] Result - Pointer to the result of addition

  @retval RETURN_SUCCESS          - Successful addition
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt8Add (
    IN UINT8 Augend,
    IN UINT8 Addend,
    OUT UINT8* Result
    );

/**
  UINT16 addition

  @param[in]  Augend - A number to which addend will be added
  @param[in]  Addend - A number to be added to another
  @param[out] Result - Pointer to the result of addition

  @retval RETURN_SUCCESS          - Successful addition
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt16Add (
    IN UINT16 Augend,
    IN UINT16 Addend,
    OUT UINT16* Result
    );

/**
  UINT32 addition

  @param[in]  Augend - A number to which addend will be added
  @param[in]  Addend - A number to be added to another
  @param[out] Result - Pointer to the result of addition

  @retval RETURN_SUCCESS          - Successful addition
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt32Add (
    IN UINT32 Augend,
    IN UINT32 Addend,
    OUT UINT32* Result
    );

/**
  UINTN addition

  @param[in]  Augend - A number to which addend will be added
  @param[in]  Addend - A number to be added to another
  @param[out] Result - Pointer to the result of addition

  @retval RETURN_SUCCESS          - Successful addition
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUIntNAdd (
    IN UINTN Augend,
    IN UINTN Addend,
    OUT UINTN* Result
    );

/**
  UINT64 addition

  @param[in]  Augend - A number to which addend will be added
  @param[in]  Addend - A number to be added to another
  @param[out] Result - Pointer to the result of addition

  @retval RETURN_SUCCESS          - Successful addition
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt64Add (
    IN UINT64 Augend,
    IN UINT64 Addend,
    OUT UINT64* Result
    );

//
// Subtraction functions
//

/**
  UINT8 subtraction

  @param[in]  Minuend    - A number from which another is to be subtracted.
  @param[in]  Subtrahend - A number to be subtracted from another
  @param[out] Result     - Pointer to the result of subtraction

  @retval RETURN_SUCCESS          - Successful subtraction
  @retval RETURN_BUFFER_TOO_SMALL - Underflow
**/
RETURN_STATUS
EFIAPI
SafeUInt8Sub (
    IN UINT8 Minuend,
    IN UINT8 Subtrahend,
    OUT UINT8* Result
    );

/**
  UINT16 subtraction

  @param[in]  Minuend    - A number from which another is to be subtracted.
  @param[in]  Subtrahend - A number to be subtracted from another
  @param[out] Result     - Pointer to the result of subtraction

  @retval RETURN_SUCCESS          - Successful subtraction
  @retval RETURN_BUFFER_TOO_SMALL - Underflow
**/
RETURN_STATUS
EFIAPI
SafeUInt16Sub (
    IN UINT16 Minuend,
    IN UINT16 Subtrahend,
    OUT UINT16* Result
    );

/**
  UINT32 subtraction

  @param[in]  Minuend    - A number from which another is to be subtracted.
  @param[in]  Subtrahend - A number to be subtracted from another
  @param[out] Result     - Pointer to the result of subtraction

  @retval RETURN_SUCCESS          - Successful subtraction
  @retval RETURN_BUFFER_TOO_SMALL - Underflow
**/
RETURN_STATUS
EFIAPI
SafeUInt32Sub (
    IN UINT32 Minuend,
    IN UINT32 Subtrahend,
    OUT UINT32* Result
    );

/**
  UINTN subtraction

  @param[in]  Minuend    - A number from which another is to be subtracted.
  @param[in]  Subtrahend - A number to be subtracted from another
  @param[out] Result     - Pointer to the result of subtraction

  @retval RETURN_SUCCESS          - Successful subtraction
  @retval RETURN_BUFFER_TOO_SMALL - Underflow
**/
RETURN_STATUS
EFIAPI
SafeUIntNSub (
    IN UINTN Minuend,
    IN UINTN Subtrahend,
    OUT UINTN* Result
    );

/**
  UINT64 subtraction

  @param[in]  Minuend    - A number from which another is to be subtracted.
  @param[in]  Subtrahend - A number to be subtracted from another
  @param[out] Result     - Pointer to the result of subtraction

  @retval RETURN_SUCCESS          - Successful subtraction
  @retval RETURN_BUFFER_TOO_SMALL - Underflow
**/
RETURN_STATUS
EFIAPI
SafeUInt64Sub (
    IN UINT64 Minuend,
    IN UINT64 Subtrahend,
    OUT UINT64* Result
    );

//
// Multiplication functions
//

/**
  UINT8 multiplication

  @param[in]  Multiplicand - A number that is to be multiplied by another
  @param[in]  Multiplier   - A number by which the multiplicand is to be multiplied
  @param[out] Result       - Pointer to the result of multiplication

  @retval RETURN_SUCCESS          - Successful multiplication
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt8Mult (
    IN UINT8 Multiplicand,
    IN UINT8 Multiplier,
    OUT UINT8* Result
    );

/**
  UINT16 multiplication

  @param[in]  Multiplicand - A number that is to be multiplied by another
  @param[in]  Multiplier   - A number by which the multiplicand is to be multiplied
  @param[out] Result       - Pointer to the result of multiplication

  @retval RETURN_SUCCESS          - Successful multiplication
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt16Mult (
    IN UINT16 Multiplicand,
    IN UINT16 Multiplier,
    OUT UINT16* Result
    );

/**
  UINT32 multiplication

  @param[in]  Multiplicand - A number that is to be multiplied by another
  @param[in]  Multiplier   - A number by which the multiplicand is to be multiplied
  @param[out] Result       - Pointer to the result of multiplication

  @retval RETURN_SUCCESS          - Successful multiplication
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt32Mult (
    IN UINT32 Multiplicand,
    IN UINT32 Multiplier,
    OUT UINT32* Result
    );

/**
// UINTN multiplication

  @param[in]  Multiplicand - A number that is to be multiplied by another
  @param[in]  Multiplier   - A number by which the multiplicand is to be multiplied
  @param[out] Result       - Pointer to the result of multiplication

  @retval RETURN_SUCCESS          - Successful multiplication
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUIntNMult (
    IN UINTN Multiplicand,
    IN UINTN Multiplier,
    OUT UINTN* Result
    );

/**
  UINT64 multiplication

  @param[in]  Multiplicand - A number that is to be multiplied by another
  @param[in]  Multiplier   - A number by which the multiplicand is to be multiplied
  @param[out] Result       - Pointer to the result of multiplication

  @retval RETURN_SUCCESS          - Successful multiplication
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeUInt64Mult (
    IN UINT64 Multiplicand,
    IN UINT64 Multiplier,
    OUT UINT64* Result
    );

//
// Signed operations
//
// Strongly consider using unsigned numbers.
//
// Signed numbers are often used where unsigned numbers should be used.
// For example file sizes and array indices should always be unsigned.
// Subtracting a larger positive signed number from a smaller positive
// signed number with SafeInt32Sub will succeed, producing a negative number,
// that then must not be used as an array index (but can occasionally be
// used as a pointer index.) Similarly for adding a larger magnitude
// negative number to a smaller magnitude positive number.
//
// This library does not protect you from such errors. It tells you if your
// integer operations overflowed, not if you are doing the right thing
// with your non-overflowed integers.
//
// Likewise you can overflow a buffer with a non-overflowed unsigned index.
//

//
// Signed addition functions
//

/**
  INT8 Addition

  @param[in]  Augend - A number to which addend will be added
  @param[in]  Addend - A number to be added to another
  @param[out] Result - Pointer to the result of addition

  @retval RETURN_SUCCESS          - Successful addition
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt8Add (
    IN INT8 Augend,
    IN INT8 Addend,
    OUT INT8* Result
    );

/**
  INT16 Addition

  @param[in]  Augend - A number to which addend will be added
  @param[in]  Addend - A number to be added to another
  @param[out] Result - Pointer to the result of addition

  @retval RETURN_SUCCESS          - Successful addition
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt16Add (
    IN INT16 Augend,
    IN INT16 Addend,
    OUT INT16* Result
    );

/**
  INT32 Addition

  @param[in]  Augend - A number to which addend will be added
  @param[in]  Addend - A number to be added to another
  @param[out] Result - Pointer to the result of addition

  @retval RETURN_SUCCESS          - Successful addition
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt32Add (
    IN INT32 Augend,
    IN INT32 Addend,
    OUT INT32* Result
    );

/**
  INTN Addition

  @param[in]  Augend - A number to which addend will be added
  @param[in]  Addend - A number to be added to another
  @param[out] Result - Pointer to the result of addition

  @retval RETURN_SUCCESS          - Successful addition
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeIntNAdd (
    IN INTN Augend,
    IN INTN Addend,
    OUT INTN* Result
    );

/**
  INT64 Addition

  @param[in]  Augend - A number to which addend will be added
  @param[in]  Addend - A number to be added to another
  @param[out] Result - Pointer to the result of addition

  @retval RETURN_SUCCESS          - Successful addition
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt64Add (
    IN INT64 Augend,
    IN INT64 Addend,
    OUT INT64* Result
    );

//
// Signed subtraction functions
//

/**
  INT8 Subtraction

  @param[in]  Minuend    - A number from which another is to be subtracted.
  @param[in]  Subtrahend - A number to be subtracted from another
  @param[out] Result     - Pointer to the result of subtraction

  @retval RETURN_SUCCESS          - Successful subtraction
  @retval RETURN_BUFFER_TOO_SMALL - Underflow
**/
RETURN_STATUS
EFIAPI
SafeInt8Sub (
    IN INT8 Minuend,
    IN INT8 Subtrahend,
    OUT INT8* Result
    );

/**
  INT16 Subtraction

  @param[in]  Minuend    - A number from which another is to be subtracted.
  @param[in]  Subtrahend - A number to be subtracted from another
  @param[out] Result     - Pointer to the result of subtraction

  @retval RETURN_SUCCESS          - Successful subtraction
  @retval RETURN_BUFFER_TOO_SMALL - Underflow
**/
RETURN_STATUS
EFIAPI
SafeInt16Sub (
    IN INT16 Minuend,
    IN INT16 Subtrahend,
    OUT INT16* Result
    );

/**
  INT32 Subtraction

  @param[in]  Minuend    - A number from which another is to be subtracted.
  @param[in]  Subtrahend - A number to be subtracted from another
  @param[out] Result     - Pointer to the result of subtraction

  @retval RETURN_SUCCESS          - Successful subtraction
  @retval RETURN_BUFFER_TOO_SMALL - Underflow
**/
RETURN_STATUS
EFIAPI
SafeInt32Sub (
    IN INT32 Minuend,
    IN INT32 Subtrahend,
    OUT INT32* Result
    );

/**
  INTN Subtraction

  @param[in]  Minuend    - A number from which another is to be subtracted.
  @param[in]  Subtrahend - A number to be subtracted from another
  @param[out] Result     - Pointer to the result of subtraction

  @retval RETURN_SUCCESS          - Successful subtraction
  @retval RETURN_BUFFER_TOO_SMALL - Underflow
**/
RETURN_STATUS
EFIAPI
SafeIntNSub (
    IN INTN Minuend,
    IN INTN Subtrahend,
    OUT INTN* Result
    );

/**
  INT64 Subtraction

  @param[in]  Minuend    - A number from which another is to be subtracted.
  @param[in]  Subtrahend - A number to be subtracted from another
  @param[out] Result     - Pointer to the result of subtraction

  @retval RETURN_SUCCESS          - Successful subtraction
  @retval RETURN_BUFFER_TOO_SMALL - Underflow
**/
RETURN_STATUS
EFIAPI
SafeInt64Sub (
    IN INT64 Minuend,
    IN INT64 Subtrahend,
    OUT INT64* Result
    );

//
// Signed multiplication functions
//

/**
  INT8 multiplication

  @param[in]  Multiplicand - A number that is to be multiplied by another
  @param[in]  Multiplier   - A number by which the multiplicand is to be multiplied
  @param[out] Result       - Pointer to the result of multiplication

  @retval RETURN_SUCCESS          - Successful multiplication
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt8Mult (
    IN INT8 Multiplicand,
    IN INT8 Multiplier,
    OUT INT8* Result
    );

/**
  INT16 multiplication

  @param[in]  Multiplicand - A number that is to be multiplied by another
  @param[in]  Multiplier   - A number by which the multiplicand is to be multiplied
  @param[out] Result       - Pointer to the result of multiplication

  @retval RETURN_SUCCESS          - Successful multiplication
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt16Mult (
    IN INT16 Multiplicand,
    IN INT16 Multiplier,
    OUT INT16* Result
    );

/**
  INT32 multiplication

  @param[in]  Multiplicand - A number that is to be multiplied by another
  @param[in]  Multiplier   - A number by which the multiplicand is to be multiplied
  @param[out] Result       - Pointer to the result of multiplication

  @retval RETURN_SUCCESS          - Successful multiplication
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt32Mult (
    IN INT32 Multiplicand,
    IN INT32 Multiplier,
    OUT INT32* Result
    );

/**
  INTN multiplication

  @param[in]  Multiplicand - A number that is to be multiplied by another
  @param[in]  Multiplier   - A number by which the multiplicand is to be multiplied
  @param[out] Result       - Pointer to the result of multiplication

  @retval RETURN_SUCCESS          - Successful multiplication
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeIntNMult (
    IN INTN Multiplicand,
    IN INTN Multiplier,
    OUT INTN* Result
    );

/**
  INT64 multiplication

  @param[in]  Multiplicand - A number that is to be multiplied by another
  @param[in]  Multiplier   - A number by which the multiplicand is to be multiplied
  @param[out] Result       - Pointer to the result of multiplication

  @retval RETURN_SUCCESS          - Successful multiplication
  @retval RETURN_BUFFER_TOO_SMALL - Overflow
**/
RETURN_STATUS
EFIAPI
SafeInt64Mult (
    IN INT64 Multiplicand,
    IN INT64 Multiplier,
    OUT INT64* Result
    );

#endif // __INT_SAFE_LIB_H__
