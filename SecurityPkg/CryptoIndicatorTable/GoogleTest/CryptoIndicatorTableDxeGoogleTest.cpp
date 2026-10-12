/** @file
  Unit tests for the ECIT collector driver.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockHobLib.h>
#include <GoogleTest/Library/MockUefiBootServicesTableLib.h>
#include <GoogleTest/Library/MockUefiLib.h>

#include <vector>

extern "C" {
  #include <Uefi.h>
  #include <Pi/PiHob.h>
  #include <Guid/CryptoIndicatorTable.h>
  #include <Guid/EcitCapabilityHob.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/MemoryAllocationLib.h>
  #include <Protocol/CryptoIndicatorRegistration.h>

  EFI_STATUS
  EFIAPI
  CryptoIndicatorTableDxeEntryPoint (
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
    );
}

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrictMock;

TEST (CryptoIndicatorTableDxeTest, RegistersAndPublishesEntries) {
  StrictMock<MockHobLib>                      HobLib;
  StrictMock<MockUefiBootServicesTableLib>    BootServices;
  StrictMock<MockUefiLib>                     UefiLib;
  EFI_CRYPTO_INDICATOR_REGISTRATION_PROTOCOL  *Registration;
  EFI_EVENT_NOTIFY                            ReadyToBootNotify;
  EFI_EVENT                                   ReadyToBootEvent;
  EFI_CRYPTO_INDICATOR_TABLE                  *PublishedTable;
  EFI_HOB_GUID_TYPE                           *GuidHob;
  EDKII_ECIT_CAPABILITY_HOB                   *CapabilityHob;
  STATIC CONST EFI_GUID                       Feature1          = EFI_ECIT_FEATURE_SB_IMAGE_VERIFICATION_GUID;
  STATIC CONST EFI_GUID                       Feature2          = EFI_ECIT_FEATURE_SB_IMAGE_AUTHORIZATION_GUID;
  STATIC CONST UINT8                          PeiPayload[]      = { 0xA, 0xB };
  STATIC CONST UINT8                          ExpectedPayload[] = { 1, 2, 3 };
  UINT8                                       Payload[]         = { 1, 2, 3 };
  CONST UINTN                                 HobDataSize       = ALIGN_VALUE (
                                                                    sizeof (EDKII_ECIT_CAPABILITY_HOB) + sizeof (PeiPayload),
                                                                    8
                                                                    );
  std::vector<UINT8>  Hob;

  Registration      = nullptr;
  ReadyToBootNotify = nullptr;
  ReadyToBootEvent  = (EFI_EVENT)(UINTN)0x1234;
  PublishedTable    = nullptr;
  Hob.resize (sizeof (*GuidHob) + HobDataSize, 0);

  GuidHob                   = (EFI_HOB_GUID_TYPE *)Hob.data ();
  GuidHob->Header.HobType   = EFI_HOB_TYPE_GUID_EXTENSION;
  GuidHob->Header.HobLength = (UINT16)Hob.size ();
  CopyGuid (&GuidHob->Name, &gEdkiiEcitCapabilityHobGuid);
  CapabilityHob                    = (EDKII_ECIT_CAPABILITY_HOB *)GET_GUID_HOB_DATA (GuidHob);
  CapabilityHob->Revision          = EDKII_ECIT_CAPABILITY_HOB_REVISION;
  CapabilityHob->HeaderSize        = sizeof (*CapabilityHob);
  CapabilityHob->EntryDataSize     = sizeof (PeiPayload);
  CapabilityHob->FeatureIdentifier = Feature1;
  CopyMem (CapabilityHob->EntryData, PeiPayload, sizeof (PeiPayload));

  EXPECT_CALL (HobLib, GetFirstGuidHob (_))
    .WillOnce (Return (GuidHob));
  EXPECT_CALL (HobLib, GetNextGuidHob (_, _))
    .WillOnce (Return (nullptr));
  EXPECT_CALL (UefiLib, EfiCreateEventReadyToBootEx (_, _, _, _))
    .WillOnce (
       Invoke (
         [&ReadyToBootNotify, ReadyToBootEvent] (
                                                 EFI_TPL           NotifyTpl,
                                                 EFI_EVENT_NOTIFY  NotifyFunction,
                                                 VOID              *NotifyContext,
                                                 EFI_EVENT         *Event
         ) -> EFI_STATUS {
    ReadyToBootNotify = NotifyFunction;
    *Event            = ReadyToBootEvent;
    return EFI_SUCCESS;
  }
         )
       );
  EXPECT_CALL (BootServices, gBS_InstallProtocolInterface (_, _, _, _))
    .WillOnce (
       Invoke (
         [&Registration] (
                          EFI_HANDLE         *Handle,
                          EFI_GUID           *Protocol,
                          EFI_INTERFACE_TYPE InterfaceType,
                          VOID               *Interface
         ) -> EFI_STATUS {
    Registration = (EFI_CRYPTO_INDICATOR_REGISTRATION_PROTOCOL *)Interface;
    return EFI_SUCCESS;
  }
         )
       );

  EXPECT_EQ (CryptoIndicatorTableDxeEntryPoint (nullptr, nullptr), EFI_SUCCESS);
  ASSERT_NE (Registration, nullptr);
  ASSERT_NE (ReadyToBootNotify, nullptr);
  EXPECT_EQ (Registration->Revision, (UINT32)EFI_CRYPTO_INDICATOR_REGISTRATION_PROTOCOL_REVISION);

  EXPECT_EQ (Registration->RegisterEntry (nullptr, &Feature1, Payload, sizeof (Payload)), EFI_INVALID_PARAMETER);
  EXPECT_EQ (Registration->RegisterEntry (Registration, nullptr, Payload, sizeof (Payload)), EFI_INVALID_PARAMETER);
  EXPECT_EQ (Registration->RegisterEntry (Registration, &Feature1, nullptr, sizeof (Payload)), EFI_INVALID_PARAMETER);
  EXPECT_EQ (Registration->RegisterEntry (Registration, &Feature1, Payload, MAX_UINT16), EFI_INVALID_PARAMETER);
  EXPECT_EQ (Registration->RegisterEntry (Registration, &Feature1, Payload, sizeof (Payload)), EFI_ALREADY_STARTED);
  EXPECT_EQ (Registration->RegisterEntry (Registration, &Feature2, Payload, sizeof (Payload)), EFI_SUCCESS);
  EXPECT_EQ (Registration->RegisterEntry (Registration, &Feature2, Payload, sizeof (Payload)), EFI_ALREADY_STARTED);

  //
  // The protocol owns a copy and must not observe later caller changes.
  //
  SetMem (Payload, sizeof (Payload), 0xFF);

  EXPECT_CALL (BootServices, gBS_AllocatePool (EfiACPIReclaimMemory, _, _))
    .WillOnce (
       Invoke (
         [] (
             EFI_MEMORY_TYPE  PoolType,
             UINTN            Size,
             VOID             **Buffer
         ) -> EFI_STATUS {
    *Buffer = AllocateZeroPool (Size);
    return (*Buffer == nullptr) ? EFI_OUT_OF_RESOURCES : EFI_SUCCESS;
  }
         )
       );
  EXPECT_CALL (BootServices, gBS_InstallConfigurationTable (_, _))
    .WillOnce (
       Invoke (
         [&PublishedTable] (
                            EFI_GUID  *Guid,
                            VOID      *TableBuffer
         ) -> EFI_STATUS {
    UINT8  Checksum;

    EXPECT_TRUE (CompareGuid (Guid, &gEfiCryptoIndicatorTableGuid));
    PublishedTable = (EFI_CRYPTO_INDICATOR_TABLE *)TableBuffer;
    EXPECT_EQ (PublishedTable->NumberOfEntries, 2);
    EXPECT_EQ (PublishedTable->Length, sizeof (EFI_CRYPTO_INDICATOR_TABLE) + 64);

    Checksum = 0;
    for (UINTN Index = 0; Index < PublishedTable->Length; Index++) {
      Checksum = (UINT8)(Checksum + ((UINT8 *)PublishedTable)[Index]);
    }

    EXPECT_EQ (Checksum, 0);
    return EFI_SUCCESS;
  }
         )
       );
  EXPECT_CALL (BootServices, gBS_LocateProtocol (_, nullptr, _))
    .WillOnce (Return (EFI_NOT_FOUND));
  EXPECT_CALL (BootServices, gBS_CloseEvent (ReadyToBootEvent))
    .WillOnce (Return (EFI_SUCCESS));

  ReadyToBootNotify (ReadyToBootEvent, nullptr);
  ASSERT_NE (PublishedTable, nullptr);

  EFI_CRYPTO_INDICATOR_ENTRY  *Entry;

  Entry = (EFI_CRYPTO_INDICATOR_ENTRY *)((UINT8 *)PublishedTable + sizeof (EFI_CRYPTO_INDICATOR_TABLE));
  EXPECT_TRUE (CompareGuid (&Entry->FeatureIdentifier, &Feature1));
  EXPECT_EQ (Entry->EntryLength, 32);
  EXPECT_EQ (CompareMem ((UINT8 *)Entry + sizeof (*Entry), PeiPayload, sizeof (PeiPayload)), 0);

  Entry = (EFI_CRYPTO_INDICATOR_ENTRY *)((UINT8 *)Entry + Entry->EntryLength);
  EXPECT_TRUE (CompareGuid (&Entry->FeatureIdentifier, &Feature2));
  EXPECT_EQ (Entry->EntryLength, 32);
  EXPECT_EQ (CompareMem ((UINT8 *)Entry + sizeof (*Entry), ExpectedPayload, sizeof (ExpectedPayload)), 0);

  EXPECT_EQ (Registration->RegisterEntry (Registration, &Feature1, Payload, sizeof (Payload)), EFI_ACCESS_DENIED);
  ReadyToBootNotify (ReadyToBootEvent, nullptr);

  FreePool (PublishedTable);
}

int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
