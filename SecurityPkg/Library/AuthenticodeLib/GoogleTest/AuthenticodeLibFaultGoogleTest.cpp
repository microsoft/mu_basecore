/** @file
  Fault-injection tests for AuthenticodeLib's parsing and allocation paths.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

// MU_CHANGE - NEW FILE

#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockMemoryAllocationLib.h>
#include <GoogleTest/Library/MockPeCoffLib.h>
#include <GoogleTest/Library/MockSafeIntLib.h>

#include <cstdlib>
#include <vector>

extern "C" {
  #include <Uefi.h>
  #include <IndustryStandard/PeImage.h>
  #include <Library/AuthenticodeLib.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/MemoryAllocationLib.h>
  #include "AuthenticodeLibInternalTest.h"
}

using namespace testing;

class AuthenticodeLibFaultTest : public Test {
protected:
  NiceMock<MockMemoryAllocationLib> Memory;
  NiceMock<MockPeCoffLib> PeCoff;
  NiceMock<MockSafeIntLib> SafeInt;
  std::vector<UINT8> Image;
  UINT32 PeOffset;
  UINTN SizeOfHeaders;
  BOOLEAN IsTeImage;
  EFI_IMAGE_DATA_DIRECTORY SecurityDirectory;
  RETURN_STATUS LoaderStatus;

  EFI_IMAGE_NT_HEADERS64 *
  Header (
    )
  {
    return (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + 0x40);
  }

  void
  PopulateContext (
    PE_COFF_LOADER_IMAGE_CONTEXT  *Context
    )
  {
    Context->PeCoffHeaderOffset    = PeOffset;
    Context->SizeOfHeaders         = SizeOfHeaders;
    Context->IsTeImage             = IsTeImage;
    Context->SecurityDataDirectory = SecurityDirectory;
  }

  void
  SetUp (
    ) override
  {
    Image.assign (0x400, 0);
    PeOffset          = 0x40;
    SizeOfHeaders     = 0x200;
    IsTeImage         = FALSE;
    SecurityDirectory = { };
    LoaderStatus      = RETURN_SUCCESS;

    Header ()->Signature                          = EFI_IMAGE_NT_SIGNATURE;
    Header ()->FileHeader.SizeOfOptionalHeader    = sizeof (EFI_IMAGE_OPTIONAL_HEADER64);
    Header ()->OptionalHeader.Magic               = EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    Header ()->OptionalHeader.NumberOfRvaAndSizes = EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES;
    Image[Image.size () - 2]                      = 0xA1;
    Image[Image.size () - 1]                      = 0xB2;

    ON_CALL (PeCoff, PeCoffLoaderGetImageInfo (_))
      .WillByDefault (
         Invoke (
           [this] (PE_COFF_LOADER_IMAGE_CONTEXT *Context) -> RETURN_STATUS {
      PopulateContext (Context);
      return LoaderStatus;
    }
           )
         );
    EXPECT_CALL (PeCoff, PeCoffLoaderGetImageInfo (_)).Times (AnyNumber ());

    ON_CALL (Memory, AllocatePool (_))
      .WillByDefault (
         Invoke (
           [] (UINTN Size) -> VOID * {
      return std::malloc (Size);
    }
           )
         );
    ON_CALL (Memory, FreePool (_))
      .WillByDefault (
         Invoke (
           [] (VOID *Buffer) {
      std::free (Buffer);
    }
           )
         );
    EXPECT_CALL (Memory, AllocatePool (_)).Times (AnyNumber ());
    EXPECT_CALL (Memory, FreePool (_)).Times (AnyNumber ());

    ON_CALL (SafeInt, SafeUintnAdd (_, _, _))
      .WillByDefault (
         Invoke (
           [] (UINTN Left, UINTN Right, UINTN *Result) -> RETURN_STATUS {
      if (Left > MAX_UINTN - Right) {
        *Result = MAX_UINTN;
        return RETURN_BUFFER_TOO_SMALL;
      }

      *Result = Left + Right;
      return RETURN_SUCCESS;
    }
           )
         );
    ON_CALL (SafeInt, SafeUintnMult (_, _, _))
      .WillByDefault (
         Invoke (
           [] (UINTN Left, UINTN Right, UINTN *Result) -> RETURN_STATUS {
      if ((Right != 0) && (Left > MAX_UINTN / Right)) {
        *Result = MAX_UINTN;
        return RETURN_BUFFER_TOO_SMALL;
      }

      *Result = Left * Right;
      return RETURN_SUCCESS;
    }
           )
         );
    EXPECT_CALL (SafeInt, SafeUintnAdd (_, _, _)).Times (AnyNumber ());
    EXPECT_CALL (SafeInt, SafeUintnMult (_, _, _)).Times (AnyNumber ());
  }

  void
  ExpectBuildError (
    EFI_STATUS  Expected
    )
  {
    UINT8  *AuthImage    = (UINT8 *)(UINTN)1;
    UINTN  AuthImageSize = 123;

    EXPECT_EQ (BuildAuthenticodeImage (Image.data (), Image.size (), &AuthImage, &AuthImageSize), Expected);
    EXPECT_EQ (AuthImage, nullptr);
    EXPECT_EQ (AuthImageSize, (UINTN)0);
  }

  void
  AddSection (
    UINT32  Offset,
    UINT32  Size
    )
  {
    Header ()->FileHeader.NumberOfSections = 1;
    EFI_IMAGE_SECTION_HEADER  *Section = (EFI_IMAGE_SECTION_HEADER *)((UINT8 *)Header () + sizeof (*Header ()));

    Section->PointerToRawData = Offset;
    Section->SizeOfRawData    = Size;
  }
};

TEST_F (AuthenticodeLibFaultTest, ImageReadChecksInvalidAndTruncatedReads) {
  EXPECT_CALL (PeCoff, PeCoffLoaderGetImageInfo (_))
    .WillOnce (
       Invoke (
         [this] (PE_COFF_LOADER_IMAGE_CONTEXT *Context) -> RETURN_STATUS {
    UINT8  Buffer[4] = { 0 };
    UINTN  Size      = sizeof (Buffer);

    EXPECT_EQ (Context->ImageRead (NULL, 0, &Size, Buffer), RETURN_INVALID_PARAMETER);
    EXPECT_EQ (Context->ImageRead (Context->Handle, 0, NULL, Buffer), RETURN_INVALID_PARAMETER);
    EXPECT_EQ (Context->ImageRead (Context->Handle, 0, &Size, NULL), RETURN_INVALID_PARAMETER);
    EXPECT_EQ (Context->ImageRead (Context->Handle, Image.size (), &Size, Buffer), RETURN_SUCCESS);
    EXPECT_EQ (Size, (UINTN)0);
    Size = sizeof (Buffer);
    EXPECT_EQ (Context->ImageRead (Context->Handle, Image.size () - 2, &Size, Buffer), RETURN_SUCCESS);
    EXPECT_EQ (Size, (UINTN)2);
    EXPECT_EQ (Buffer[0], 0xA1);
    EXPECT_EQ (Buffer[1], 0xB2);
    PopulateContext (Context);
    return RETURN_SUCCESS;
  }
         )
       );

  UINT8  *AuthImage    = NULL;
  UINTN  AuthImageSize = 0;

  ASSERT_EQ (BuildAuthenticodeImage (Image.data (), Image.size (), &AuthImage, &AuthImageSize), EFI_SUCCESS);
  FreePool (AuthImage);
}

TEST_F (AuthenticodeLibFaultTest, LoaderFailureIsReportedByBothApis) {
  LoaderStatus = RETURN_UNSUPPORTED;
  ExpectBuildError (EFI_VOLUME_CORRUPTED);

  CONST UINT8  *Certificates = (CONST UINT8 *)(UINTN)1;
  UINTN        Length        = 123;

  EXPECT_EQ (GetWinCertificates (Image.data (), Image.size (), &Certificates, &Length), EFI_VOLUME_CORRUPTED);
  EXPECT_EQ (Certificates, nullptr);
  EXPECT_EQ (Length, (UINTN)0);
}

TEST_F (AuthenticodeLibFaultTest, TeImageIsUnsupportedForAssembly) {
  IsTeImage = TRUE;
  ExpectBuildError (EFI_UNSUPPORTED);
}

TEST_F (AuthenticodeLibFaultTest, RejectsHeaderSizeBeyondFile) {
  SizeOfHeaders = Image.size () + 1;
  ExpectBuildError (EFI_VOLUME_CORRUPTED);
}

TEST_F (AuthenticodeLibFaultTest, RejectsHeaderOffsetBeyondFile) {
  PeOffset = MAX_UINT32;
  ExpectBuildError (EFI_VOLUME_CORRUPTED);
}

TEST_F (AuthenticodeLibFaultTest, RejectsTruncatedPe32PlusHeader) {
  Image.resize (PeOffset + sizeof (EFI_IMAGE_NT_HEADERS32));
  SizeOfHeaders = 0;
  ExpectBuildError (EFI_VOLUME_CORRUPTED);
}

TEST_F (AuthenticodeLibFaultTest, RejectsUnknownOptionalHeaderMagic) {
  Header ()->OptionalHeader.Magic = 0xABCD;
  ExpectBuildError (EFI_VOLUME_CORRUPTED);
}

TEST_F (AuthenticodeLibFaultTest, RejectsTooManyDataDirectories) {
  Header ()->OptionalHeader.NumberOfRvaAndSizes = EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES + 1;
  ExpectBuildError (EFI_VOLUME_CORRUPTED);
}

TEST_F (AuthenticodeLibFaultTest, RejectsInconsistentOptionalHeaderSize) {
  Header ()->FileHeader.SizeOfOptionalHeader--;
  ExpectBuildError (EFI_VOLUME_CORRUPTED);
}

TEST_F (AuthenticodeLibFaultTest, RejectsChecksumOutsideHeaders) {
  SizeOfHeaders = PeOffset + OFFSET_OF (EFI_IMAGE_NT_HEADERS64, OptionalHeader.CheckSum) + sizeof (UINT32) - 1;
  ExpectBuildError (EFI_VOLUME_CORRUPTED);
}

TEST_F (AuthenticodeLibFaultTest, RejectsSecurityDirectoryOutsideHeaders) {
  SizeOfHeaders = PeOffset + OFFSET_OF (EFI_IMAGE_NT_HEADERS64, OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY]) + sizeof (EFI_IMAGE_DATA_DIRECTORY) - 1;
  ExpectBuildError (EFI_VOLUME_CORRUPTED);
}

TEST_F (AuthenticodeLibFaultTest, RejectsSectionTableOutsideHeaders) {
  Header ()->FileHeader.NumberOfSections = 1;
  SizeOfHeaders                          = PeOffset + sizeof (EFI_IMAGE_NT_HEADERS64) + sizeof (EFI_IMAGE_SECTION_HEADER) - 1;
  ExpectBuildError (EFI_VOLUME_CORRUPTED);
}

TEST_F (AuthenticodeLibFaultTest, RejectsOutOfBoundsCertificateRange) {
  SecurityDirectory.VirtualAddress = (UINT32)Image.size ();
  SecurityDirectory.Size           = sizeof (WIN_CERTIFICATE);
  ExpectBuildError (EFI_VOLUME_CORRUPTED);

  CONST UINT8  *Certificates = (CONST UINT8 *)(UINTN)1;
  UINTN        Length        = 123;

  EXPECT_EQ (GetWinCertificates (Image.data (), Image.size (), &Certificates, &Length), EFI_VOLUME_CORRUPTED);
  EXPECT_EQ (Certificates, nullptr);
  EXPECT_EQ (Length, (UINTN)0);
}

TEST_F (AuthenticodeLibFaultTest, ReportsOutputAllocationFailure) {
  EXPECT_CALL (Memory, AllocatePool (_)).WillOnce (Return (nullptr));
  ExpectBuildError (EFI_OUT_OF_RESOURCES);
}

TEST_F (AuthenticodeLibFaultTest, ReportsSectionAllocationFailureAndFreesOutput) {
  AddSection (0x200, 0x80);
  EXPECT_CALL (Memory, AllocatePool (_))
    .WillOnce (
       Invoke (
         [] (UINTN Size) -> VOID * {
    return std::malloc (Size);
  }
         )
       )
    .WillOnce (Return (nullptr));
  EXPECT_CALL (Memory, FreePool (_)).Times (1);
  ExpectBuildError (EFI_OUT_OF_RESOURCES);
}

TEST_F (AuthenticodeLibFaultTest, ReportsChecksumEndArithmeticFailure) {
  const UINTN  Offset = PeOffset + OFFSET_OF (EFI_IMAGE_NT_HEADERS64, OptionalHeader.CheckSum);

  EXPECT_CALL (SafeInt, SafeUintnAdd (Offset, sizeof (UINT32), _))
    .WillOnce (Return (RETURN_BUFFER_TOO_SMALL));
  ExpectBuildError (EFI_VOLUME_CORRUPTED);
}

TEST_F (AuthenticodeLibFaultTest, ReportsSecurityDirectoryEndArithmeticFailure) {
  const UINTN  Offset = PeOffset + OFFSET_OF (EFI_IMAGE_NT_HEADERS64, OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY]);

  EXPECT_CALL (SafeInt, SafeUintnAdd (Offset, sizeof (EFI_IMAGE_DATA_DIRECTORY), _))
    .WillOnce (Return (RETURN_BUFFER_TOO_SMALL));
  ExpectBuildError (EFI_VOLUME_CORRUPTED);
}

TEST_F (AuthenticodeLibFaultTest, ReportsSectionAllocationSizeArithmeticFailure) {
  AddSection (0x200, 0x80);
  EXPECT_CALL (SafeInt, SafeUintnMult (1, sizeof (UINT32) * 2, _))
    .WillOnce (Return (RETURN_BUFFER_TOO_SMALL));
  ExpectBuildError (EFI_VOLUME_CORRUPTED);
}

TEST_F (AuthenticodeLibFaultTest, ReportsSectionHeaderOffsetArithmeticFailure) {
  AddSection (0x200, 0x80);
  const UINTN  Offset = PeOffset + sizeof (EFI_IMAGE_NT_HEADERS64);

  EXPECT_CALL (SafeInt, SafeUintnAdd (Offset, sizeof (EFI_IMAGE_SECTION_HEADER), _))
    .WillOnce (Return (RETURN_BUFFER_TOO_SMALL));
  ExpectBuildError (EFI_VOLUME_CORRUPTED);
}

TEST_F (AuthenticodeLibFaultTest, ReportsSectionEndArithmeticFailure) {
  AddSection (0x200, 0x80);
  EXPECT_CALL (SafeInt, SafeUintnAdd (0x200, 0x80, _))
    .WillOnce (Return (RETURN_BUFFER_TOO_SMALL));
  ExpectBuildError (EFI_VOLUME_CORRUPTED);
}

TEST_F (AuthenticodeLibFaultTest, RejectsSectionStartingInsideHeaders) {
  AddSection (0x1F0, 0x20);
  ExpectBuildError (EFI_VOLUME_CORRUPTED);
}

TEST_F (AuthenticodeLibFaultTest, AppendImageRegionChecksBothBufferBoundaries) {
  CONST UINT8  Input[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
  UINT8        Output[10];

  SetMem (Output, sizeof (Output), 0xCC);
  const auto  ExpectRejected = [&] (UINTN Offset, UINTN Size, UINTN Position) {
                                 UINTN  OutPos = Position;

                                 EXPECT_EQ (TestAppendImageRegion (Input, sizeof (Input), Offset, Size, Output + 1, &OutPos), EFI_VOLUME_CORRUPTED);
                                 EXPECT_EQ (OutPos, Position);
                                 for (UINT8 Byte : Output) {
                                   EXPECT_EQ (Byte, 0xCC);
                                 }
                               };

  ExpectRejected (sizeof (Input) + 1, 0, 0);
  ExpectRejected (sizeof (Input), 1, 0);
  ExpectRejected (MAX_UINTN, 1, 0);
  ExpectRejected (0, 1, sizeof (Input) + 1);
  ExpectRejected (0, 2, sizeof (Input) - 1);
  ExpectRejected (0, MAX_UINTN, 0);

  UINTN  OutPos = sizeof (Input) - 1;

  ASSERT_EQ (TestAppendImageRegion (Input, sizeof (Input), sizeof (Input) - 1, 1, Output + 1, &OutPos), EFI_SUCCESS);
  EXPECT_EQ (OutPos, sizeof (Input));
  EXPECT_EQ (Output[8], Input[7]);
  EXPECT_EQ (Output[0], 0xCC);
  EXPECT_EQ (Output[9], 0xCC);
  EXPECT_EQ (TestAppendImageRegion (Input, sizeof (Input), sizeof (Input), 0, Output + 1, &OutPos), EFI_SUCCESS);
  EXPECT_EQ (OutPos, sizeof (Input));
}

TEST_F (AuthenticodeLibFaultTest, HeaderAssemblyRejectsInvalidRanges) {
  UINT8  Input[16]  = { 0 };
  UINT8  Output[16] = { 0 };
  UINTN  OutPos     = 0;

  EXPECT_EQ (TestAppendAuthenticodeHeaders (Input, sizeof (Input), 17, 0, 16, 5, Output, &OutPos), EFI_VOLUME_CORRUPTED);
  EXPECT_EQ (OutPos, (UINTN)0);
  EXPECT_EQ (TestAppendAuthenticodeHeaders (Input, sizeof (Input), 0, 0, 3, 4, Output, &OutPos), EFI_VOLUME_CORRUPTED);
  EXPECT_EQ (TestAppendAuthenticodeHeaders (Input, sizeof (Input), 0, 3, 16, 5, Output, &OutPos), EFI_VOLUME_CORRUPTED);
  EXPECT_EQ (TestAppendAuthenticodeHeaders (Input, sizeof (Input), 0, 17, 16, 5, Output, &OutPos), EFI_VOLUME_CORRUPTED);
  EXPECT_EQ (TestAppendAuthenticodeHeaders (Input, sizeof (Input), 0, 8, 15, 5, Output, &OutPos), EFI_VOLUME_CORRUPTED);
}

TEST_F (AuthenticodeLibFaultTest, SectionAssemblyRejectsTruncatedSectionHeader) {
  UINT8  Input[128]  = { 0 };
  UINT8  Output[128] = { 0 };
  UINTN  OutPos      = 0;
  UINTN  NextOffset  = 0;

  EXPECT_EQ (
    TestAppendSections (Input, sizeof (Input), 100, 1, 0, sizeof (Input), Output, &OutPos, &NextOffset),
    EFI_VOLUME_CORRUPTED
    );
  EXPECT_EQ (OutPos, (UINTN)0);
  EXPECT_EQ (NextOffset, (UINTN)0);
}

TEST_F (AuthenticodeLibFaultTest, SectionAssemblyRejectsDestinationOverflow) {
  UINT8                     Input[128]  = { 0 };
  UINT8                     Output[130] = { 0 };
  EFI_IMAGE_SECTION_HEADER  Section     = { };
  UINTN                     OutPos      = 127;
  UINTN                     NextOffset  = 40;

  Section.PointerToRawData = 48;
  Section.SizeOfRawData    = 8;
  CopyMem (Input, &Section, sizeof (Section));
  EXPECT_EQ (
    TestAppendSections (Input, sizeof (Input), 0, 1, 40, sizeof (Input), Output + 1, &OutPos, &NextOffset),
    EFI_VOLUME_CORRUPTED
    );
  EXPECT_EQ (OutPos, (UINTN)127);
  EXPECT_EQ (NextOffset, (UINTN)40);

  Section.PointerToRawData = 40;
  CopyMem (Input, &Section, sizeof (Section));
  OutPos = 124;
  EXPECT_EQ (
    TestAppendSections (Input, sizeof (Input), 0, 1, 40, sizeof (Input), Output + 1, &OutPos, &NextOffset),
    EFI_VOLUME_CORRUPTED
    );
  EXPECT_EQ (OutPos, (UINTN)124);
  EXPECT_EQ (NextOffset, (UINTN)40);
  EXPECT_EQ (Output[0], 0);
  EXPECT_EQ (Output[129], 0);
}

TEST_F (AuthenticodeLibFaultTest, RemainingDataChecksSourceAndDestinationBounds) {
  CONST UINT8  Input[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
  UINT8        Output[10];
  UINTN        OutPos = 0;

  SetMem (Output, sizeof (Output), 0xCC);
  EXPECT_EQ (TestAppendRemainingImageData (Input, sizeof (Input), 5, 4, Output + 1, &OutPos), EFI_VOLUME_CORRUPTED);
  EXPECT_EQ (TestAppendRemainingImageData (Input, sizeof (Input), 4, 9, Output + 1, &OutPos), EFI_VOLUME_CORRUPTED);
  OutPos = 7;
  EXPECT_EQ (TestAppendRemainingImageData (Input, sizeof (Input), 4, 6, Output + 1, &OutPos), EFI_VOLUME_CORRUPTED);
  EXPECT_EQ (OutPos, (UINTN)7);
  EXPECT_EQ (Output[0], 0xCC);
  EXPECT_EQ (Output[9], 0xCC);

  OutPos = 6;
  ASSERT_EQ (TestAppendRemainingImageData (Input, sizeof (Input), 4, 6, Output + 1, &OutPos), EFI_SUCCESS);
  EXPECT_EQ (OutPos, (UINTN)8);
  EXPECT_EQ (Output[7], Input[4]);
  EXPECT_EQ (Output[8], Input[5]);
  EXPECT_EQ (Output[0], 0xCC);
  EXPECT_EQ (Output[9], 0xCC);
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
