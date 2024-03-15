/** @file
  Common Firmware Volume Block (FVB) driver instance functionality.

  Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "FvbVariableStorage.h"

//
// Module globals
//
EDKII_VARIABLE_STORAGE_PROTOCOL  mFvbVariableStorageProtocol = {
  FvbVariableStorageGetId,
  FvbVariableStorageGetVariable,
  FvbVariableStorageGetAuthenticatedVariable,
  FvbVariableStorageGetNextVariableName,
  FvbVariableStorageGetStorageUsage,
  FvbVariableStorageGetAuthenticatedSupport,
  FvbVariableStorageSetVariable,
  FvbVariableStorageWriteServiceIsReady,
  FvbVariableStorageGarbageCollect
};

///
/// FVB Instance for writting to NV Storage
///
EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *mFvbInstance = NULL;

///
/// VSS Protocol for communicating with the UEFI variable driver
///
EDKII_VARIABLE_STORAGE_SUPPORT_PROTOCOL  *mVariableStorageSupport = NULL;

///
/// MMIO address of the FLASH device which contains the NV Storage FV
///
EFI_PHYSICAL_ADDRESS  mNonVolatileVariableBase = (EFI_PHYSICAL_ADDRESS)0;

///
/// TRUE if the NV Storage FV uses the authenticated variable format
///
BOOLEAN  mAuthFormat = FALSE;

///
/// Size of the scratch buffer at the end of mNvVariableCache
///
UINTN  mScratchBufferSize = 0;

///
/// A memory cache that improves the search performance and enables reclaim
///
VARIABLE_STORE_HEADER  *mNvVariableCache = NULL;

/**
  Retrieves a protocol instance-specific GUID.

  Returns a unique GUID per EDKII_VARIABLE_STORAGE_PROTOCOL instance.

  @param[in]       This                   A pointer to this instance of the EDKII_VARIABLE_STORAGE_PROTOCOL.
  @param[out]      VariableGuid           A pointer to an EFI_GUID that is this protocol instance's GUID.

  @retval          EFI_SUCCESS            The data was returned successfully.
  @retval          EFI_INVALID_PARAMETER  A required parameter is NULL.

**/
EFI_STATUS
EFIAPI
FvbVariableStorageGetId (
  IN CONST  EDKII_VARIABLE_STORAGE_PROTOCOL  *This,
  OUT       EFI_GUID                         *InstanceGuid
  )
{
  if (InstanceGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (InstanceGuid, &gFvbVariableStorageProtocolInstanceGuid, sizeof (EFI_GUID));

  return EFI_SUCCESS;
}

/**

  This code checks if variable header is valid.

  @param[in] Variable           Pointer to the Variable Header.
  @param[in] VariableStoreEnd   Pointer to the Variable Store End.

  @retval TRUE              Variable header is valid.
  @retval FALSE             Variable header is not valid.

**/
BOOLEAN
IsValidVariableHeader (
  IN  VARIABLE_HEADER  *Variable,
  IN  VARIABLE_HEADER  *VariableStoreEnd
  )
{
  if ((Variable == NULL) || (Variable >= VariableStoreEnd) || (Variable->StartId != VARIABLE_DATA)) {
    //
    // Variable is NULL or has reached the end of variable store,
    // or the StartId is not correct.
    //
    return FALSE;
  }

  return TRUE;
}

/**
  This function writes data to the FWH at the correct LBA even if the LBAs
  are fragmented.

  @param[in] SetByIndex       TRUE if target pointer is given as index.
                              FALSE if target pointer is absolute.
  @param[in] Fvb              Pointer to the writable FVB protocol.
  @param[in] DataPtrIndex     Pointer to the Data from the end of VARIABLE_STORE_HEADER
                              structure.
  @param[in] DataSize         Size of data to be written.
  @param[in] Buffer           Pointer to the buffer from which data is written.

  @retval EFI_INVALID_PARAMETER  Parameters not valid.
  @retval EFI_SUCCESS            Variable store successfully updated.

**/
EFI_STATUS
UpdateVariableStore (
  IN  BOOLEAN                             SetByIndex,
  IN  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb,
  IN  UINTN                               DataPtrIndex,
  IN  UINT32                              DataSize,
  IN  UINT8                               *Buffer
  )
{
  EFI_FV_BLOCK_MAP_ENTRY      *PtrBlockMapEntry;
  UINTN                       BlockIndex2;
  UINTN                       LinearOffset;
  UINTN                       CurrWriteSize;
  UINTN                       CurrWritePtr;
  UINT8                       *CurrBuffer;
  EFI_LBA                     LbaNumber;
  UINTN                       Size;
  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader;
  EFI_PHYSICAL_ADDRESS        FvVolHdr;
  EFI_PHYSICAL_ADDRESS        DataPtr;
  EFI_STATUS                  Status;

  FwVolHeader = NULL;
  DataPtr     = DataPtrIndex;

  if (Fvb == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = Fvb->GetPhysicalAddress (Fvb, &FvVolHdr);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)((UINTN)FvVolHdr);
  //
  // Data Pointer should point to the actual Address where data is to be
  // written.
  //
  if (SetByIndex) {
    DataPtr += mNonVolatileVariableBase;
  }

  if ((DataPtr + DataSize) >= ((EFI_PHYSICAL_ADDRESS)(UINTN)((UINT8 *)FwVolHeader + FwVolHeader->FvLength))) {
    return EFI_INVALID_PARAMETER;
  }

  LinearOffset  = (UINTN)FwVolHeader;
  CurrWritePtr  = (UINTN)DataPtr;
  CurrWriteSize = DataSize;
  CurrBuffer    = Buffer;
  LbaNumber     = 0;

  if (CurrWritePtr < LinearOffset) {
    return EFI_INVALID_PARAMETER;
  }

  for (PtrBlockMapEntry = FwVolHeader->BlockMap; PtrBlockMapEntry->NumBlocks != 0; PtrBlockMapEntry++) {
    for (BlockIndex2 = 0; BlockIndex2 < PtrBlockMapEntry->NumBlocks; BlockIndex2++) {
      //
      // Check to see if the Variable Writes are spanning through multiple
      // blocks.
      //
      if ((CurrWritePtr >= LinearOffset) && (CurrWritePtr < LinearOffset + PtrBlockMapEntry->Length)) {
        if ((CurrWritePtr + CurrWriteSize) <= (LinearOffset + PtrBlockMapEntry->Length)) {
          Status = Fvb->Write (
                          Fvb,
                          LbaNumber,
                          (UINTN)(CurrWritePtr - LinearOffset),
                          &CurrWriteSize,
                          CurrBuffer
                          );
          return Status;
        } else {
          Size   = (UINT32)(LinearOffset + PtrBlockMapEntry->Length - CurrWritePtr);
          Status = Fvb->Write (
                          Fvb,
                          LbaNumber,
                          (UINTN)(CurrWritePtr - LinearOffset),
                          &Size,
                          CurrBuffer
                          );
          if (EFI_ERROR (Status)) {
            return Status;
          }

          CurrWritePtr  = LinearOffset + PtrBlockMapEntry->Length;
          CurrBuffer    = CurrBuffer + Size;
          CurrWriteSize = CurrWriteSize - Size;
        }
      }

      LinearOffset += PtrBlockMapEntry->Length;
      LbaNumber++;
    }
  }

  return EFI_SUCCESS;
}

/**
  Gets the current status of the variable store.

  @param[in] VarStoreHeader  Pointer to the variable store header.

  @retval EfiRaw         Variable store status is raw.
  @retval EfiValid       Variable store status is valid.
  @retval EfiInvalid     Variable store status is invalid.

**/
VARIABLE_STORE_STATUS
GetVariableStoreStatus (
  IN VARIABLE_STORE_HEADER  *VarStoreHeader
  )
{
  if ((CompareGuid (&VarStoreHeader->Signature, &gEfiAuthenticatedVariableGuid) ||
       CompareGuid (&VarStoreHeader->Signature, &gEfiVariableGuid)) &&
      (VarStoreHeader->Format == VARIABLE_STORE_FORMATTED) &&
      (VarStoreHeader->State == VARIABLE_STORE_HEALTHY)
      )
  {
    return EfiValid;
  } else if ((((UINT32 *)(&VarStoreHeader->Signature))[0] == 0xffffffff) &&
             (((UINT32 *)(&VarStoreHeader->Signature))[1] == 0xffffffff) &&
             (((UINT32 *)(&VarStoreHeader->Signature))[2] == 0xffffffff) &&
             (((UINT32 *)(&VarStoreHeader->Signature))[3] == 0xffffffff) &&
             (VarStoreHeader->Size == 0xffffffff) &&
             (VarStoreHeader->Format == 0xff) &&
             (VarStoreHeader->State == 0xff)
             )
  {
    return EfiRaw;
  } else {
    return EfiInvalid;
  }
}

/**
  Gets the size of variable header.

  @return Size of variable header in bytes in type UINTN.

**/
UINTN
GetVariableHeaderSize (
  VOID
  )
{
  UINTN  Value;

  if (mAuthFormat) {
    Value = sizeof (AUTHENTICATED_VARIABLE_HEADER);
  } else {
    Value = sizeof (VARIABLE_HEADER);
  }

  return Value;
}

/**

  Gets the size of name of variable.

  @param[in] Variable    Pointer to the Variable Header.

  @return UINTN          Size of variable in bytes.

**/
UINTN
NameSizeOfVariable (
  IN  VARIABLE_HEADER  *Variable
  )
{
  AUTHENTICATED_VARIABLE_HEADER  *AuthVariable;

  AuthVariable = (AUTHENTICATED_VARIABLE_HEADER *)Variable;
  if (mAuthFormat) {
    if ((AuthVariable->State == (UINT8)(-1)) ||
        (AuthVariable->DataSize == (UINT32)(-1)) ||
        (AuthVariable->NameSize == (UINT32)(-1)) ||
        (AuthVariable->Attributes == (UINT32)(-1)))
    {
      return 0;
    }

    return (UINTN)AuthVariable->NameSize;
  } else {
    if ((Variable->State == (UINT8)(-1)) ||
        (Variable->DataSize == (UINT32)(-1)) ||
        (Variable->NameSize == (UINT32)(-1)) ||
        (Variable->Attributes == (UINT32)(-1)))
    {
      return 0;
    }

    return (UINTN)Variable->NameSize;
  }
}

/**
  This code sets the size of name of variable.

  @param[in] Variable   Pointer to the Variable Header.
  @param[in] NameSize   Name size to set.

**/
VOID
SetNameSizeOfVariable (
  IN VARIABLE_HEADER  *Variable,
  IN UINTN            NameSize
  )
{
  AUTHENTICATED_VARIABLE_HEADER  *AuthVariable;

  AuthVariable = (AUTHENTICATED_VARIABLE_HEADER *)Variable;
  if (mAuthFormat) {
    AuthVariable->NameSize = (UINT32)NameSize;
  } else {
    Variable->NameSize = (UINT32)NameSize;
  }
}

/**

  Gets the size of variable data.

  @param Variable        Pointer to the Variable Header.

  @return Size of variable in bytes.

**/
UINTN
DataSizeOfVariable (
  IN  VARIABLE_HEADER  *Variable
  )
{
  AUTHENTICATED_VARIABLE_HEADER  *AuthVariable;

  AuthVariable = (AUTHENTICATED_VARIABLE_HEADER *)Variable;
  if (mAuthFormat) {
    if ((AuthVariable->State == (UINT8)(-1)) ||
        (AuthVariable->DataSize == (UINT32)(-1)) ||
        (AuthVariable->NameSize == (UINT32)(-1)) ||
        (AuthVariable->Attributes == (UINT32)(-1)))
    {
      return 0;
    }

    return (UINTN)AuthVariable->DataSize;
  } else {
    if ((Variable->State == (UINT8)(-1)) ||
        (Variable->DataSize == (UINT32)(-1)) ||
        (Variable->NameSize == (UINT32)(-1)) ||
        (Variable->Attributes == (UINT32)(-1)))
    {
      return 0;
    }

    return (UINTN)Variable->DataSize;
  }
}

/**
  This code sets the size of variable data.

  @param[in] Variable   Pointer to the Variable Header.
  @param[in] DataSize   Data size to set.

**/
VOID
SetDataSizeOfVariable (
  IN VARIABLE_HEADER  *Variable,
  IN UINTN            DataSize
  )
{
  AUTHENTICATED_VARIABLE_HEADER  *AuthVariable;

  AuthVariable = (AUTHENTICATED_VARIABLE_HEADER *)Variable;
  if (mAuthFormat) {
    AuthVariable->DataSize = (UINT32)DataSize;
  } else {
    Variable->DataSize = (UINT32)DataSize;
  }
}

/**

  Gets the pointer to the variable name.

  @param[in] Variable        Pointer to the Variable Header.

  @return Pointer to Variable Name which is Unicode encoding.

**/
CHAR16 *
GetVariableNamePtr (
  IN  VARIABLE_HEADER  *Variable
  )
{
  return (CHAR16 *)((UINTN)Variable + GetVariableHeaderSize ());
}

/**
  Gets the pointer to the variable guid.

  @param[in] Variable   Pointer to the Variable Header.

  @return A EFI_GUID* pointer to Vendor Guid.

**/
EFI_GUID *
GetVendorGuidPtr (
  IN VARIABLE_HEADER  *Variable
  )
{
  AUTHENTICATED_VARIABLE_HEADER  *AuthVariable;

  AuthVariable = (AUTHENTICATED_VARIABLE_HEADER *)Variable;
  if (mAuthFormat) {
    return &AuthVariable->VendorGuid;
  } else {
    return &Variable->VendorGuid;
  }
}

/**

  Gets the pointer to the variable data.

  @param[in] Variable        Pointer to the Variable Header.

  @return Pointer to Variable Data.

**/
UINT8 *
GetVariableDataPtr (
  IN  VARIABLE_HEADER  *Variable
  )
{
  UINTN  Value;

  //
  // Be careful about pad size for alignment.
  //
  Value  =  (UINTN)GetVariableNamePtr (Variable);
  Value += NameSizeOfVariable (Variable);
  Value += GET_PAD_SIZE (NameSizeOfVariable (Variable));

  return (UINT8 *)Value;
}

/**

  Gets the pointer to the next variable header.

  @param[in] Variable        Pointer to the Variable Header.

  @return Pointer to next variable header.

**/
VARIABLE_HEADER *
GetNextVariablePtr (
  IN  VARIABLE_HEADER  *Variable
  )
{
  UINTN  Value;

  Value  =  (UINTN)GetVariableDataPtr (Variable);
  Value += DataSizeOfVariable (Variable);
  Value += GET_PAD_SIZE (DataSizeOfVariable (Variable));

  //
  // Be careful about pad size for alignment.
  //
  return (VARIABLE_HEADER *)HEADER_ALIGN (Value);
}

/**
  Gets the pointer to the first variable header in given variable store area.

  @param[in] VarStoreHeader  Pointer to the Variable Store Header.

  @return Pointer to the first variable header.

**/
VARIABLE_HEADER *
GetStartPointer (
  IN VARIABLE_STORE_HEADER  *VarStoreHeader
  )
{
  //
  // The start of variable store.
  //
  return (VARIABLE_HEADER *)HEADER_ALIGN (VarStoreHeader + 1);
}

/**
  Gets the pointer to the end of the variable storage area.

  This function gets pointer to the end of the variable storage
  area, according to the input variable store header.

  @param[in] VarStoreHeader  Pointer to the Variable Store Header.

  @return Pointer to the end of the variable storage area.

**/
VARIABLE_HEADER *
GetEndPointer (
  IN VARIABLE_STORE_HEADER  *VarStoreHeader
  )
{
  //
  // The end of variable store
  //
  return (VARIABLE_HEADER *)HEADER_ALIGN ((UINTN)VarStoreHeader + VarStoreHeader->Size);
}

/**
  Find the variable in the specified variable store.

  @param[in]       VariableName        Name of the variable to be found
  @param[in]       VendorGuid          Vendor GUID to be found.
  @param[in, out]  PtrTrack            Variable Track Pointer structure that contains Variable Information.

  @retval          EFI_SUCCESS         Variable found successfully
  @retval          EFI_NOT_FOUND       Variable not found
**/
EFI_STATUS
FindVariableEx (
  IN CONST CHAR16                  *VariableName,
  IN CONST EFI_GUID                *VendorGuid,
  IN OUT   VARIABLE_POINTER_TRACK  *PtrTrack
  )
{
  VARIABLE_HEADER  *InDeletedVariable;
  VOID             *Point;

  PtrTrack->InDeletedTransitionPtr = NULL;

  //
  // Find the variable by walk through HOB, volatile and non-volatile variable store.
  //
  InDeletedVariable = NULL;

  for ( PtrTrack->CurrPtr = PtrTrack->StartPtr
        ; IsValidVariableHeader (PtrTrack->CurrPtr, PtrTrack->EndPtr)
        ; PtrTrack->CurrPtr = GetNextVariablePtr (PtrTrack->CurrPtr)
        )
  {
    if ((PtrTrack->CurrPtr->State == VAR_ADDED) ||
        (PtrTrack->CurrPtr->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED))
        )
    {
      if (VariableName[0] == 0) {
        if (PtrTrack->CurrPtr->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) {
          InDeletedVariable = PtrTrack->CurrPtr;
        } else {
          PtrTrack->InDeletedTransitionPtr = InDeletedVariable;
          return EFI_SUCCESS;
        }
      } else {
        if (CompareGuid (VendorGuid, GetVendorGuidPtr (PtrTrack->CurrPtr))) {
          Point = (VOID *)GetVariableNamePtr (PtrTrack->CurrPtr);

          ASSERT (NameSizeOfVariable (PtrTrack->CurrPtr) != 0);
          if (CompareMem (VariableName, Point, NameSizeOfVariable (PtrTrack->CurrPtr)) == 0) {
            if (PtrTrack->CurrPtr->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) {
              InDeletedVariable = PtrTrack->CurrPtr;
            } else {
              PtrTrack->InDeletedTransitionPtr = InDeletedVariable;
              return EFI_SUCCESS;
            }
          }
        }
      }
    }
  }

  PtrTrack->CurrPtr = InDeletedVariable;
  return (PtrTrack->CurrPtr  == NULL) ? EFI_NOT_FOUND : EFI_SUCCESS;
}

/**
  Finds variable in storage blocks of non-volatile storage area.

  This code finds variable in storage blocks of non-volatile storage area.
  If VariableName is an empty string, then we just return the first
  qualified variable without comparing VariableName and VendorGuid.
  If IgnoreRtCheck is TRUE, then we ignore the EFI_VARIABLE_RUNTIME_ACCESS attribute check
  at runtime when searching existing variable, only VariableName and VendorGuid are compared.
  Otherwise, variables without EFI_VARIABLE_RUNTIME_ACCESS are not visible at runtime.

  @param[in]   VariableName           Name of the variable to be found.
  @param[in]   VendorGuid             Vendor GUID to be found.
  @param[out]  PtrTrack               VARIABLE_POINTER_TRACK structure for output,
                                      including the range searched and the target position.

  @retval EFI_INVALID_PARAMETER       If VariableName is not an empty string, while
                                      VendorGuid is NULL.
  @retval EFI_SUCCESS                 Variable successfully found.
  @retval EFI_NOT_FOUND               Variable not found

**/
EFI_STATUS
FindVariable (
  IN  CONST CHAR16                  *VariableName,
  IN  CONST EFI_GUID                *VendorGuid,
  OUT       VARIABLE_POINTER_TRACK  *PtrTrack
  )
{
  EFI_STATUS  Status;

  if ((VariableName[0] != 0) && (VendorGuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  PtrTrack->StartPtr = GetStartPointer (mNvVariableCache);
  PtrTrack->EndPtr   = GetEndPointer (mNvVariableCache);

  Status = FindVariableEx (VariableName, VendorGuid, PtrTrack);
  return Status;
}

/**
  Retrieves an authenticated variable's value using its name and GUID.

  Reads the specified authenticated variable from the UEFI variable store. If the Data
  buffer is too small to hold the contents of the variable, the error EFI_BUFFER_TOO_SMALL
  is returned and DataSize is set to the required buffer size to obtain the data.

  @param[in]       This                   A pointer to this instance of the EDKII_VARIABLE_STORAGE_PROTOCOL.
  @param[in]       AtRuntime              TRUE if the platform is in OS Runtime, FALSE if still in Pre-OS stage.
  @param[in]       FromSmm                TRUE if GetVariable() is being called by SMM code, FALSE if called by DXE code.
  @param[in]       VariableName           A pointer to a null-terminated string that is the variable's name.
  @param[in]       VariableGuid           A pointer to an EFI_GUID that is the variable's GUID. The combination of
                                          VariableGuid and VariableName must be unique.
  @param[out]      Attributes             If non-NULL, on return, points to the variable's attributes.
  @param[in, out]  DataSize               On entry, points to the size in bytes of the Data buffer.
                                          On return, points to the size of the data returned in Data.
  @param[out]      Data                   Points to the buffer which will hold the returned variable value.
  @param[out]      KeyIndex               Index of associated public key in database.
  @param[out]      MonotonicCount         Associated monotonic count value to protect against replay attack.
  @param[out]      TimeStamp              Associated TimeStamp value to protect against replay attack.

  @retval          EFI_SUCCESS            The variable was read successfully.
  @retval          EFI_NOT_FOUND          The variable could not be found.
  @retval          EFI_BUFFER_TOO_SMALL   The DataSize is too small for the resulting data.
                                          DataSize is updated with the size required for
                                          the specified variable.
  @retval          EFI_INVALID_PARAMETER  VariableName, VariableGuid, DataSize or Data is NULL.
  @retval          EFI_DEVICE_ERROR       The variable could not be retrieved because of a device error.

**/
EFI_STATUS
EFIAPI
FvbVariableStorageGetAuthenticatedVariable (
  IN CONST  EDKII_VARIABLE_STORAGE_PROTOCOL  *This,
  IN        BOOLEAN                          AtRuntime,
  IN        BOOLEAN                          FromSmm,
  IN CONST  CHAR16                           *VariableName,
  IN CONST  EFI_GUID                         *VariableGuid,
  OUT       UINT32                           *Attributes,
  IN OUT    UINTN                            *DataSize,
  OUT       VOID                             *Data,
  OUT       UINT32                           *KeyIndex,
  OUT       UINT64                           *MonotonicCount,
  OUT       EFI_TIME                         *TimeStamp
  )
{
  EFI_STATUS              Status;
  VARIABLE_POINTER_TRACK  Variable;
  UINTN                   VarDataSize;

  if ((VariableName == NULL) || (VariableGuid == NULL) || (DataSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = FindVariable (VariableName, VariableGuid, &Variable);
  if (EFI_ERROR (Status) || (Variable.CurrPtr == NULL)) {
    goto Done;
  }

  //
  // Get data size
  //
  VarDataSize = DataSizeOfVariable (Variable.CurrPtr);
  ASSERT (VarDataSize != 0);

  if (*DataSize >= VarDataSize) {
    if (Data == NULL) {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }

    CopyMem (Data, GetVariableDataPtr (Variable.CurrPtr), VarDataSize);

    *DataSize = VarDataSize;
    if (mAuthFormat) {
      if (KeyIndex != NULL) {
        *KeyIndex = ((AUTHENTICATED_VARIABLE_HEADER *)Variable.CurrPtr)->PubKeyIndex;
      }

      if (MonotonicCount != NULL) {
        *MonotonicCount = ((AUTHENTICATED_VARIABLE_HEADER *)Variable.CurrPtr)->MonotonicCount;
      }

      if (TimeStamp != NULL) {
        CopyMem (TimeStamp, &(((AUTHENTICATED_VARIABLE_HEADER *)Variable.CurrPtr)->TimeStamp), sizeof (EFI_TIME));
      }
    } else {
      if (KeyIndex != NULL) {
        *KeyIndex = 0;
      }

      if (MonotonicCount != NULL) {
        *MonotonicCount = 0;
      }

      if (TimeStamp != NULL) {
        ZeroMem (TimeStamp, sizeof (EFI_TIME));
      }
    }

    Status = EFI_SUCCESS;
    goto Done;
  } else {
    *DataSize = VarDataSize;
    Status    = EFI_BUFFER_TOO_SMALL;
    goto Done;
  }

Done:
  if ((Status == EFI_SUCCESS) || (Status == EFI_BUFFER_TOO_SMALL)) {
    if ((Attributes != NULL) && (Variable.CurrPtr != NULL)) {
      *Attributes = Variable.CurrPtr->Attributes;
    }
  }

  return Status;
}

/**
  Retrieves a variable's value using its name and GUID.

  Reads the specified variable from the UEFI variable store. If the Data buffer is too small
  to hold the contents of the variable the error EFI_BUFFER_TOO_SMALL is returned and DataSize
  is set to the required buffer size to obtain the data.

  @param[in]       This                   A pointer to this instance of the EDKII_VARIABLE_STORAGE_PROTOCOL.
  @param[in]       AtRuntime              TRUE if the platform is in OS Runtime, FALSE if still in Pre-OS stage.
  @param[in]       FromSmm                TRUE if GetVariable() is being called by SMM code, FALSE if called by DXE code.
  @param[in]       VariableName           A pointer to a null-terminated string that is the variable's name.
  @param[in]       VariableGuid           A pointer to an EFI_GUID that is the variable's vendor GUID. The combination
                                          of VariableGuid and VariableName must be unique.
  @param[out]      Attributes             If non-NULL, on return, points to the variable's attributes.
  @param[in, out]  DataSize               On entry, points to the size in bytes of the Data buffer.
                                          On return, points to the size of the data returned in Data.
  @param[out]      Data                   Points to the buffer which will hold the returned variable value.

  @retval          EFI_SUCCESS            The variable was read successfully.
  @retval          EFI_NOT_FOUND          The variable could not be found.
  @retval          EFI_BUFFER_TOO_SMALL   The DataSize is too small for the resulting data.
                                          DataSize is updated with the size required for the specified variable.
  @retval          EFI_INVALID_PARAMETER  VariableName, VariableGuid, DataSize or Data is NULL.
  @retval          EFI_DEVICE_ERROR       The variable could not be retrieved because of a device error.

**/
EFI_STATUS
EFIAPI
FvbVariableStorageGetVariable (
  IN CONST  EDKII_VARIABLE_STORAGE_PROTOCOL  *This,
  IN        BOOLEAN                          AtRuntime,
  IN        BOOLEAN                          FromSmm,
  IN CONST  CHAR16                           *VariableName,
  IN CONST  EFI_GUID                         *VariableGuid,
  OUT       UINT32                           *Attributes,
  IN OUT    UINTN                            *DataSize,
  OUT       VOID                             *Data
  )
{
  return FvbVariableStorageGetAuthenticatedVariable (
           This,
           AtRuntime,
           FromSmm,
           VariableName,
           VariableGuid,
           Attributes,
           DataSize,
           Data,
           NULL,
           NULL,
           NULL
           );
}

/**
  This code Finds the Next available variable.

  @param[in]  VariableName  Pointer to variable name.
  @param[in]  VendorGuid    Variable Vendor Guid.
  @param[out] VariablePtr   Pointer to variable header address.

  @return EFI_SUCCESS       Find the specified variable.
  @return EFI_NOT_FOUND     Not found.

**/
EFI_STATUS
EFIAPI
GetNextVariableInternal (
  IN  CHAR16           *VariableName,
  IN  EFI_GUID         *VendorGuid,
  OUT VARIABLE_HEADER  **VariablePtr
  )
{
  VARIABLE_POINTER_TRACK  Variable;
  VARIABLE_POINTER_TRACK  VariablePtrTrack;
  EFI_STATUS              Status;

  Status = FindVariable (VariableName, VendorGuid, &Variable);
  if (EFI_ERROR (Status) || (Variable.CurrPtr == NULL)) {
    goto Done;
  }

  if (VariableName[0] != 0) {
    //
    // If variable name is not NULL, get next variable.
    //
    Variable.CurrPtr = GetNextVariablePtr (Variable.CurrPtr);
  }

  while (TRUE) {
    if (!IsValidVariableHeader (Variable.CurrPtr, Variable.EndPtr)) {
      Status = EFI_NOT_FOUND;
      goto Done;
    }

    //
    // Variable is found
    //
    if ((Variable.CurrPtr->State == VAR_ADDED) || (Variable.CurrPtr->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED))) {
      if (Variable.CurrPtr->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) {
        //
        // If it is a IN_DELETED_TRANSITION variable,
        // and there is also a same ADDED one at the same time,
        // don't return it.
        //
        VariablePtrTrack.StartPtr = Variable.StartPtr;
        VariablePtrTrack.EndPtr   = Variable.EndPtr;
        Status                    = FindVariableEx (
                                      GetVariableNamePtr (Variable.CurrPtr),
                                      GetVendorGuidPtr (Variable.CurrPtr),
                                      &VariablePtrTrack
                                      );
        if (!EFI_ERROR (Status) && (VariablePtrTrack.CurrPtr->State == VAR_ADDED)) {
          Variable.CurrPtr = GetNextVariablePtr (Variable.CurrPtr);
          continue;
        }
      }

      *VariablePtr = Variable.CurrPtr;
      Status       = EFI_SUCCESS;
      goto Done;
    }

    Variable.CurrPtr = GetNextVariablePtr (Variable.CurrPtr);
  }

Done:
  return Status;
}

/**
  Return the next variable name and GUID.

  This function is called multiple times to retrieve the VariableName and VariableGuid of all variables
  currently available in the system. On each call, the previous results are passed into the interface,
  and, on return, the interface returns the data for the next interface. When the entire variable list
  has been returned, EFI_NOT_FOUND is returned.

  @param[in]      This                   A pointer to this instance of the EDKII_VARIABLE_STORAGE_PROTOCOL.
  @param[in, out] VariableNameSize       On entry, points to the size of the buffer pointed to by
                                         VariableName. On return, the size of the variable name buffer.
  @param[in, out] VariableName           On entry, a pointer to a null-terminated string that is the
                                         variable's name. On return, points to the next variable's
                                         null-terminated name string.
  @param[in, out] VariableGuid           On entry, a pointer to an EFI_GUID that is the variable's GUID.
                                         On return, a pointer to the next variable's GUID.
  @param[out]     VariableAttributes     A pointer to the variable attributes.

  @retval         EFI_SUCCESS            The variable was read successfully.
  @retval         EFI_NOT_FOUND          The variable could not be found.
  @retval         EFI_BUFFER_TOO_SMALL   The VariableNameSize is too small for the resulting data.
                                         VariableNameSize is updated with the size required for the specified
                                         variable.
  @retval         EFI_INVALID_PARAMETER  VariableName, VariableGuid or VariableNameSize is NULL.
  @retval         EFI_DEVICE_ERROR       The variable could not be retrieved because of a device error.
**/
EFI_STATUS
EFIAPI
FvbVariableStorageGetNextVariableName (
  IN CONST  EDKII_VARIABLE_STORAGE_PROTOCOL  *This,
  IN OUT    UINTN                            *VariableNameSize,
  IN OUT    CHAR16                           *VariableName,
  IN OUT    EFI_GUID                         *VariableGuid,
  OUT       UINT32                           *VariableAttributes
  )
{
  EFI_STATUS       Status;
  VARIABLE_HEADER  *VariablePtr;
  UINTN            VarNameSize;

  VariablePtr = NULL;
  Status      = GetNextVariableInternal (VariableName, VariableGuid, &VariablePtr);
  if (!EFI_ERROR (Status)) {
    ASSERT (VariablePtr != NULL);
    if (VariablePtr == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    VarNameSize = NameSizeOfVariable (VariablePtr);
    ASSERT (VarNameSize != 0);
    if (VarNameSize <= *VariableNameSize) {
      CopyMem (VariableName, GetVariableNamePtr (VariablePtr), VarNameSize);
      CopyMem (VariableGuid, GetVendorGuidPtr (VariablePtr), sizeof (EFI_GUID));
      Status = EFI_SUCCESS;
    } else {
      Status = EFI_BUFFER_TOO_SMALL;
    }

    *VariableNameSize   = VarNameSize;
    *VariableAttributes = VariablePtr->Attributes;
  }

  return Status;
}

/**
  Returns information on the amount of space available in the variable store. If the amount of data that can be written
  depends on if the platform is in Pre-OS stage or OS stage, the AtRuntime parameter should be used to compute usage.

  @param[in]  This                           A pointer to this instance of the EDKII_VARIABLE_STORAGE_PROTOCOL.
  @param[in]  AtRuntime                      TRUE if the platform is in OS Runtime, FALSE if still in Pre-OS stage.
  @param[out] VariableStoreSize              The total size of the NV storage. Indicates the maximum amount
                                             of data that can be stored in this NV storage area.
  @param[out] CommonVariablesTotalSize       The total combined size of all the common UEFI variables that are
                                             stored in this NV storage area. Excludes variables with the
                                             EFI_VARIABLE_HARDWARE_ERROR_RECORD attribute set.
  @param[out] HwErrVariablesTotalSize        The total combined size of all the UEFI variables that have the
                                             EFI_VARIABLE_HARDWARE_ERROR_RECORD attribute set and which are
                                             stored in this NV storage area. Excludes all other variables.

  @retval     EFI_SUCCESS                    Space information was returned successfully.
  @retval     EFI_INVALID_PARAMETER          Any of the given parameters are NULL.

**/
EFI_STATUS
EFIAPI
FvbVariableStorageGetStorageUsage (
  IN CONST    EDKII_VARIABLE_STORAGE_PROTOCOL  *This,
  IN          BOOLEAN                          AtRuntime,
  OUT         UINT32                           *VariableStoreSize,
  OUT         UINT32                           *CommonVariablesTotalSize,
  OUT         UINT32                           *HwErrVariablesTotalSize
  )
{
  VARIABLE_HEADER         *Variable;
  VARIABLE_HEADER         *NextVariable;
  VARIABLE_POINTER_TRACK  VariablePtrTrack;
  EFI_STATUS              Status;
  UINTN                   VariableSize;
  UINTN                   LocalCommonVariablesTotalSize;
  UINTN                   LocalHwErrVariablesTotalSize;

  LocalCommonVariablesTotalSize = 0;
  LocalHwErrVariablesTotalSize  = 0;
  if (mNvVariableCache == NULL) {
    return EFI_NOT_AVAILABLE_YET;
  }

  //
  // Parse non-volatile variable data and get last variable offset.
  //
  Variable = GetStartPointer (mNvVariableCache);
  while (IsValidVariableHeader (Variable, GetEndPointer (mNvVariableCache))) {
    NextVariable = GetNextVariablePtr (Variable);
    VariableSize = (UINTN)NextVariable - (UINTN)Variable;

    if (AtRuntime) {
      //
      // We don't take the state of the variables in mind
      // when calculating RemainingVariableStorageSize,
      // since the space occupied by variables not marked with
      // VAR_ADDED is not allowed to be reclaimed in Runtime.
      //
      if ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
        LocalHwErrVariablesTotalSize += VariableSize;
      } else {
        LocalCommonVariablesTotalSize += VariableSize;
      }
    } else {
      //
      // Only care about Variables with State VAR_ADDED, because
      // the space not marked as VAR_ADDED is reclaimable now.
      //
      if (Variable->State == VAR_ADDED) {
        if ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
          LocalHwErrVariablesTotalSize += VariableSize;
        } else {
          LocalCommonVariablesTotalSize += VariableSize;
        }
      } else if (Variable->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) {
        //
        // If it is a IN_DELETED_TRANSITION variable,
        // and there is not also a same ADDED one at the same time,
        // this IN_DELETED_TRANSITION variable is valid.
        //
        VariablePtrTrack.StartPtr = GetStartPointer (mNvVariableCache);
        VariablePtrTrack.EndPtr   = GetEndPointer (mNvVariableCache);
        Status                    = FindVariableEx (
                                      GetVariableNamePtr (Variable),
                                      GetVendorGuidPtr (Variable),
                                      &VariablePtrTrack
                                      );
        if (!EFI_ERROR (Status) && (VariablePtrTrack.CurrPtr->State != VAR_ADDED)) {
          if ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
            LocalHwErrVariablesTotalSize += VariableSize;
          } else {
            LocalCommonVariablesTotalSize += VariableSize;
          }
        }
      }
    }

    Variable = NextVariable;
  }

  *VariableStoreSize        = mNvVariableCache->Size;
  *CommonVariablesTotalSize = (UINT32)LocalCommonVariablesTotalSize;
  *HwErrVariablesTotalSize  = (UINT32)LocalHwErrVariablesTotalSize;

  return EFI_SUCCESS;
}

/**
  Returns the offset of the last variable in the store.

  @return     The byte offset of the last variable in the store.

**/
UINTN
EFIAPI
GetNonVolatileLastVariableOffset (
  VOID
  )
{
  VARIABLE_HEADER  *Variable;
  VARIABLE_HEADER  *NextVariable;

  if (mNvVariableCache == NULL) {
    return 0;
  }

  //
  // Parse non-volatile variable data and get last variable offset.
  //
  Variable = GetStartPointer (mNvVariableCache);
  while (IsValidVariableHeader (Variable, GetEndPointer (mNvVariableCache))) {
    NextVariable = GetNextVariablePtr (Variable);
    Variable     = NextVariable;
  }

  return (UINTN)Variable - (UINTN)mNvVariableCache;
}

/**
  Returns whether this NV storage area supports storing authenticated variables.

  @param[in]  This                           A pointer to this instance of the EDKII_VARIABLE_STORAGE_PROTOCOL.
  @param[out] AuthSupported                  TRUE if this NV storage area can store authenticated variables,
                                             FALSE otherwise.

  @retval     EFI_SUCCESS                    AuthSupported was returned successfully.
  @retval     EFI_INVALID_PARAMETER          A pointer parameter is NULL.

**/
EFI_STATUS
EFIAPI
FvbVariableStorageGetAuthenticatedSupport (
  IN CONST    EDKII_VARIABLE_STORAGE_PROTOCOL  *This,
  OUT         BOOLEAN                          *AuthSupported
  )
{
  *AuthSupported = mAuthFormat;
  return EFI_SUCCESS;
}

/**
  Record variable error flag.

  @param[in] Flag               Variable error flag to record.
  @param[in] VariableName       Name of variable.
  @param[in] VendorGuid         Guid of variable.
  @param[in] Attributes         Attributes of the variable.
  @param[in] VariableSize       Size of the variable.

**/
VOID
RecordVarErrorFlag (
  IN VAR_ERROR_FLAG  Flag,
  IN CHAR16          *VariableName,
  IN EFI_GUID        *VendorGuid,
  IN UINT32          Attributes,
  IN UINTN           VariableSize
  )
{
  EFI_STATUS      Status;
  UINTN           DataSize;
  UINT32          TempAttributes;
  VAR_ERROR_FLAG  TempFlag;

  DEBUG ((DEBUG_ERROR, "RecordVarErrorFlag (0x%02x) %s:%g - 0x%08x - 0x%x\n", Flag, VariableName, VendorGuid, Attributes, VariableSize));
  //
  // It is unknown if the VarErrorFlag variable is stored using FVB, so one can't assume that
  // simply writting to NvStorage is sufficient. The write must be processed by the UEFI variable services
  //
  DataSize = sizeof (VAR_ERROR_FLAG);
  Status   = CoreGetVariable (
               VAR_ERROR_FLAG_NAME,
               &gEdkiiVarErrorFlagGuid,
               &TempAttributes,
               &DataSize,
               (VOID *)&TempFlag
               );
  if (EFI_ERROR (Status) || (DataSize < sizeof (VAR_ERROR_FLAG))) {
    TempFlag = 0xFF;
  }

  TempFlag &= Flag;
  if (TempFlag == Flag) {
    return;
  }

  Status = CoreSetVariable (
             VAR_ERROR_FLAG_NAME,
             &gEdkiiVarErrorFlagGuid,
             TempAttributes,
             sizeof (VAR_ERROR_FLAG),
             (VOID *)&TempFlag
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error writing VarErrorFlag: %r\n", Status));
  }
}

/**
  Writes the variable error flag to non-volatile memory.

  @param[in] Flag               Variable error flag to record.

**/
VOID
WriteVarErrorFlagToNvm (
  IN VAR_ERROR_FLAG  Flag
  )
{
  EFI_STATUS              Status;
  VARIABLE_POINTER_TRACK  Variable;
  VAR_ERROR_FLAG          *VarErrFlag;
  VAR_ERROR_FLAG          TempFlag;

  //
  // Record error flag (it should have be initialized).
  //
  Status = FindVariable (
             VAR_ERROR_FLAG_NAME,
             &gEdkiiVarErrorFlagGuid,
             &Variable
             );
  if (!EFI_ERROR (Status)) {
    VarErrFlag = (VAR_ERROR_FLAG *)GetVariableDataPtr (Variable.CurrPtr);
    TempFlag   = *VarErrFlag;
    if (TempFlag == Flag) {
      return;
    }

    TempFlag = Flag;
    Status   = UpdateVariableStore (
                 FALSE,
                 mFvbInstance,
                 (UINTN)VarErrFlag - (UINTN)mNvVariableCache + (UINTN)mNonVolatileVariableBase,
                 sizeof (TempFlag),
                 &TempFlag
                 );
    if (!EFI_ERROR (Status)) {
      //
      // Update the data in NV cache.
      //
      *VarErrFlag = Flag;
    }
  }
}

/**
  Reclaim space in the variable store from deleted variables.

  @param[out]     LastVariableOffset    Offset in bytes of the last variable in the store.
  @param[in, out] UpdatingPtrTrack      Variable tracking structure.
  @param[in]      NewVariable           New variable information.
  @param[in]      NewVariableSize       New variable size.

  @return EFI_SUCCESS                  The reclaim operation finished successfully.
  @return EFI_OUT_OF_RESOURCES         Not enough memory resources or variable space.
  @return Others                       An unexpected error occurredd during reclaim.

**/
EFI_STATUS
Reclaim (
  OUT    UINTN                   *LastVariableOffset,
  IN OUT VARIABLE_POINTER_TRACK  *UpdatingPtrTrack,
  IN     VARIABLE_HEADER         *NewVariable,
  IN     UINTN                   NewVariableSize
  )
{
  VARIABLE_HEADER        *Variable;
  VARIABLE_HEADER        *AddedVariable;
  VARIABLE_HEADER        *NextVariable;
  VARIABLE_HEADER        *NextAddedVariable;
  VARIABLE_STORE_HEADER  *VariableStoreHeader;
  UINT8                  *ValidBuffer;
  UINTN                  MaximumBufferSize;
  UINTN                  VariableSize;
  UINTN                  NameSize;
  UINT8                  *CurrPtr;
  VOID                   *Point0;
  VOID                   *Point1;
  BOOLEAN                FoundAdded;
  EFI_STATUS             Status;
  VARIABLE_HEADER        *UpdatingVariable;
  VARIABLE_HEADER        *UpdatingInDeletedTransition;

  UpdatingVariable            = NULL;
  UpdatingInDeletedTransition = NULL;
  if (UpdatingPtrTrack != NULL) {
    UpdatingVariable            = UpdatingPtrTrack->CurrPtr;
    UpdatingInDeletedTransition = UpdatingPtrTrack->InDeletedTransitionPtr;
  }

  VariableStoreHeader = (VARIABLE_STORE_HEADER *)((UINTN)mNonVolatileVariableBase);

  //
  // For NV variable reclaim, don't allocate pool here and just use mNvVariableCache
  // as the buffer to reduce SMRAM consumption for SMM variable driver.
  //
  MaximumBufferSize = mNvVariableCache->Size;
  ValidBuffer       = (UINT8 *)mNvVariableCache;

  SetMem (ValidBuffer, MaximumBufferSize, 0xff);

  //
  // Copy variable store header.
  //
  CopyMem (ValidBuffer, VariableStoreHeader, sizeof (VARIABLE_STORE_HEADER));
  CurrPtr = (UINT8 *)GetStartPointer ((VARIABLE_STORE_HEADER *)ValidBuffer);

  //
  // Reinstall all ADDED variables as long as they are not identical to Updating Variable.
  //
  Variable = GetStartPointer (VariableStoreHeader);
  while (IsValidVariableHeader (Variable, GetEndPointer (VariableStoreHeader))) {
    NextVariable = GetNextVariablePtr (Variable);
    if ((Variable != UpdatingVariable) && (Variable->State == VAR_ADDED)) {
      VariableSize = (UINTN)NextVariable - (UINTN)Variable;
      CopyMem (CurrPtr, (UINT8 *)Variable, VariableSize);
      CurrPtr += VariableSize;
    }

    Variable = NextVariable;
  }

  //
  // Reinstall all in delete transition variables.
  //
  Variable = GetStartPointer (VariableStoreHeader);
  while (IsValidVariableHeader (Variable, GetEndPointer (VariableStoreHeader))) {
    NextVariable = GetNextVariablePtr (Variable);
    if ((Variable != UpdatingVariable) && (Variable != UpdatingInDeletedTransition) && (Variable->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED))) {
      //
      // Buffer has cached all ADDED variable.
      // Per IN_DELETED variable, we have to guarantee that
      // no ADDED one in previous buffer.
      //

      FoundAdded    = FALSE;
      AddedVariable = GetStartPointer ((VARIABLE_STORE_HEADER *)ValidBuffer);
      while (IsValidVariableHeader (AddedVariable, GetEndPointer ((VARIABLE_STORE_HEADER *)ValidBuffer))) {
        NextAddedVariable = GetNextVariablePtr (AddedVariable);
        NameSize          = NameSizeOfVariable (AddedVariable);
        if (CompareGuid (GetVendorGuidPtr (AddedVariable), GetVendorGuidPtr (Variable)) &&
            (NameSize == NameSizeOfVariable (Variable))
            )
        {
          Point0 = (VOID *)GetVariableNamePtr (AddedVariable);
          Point1 = (VOID *)GetVariableNamePtr (Variable);
          if (CompareMem (Point0, Point1, NameSize) == 0) {
            FoundAdded = TRUE;
            break;
          }
        }

        AddedVariable = NextAddedVariable;
      }

      if (!FoundAdded) {
        //
        // Promote VAR_IN_DELETED_TRANSITION to VAR_ADDED.
        //
        VariableSize = (UINTN)NextVariable - (UINTN)Variable;
        CopyMem (CurrPtr, (UINT8 *)Variable, VariableSize);
        ((VARIABLE_HEADER *)CurrPtr)->State = VAR_ADDED;
        CurrPtr                            += VariableSize;
      }
    }

    Variable = NextVariable;
  }

  //
  // Install the new variable if it is not NULL.
  //
  if (NewVariable != NULL) {
    if ((UINTN)(CurrPtr - ValidBuffer) + NewVariableSize > VariableStoreHeader->Size) {
      //
      // No enough space to store the new variable.
      //
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    CopyMem (CurrPtr, (UINT8 *)NewVariable, NewVariableSize);
    ((VARIABLE_HEADER *)CurrPtr)->State = VAR_ADDED;
    if (UpdatingVariable != NULL) {
      UpdatingPtrTrack->CurrPtr                = (VARIABLE_HEADER *)((UINTN)UpdatingPtrTrack->StartPtr + ((UINTN)CurrPtr - (UINTN)GetStartPointer ((VARIABLE_STORE_HEADER *)ValidBuffer)));
      UpdatingPtrTrack->InDeletedTransitionPtr = NULL;
    }

    CurrPtr += NewVariableSize;
  }

  //
  // Perform FTW here.
  //
  Status = FtwVariableSpace (
             mNonVolatileVariableBase,
             (VARIABLE_STORE_HEADER *)ValidBuffer
             );
  if (!EFI_ERROR (Status)) {
    *LastVariableOffset = (UINTN)(CurrPtr - ValidBuffer);
  } else {
    Variable = GetStartPointer ((VARIABLE_STORE_HEADER *)(UINTN)mNonVolatileVariableBase);
    while (IsValidVariableHeader (Variable, GetEndPointer ((VARIABLE_STORE_HEADER *)(UINTN)mNonVolatileVariableBase))) {
      NextVariable = GetNextVariablePtr (Variable);
      VariableSize = (UINTN)NextVariable - (UINTN)Variable;
      Variable     = NextVariable;
    }

    *LastVariableOffset = (UINTN)Variable - (UINTN)mNonVolatileVariableBase;
  }

Done:
  //
  // Update mNvVariableCache now that the FTW operation is complete
  //
  CopyMem (mNvVariableCache, (UINT8 *)(UINTN)mNonVolatileVariableBase, VariableStoreHeader->Size);

  return Status;
}

/**
  Performs variable store garbage collection/reclaim operation.

  @param[in]  This                             A pointer to this instance of the EDKII_VARIABLE_STORAGE_PROTOCOL.

  @retval     EFI_INVALID_PARAMETER            Invalid parameter.
  @retval     EFI_SUCCESS                      Garbage Collection Successful.
  @retval     EFI_OUT_OF_RESOURCES             Insufficient resource to complete garbage collection.
  @retval     EFI_WRITE_PROTECTED              Write services are not yet ready.

**/
EFI_STATUS
EFIAPI
FvbVariableStorageGarbageCollect (
  IN CONST    EDKII_VARIABLE_STORAGE_PROTOCOL  *This
  )
{
  UINTN  NonVolatileLastVariableOffset;

  return Reclaim (
           &NonVolatileLastVariableOffset,
           NULL,
           NULL,
           0
           );
}

/**
  Update the variable region with Variable information. If EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS is set,
  index of associated public key is needed.

  @param[in] VariableName       Name of variable.
  @param[in] VendorGuid         Guid of variable.
  @param[in] Data               Variable data.
  @param[in] DataSize           Size of data. 0 means delete.
  @param[in] AtRuntime          TRUE is the platform is in OS Runtime, FALSE if still in Pre-OS stage
  @param[in] Attributes         Attributes of the variable.
  @param[in] KeyIndex           Index of associated public key.
  @param[in] MonotonicCount     Value of associated monotonic count.
  @param[in, out] CacheVariable The variable information which is used to keep track of variable usage.
  @param[in] TimeStamp          Value of associated TimeStamp.

  @retval EFI_SUCCESS           The update operation is success.
  @retval EFI_OUT_OF_RESOURCES  Variable region is full, can not write other data into this region.

**/
EFI_STATUS
UpdateVariable (
  IN      CHAR16                  *VariableName,
  IN      EFI_GUID                *VendorGuid,
  IN      VOID                    *Data,
  IN      UINTN                   DataSize,
  IN      BOOLEAN                 AtRuntime,
  IN      UINT32                  Attributes      OPTIONAL,
  IN      UINT32                  KeyIndex        OPTIONAL,
  IN      UINT64                  MonotonicCount  OPTIONAL,
  IN OUT  VARIABLE_POINTER_TRACK  *CacheVariable,
  IN      EFI_TIME                *TimeStamp      OPTIONAL
  )
{
  EFI_STATUS                          Status;
  VARIABLE_HEADER                     *NextVariable;
  UINTN                               ScratchSize;
  UINTN                               VarNameOffset;
  UINTN                               VarDataOffset;
  UINTN                               VarNameSize;
  UINTN                               VarSize;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  UINT8                               State;
  VARIABLE_POINTER_TRACK              *Variable;
  VARIABLE_POINTER_TRACK              NvVariable;
  VARIABLE_STORE_HEADER               *VariableStoreHeader;
  UINTN                               CacheOffset;
  UINTN                               NonVolatileLastVariableOffset;
  AUTHENTICATED_VARIABLE_HEADER       *AuthVariable;

  if (mFvbInstance == NULL) {
    //
    // Trying to update NV variable prior the FVB protocol being ready and prior
    // to the installation of EFI_VARIABLE_WRITE_ARCH_PROTOCOL
    //
    DEBUG ((DEBUG_ERROR, "Update NV variable before FVB protocol ready - %r\n", EFI_NOT_AVAILABLE_YET));
    return EFI_NOT_AVAILABLE_YET;
  }

  if (CacheVariable->CurrPtr == NULL) {
    Variable = CacheVariable;
  } else {
    //
    // Update/Delete existing NV variable.
    // CacheVariable points to the variable in the memory copy of Flash area
    // Now let Variable points to the same variable in Flash area.
    //
    VariableStoreHeader = (VARIABLE_STORE_HEADER *)((UINTN)mNonVolatileVariableBase);
    Variable            = &NvVariable;
    Variable->StartPtr  = GetStartPointer (VariableStoreHeader);
    Variable->EndPtr    = GetEndPointer (VariableStoreHeader);
    Variable->CurrPtr   = (VARIABLE_HEADER *)((UINTN)Variable->StartPtr + ((UINTN)CacheVariable->CurrPtr - (UINTN)CacheVariable->StartPtr));
    if (CacheVariable->InDeletedTransitionPtr != NULL) {
      Variable->InDeletedTransitionPtr = (VARIABLE_HEADER *)((UINTN)Variable->StartPtr + ((UINTN)CacheVariable->InDeletedTransitionPtr - (UINTN)CacheVariable->StartPtr));
    } else {
      Variable->InDeletedTransitionPtr = NULL;
    }
  }

  Fvb = mFvbInstance;

  //
  // Tricky part: Use scratch data area at the end of variable store cache
  // as a temporary storage.
  //
  NextVariable = GetEndPointer (mNvVariableCache);
  ScratchSize  = mScratchBufferSize;
  SetMem (NextVariable, ScratchSize, 0xff);

  //
  // Check if Updating/Deleting existing variable.
  //
  if (Variable->CurrPtr != NULL) {
    //
    // Special handling for VarErrorFlag
    //
    if (CompareGuid (VendorGuid, &gEdkiiVarErrorFlagGuid) &&
        (StrCmp (VariableName, VAR_ERROR_FLAG_NAME) == 0) &&
        (DataSize == sizeof (VAR_ERROR_FLAG)))
    {
      WriteVarErrorFlagToNvm (*((VAR_ERROR_FLAG *)Data));
      return EFI_SUCCESS;
    }

    //
    // Setting a data variable with no access, or zero DataSize attributes
    // causes it to be deleted.
    //
    if ((DataSize == 0) || ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == 0)) {
      if (Variable->InDeletedTransitionPtr != NULL) {
        //
        // Both ADDED and IN_DELETED_TRANSITION variable are present,
        // set IN_DELETED_TRANSITION one to DELETED state first.
        //
        State  = Variable->InDeletedTransitionPtr->State;
        State &= VAR_DELETED;
        Status = UpdateVariableStore (
                   FALSE,
                   Fvb,
                   (UINTN)&Variable->InDeletedTransitionPtr->State,
                   sizeof (UINT8),
                   &State
                   );
        if (!EFI_ERROR (Status)) {
          ASSERT (CacheVariable->InDeletedTransitionPtr != NULL);
          CacheVariable->InDeletedTransitionPtr->State = State;
        } else {
          goto Done;
        }
      }

      State  = Variable->CurrPtr->State;
      State &= VAR_DELETED;

      Status = UpdateVariableStore (
                 FALSE,
                 Fvb,
                 (UINTN)&Variable->CurrPtr->State,
                 sizeof (UINT8),
                 &State
                 );
      if (!EFI_ERROR (Status)) {
        CacheVariable->CurrPtr->State = State;
      }

      goto Done;
    }

    //
    // If the variable is marked valid, and the same data has been passed in,
    // then return to the caller immediately.
    //
    if ((DataSizeOfVariable (Variable->CurrPtr) == DataSize) &&
        (CompareMem (Data, GetVariableDataPtr (Variable->CurrPtr), DataSize) == 0) &&
        ((Attributes & EFI_VARIABLE_APPEND_WRITE) == 0) &&
        (TimeStamp == NULL))
    {
      //
      // Variable content unchanged and no need to update timestamp, just return.
      //
      Status = EFI_SUCCESS;
      goto Done;
    } else if ((Variable->CurrPtr->State == VAR_ADDED) ||
               (Variable->CurrPtr->State == (VAR_ADDED & VAR_IN_DELETED_TRANSITION)))
    {
      //
      // Mark the old variable as in delete transition.
      //
      State  = Variable->CurrPtr->State;
      State &= VAR_IN_DELETED_TRANSITION;

      Status = UpdateVariableStore (
                 FALSE,
                 Fvb,
                 (UINTN)&Variable->CurrPtr->State,
                 sizeof (UINT8),
                 &State
                 );
      if (EFI_ERROR (Status)) {
        goto Done;
      }

      CacheVariable->CurrPtr->State = State;
    }
  } else {
    //
    // Not found existing variable. Create a new variable.
    //

    //
    // Make sure we are trying to create a new variable.
    // Setting a data variable with zero DataSize or no access attributes means to delete it.
    //
    if ((DataSize == 0) || ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == 0)) {
      Status = EFI_NOT_FOUND;
      goto Done;
    }
  }

  //
  // Function part - create a new variable and copy the data.
  // Both update a variable and create a variable will come here.
  //
  NextVariable->StartId = VARIABLE_DATA;
  //
  // NextVariable->State    = VAR_ADDED;
  //
  NextVariable->Reserved = 0;
  if (mAuthFormat) {
    AuthVariable                 = (AUTHENTICATED_VARIABLE_HEADER *)NextVariable;
    AuthVariable->PubKeyIndex    = KeyIndex;
    AuthVariable->MonotonicCount = MonotonicCount;
    ZeroMem (&AuthVariable->TimeStamp, sizeof (EFI_TIME));

    if (((Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) != 0) &&
        (TimeStamp != NULL))
    {
      CopyMem (&AuthVariable->TimeStamp, TimeStamp, sizeof (EFI_TIME));
    }
  }

  //
  // The EFI_VARIABLE_APPEND_WRITE attribute will never be set in the returned
  // Attributes bitmask parameter of a GetVariable() call.
  //
  NextVariable->Attributes = Attributes & (~EFI_VARIABLE_APPEND_WRITE);

  VarNameOffset = GetVariableHeaderSize ();
  VarNameSize   = StrSize (VariableName);
  CopyMem (
    (UINT8 *)((UINTN)NextVariable + VarNameOffset),
    VariableName,
    VarNameSize
    );
  VarDataOffset = VarNameOffset + VarNameSize + GET_PAD_SIZE (VarNameSize);
  CopyMem ((UINT8 *)((UINTN)NextVariable + VarDataOffset), Data, DataSize);

  CopyMem (GetVendorGuidPtr (NextVariable), VendorGuid, sizeof (EFI_GUID));
  //
  // There will be pad bytes after Data, the NextVariable->NameSize and
  // NextVariable->DataSize should not include pad size so that variable
  // service can get actual size in GetVariable.
  //
  SetNameSizeOfVariable (NextVariable, VarNameSize);
  SetDataSizeOfVariable (NextVariable, DataSize);

  //
  // The actual size of the variable that stores in storage should
  // include pad size.
  //
  VarSize = VarDataOffset + DataSize + GET_PAD_SIZE (DataSize);

  NonVolatileLastVariableOffset = GetNonVolatileLastVariableOffset ();
  if ((NonVolatileLastVariableOffset + VarSize) > mNvVariableCache->Size) {
    if (AtRuntime) {
      //
      // We choose VAR_ERROR_FLAG_SYSTEM_ERROR here because USER_ERRORs should be caught by the variable driver
      //
      RecordVarErrorFlag (VAR_ERROR_FLAG_SYSTEM_ERROR, VariableName, VendorGuid, Attributes, VarSize);
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    //
    // Perform garbage collection & reclaim operation, and integrate the new variable at the same time.
    //
    Status = Reclaim (
               &NonVolatileLastVariableOffset,
               Variable,
               NextVariable,
               HEADER_ALIGN (VarSize)
               );
    if (!EFI_ERROR (Status)) {
      //
      // The new variable has been integrated successfully during reclaiming.
      //
      if (Variable->CurrPtr != NULL) {
        CacheVariable->CurrPtr                = (VARIABLE_HEADER *)((UINTN)CacheVariable->StartPtr + ((UINTN)Variable->CurrPtr - (UINTN)Variable->StartPtr));
        CacheVariable->InDeletedTransitionPtr = NULL;
      }
    } else {
      RecordVarErrorFlag (VAR_ERROR_FLAG_SYSTEM_ERROR, VariableName, VendorGuid, Attributes, VarSize);
    }

    goto Done;
  }

  //
  // Four steps
  // 1. Write variable header
  // 2. Set variable state to header valid
  // 3. Write variable data
  // 4. Set variable state to valid
  //
  //
  // Step 1:
  //
  CacheOffset = NonVolatileLastVariableOffset;
  Status      = UpdateVariableStore (
                  TRUE,
                  Fvb,
                  NonVolatileLastVariableOffset,
                  (UINT32)GetVariableHeaderSize (),
                  (UINT8 *)NextVariable
                  );

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Step 2:
  //
  NextVariable->State = VAR_HEADER_VALID_ONLY;
  Status              = UpdateVariableStore (
                          TRUE,
                          Fvb,
                          NonVolatileLastVariableOffset + OFFSET_OF (VARIABLE_HEADER, State),
                          sizeof (UINT8),
                          &NextVariable->State
                          );

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Step 3:
  //
  Status = UpdateVariableStore (
             TRUE,
             Fvb,
             NonVolatileLastVariableOffset + GetVariableHeaderSize (),
             (UINT32)(VarSize - GetVariableHeaderSize ()),
             (UINT8 *)NextVariable + GetVariableHeaderSize ()
             );

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Step 4:
  //
  NextVariable->State = VAR_ADDED;
  Status              = UpdateVariableStore (
                          TRUE,
                          Fvb,
                          NonVolatileLastVariableOffset + OFFSET_OF (VARIABLE_HEADER, State),
                          sizeof (UINT8),
                          &NextVariable->State
                          );

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // update the memory copy of Flash region.
  //
  CopyMem ((UINT8 *)mNvVariableCache + CacheOffset, (UINT8 *)NextVariable, VarSize);

  //
  // Mark the old variable as deleted.
  //
  if (!EFI_ERROR (Status) && (Variable->CurrPtr != NULL)) {
    if (Variable->InDeletedTransitionPtr != NULL) {
      //
      // Both ADDED and IN_DELETED_TRANSITION old variable are present,
      // set IN_DELETED_TRANSITION one to DELETED state first.
      //
      State  = Variable->InDeletedTransitionPtr->State;
      State &= VAR_DELETED;
      Status = UpdateVariableStore (
                 FALSE,
                 Fvb,
                 (UINTN)&Variable->InDeletedTransitionPtr->State,
                 sizeof (UINT8),
                 &State
                 );
      if (!EFI_ERROR (Status)) {
        ASSERT (CacheVariable->InDeletedTransitionPtr != NULL);
        CacheVariable->InDeletedTransitionPtr->State = State;
      } else {
        goto Done;
      }
    }

    State  = Variable->CurrPtr->State;
    State &= VAR_DELETED;

    Status = UpdateVariableStore (
               FALSE,
               Fvb,
               (UINTN)&Variable->CurrPtr->State,
               sizeof (UINT8),
               &State
               );
    if (!EFI_ERROR (Status)) {
      CacheVariable->CurrPtr->State = State;
    }
  }

Done:
  return Status;
}

/**
  This code sets a variable's value using its name and GUID.

  Caution: This function may receive untrusted input.

  - This function may be invoked in SMM mode and will be given untrusted input data.
  - This function must perform basic validation before parsing the data.
  - This function must parse the authentication carefully to prevent security issues, like
    buffer overflow and integer overflow.
  - This function must check variable attributes carefully to prevent authentication bypass.

  @param[in]  This                             A pointer to this instance of the EDKII_VARIABLE_STORAGE_PROTOCOL.
  @param[in]  AtRuntime                        TRUE if the platform is in OS Runtime, FALSE if still in Pre-OS stage.
  @param[in]  FromSmm                          TRUE if SetVariable() is being called by SMM code, FALSE if called by
                                               DXE code.
  @param[in]  VariableName                     Name of Variable to be found.
  @param[in]  VendorGuid                       Variable vendor GUID.
  @param[in]  Attributes                       Attribute value of the variable found.
  @param[in]  DataSize                         Size of Data found. If size is less than the data, this value contains
                                               the required size.
  @param[in]  Data                             Data pointer.
  @param[in]  KeyIndex                         If writing an authenticated variable, the public key index
  @param[in]  MonotonicCount                   If writing a monotonic counter authenticated variable, the counter value
  @param[in]  TimeStamp                        If writing a timestamp authenticated variable, the timestamp value

  @retval     EFI_SUCCESS                      The variable was set successfully.
  @retval     EFI_INVALID_PARAMETER            An invalid parameter was given.
  @retval     EFI_OUT_OF_RESOURCES             Insufficient resources to set the variable.
  @retval     EFI_WRITE_PROTECTED              The variable is read-only.
**/
EFI_STATUS
EFIAPI
FvbVariableStorageSetVariable (
  IN CONST    EDKII_VARIABLE_STORAGE_PROTOCOL  *This,
  IN          BOOLEAN                          AtRuntime,
  IN          BOOLEAN                          FromSmm,
  IN          CHAR16                           *VariableName,
  IN          EFI_GUID                         *VendorGuid,
  IN          UINT32                           Attributes,
  IN          UINTN                            DataSize,
  IN          VOID                             *Data,
  IN          UINT32                           KeyIndex       OPTIONAL,
  IN          UINT64                           MonotonicCount OPTIONAL,
  IN          EFI_TIME                         *TimeStamp     OPTIONAL
  )
{
  VARIABLE_POINTER_TRACK  Variable;
  EFI_STATUS              Status;

  //
  // Check input parameters.
  //
  if ((VariableName == NULL) || (VariableName[0] == 0) || (VendorGuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DataSize != 0) && (Data == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check for reserverd bit in variable attribute.
  //
  if ((Attributes & (~EFI_VARIABLE_ATTRIBUTES_MASK)) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether the input variable is already existed.
  //
  Status = FindVariable (VariableName, VendorGuid, &Variable);
  if (EFI_ERROR (Status)) {
    Variable.CurrPtr                = NULL;
    Variable.InDeletedTransitionPtr = NULL;
  }

  Status = UpdateVariable (VariableName, VendorGuid, Data, DataSize, AtRuntime, Attributes, KeyIndex, MonotonicCount, &Variable, TimeStamp);

  return Status;
}

/**
  Get non-volatile maximum variable size.

  @return Non-volatile maximum variable size.

**/
UINTN
GetNonVolatileMaxVariableSize (
  VOID
  )
{
  if (PcdGet32 (PcdHwErrStorageSize) != 0) {
    return MAX (
             MAX (PcdGet32 (PcdMaxVariableSize), PcdGet32 (PcdMaxAuthVariableSize)),
             PcdGet32 (PcdMaxHardwareErrorVariableSize)
             );
  } else {
    return MAX (PcdGet32 (PcdMaxVariableSize), PcdGet32 (PcdMaxAuthVariableSize));
  }
}

/**
  Performs common initialization needed for this module.

  @retval EFI_SUCCESS  The module was initialized successfully.
  @retval Others       The module could not be initialized.
**/
EFI_STATUS
EFIAPI
FvbVariableStorageCommonInitialize (
  VOID
  )
{
  EFI_STATUS                            Status;
  EFI_FIRMWARE_VOLUME_HEADER            *FvHeader;
  EFI_HOB_GUID_TYPE                     *GuidHob;
  EFI_PHYSICAL_ADDRESS                  VariableStoreBase;
  UINT64                                VariableStoreLength;
  EFI_PHYSICAL_ADDRESS                  NvStorageBase;
  UINT32                                NvStorageSize;
  UINT64                                NvStorageSize64;
  UINT8                                 *NvStorageData;
  FAULT_TOLERANT_WRITE_LAST_WRITE_DATA  *FtwLastWriteData;
  UINT32                                BackUpOffset;
  UINT32                                BackUpSize;

  mFvbInstance = NULL;

  Status = GetVariableFlashNvStorageInfo (&NvStorageBase, &NvStorageSize64);
  ASSERT_EFI_ERROR (Status);

  Status = SafeUint64ToUint32 (NvStorageSize64, &NvStorageSize);
  // This driver currently assumes the size will be UINT32 so assert the value is safe for now.
  ASSERT_EFI_ERROR (Status);

  //
  // A memory copy of the FLASH region, will be kept in sync as updates occur.
  //
  mScratchBufferSize = GetNonVolatileMaxVariableSize ();
  NvStorageData      = AllocateRuntimeZeroPool (NvStorageSize + mScratchBufferSize);
  if (NvStorageData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Copy NV storage data to the memory buffer.
  //
  CopyMem (NvStorageData, (UINT8 *)(UINTN)NvStorageBase, NvStorageSize);

  //
  // Check the FTW last write data hob.
  //
  GuidHob = GetFirstGuidHob (&gEdkiiFaultTolerantWriteGuid);
  if (GuidHob != NULL) {
    FtwLastWriteData = (FAULT_TOLERANT_WRITE_LAST_WRITE_DATA *)GET_GUID_HOB_DATA (GuidHob);
    if (FtwLastWriteData->TargetAddress == NvStorageBase) {
      DEBUG ((DEBUG_INFO, "Variable: NV storage is backed up in spare block: 0x%x\n", (UINTN)FtwLastWriteData->SpareAddress));
      //
      // Copy the backed up NV storage data to the memory buffer from spare block.
      //
      CopyMem (NvStorageData, (UINT8 *)(UINTN)(FtwLastWriteData->SpareAddress), NvStorageSize);
    } else if ((FtwLastWriteData->TargetAddress > NvStorageBase) &&
               (FtwLastWriteData->TargetAddress < (NvStorageBase + NvStorageSize)))
    {
      //
      // Flash NV storage from the Offset is backed up in spare block.
      //
      BackUpOffset = (UINT32)(FtwLastWriteData->TargetAddress - NvStorageBase);
      BackUpSize   = NvStorageSize - BackUpOffset;
      DEBUG ((DEBUG_INFO, "Variable: High partial NV storage from offset: %x is backed up in spare block: 0x%x\n", BackUpOffset, (UINTN)FtwLastWriteData->SpareAddress));
      //
      // Copy the partial backed up NV storage data to the memory buffer from spare block.
      //
      CopyMem (NvStorageData + BackUpOffset, (UINT8 *)(UINTN)FtwLastWriteData->SpareAddress, BackUpSize);
    }
  }

  FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)NvStorageData;

  //
  // Check if the Firmware Volume is not corrupted
  //
  if ((FvHeader->Signature != EFI_FVH_SIGNATURE) || (!CompareGuid (&gEfiSystemNvDataFvGuid, &FvHeader->FileSystemGuid))) {
    FreePool (NvStorageData);
    DEBUG ((DEBUG_ERROR, "Firmware Volume for Variable Store is corrupted\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  VariableStoreBase   = (EFI_PHYSICAL_ADDRESS)((UINTN)FvHeader + FvHeader->HeaderLength);
  VariableStoreLength = (UINT64)(NvStorageSize - FvHeader->HeaderLength);

  mNonVolatileVariableBase = VariableStoreBase;
  mNvVariableCache         = (VARIABLE_STORE_HEADER *)(UINTN)VariableStoreBase;
  if (GetVariableStoreStatus (mNvVariableCache) != EfiValid) {
    FreePool (NvStorageData);
    DEBUG ((DEBUG_ERROR, "Variable Store header is corrupted\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  ASSERT (mNvVariableCache->Size == VariableStoreLength);
  ASSERT (sizeof (VARIABLE_STORE_HEADER) <= VariableStoreLength);

  mAuthFormat = (BOOLEAN)(CompareGuid (&mNvVariableCache->Signature, &gEfiAuthenticatedVariableGuid));

  return EFI_SUCCESS;
}

/**
  Initializes FVB write service after FTW was ready.

  @retval EFI_SUCCESS          Function successfully executed.
  @retval Others               Fail to initialize the variable service.

**/
EFI_STATUS
FvbVariableStorageWriteServiceInitialize (
  VOID
  )
{
  EFI_STATUS            Status;
  VARIABLE_HEADER       *Variable;
  VARIABLE_HEADER       *NextVariable;
  UINTN                 NonVolatileLastVariableOffset;
  UINTN                 Index;
  UINT8                 Data;
  EFI_PHYSICAL_ADDRESS  VariableStoreBase;
  EFI_PHYSICAL_ADDRESS  NvStorageBase;
  UINT32                NvStorageSize;
  UINT64                NvStorageSize64;

  Status = GetVariableFlashNvStorageInfo (&NvStorageBase, &NvStorageSize64);
  ASSERT_EFI_ERROR (Status);

  Status = SafeUint64ToUint32 (NvStorageSize64, &NvStorageSize);
  // This driver currently assumes the size will be UINT32 so assert the value is safe for now.
  ASSERT_EFI_ERROR (Status);

  VariableStoreBase = NvStorageBase + (((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)(NvStorageBase))->HeaderLength);

  //
  // Let NonVolatileVariableBase point to flash variable store base directly after FTW ready.
  //
  mNonVolatileVariableBase = VariableStoreBase;

  //
  // Parse non-volatile variable data and get last variable offset.
  //
  Variable = GetStartPointer (mNvVariableCache);
  while (IsValidVariableHeader (Variable, GetEndPointer (mNvVariableCache))) {
    NextVariable = GetNextVariablePtr (Variable);
    Variable     = NextVariable;
  }

  NonVolatileLastVariableOffset = (UINTN)Variable - (UINTN)mNvVariableCache;

  //
  // Check if the free area is really free.
  //
  for (Index = NonVolatileLastVariableOffset; Index < mNvVariableCache->Size; Index++) {
    Data = ((UINT8 *)mNvVariableCache)[Index];
    if (Data != 0xff) {
      //
      // There must be something wrong in variable store, do reclaim operation.
      //
      Status = Reclaim (
                 &NonVolatileLastVariableOffset,
                 NULL,
                 NULL,
                 0
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      break;
    }
  }

  return EFI_SUCCESS;
}

/**
  Returns whether this NV storage area is ready to accept calls to SetVariable().

  @param[in]  This                           A pointer to this instance of the EDKII_VARIABLE_STORAGE_PROTOCOL.

  @retval     TRUE                           The NV storage area is ready to accept calls to SetVariable().
  @retval     FALSE                          The NV storage area is not ready to accept calls to SetVariable().

**/
BOOLEAN
EFIAPI
FvbVariableStorageWriteServiceIsReady (
  IN CONST    EDKII_VARIABLE_STORAGE_PROTOCOL  *This
  )
{
  return (mFvbInstance == NULL) ? FALSE : TRUE;
}

/**
  Get the proper fvb handle and/or fvb protocol by the given Flash address.

  @param[in]  Address       The Flash address.
  @param[out] FvbHandle     In output, if it is not NULL, it points to the proper FVB handle.
  @param[out] FvbProtocol   In output, if it is not NULL, it points to the proper FVB protocol.

**/
EFI_STATUS
GetFvbInfoByAddress (
  IN  EFI_PHYSICAL_ADDRESS                Address,
  OUT EFI_HANDLE                          *FvbHandle OPTIONAL,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  **FvbProtocol OPTIONAL
  )
{
  EFI_STATUS                          Status;
  EFI_HANDLE                          *HandleBuffer;
  UINTN                               HandleCount;
  UINTN                               Index;
  EFI_PHYSICAL_ADDRESS                FvbBaseAddress;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  EFI_FVB_ATTRIBUTES_2                Attributes;
  UINTN                               BlockSize;
  UINTN                               NumberOfBlocks;

  HandleBuffer = NULL;
  //
  // Get all FVB handles.
  //
  Status = GetFvbCountAndBuffer (&HandleCount, &HandleBuffer);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // Get the FVB to access variable store.
  //
  Fvb = NULL;
  for (Index = 0; Index < HandleCount; Index += 1, Status = EFI_NOT_FOUND, Fvb = NULL) {
    Status = GetFvbByHandle (HandleBuffer[Index], &Fvb);
    if (EFI_ERROR (Status)) {
      Status = EFI_NOT_FOUND;
      break;
    }

    //
    // Ensure this FVB protocol supported Write operation.
    //
    Status = Fvb->GetAttributes (Fvb, &Attributes);
    if (EFI_ERROR (Status) || ((Attributes & EFI_FVB2_WRITE_STATUS) == 0)) {
      continue;
    }

    //
    // Compare the address and select the right one.
    //
    Status = Fvb->GetPhysicalAddress (Fvb, &FvbBaseAddress);
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Assume one FVB has one type of BlockSize.
    //
    Status = Fvb->GetBlockSize (Fvb, 0, &BlockSize, &NumberOfBlocks);
    if (EFI_ERROR (Status)) {
      continue;
    }

    if ((Address >= FvbBaseAddress) && (Address < (FvbBaseAddress + BlockSize * NumberOfBlocks))) {
      if (FvbHandle != NULL) {
        *FvbHandle = HandleBuffer[Index];
      }

      if (FvbProtocol != NULL) {
        *FvbProtocol = Fvb;
      }

      Status = EFI_SUCCESS;
      break;
    }
  }

  FreePool (HandleBuffer);

  if (Fvb == NULL) {
    Status = EFI_NOT_FOUND;
  }

  return Status;
}
