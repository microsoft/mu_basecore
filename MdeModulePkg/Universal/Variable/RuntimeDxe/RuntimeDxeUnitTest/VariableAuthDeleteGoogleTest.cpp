/** @file
  Tests for deletion-only admission of deprecated private authenticated variables.

  The real SetVariable entry point, lookup and policy checks are used.
  Authentication and platform hardware are isolated by test doubles.
  Platform initialization fails the test if invoked.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <cstdio>
#include <vector>

extern "C" {
  #include "../VariableParsing.h"
  #include "../VariableNonVolatile.h"
  #include <Library/VariablePolicyLib.h>
  #include <Library/VariablePolicyHelperLib.h>

  /**
    Keep the variable-service tests in boot-services mode.

    @retval FALSE  Runtime services have not started in this test environment.
  **/
  BOOLEAN
  AtRuntime (
    VOID
    )
  {
    return FALSE;
  }

  /**
    Acquire the fixture lock and verify that it was previously released.

    @param[in,out] Lock  Variable-services lock to acquire.
  **/
  VOID
  AcquireLockOnlyAtBootTime (
    IN EFI_LOCK  *Lock
    )
  {
    EXPECT_EQ (Lock->Lock, EfiLockReleased);
    Lock->Lock = EfiLockAcquired;
  }

  /**
    Release the fixture lock and verify that it was previously acquired.

    @param[in,out] Lock  Variable-services lock to release.
  **/
  VOID
  ReleaseLockOnlyAtBootTime (
    IN EFI_LOCK  *Lock
    )
  {
    EXPECT_EQ (Lock->Lock, EfiLockAcquired);
    Lock->Lock = EfiLockReleased;
  }

  /**
    Fail the test if variable processing initializes the driver lock.

    @param[in,out] Lock      Unused driver lock.
    @param[in]     Priority  Unused lock priority.
    @retval NULL  Driver initialization is unexpected.
  **/
  EFI_LOCK *
  InitializeLock (
    IN OUT EFI_LOCK  *Lock,
    IN EFI_TPL       Priority
    )
  {
    ADD_FAILURE () << "Unexpected driver lock initialization";
    return NULL;
  }

  /**
    Fail the test if variable processing searches the platform HOB list.

    @param[in] Guid  Unused HOB GUID.
    @retval NULL  Platform HOB access is unexpected.
  **/
  VOID *
  EFIAPI
  GetFirstGuidHob (
    IN CONST EFI_GUID  *Guid
    )
  {
    ADD_FAILURE () << "Unexpected platform HOB access";
    return NULL;
  }

  /**
    Fail the test if variable processing continues a platform HOB search.

    @param[in] Guid      Unused HOB GUID.
    @param[in] HobStart  Unused starting HOB.
    @retval NULL  Platform HOB access is unexpected.
  **/
  VOID *
  EFIAPI
  GetNextGuidHob (
    IN CONST EFI_GUID  *Guid,
    IN CONST VOID      *HobStart
    )
  {
    ADD_FAILURE () << "Unexpected platform HOB enumeration";
    return NULL;
  }

  /**
    Fail the test if variable processing initializes authentication.

    @param[in]  AuthVarLibContextIn   Unused driver context.
    @param[out] AuthVarLibContextOut  Unused authentication context.
    @retval EFI_ABORTED  Authentication initialization is unexpected.
  **/
  EFI_STATUS
  EFIAPI
  AuthVariableLibInitialize (
    IN  AUTH_VAR_LIB_CONTEXT_IN   *AuthVarLibContextIn,
    OUT AUTH_VAR_LIB_CONTEXT_OUT  *AuthVarLibContextOut
    )
  {
    ADD_FAILURE () << "Unexpected authentication initialization";
    return EFI_ABORTED;
  }

  /**
    Fail the test if variable processing initializes platform MOR protection.

    @retval EFI_ABORTED  MOR initialization is unexpected.
  **/
  EFI_STATUS
  MorLockInit (
    VOID
    )
  {
    ADD_FAILURE () << "Unexpected MOR initialization";
    return EFI_ABORTED;
  }

  /**
    Fail the test if variable processing queries platform store sizing.

    @retval 0  Platform store sizing is unexpected.
  **/
  UINTN
  GetNonVolatileMaxVariableSize (
    VOID
    )
  {
    ADD_FAILURE () << "Unexpected platform store sizing";
    return 0;
  }

  /**
    Fail the test if variable processing initializes non-volatile storage.

    @retval EFI_ABORTED  Platform store initialization is unexpected.
  **/
  EFI_STATUS
  InitNonVolatileVariableStore (
    VOID
    )
  {
    ADD_FAILURE () << "Unexpected platform store initialization";
    return EFI_ABORTED;
  }

  /**
    Suppress platform Secure Boot notifications in the test environment.

    @param[in] VariableName  Name of the updated variable.
    @param[in] VendorGuid    Variable namespace.
  **/
  VOID
  SecureBootHook (
    IN CHAR16    *VariableName,
    IN EFI_GUID  *VendorGuid
    )
  {
  }

  /**
    Accept the update without imposing platform MOR variable restrictions.

    @param[in] VariableName  Name of the variable to update.
    @param[in] VendorGuid    Variable namespace.
    @param[in] Attributes    Requested variable attributes.
    @param[in] DataSize      Size of Data in bytes.
    @param[in] Data          Requested variable data.
    @retval EFI_SUCCESS  The test update passes the MOR check.
  **/
  EFI_STATUS
  SetVariableCheckHandlerMor (
    IN CHAR16    *VariableName,
    IN EFI_GUID  *VendorGuid,
    IN UINT32    Attributes,
    IN UINTN     DataSize,
    IN VOID      *Data
    )
  {
    return EFI_SUCCESS;
  }

  /**
    Fail the test if variable processing attempts a flash write.

    @param[in] VariableBase    Address of the variable store to update.
    @param[in] VariableBuffer  Replacement variable store.
    @retval EFI_ABORTED  Flash writes are not supported by this test stub.
  **/
  EFI_STATUS
  FtwVariableSpace (
    IN EFI_PHYSICAL_ADDRESS   VariableBase,
    IN VARIABLE_STORE_HEADER  *VariableBuffer
    )
  {
    ADD_FAILURE () << "Unexpected flash write";
    return EFI_ABORTED;
  }

  /**
    Fail the test if variable processing requests a firmware-volume protocol.

    @param[in]  FvBlockHandle  Firmware-volume handle to query.
    @param[out] FvBlock       Unused protocol output.
    @retval EFI_ABORTED  Firmware-volume access is unexpected.
  **/
  EFI_STATUS
  GetFvbByHandle (
    IN EFI_HANDLE                           FvBlockHandle,
    OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  **FvBlock
    )
  {
    ADD_FAILURE () << "Unexpected firmware-volume access";
    return EFI_ABORTED;
  }

  /**
    Fail the test if variable processing enumerates firmware volumes.

    @param[out] NumberHandles  Unused handle-count output.
    @param[out] Buffer         Unused handle-buffer output.
    @retval EFI_ABORTED  Firmware-volume enumeration is unexpected.
  **/
  EFI_STATUS
  GetFvbCountAndBuffer (
    OUT UINTN       *NumberHandles,
    OUT EFI_HANDLE  **Buffer
    )
  {
    ADD_FAILURE () << "Unexpected firmware-volume enumeration";
    return EFI_ABORTED;
  }
}

STATIC CONST UINT32  mPrivateAttributes = EFI_VARIABLE_NON_VOLATILE |
                                          EFI_VARIABLE_BOOTSERVICE_ACCESS |
                                          EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;
STATIC CHAR16                                                                           mPrivateName[] = { 'P', 'r', 'i', 'v', 'a', 't', 'e', 0 };
STATIC EFI_GUID                                                                         mPrivateGuid   = { 0xa8b47658, 0x42ae, 0x42b4, { 0x9f, 0x0b, 0xc0, 0x9f, 0x4f, 0x72, 0x64, 0xf2 }
};
STATIC testing::MockFunction<EFI_STATUS (CHAR16 *, EFI_GUID *, VOID *, UINTN, UINT32)>  *mAuthProcess;

/**
  Dispatch an authenticated update to the active fixture's strict mock.

  @param[in] VariableName  Name of the variable to update.
  @param[in] VendorGuid    Variable namespace.
  @param[in] Data          Authentication descriptor and payload.
  @param[in] DataSize      Size of Data in bytes.
  @param[in] Attributes    Requested variable attributes.
  @return Status supplied by the authentication mock.
  @retval EFI_ABORTED  No authentication fixture is installed.
**/
extern "C"
EFI_STATUS
EFIAPI
AuthVariableLibProcessVariable (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN VOID      *Data,
  IN UINTN     DataSize,
  IN UINT32    Attributes
  )
{
  if (mAuthProcess == NULL) {
    ADD_FAILURE () << "Authentication called without a fixture";
    return EFI_ABORTED;
  }

  return mAuthProcess->Call (VariableName, VendorGuid, Data, DataSize, Attributes);
}

/**
  Provide an empty variable lookup for variable-policy initialization.

  @param[in]     VariableName  Name of the variable to query.
  @param[in]     VendorGuid    Variable namespace.
  @param[out]    Attributes    Unused variable-attributes output.
  @param[in,out] DataSize      Unused data-buffer size.
  @param[out]    Data          Unused variable-data output.
  @retval EFI_NOT_FOUND  No variables are provided by this callback.
**/
STATIC
EFI_STATUS
EFIAPI
PolicyGetVariableNotFound (
  IN CHAR16     *VariableName,
  IN EFI_GUID   *VendorGuid,
  OUT UINT32    *Attributes OPTIONAL,
  IN OUT UINTN  *DataSize,
  OUT VOID      *Data OPTIONAL
  )
{
  return EFI_NOT_FOUND;
}

class VariableServicePrivateDeletionTest : public testing::Test {
protected:
  testing::StrictMock<testing::MockFunction<EFI_STATUS (CHAR16 *, EFI_GUID *, VOID *, UINTN, UINT32)> > AuthProcess;
  VARIABLE_MODULE_GLOBAL Global;
  alignas (UINT64) UINT8 Store[256];
  AUTHENTICATED_VARIABLE_HEADER *Existing;
  std::vector<UINT8> Data;

  /**
    Initialize an authenticated variable store and install the test callbacks.
  **/
  VOID
  SetUp (
    ) override
  {
    ZeroMem (&Global, sizeof (Global));
    SetMem (Store, sizeof (Store), 0xFF);
    VARIABLE_STORE_HEADER  *StoreHeader = reinterpret_cast<VARIABLE_STORE_HEADER *>(Store);

    StoreHeader->Signature                          = gEfiAuthenticatedVariableGuid;
    StoreHeader->Size                               = sizeof (Store);
    StoreHeader->Format                             = VARIABLE_STORE_FORMATTED;
    StoreHeader->State                              = VARIABLE_STORE_HEALTHY;
    Global.VariableGlobal.NonVolatileVariableBase   = reinterpret_cast<UINTN> (Store);
    Global.VariableGlobal.AuthFormat                = TRUE;
    Global.VariableGlobal.AuthSupport               = TRUE;
    Global.VariableGlobal.VariableServicesLock.Lock = EfiLockReleased;
    Global.MaxAuthVariableSize                      = sizeof (Store);
    mVariableModuleGlobal                           = &Global;
    mNvVariableCache                                = StoreHeader;
    mAuthProcess                                    = &AuthProcess;
    Existing                                        = reinterpret_cast<AUTHENTICATED_VARIABLE_HEADER *>(GetStartPointer (StoreHeader));
    ZeroMem (Existing, sizeof (*Existing));
    Existing->StartId    = VARIABLE_DATA;
    Existing->State      = VAR_ADDED;
    Existing->Attributes = mPrivateAttributes;
    Existing->VendorGuid = mPrivateGuid;
    Existing->NameSize   = (UINT32)StrSize (mPrivateName);
    Existing->DataSize   = 1;
    CopyMem (GetVariableNamePtr (reinterpret_cast<VARIABLE_HEADER *>(Existing), TRUE), mPrivateName, Existing->NameSize);
    *GetVariableDataPtr (reinterpret_cast<VARIABLE_HEADER *>(Existing), TRUE) = 0xAA;
    Data.assign (OFFSET_OF_AUTHINFO2_CERT_DATA + 22, 0);
    reinterpret_cast<EFI_VARIABLE_AUTHENTICATION_2 *>(Data.data ())->AuthInfo.Hdr.dwLength =
      OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData) + 22;
    ASSERT_EQ (InitVariablePolicyLib (PolicyGetVariableNotFound), EFI_SUCCESS);
  }

  /**
    Verify lock cleanup and restore the variable-service and policy state.
  **/
  VOID
  TearDown (
    ) override
  {
    EXPECT_EQ (Global.VariableGlobal.ReentrantState, 0U);
    EXPECT_EQ (Global.VariableGlobal.VariableServicesLock.Lock, EfiLockReleased);
    EXPECT_EQ (DeinitVariablePolicyLib (), EFI_SUCCESS);
    mAuthProcess          = NULL;
    mVariableModuleGlobal = NULL;
    mNvVariableCache      = NULL;
  }

  /**
    Submit the fixture's private-variable update through the real entry point.

    @param[in] Attributes  Attributes to pass to SetVariable.
    @return Status returned by VariableServiceSetVariable.
  **/
  EFI_STATUS
  Set (
    UINT32  Attributes = mPrivateAttributes
    )
  {
    return VariableServiceSetVariable (mPrivateName, &mPrivateGuid, Attributes, Data.size (), Data.data ());
  }
};

/**
  Verify that deletion of an existing private variable reaches authentication.
**/
TEST_F (VariableServicePrivateDeletionTest, ExistingDeletionReachesAuthentication) {
  EXPECT_CALL (AuthProcess, Call (mPrivateName, &mPrivateGuid, Data.data (), Data.size (), mPrivateAttributes))
    .WillOnce (testing::Return (EFI_SECURITY_VIOLATION));
  EXPECT_EQ (Set (), EFI_SECURITY_VIOLATION);
}

/**
  Verify that policy-disabled recovery synthesizes an authentication descriptor.
**/
TEST_F (VariableServicePrivateDeletionTest, DisabledPolicyAllowsHeaderlessRecoveryToReachAuthentication) {
  ASSERT_EQ (DisableVariablePolicy (), EFI_SUCCESS);
  VOID  *RecoveryData[] = { NULL, Data.data () };

  for (VOID *RequestData : RecoveryData) {
    EXPECT_CALL (AuthProcess, Call (mPrivateName, &mPrivateGuid, testing::_, OFFSET_OF_AUTHINFO2_CERT_DATA, mPrivateAttributes))
      .WillOnce (

         /**
           Check the synthesized descriptor size supplied to authentication.

           @param[in] Buffer  Synthesized authentication descriptor.
           @param[in] Size    Size of Buffer in bytes.
           @retval EFI_SUCCESS  The authentication mock accepts the recovery.
         **/
         [](CHAR16 *, EFI_GUID *, VOID *Buffer, UINTN Size, UINT32) {
      EXPECT_EQ (AUTHINFO2_SIZE (Buffer), Size);
      return EFI_SUCCESS;
    }
         );
    EXPECT_EQ (VariableServiceSetVariable (mPrivateName, &mPrivateGuid, mPrivateAttributes, 0, RequestData), EFI_SUCCESS);
  }
}

/**
  Verify that enabled variable policy rejects headerless recovery requests.
**/
TEST_F (VariableServicePrivateDeletionTest, EnabledPolicyRejectsHeaderlessRecovery) {
  ASSERT_TRUE (IsVariablePolicyEnabled ());
  EXPECT_EQ (VariableServiceSetVariable (mPrivateName, &mPrivateGuid, mPrivateAttributes, 0, NULL), EFI_SECURITY_VIOLATION);
  EXPECT_EQ (VariableServiceSetVariable (mPrivateName, &mPrivateGuid, mPrivateAttributes, 0, Data.data ()), EFI_SECURITY_VIOLATION);
}

/**
  Verify that policy-disabled headerless requests cannot create private variables.
**/
TEST_F (VariableServicePrivateDeletionTest, DisabledPolicyRejectsHeaderlessCreation) {
  ASSERT_EQ (DisableVariablePolicy (), EFI_SUCCESS);
  Existing->State = VAR_DELETED;
  EXPECT_EQ (VariableServiceSetVariable (mPrivateName, &mPrivateGuid, mPrivateAttributes, 0, Data.data ()), EFI_UNSUPPORTED);
}

/**
  Verify that an empty payload cannot create a private authenticated variable.
**/
TEST_F (VariableServicePrivateDeletionTest, RejectsCreationEvenWithEmptyPayload) {
  Existing->State = VAR_DELETED;
  EXPECT_EQ (Set (), EFI_UNSUPPORTED);
}

/**
  Verify that private authenticated-variable payload updates are unsupported.
**/
TEST_F (VariableServicePrivateDeletionTest, RejectsPayloadUpdates) {
  Data.push_back (0xBB);
  EXPECT_EQ (Set (), EFI_UNSUPPORTED);
}

/**
  Verify that an append request is rejected even when its payload is empty.
**/
TEST_F (VariableServicePrivateDeletionTest, RejectsEmptyAppend) {
  EXPECT_EQ (Set (mPrivateAttributes | EFI_VARIABLE_APPEND_WRITE), EFI_UNSUPPORTED);
}

/**
  Verify that deletion attributes must match the existing variable.
**/
TEST_F (VariableServicePrivateDeletionTest, RejectsMismatchedAttributes) {
  EXPECT_EQ (Set (mPrivateAttributes | EFI_VARIABLE_RUNTIME_ACCESS), EFI_INVALID_PARAMETER);
}

/**
  Verify that a truncated authentication descriptor is rejected before dispatch.
**/
TEST_F (VariableServicePrivateDeletionTest, RejectsTruncatedAuthenticationDescriptor) {
  Data.resize (OFFSET_OF_AUTHINFO2_CERT_DATA - 1);
  EXPECT_EQ (Set (), EFI_SECURITY_VIOLATION);
}

/**
  Verify that the authentication certificate length cannot exceed the buffer.
**/
TEST_F (VariableServicePrivateDeletionTest, RejectsCertificateLengthBeyondBuffer) {
  reinterpret_cast<EFI_VARIABLE_AUTHENTICATION_2 *>(Data.data ())->AuthInfo.Hdr.dwLength++;
  EXPECT_EQ (Set (), EFI_SECURITY_VIOLATION);
}

/**
  Verify that the authentication certificate length includes its fixed header.
**/
TEST_F (VariableServicePrivateDeletionTest, RejectsCertificateLengthBelowHeader) {
  reinterpret_cast<EFI_VARIABLE_AUTHENTICATION_2 *>(Data.data ())->AuthInfo.Hdr.dwLength =
    OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData) - 1;
  EXPECT_EQ (Set (), EFI_SECURITY_VIOLATION);
}

/**
  Verify that a lock-now variable policy still prevents authenticated deletion.
**/
TEST_F (VariableServicePrivateDeletionTest, LockedVariablePolicyStillBlocksDeletion) {
  EDKII_VARIABLE_POLICY_PROTOCOL  Policy = { 0 };

  Policy.RegisterVariablePolicy = RegisterVariablePolicy;
  ASSERT_EQ (
    RegisterBasicVariablePolicy (
      &Policy,
      &mPrivateGuid,
      mPrivateName,
      0,
      MAX_UINT32,
      0,
      0,
      VARIABLE_POLICY_TYPE_LOCK_NOW
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Set (), EFI_WRITE_PROTECTED);
}

/**
  Verify that PK payload updates retain the Secure Boot authentication path.
**/
TEST_F (VariableServicePrivateDeletionTest, SecureBootUpdatesStillReachAuthentication) {
  CHAR16  PkName[] = { 'P', 'K', 0 };

  Data.push_back (0xBB);
  EXPECT_CALL (AuthProcess, Call (PkName, &gEfiGlobalVariableGuid, Data.data (), Data.size (), mPrivateAttributes))
    .WillOnce (testing::Return (EFI_SECURITY_VIOLATION));
  EXPECT_EQ (
    VariableServiceSetVariable (PkName, &gEfiGlobalVariableGuid, mPrivateAttributes, Data.size (), Data.data ()),
    EFI_SECURITY_VIOLATION
    );
}

/**
  Register the variable-policy check and run the variable-service host tests.

  @param[in] argc  Number of command-line arguments.
  @param[in] argv  Command-line arguments for GoogleTest.
  @retval 0  All tests passed.
  @retval 1  Policy-check registration or a test failed.
**/
int
main (
  int   argc,
  char  *argv[]
  )
{
  ::testing::InitGoogleTest (&argc, argv);
  if (EFI_ERROR (VarCheckLibRegisterSetVariableCheckHandler (ValidateSetVariable))) {
    fprintf (stderr, "Failed to register the variable-policy check\n");
    return 1;
  }

  VarCheckLibInitializeAtEndOfDxe (NULL);
  return RUN_ALL_TESTS ();
}
