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
    )
{
    return SafeInt32ToUInt64 (Operand, (UINT64*) Result);
}

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
    )
{
    *Result = Operand;
    return RETURN_SUCCESS;
}

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
    )
{
    return SafeInt64ToInt32 ((INT64) Operand, Result);
}

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
    )
{
    return SafeInt64ToUInt32 ((INT64)Operand, Result);
}

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
    )
{
    return SafeUInt64ToUInt32 ((UINT64)Operand, Result);
}

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
    )
{
    return SafeUInt64ToInt64 ((UINT64)Operand, Result);
}

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
    )
{
    *Result = (INTN)Operand;
    return RETURN_SUCCESS;
}

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
    )
{
    return SafeInt64ToUInt64 (Operand, (UINT64*)Result);
}

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
    )
{
    *Result = Operand;
    return RETURN_SUCCESS;
}

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
    )
{
    return SafeUInt64Add ((UINT64)Augend, (UINT64)Addend, (UINT64*)Result);
}

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
    )
{
    return SafeUInt64Sub ((UINT64)Minuend, (UINT64)Subtrahend, (UINT64*)Result);
}

/**
  UINTN multiplication

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
    )
{
    return SafeUInt64Mult ((UINT64)Multiplicand, (UINT64)Multiplier, (UINT64*)Result);
}

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
    )
{
    return SafeInt64Add ((INT64)Augend, (INT64)Addend, (INT64*)Result);
}

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
    )
{
    return SafeInt64Sub ((INT64)Minuend, (INT64)Subtrahend, (INT64*)Result);
}

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
    )
{
    return SafeInt64Mult ((INT64)Multiplicand, (INT64)Multiplier, (INT64*)Result);
}

