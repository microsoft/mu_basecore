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

#include <Base.h>
#include <Library/IntSafeLib.h>


//
// Magnitude of MIN_INT64 as expressed by a UINT64 number.
//
#define MIN_INT64_MAGNITUDE ((((UINT64) - (MIN_INT64 + 1))) + 1)

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
    )
{
    RETURN_STATUS Status;

    if (Operand >= 0)
    {
        *Result = (UINT8)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT8_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand >= 0)
    {
        *Result = (UINT16)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT16_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand >= 0)
    {
        *Result = (UINT32)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT32_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand >= 0)
    {
        *Result = (UINTN)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINTN_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand >= 0)
    {
        *Result = (UINT64)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT64_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand <= MAX_INT8)
    {
        *Result = (INT8)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = INT8_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    return SafeUInt8ToInt8 (Operand, (INT8*)Result);
}

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
    )
{
    RETURN_STATUS Status;

    if ((Operand >= MIN_INT8) && (Operand <= MAX_INT8))
    {
        *Result = (INT8)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = INT8_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    return SafeInt16ToInt8 (Operand, (INT8*)Result);
}

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
    OUT UINT8* Result
    )
{
    RETURN_STATUS Status;

    if ((Operand >= 0) && (Operand <= MAX_UINT8))
    {
        *Result = (UINT8)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT8_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand >= 0)
    {
        *Result = (UINT16)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT16_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand >= 0)
    {
        *Result = (UINT32)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT32_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand >= 0)
    {
        *Result = (UINTN)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINTN_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand >= 0)
    {
        *Result = (UINT64)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT64_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand <= MAX_INT8)
    {
        *Result = (INT8)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = INT8_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    return SafeUInt16ToInt8 (Operand, (INT8*)Result);
}

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
    OUT UINT8* Result
    )
{
    RETURN_STATUS Status;

    if (Operand <= MAX_UINT8)
    {
        *Result = (UINT8)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT8_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand <= MAX_INT16)
    {
        *Result = (INT16)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = INT16_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if ((Operand >= MIN_INT8) && (Operand <= MAX_INT8))
    {
        *Result = (INT8)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = INT8_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    return SafeInt32ToInt8 (Operand, (INT8*)Result);
}

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
    OUT UINT8* Result
    )
{
    RETURN_STATUS Status;

    if ((Operand >= 0) && (Operand <= MAX_UINT8))
    {
        *Result = (UINT8)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT8_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if ((Operand >= MIN_INT16) && (Operand <= MAX_INT16))
    {
        *Result = (INT16)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = INT16_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if ((Operand >= 0) && (Operand <= MAX_UINT16))
    {
        *Result = (UINT16)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT16_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand >= 0)
    {
        *Result = (UINT32)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT32_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand >= 0)
    {
        *Result = (UINT64)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT64_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand <= MAX_INT8)
    {
        *Result = (INT8)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = INT8_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    return SafeUInt32ToInt8 (Operand, (INT8*)Result);
}

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
    OUT UINT8* Result
    )
{
    RETURN_STATUS Status;

    if (Operand <= MAX_UINT8)
    {
        *Result = (UINT8)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT8_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand <= MAX_INT16)
    {
        *Result = (INT16)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = INT16_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand <= MAX_UINT16)
    {
        *Result = (UINT16)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT16_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand <= MAX_INT32)
    {
        *Result = (INT32)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = INT32_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if ((Operand >= MIN_INT8) && (Operand <= MAX_INT8))
    {
        *Result = (INT8)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = INT8_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    return SafeIntNToInt8 (Operand, (INT8*)Result);
}

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
    OUT UINT8* Result
    )
{
    RETURN_STATUS Status;

    if ((Operand >= 0) && (Operand <= MAX_UINT8))
    {
        *Result = (UINT8)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT8_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if ((Operand >= MIN_INT16) && (Operand <= MAX_INT16))
    {
        *Result = (INT16)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = INT16_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if ((Operand >= 0) && (Operand <= MAX_UINT16))
    {
        *Result = (UINT16)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT16_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand >= 0)
    {
        *Result = (UINTN)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINTN_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand >= 0)
    {
        *Result = (UINT64)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT64_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand <= MAX_INT8)
    {
        *Result = (INT8)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = INT8_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    return SafeUIntNToInt8 (Operand, (INT8*)Result);
}

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
    OUT UINT8* Result
    )
{
    RETURN_STATUS Status;

    if (Operand <= MAX_UINT8)
    {
        *Result = (UINT8)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT8_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand <= MAX_INT16)
    {
        *Result = (INT16)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = INT16_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand <= MAX_UINT16)
    {
        *Result = (UINT16)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT16_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand <= MAX_INT32)
    {
        *Result = (INT32)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = INT32_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand <= MAX_INTN)
    {
        *Result = (INTN)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = INTN_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if ((Operand >= MIN_INT8) && (Operand <= MAX_INT8))
    {
        *Result = (INT8)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = INT8_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    return SafeInt64ToInt8 (Operand, (INT8*)Result);
}

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
    )
{
    RETURN_STATUS Status;

    if ((Operand >= 0) && (Operand <= MAX_UINT8))
    {
        *Result = (UINT8)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT8_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if ((Operand >= MIN_INT16) && (Operand <= MAX_INT16))
    {
        *Result = (INT16)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = INT16_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if ((Operand >= 0) && (Operand <= MAX_UINT16))
    {
        *Result = (UINT16)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT16_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if ((Operand >= MIN_INT32) && (Operand <= MAX_INT32))
    {
        *Result = (INT32)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = INT32_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if ((Operand >= 0) && (Operand <= MAX_UINT32))
    {
        *Result = (UINT32)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT32_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand >= 0)
    {
        *Result = (UINT64)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT64_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand <= MAX_INT8)
    {
        *Result = (INT8)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = INT8_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    return SafeUInt64ToInt8 (Operand, (INT8*)Result);
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand <= MAX_UINT8)
    {
        *Result = (UINT8)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT8_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand <= MAX_INT16)
    {
        *Result = (INT16)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = INT16_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand <= MAX_UINT16)
    {
        *Result = (UINT16)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT16_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand <= MAX_INT32)
    {
        *Result = (INT32)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = INT32_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand <= MAX_UINT32)
    {
        *Result = (UINT32)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT32_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand <= MAX_INTN)
    {
        *Result = (INTN)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = INTN_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Operand <= MAX_INT64)
    {
        *Result = (INT64)Operand;
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = INT64_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (((UINT8)(Augend + Addend)) >= Augend)
    {
        *Result = (UINT8)(Augend + Addend);
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT8_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (((UINT16)(Augend + Addend)) >= Augend)
    {
        *Result = (UINT16)(Augend + Addend);
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT16_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if ((Augend + Addend) >= Augend)
    {
        *Result = (Augend + Addend);
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT32_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if ((Augend + Addend) >= Augend)
    {
        *Result = (Augend + Addend);
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT64_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Minuend >= Subtrahend)
    {
        *Result = (UINT8)(Minuend - Subtrahend);
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT8_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Minuend >= Subtrahend)
    {
        *Result = (UINT16)(Minuend - Subtrahend);
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT16_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Minuend >= Subtrahend)
    {
        *Result = (Minuend - Subtrahend);
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT32_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    RETURN_STATUS Status;

    if (Minuend >= Subtrahend)
    {
        *Result = (Minuend - Subtrahend);
        Status = RETURN_SUCCESS;
    }
    else
    {
        *Result = UINT64_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }

    return Status;
}

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
    )
{
    UINT32 IntermediateResult;

    IntermediateResult = ((UINT32)Multiplicand) * ((UINT32)Multiplier);

    return SafeUInt32ToUInt8 (IntermediateResult, Result);
}

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
    )
{
    UINT32 IntermediateResult;

    IntermediateResult = ((UINT32)Multiplicand) * ((UINT32)Multiplier);

    return SafeUInt32ToUInt16 (IntermediateResult, Result);
}

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
    )
{
    UINT64 IntermediateResult;

    IntermediateResult = ((UINT64) Multiplicand) * ((UINT64) Multiplier);

    return SafeUInt64ToUInt32 (IntermediateResult, Result);
}

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
    )
{
    RETURN_STATUS Status;
    UINT32 DwordA;
    UINT32 DwordB;
    UINT32 DwordC;
    UINT32 DwordD;
    UINT64 ProductAD;
    UINT64 ProductBC;
    UINT64 ProductBD;
    UINT64 UnsignedResult;

    ProductAD = 0;
    ProductBC = 0;
    ProductBD = 0;
    UnsignedResult = 0;
    Status = RETURN_BUFFER_TOO_SMALL;

    //
    // 64x64 into 128 is like 32.32 x 32.32.
    //
    // a.b * c.d = a*(c.d) + .b*(c.d) = a*c + a*.d + .b*c + .b*.d
    // back in non-decimal notation where A=a*2^32 and C=c*2^32:
    // A*C + A*d + b*C + b*d
    // So there are four components to add together.
    //   result = (a*c*2^64) + (a*d*2^32) + (b*c*2^32) + (b*d)
    //
    // a * c must be 0 or there would be bits in the high 64-bits
    // a * d must be less than 2^32 or there would be bits in the high 64-bits
    // b * c must be less than 2^32 or there would be bits in the high 64-bits
    // then there must be no overflow of the resulting values summed up.
    //
    DwordA = (UINT32)(Multiplicand >> 32);
    DwordC = (UINT32)(Multiplier >> 32);

    //
    // common case -- if high dwords are both zero, no chance for overflow
    //
    if ((DwordA == 0) && (DwordC == 0))
    {
        DwordB = (UINT32)Multiplicand;
        DwordD = (UINT32)Multiplier;

        *Result = (((UINT64)DwordB) * (UINT64)DwordD);
        Status = RETURN_SUCCESS;
    }
    else
    {
        //
        // a * c must be 0 or there would be bits set in the high 64-bits
        //
        if ((DwordA == 0) ||
            (DwordC == 0))
        {
            DwordD = (UINT32)Multiplier;

            //
            // a * d must be less than 2^32 or there would be bits set in the high 64-bits
            //
            ProductAD = (((UINT64)DwordA) * (UINT64)DwordD);
            if ((ProductAD & 0xffffffff00000000) == 0)
            {
                DwordB = (UINT32)Multiplicand;

                //
                // b * c must be less than 2^32 or there would be bits set in the high 64-bits
                //
                ProductBC = (((UINT64)DwordB) * (UINT64)DwordC);
                if ((ProductBC & 0xffffffff00000000) == 0)
                {
                    //
                    // now sum them all up checking for overflow.
                    // shifting is safe because we already checked for overflow above
                    //
                    if (!RETURN_ERROR (SafeUInt64Add (ProductBC << 32, ProductAD << 32, &UnsignedResult)))
                    {
                        //
                        // b * d
                        //
                        ProductBD = (((UINT64)DwordB) * (UINT64)DwordD);

                        if (!RETURN_ERROR (SafeUInt64Add (UnsignedResult, ProductBD, &UnsignedResult)))
                        {
                            *Result = UnsignedResult;
                            Status = RETURN_SUCCESS;
                        }
                    }
                }
            }
        }
    }

    if (RETURN_ERROR (Status))
    {
        *Result = UINT64_ERROR;
    }
    return Status;
}

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
    )
{
    return SafeInt32ToInt8 (((INT32)Augend) + ((INT32)Addend), Result);
}

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
    )
{
    return SafeInt32ToInt16 (((INT32)Augend) + ((INT32)Addend), Result);
}

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
    )
{
    return SafeInt64ToInt32 (((INT64)Augend) + ((INT64)Addend), Result);
}

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
    )
{
    RETURN_STATUS Status;
    INT64 SignedResult;

    SignedResult = Augend + Addend;

    //
    // Adding positive to negative never overflows.
    // If you add two positive numbers, you expect a positive result.
    // If you add two negative numbers, you expect a negative result.
    // Overflow if inputs are the same sign and output is not that sign.
    //
    if (((Augend < 0) == (Addend < 0))  &&
        ((Augend < 0) != (SignedResult < 0)))
    {
        *Result = INT64_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }
    else
    {
        *Result = SignedResult;
        Status = RETURN_SUCCESS;
    }

    return Status;
}

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
    )
{
    return SafeInt32ToInt8 (((INT32)Minuend) - ((INT32)Subtrahend), Result);
}

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
    )
{
    return SafeInt32ToInt16 (((INT32)Minuend) - ((INT32)Subtrahend), Result);
}

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
    )
{
    return SafeInt64ToInt32 (((INT64)Minuend) - ((INT64)Subtrahend), Result);
}

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
    )
{
    RETURN_STATUS Status;
    INT64 SignedResult;

    SignedResult = Minuend - Subtrahend;

    //
    // Subtracting a positive number from a positive number never overflows.
    // Subtracting a negative number from a negative number never overflows.
    // If you subtract a negative number from a positive number, you expect a positive result.
    // If you subtract a positive number from a negative number, you expect a negative result.
    // Overflow if inputs vary in sign and the output does not have the same sign as the first input.
    //
    if (((Minuend < 0) != (Subtrahend < 0)) &&
        ((Minuend < 0) != (SignedResult < 0)))
    {
        *Result = INT64_ERROR;
        Status = RETURN_BUFFER_TOO_SMALL;
    }
    else
    {
        *Result = SignedResult;
        Status = RETURN_SUCCESS;
    }

    return Status;
}

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
    )
{
    return SafeInt32ToInt8 (((INT32)Multiplier) * ((INT32)Multiplicand), Result);
}

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
    )
{
    return SafeInt32ToInt16 (((INT32)Multiplicand) * ((INT32)Multiplier), Result);
}

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
    )
{
    return SafeInt64ToInt32 (((INT64)Multiplicand) * ((INT64)Multiplier), Result);
}

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
    )
{
    RETURN_STATUS Status;
    UINT64 UnsignedMultiplicand;
    UINT64 UnsignedMultiplier;
    UINT64 UnsignedResult;

    //
    // Split into sign and magnitude, do unsigned operation, apply sign.
    //
    if (Multiplicand < 0)
    {
        //
        // Avoid negating the most negative number.
        //
        UnsignedMultiplicand = ((UINT64)(- (Multiplicand + 1))) + 1;
    }
    else
    {
        UnsignedMultiplicand = (UINT64)Multiplicand;
    }

    if (Multiplier < 0)
    {
        //
        // Avoid negating the most negative number.
        //
        UnsignedMultiplier = ((UINT64)(- (Multiplier + 1))) + 1;
    }
    else
    {
        UnsignedMultiplier = (UINT64)Multiplier;
    }

    Status = SafeUInt64Mult (UnsignedMultiplicand, UnsignedMultiplier, &UnsignedResult);
    if (!RETURN_ERROR (Status))
    {
        if ((Multiplicand < 0) != (Multiplier < 0))
        {
            if (UnsignedResult > MIN_INT64_MAGNITUDE)
            {
                *Result = INT64_ERROR;
                Status = RETURN_BUFFER_TOO_SMALL;
            }
            else
            {
                *Result = - ((INT64)UnsignedResult);
            }
        }
        else
        {
            if (UnsignedResult > MAX_INT64)
            {
                *Result = INT64_ERROR;
                Status = RETURN_BUFFER_TOO_SMALL;
            }
            else
            {
                *Result = (INT64)UnsignedResult;
            }
        }
    }
    else
    {
        *Result = INT64_ERROR;
    }
    return Status;
}

