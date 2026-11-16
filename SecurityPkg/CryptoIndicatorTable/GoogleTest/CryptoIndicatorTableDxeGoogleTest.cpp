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
#include <string>

extern "C" {
  #include <Uefi.h>
  #include <Pi/PiHob.h>
  #include <Guid/CryptoIndicatorTable.h>
  #include <Guid/EcitCapabilityHob.h>
  #include <Guid/CryptoIndicatorMmBridge.h>
  #include <Library/BaseLib.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/MemoryAllocationLib.h>
  #include <Protocol/CryptoIndicatorRegistration.h>
  #include <Protocol/MmCommunication2.h>

  EFI_STATUS
  EFIAPI
  CryptoIndicatorTableDxeEntryPoint (
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
    );

  EFI_STATUS
  EFIAPI
  CryptoIndicatorBridgeDxeEntryPoint (
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
    );

  EFI_STATUS
  EFIAPI
  DumpCryptoIndicatorTableEntryPoint (
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
    );
}

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrictMock;

static std::string  mDumpOutput;
static UINTN        mBridgedRecords;

/**
  Capture a formatted ASCII string emitted by the dump application.

  @param[in] Format  Expected to be "%a", followed by the string to capture.
  @return Zero; the captured output is available in mDumpOutput.
**/
extern "C"
UINTN
EFIAPI
AsciiPrint (
  IN CONST CHAR8  *Format,
  ...
  )
{
  VA_LIST  Marker;

  EXPECT_STREQ (Format, "%a");
  VA_START (Marker, Format);
  mDumpOutput += VA_ARG (Marker, CONST CHAR8 *);
  VA_END (Marker);
  return 0;
}

/**
  Check and count a capability forwarded by the test bridge.

  @param[in] FeatureIdentifier  Feature owning the capability.
  @param[in] EntryData          Expected one-byte test payload.
  @param[in] EntryDataSize      Payload size in bytes.
  @retval EFI_SUCCESS           The mock consumed the record.
**/
extern "C"
EFI_STATUS
EFIAPI
EcitRegisterCryptoCapability (
  IN CONST EFI_GUID  *FeatureIdentifier,
  IN CONST VOID      *EntryData,
  IN UINTN           EntryDataSize
  )
{
  EXPECT_TRUE (CompareGuid (FeatureIdentifier, &gEfiEcitFeatureSbImageVerificationGuid));
  EXPECT_EQ (EntryDataSize, 1U);
  EXPECT_EQ (*(CONST UINT8 *)EntryData, 0x5A);
  mBridgedRecords++;
  return EFI_SUCCESS;
}

/**
  Return one sample record from a transport limited to a single page.

  @param[in]     This      Mock MM communication protocol.
  @param[in,out] Physical  Physical address of the communication buffer.
  @param[in,out] Virtual   Virtual address used to fill the response.
  @param[in,out] CommSize  Size of the supplied buffer in bytes.
  @retval EFI_SUCCESS            The response contains one record.
  @retval EFI_INVALID_PARAMETER  The request exceeded one page.
**/
static
EFI_STATUS
EFIAPI
CommunicateOnePage (
  IN CONST EFI_MM_COMMUNICATION2_PROTOCOL  *This,
  IN OUT VOID                              *Physical,
  IN OUT VOID                              *Virtual,
  IN OUT UINTN                             *CommSize
  )
{
  if (*CommSize > EFI_PAGE_SIZE) {
    return EFI_INVALID_PARAMETER;
  }

  auto  Header = (EFI_MM_COMMUNICATE_HEADER *)Virtual;
  auto  Comm   = (ECIT_MM_BRIDGE_COMM *)Header->Data;
  auto  Entry  = (ECIT_MM_BRIDGE_ENTRY *)(Comm + 1);

  EXPECT_EQ (Physical, Virtual);
  EXPECT_EQ (*CommSize, OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data) + Header->MessageLength);
  Comm->ReturnStatus = EFI_SUCCESS;
  Comm->EntryCount   = 1;
  Comm->EntriesSize  = sizeof (*Entry) + 1;
  CopyGuid (&Entry->FeatureIdentifier, &gEfiEcitFeatureSbImageVerificationGuid);
  Entry->DataSize         = 1;
  *((UINT8 *)(Entry + 1)) = 0x5A;
  return EFI_SUCCESS;
}

/** Verify that the bridge imports one MM record at ReadyToBoot. **/
TEST (CryptoIndicatorBridgeDxeTest, ImportsRecordsThroughOnePageTransport) {
  StrictMock<MockUefiBootServicesTableLib>  BootServices;
  StrictMock<MockUefiLib>                   UefiLib;
  EFI_MM_COMMUNICATION2_PROTOCOL            MmComm = { CommunicateOnePage };
  EFI_EVENT_NOTIFY                          Notify = nullptr;

  mBridgedRecords = 0;
  EXPECT_CALL (UefiLib, EfiCreateEventReadyToBootEx (TPL_NOTIFY, _, _, _))
    .WillOnce (
       Invoke (
         [&Notify] (EFI_TPL, EFI_EVENT_NOTIFY Callback, VOID *, EFI_EVENT *Event) {
    Notify = Callback;
    *Event = nullptr;
    return EFI_SUCCESS;
  }
         )
       );
  EXPECT_CALL (BootServices, gBS_LocateProtocol (_, nullptr, _))
    .WillOnce (
       Invoke (
         [&MmComm] (EFI_GUID *, VOID *, VOID **Interface) {
    *Interface = &MmComm;
    return EFI_SUCCESS;
  }
         )
       );
  EXPECT_CALL (BootServices, gBS_CloseEvent (_)).WillOnce (Return (EFI_SUCCESS));

  ASSERT_EQ (CryptoIndicatorBridgeDxeEntryPoint (nullptr, nullptr), EFI_SUCCESS);
  ASSERT_NE (Notify, nullptr);
  Notify (nullptr, nullptr);
  EXPECT_EQ (mBridgedRecords, 1U);
}

/** A missing MM protocol or communicate method must not publish a record. **/
TEST (CryptoIndicatorBridgeDxeTest, SkipsUnusableTransport) {
  StrictMock<MockUefiBootServicesTableLib>  BootServices;
  StrictMock<MockUefiLib>                   UefiLib;
  EFI_MM_COMMUNICATION2_PROTOCOL            MmComm = { };
  EFI_EVENT_NOTIFY                          Notify = nullptr;

  mBridgedRecords = 0;
  EXPECT_CALL (UefiLib, EfiCreateEventReadyToBootEx (TPL_NOTIFY, _, _, _))
    .WillRepeatedly (::testing::DoAll (::testing::SaveArg<1> (&Notify), ::testing::SetArgPointee<3> (nullptr), Return (EFI_SUCCESS)));
  EXPECT_CALL (BootServices, gBS_LocateProtocol (_, nullptr, _))
    .WillOnce (::testing::DoAll (::testing::SetArgPointee<2> (nullptr), Return (EFI_SUCCESS)))
    .WillOnce (::testing::DoAll (::testing::SetArgPointee<2> (&MmComm), Return (EFI_SUCCESS)));
  EXPECT_CALL (BootServices, gBS_CloseEvent (_)).Times (2).WillRepeatedly (Return (EFI_SUCCESS));
  for (UINTN Index = 0; Index < 2; Index++) {
    ASSERT_EQ (CryptoIndicatorBridgeDxeEntryPoint (nullptr, nullptr), EFI_SUCCESS);
    ASSERT_NE (Notify, nullptr);
    Notify (nullptr, nullptr);
  }

  EXPECT_EQ (mBridgedRecords, 0U);
}

/** A successful configuration table lookup may still return no ECIT pointer. **/
TEST (DumpCryptoIndicatorTableTest, ReportsNullConfigurationTable) {
  StrictMock<MockUefiLib>  UefiLib;

  EXPECT_CALL (UefiLib, EfiGetSystemConfigurationTable (&gEfiCryptoIndicatorTableGuid, _))
    .WillOnce (::testing::DoAll (::testing::SetArgPointee<1> (nullptr), Return (EFI_SUCCESS)));
  mDumpOutput.clear ();
  EXPECT_EQ (DumpCryptoIndicatorTableEntryPoint (nullptr, nullptr), EFI_NOT_FOUND);
  EXPECT_NE (mDumpOutput.find ("ECIT: not installed"), std::string::npos);
}

/** Verify that the dump parser never reads beyond the declared payload. **/
TEST (DumpCryptoIndicatorTableTest, BoundsCsvParsingToEntryPayload) {
  StrictMock<MockUefiLib>  UefiLib;
  CONST UINTN              TableSize = sizeof (EFI_CRYPTO_INDICATOR_TABLE) + sizeof (EFI_CRYPTO_INDICATOR_ENTRY) + 8;
  std::vector<UINT8>       Buffer (TableSize + 1, 0);
  auto                     Table   = (EFI_CRYPTO_INDICATOR_TABLE *)Buffer.data ();
  auto                     Entry   = (EFI_CRYPTO_INDICATOR_ENTRY *)(Table + 1);
  auto                     Payload = (CHAR8 *)(Entry + 1);

  CopyMem (Table->Signature, "ECIT", 4);
  Table->Length          = (UINT32)TableSize;
  Table->NumberOfEntries = 1;
  CopyGuid (&Entry->FeatureIdentifier, &gEfiEcitFeatureSbImageVerificationGuid);
  Entry->EntryLength = sizeof (*Entry) + 8;
  CopyMem (Payload, "1.2.3.45", 8);

  EXPECT_CALL (UefiLib, EfiGetSystemConfigurationTable (_, _))
    .Times (3)
    .WillRepeatedly (
       Invoke (
         [Table] (CONST EFI_GUID *, VOID **Result) {
    *Result = Table;
    return EFI_SUCCESS;
  }
         )
       );

  for (UINTN Case = 0; Case < 3; Case++) {
    if (Case == 1) {
      Payload[7] = '\0';
    } else if (Case == 2) {
      Payload[0] = '\0';
    }

    Table->Checksum = 0;
    Table->Checksum = CalculateCheckSum8 (Buffer.data (), TableSize);
    mDumpOutput.clear ();
    EXPECT_EQ (DumpCryptoIndicatorTableEntryPoint (nullptr, nullptr), EFI_SUCCESS);
    if (Case == 0) {
      EXPECT_NE (mDumpOutput.find ("missing NUL terminator"), std::string::npos);
    } else if (Case == 1) {
      EXPECT_EQ (mDumpOutput.find ("malformed"), std::string::npos);
      EXPECT_NE (mDumpOutput.find ("UnknownOids:"), std::string::npos);
    } else {
      EXPECT_NE (mDumpOutput.find ("(none)"), std::string::npos);
    }
  }
}

/** Verify that PEI and DXE records are copied, sealed, and published together. **/
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
  static CONST EFI_GUID                       Feature1          = EFI_ECIT_FEATURE_SB_IMAGE_VERIFICATION_GUID;
  static CONST EFI_GUID                       Feature2          = EFI_ECIT_FEATURE_SB_IMAGE_AUTHORIZATION_GUID;
  static CONST UINT8                          PeiPayload[]      = { 0xA, 0xB };
  static CONST UINT8                          ExpectedPayload[] = { 1, 2, 3 };
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
  EXPECT_EQ (Registration->RegisterEntry (Registration, &Feature1, Payload, 65505), EFI_INVALID_PARAMETER);
  EXPECT_EQ (Registration->RegisterEntry (Registration, &Feature1, Payload, MAX_UINTN), EFI_INVALID_PARAMETER);
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

/**
  Run the ECIT host tests.

  @param[in] argc  Number of command-line arguments.
  @param[in] argv  Command-line arguments for GoogleTest.
  @return GoogleTest exit status.
**/
int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
