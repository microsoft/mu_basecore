/** @file
  Image Validation unit test application.

  This UEFI application exercises the platform's secure boot image Validation at a high
  level. For each scenario it:
    1. Installs a GetVariable() hook that returns scenario-specific `db` and `dbx`
       signature databases and reports `SecureBoot` as enabled (so the handler enforces
       Validation), falling back to the real GetVariable() for all other variables.
    2. Calls gBS->LoadImage() on a built-in PE/COFF image. On a platform with Secure Boot
       enabled, the DXE core invokes the registered image Validation handler, which reads
       the `db` / `dbx` served by the hook.
    3. Asserts that LoadImage() returns the expected EFI_STATUS.

  Because Validation is performed by the platform's already-registered handler (reached
  through gBS->LoadImage()), the application itself needs no security or cryptography
  libraries.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
//
// Included for the DEBUG_LINE_NUMBER macro that UnitTestLib's UT_ASSERT_* macros expand to.
//
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PeCoffLib.h>
#include <Guid/ImageAuthentication.h>
#include <Guid/GlobalVariable.h>
#include <Protocol/DevicePath.h>
#include <IndustryStandard/PeImage.h>

#include "ImageValidationTestApp.h"
#include "TestData.h"

#define UNIT_TEST_NAME     "Image Validation Test App"
#define UNIT_TEST_VERSION  "0.1"

//
// SignatureOwner GUID stamped into every EFI_SIGNATURE_DATA this test builds.
//
STATIC CONST EFI_GUID  mTestSignatureOwner = {
  0x6f9a1c44, 0x2d8b, 0x4e3a, { 0x9b, 0x21, 0x0c, 0x7d, 0x5e, 0x14, 0xa8, 0x3f }
};

//
// Size, in bytes, of the filler payload for the deliberately-malformed signature list built
// for the DB_STATE_CORRUPT_SIGNATURE_LIST_* flags. A SHA-256 digest length is used as a
// representative signature size; its size fields (not its contents) are what make it corrupt.
//
#define CORRUPT_SIGNATURE_PAYLOAD_SIZE  32

//
// Filler payload for the deliberately-malformed signature list built for the
// DB_STATE_CORRUPT_SIGNATURE_LIST_* flags; its size fields (not its contents) are what make it
// corrupt.
//
STATIC CONST UINT8  mCorruptSignaturePayload[CORRUPT_SIGNATURE_PAYLOAD_SIZE] = { 0 };

//
// A fake device path to result in an unknown image source allowing the Image Validation handler
// to be fully invoked.
//
#pragma pack (1)
typedef struct {
  MEMMAP_DEVICE_PATH          MemMap;
  EFI_DEVICE_PATH_PROTOCOL    End;
} TEST_IMAGE_DEVICE_PATH;
#pragma pack ()

STATIC TEST_IMAGE_DEVICE_PATH  mTestImageDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_MEMMAP_DP,
      { (UINT8)sizeof (MEMMAP_DEVICE_PATH),       (UINT8)(sizeof (MEMMAP_DEVICE_PATH) >> 8) }
    },
    EfiBootServicesData,
    0,
    0
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { (UINT8)sizeof (EFI_DEVICE_PATH_PROTOCOL), 0                                         }
  }
};

//
// GetVariable() hook state.
//
// mOriginalGetVariable holds the runtime services GetVariable() that was present before the
// hook was installed; the hook chains to it for every variable it does not synthesize.
// mActiveDb / mActiveDbx point at the `db` / `dbx` databases the scenario runner built for
// the test currently executing, so the hook knows what to return for those two variables.
//
STATIC EFI_GET_VARIABLE  mOriginalGetVariable = NULL;
STATIC CONST UINT8       *mActiveDb           = NULL;
STATIC UINTN             mActiveDbSize        = 0;
STATIC CONST UINT8       *mActiveDbx          = NULL;
STATIC UINTN             mActiveDbxSize       = 0;

//
// Value served for the `SecureBoot` variable. The Validation handler skips Validation
// (returns EFI_SUCCESS) when `SecureBoot` is absent or disabled, so the hook always reports
// it enabled to force the handler down the real signature/hash validation path regardless
// of the platform's actual state.
//
STATIC CONST UINT8  mSecureBootEnabled = SECURE_BOOT_MODE_ENABLE;

/**
  Copy the source buffer holding a variable value into the caller's buffer.

  Update the attributes and data size as needed.

  @param[in]      Source            Buffer holding the variable value, or NULL if the
                                    variable does not exist.
  @param[in]      SourceSize        Size, in bytes, of Source.
  @param[in]      SourceAttributes  Attributes to report for the variable.
  @param[out]     Attributes        Optional. Receives SourceAttributes.
  @param[in, out] DataSize          On input, the size of Data. On output, the size of the
                                    variable value.
  @param[out]     Data              Optional. Receives the variable value.

  @retval EFI_SUCCESS            The value was returned in Data.
  @retval EFI_NOT_FOUND          Source is NULL or SourceSize is 0.
  @retval EFI_BUFFER_TOO_SMALL   Data was too small; DataSize has been updated.
  @retval EFI_INVALID_PARAMETER  Data was NULL but the buffer was large enough.
**/
STATIC
EFI_STATUS
ServeVariable (
  IN     CONST UINT8  *Source,
  IN     UINTN        SourceSize,
  IN     UINT32       SourceAttributes,
  OUT    UINT32       *Attributes  OPTIONAL,
  IN OUT UINTN        *DataSize,
  OUT    VOID         *Data         OPTIONAL
  )
{
  if ((Source == NULL) || (SourceSize == 0)) {
    return EFI_NOT_FOUND;
  }

  if (Attributes != NULL) {
    *Attributes = SourceAttributes;
  }

  if (*DataSize < SourceSize) {
    *DataSize = SourceSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  if (Data == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (Data, Source, SourceSize);
  *DataSize = SourceSize;
  return EFI_SUCCESS;
}

/**
  GetVariable() hook that returns scenario-specific secure boot databases instead of the real ones.

  Falls back to the original GetVariable() implementation for all other variables.

  @param[in]      VariableName  The name of the variable to read.
  @param[in]      VendorGuid    The vendor GUID of the variable.
  @param[out]     Attributes    Optional. Receives the variable attributes.
  @param[in, out] DataSize      On input, the size of Data; on output, the size of the
                                variable value.
  @param[out]     Data          Optional. Receives the variable value.

  @return The status of the synthesized or forwarded GetVariable() call.
**/
STATIC
EFI_STATUS
EFIAPI
HookedGetVariable (
  IN     CHAR16    *VariableName,
  IN     EFI_GUID  *VendorGuid,
  OUT    UINT32    *Attributes  OPTIONAL,
  IN OUT UINTN     *DataSize,
  OUT    VOID      *Data         OPTIONAL
  )
{
  if ((VariableName == NULL) || (VendorGuid == NULL) || (DataSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((StrCmp (VariableName, EFI_IMAGE_SECURITY_DATABASE) == 0) &&
      CompareGuid (VendorGuid, &gEfiImageSecurityDatabaseGuid))
  {
    return ServeVariable (
             mActiveDb,
             mActiveDbSize,
             EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
             Attributes,
             DataSize,
             Data
             );
  }

  if ((StrCmp (VariableName, EFI_IMAGE_SECURITY_DATABASE1) == 0) &&
      CompareGuid (VendorGuid, &gEfiImageSecurityDatabaseGuid))
  {
    return ServeVariable (
             mActiveDbx,
             mActiveDbxSize,
             EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
             Attributes,
             DataSize,
             Data
             );
  }

  if ((StrCmp (VariableName, EFI_SECURE_BOOT_MODE_NAME) == 0) &&
      CompareGuid (VendorGuid, &gEfiGlobalVariableGuid))
  {
    //
    // Force Secure Boot to appear enabled so the Validation handler enforces
    // Validation instead of short-circuiting to EFI_SUCCESS.
    //
    return ServeVariable (
             &mSecureBootEnabled,
             sizeof (mSecureBootEnabled),
             EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
             Attributes,
             DataSize,
             Data
             );
  }

  return mOriginalGetVariable (VariableName, VendorGuid, Attributes, DataSize, Data);
}

/**
  Install the GetVariable() hook.
**/
STATIC
VOID
InstallGetVariableHook (
  VOID
  )
{
  if (mOriginalGetVariable == NULL) {
    mOriginalGetVariable = gRT->GetVariable;
    gRT->GetVariable     = HookedGetVariable;
  }
}

/**
  Restore the original GetVariable() implementation.
**/
STATIC
VOID
RestoreGetVariableHook (
  VOID
  )
{
  if (mOriginalGetVariable != NULL) {
    gRT->GetVariable     = mOriginalGetVariable;
    mOriginalGetVariable = NULL;
  }

  mActiveDb      = NULL;
  mActiveDbSize  = 0;
  mActiveDbx     = NULL;
  mActiveDbxSize = 0;
}

/**
  Convert a DB_STATE combination to a human-readable string.

  This function builds a comma-separated list of the individual state names (e.g.,
  "Image Digest, Leaf, Root"). An empty state (DB_STATE_EMPTY) returns "Empty".

  @param[in]  DbState  A combination of DB_STATE_* flags.
  @param[out] Buffer   A caller-allocated string buffer to receive the result.
  @param[in]  BufSize  The size, in bytes, of Buffer.

  @retval EFI_SUCCESS        The description was written to Buffer.
  @retval EFI_BUFFER_TOO_SMALL  Buffer was too small for the description.
**/
STATIC
EFI_STATUS
DbStateToString (
  IN  UINT64  DbState,
  OUT CHAR8   *Buffer,
  IN  UINTN   BufSize
  )
{
  typedef struct {
    UINT64         Flag;
    CONST CHAR8    *Name;
  } DB_STATE_NAME_ENTRY;

  CONST DB_STATE_NAME_ENTRY  StateNames[] = {
    { DB_STATE_IMAGE_DIGEST_SHA256,                   "Image Digest (SHA-256)"                     },
    { DB_STATE_IMAGE_DIGEST_SHA384,                   "Image Digest (SHA-384)"                     },
    { DB_STATE_IMAGE_DIGEST_SHA512,                   "Image Digest (SHA-512)"                     },
    { DB_STATE_IMAGE_DIGEST_SHA1,                     "Image Digest (SHA-1)"                       },
    { DB_STATE_SIGNER1_LEAF_CERT,                     "Signer 1 Leaf Cert"                         },
    { DB_STATE_SIGNER1_INTERMEDIATE1_CERT,            "Signer 1 Intermediate 1 Cert"               },
    { DB_STATE_SIGNER1_INTERMEDIATE2_CERT,            "Signer 1 Intermediate 2 Cert"               },
    { DB_STATE_SIGNER1_ROOT_CERT,                     "Signer 1 Root Cert"                         },
    { DB_STATE_SIGNER2_CERT,                          "Signer 2 Cert"                              },
    { DB_STATE_SIGNER2_UNRELATED_CERT,                "Signer 2 Unrelated Cert"                    },
    { DB_STATE_SIGNER1_LEAF_TBS_HASH_SHA256,          "Signer 1 Leaf TBS Hash (SHA-256)"           },
    { DB_STATE_SIGNER1_LEAF_TBS_HASH_SHA384,          "Signer 1 Leaf TBS Hash (SHA-384)"           },
    { DB_STATE_SIGNER1_LEAF_TBS_HASH_SHA512,          "Signer 1 Leaf TBS Hash (SHA-512)"           },
    { DB_STATE_SIGNER1_INTERMEDIATE1_TBS_HASH_SHA256, "Signer 1 Intermediate 1 TBS Hash (SHA-256)" },
    { DB_STATE_SIGNER1_INTERMEDIATE1_TBS_HASH_SHA384, "Signer 1 Intermediate 1 TBS Hash (SHA-384)" },
    { DB_STATE_SIGNER1_INTERMEDIATE1_TBS_HASH_SHA512, "Signer 1 Intermediate 1 TBS Hash (SHA-512)" },
    { DB_STATE_SIGNER1_INTERMEDIATE2_TBS_HASH_SHA256, "Signer 1 Intermediate 2 TBS Hash (SHA-256)" },
    { DB_STATE_SIGNER1_INTERMEDIATE2_TBS_HASH_SHA384, "Signer 1 Intermediate 2 TBS Hash (SHA-384)" },
    { DB_STATE_SIGNER1_INTERMEDIATE2_TBS_HASH_SHA512, "Signer 1 Intermediate 2 TBS Hash (SHA-512)" },
    { DB_STATE_SIGNER1_ROOT_TBS_HASH_SHA256,          "Signer 1 Root TBS Hash (SHA-256)"           },
    { DB_STATE_SIGNER1_ROOT_TBS_HASH_SHA384,          "Signer 1 Root TBS Hash (SHA-384)"           },
    { DB_STATE_SIGNER1_ROOT_TBS_HASH_SHA512,          "Signer 1 Root TBS Hash (SHA-512)"           },
    { DB_STATE_SIGNER2_TBS_HASH_SHA256,               "Signer 2 TBS Hash (SHA-256)"                },
    { DB_STATE_SIGNER2_TBS_HASH_SHA384,               "Signer 2 TBS Hash (SHA-384)"                },
    { DB_STATE_SIGNER2_TBS_HASH_SHA512,               "Signer 2 TBS Hash (SHA-512)"                },
    { DB_STATE_CORRUPT_SIGNATURE_LIST_FIRST,          "Corrupt Signature List (First)"             },
    { DB_STATE_CORRUPT_SIGNATURE_LIST_LAST,           "Corrupt Signature List (Last)"              },
    { DB_STATE_SIGNER2_UNRELATED_TBS_HASH_SHA256,     "Signer 2 Unrelated TBS Hash (SHA-256)"      },
    { DB_STATE_SIGNER2_UNRELATED_TBS_HASH_SHA384,     "Signer 2 Unrelated TBS Hash (SHA-384)"      },
    { DB_STATE_SIGNER2_UNRELATED_TBS_HASH_SHA512,     "Signer 2 Unrelated TBS Hash (SHA-512)"      }
  };

  UINTN    Index;
  UINTN    Offset;
  UINTN    NameLen;
  BOOLEAN  FirstFlag;

  if ((Buffer == NULL) || (BufSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if (DbState == DB_STATE_EMPTY) {
    if (BufSize < 6) {
      return EFI_BUFFER_TOO_SMALL;
    }

    AsciiStrCpyS (Buffer, BufSize, "Empty");
    return EFI_SUCCESS;
  }

  Offset    = 0;
  FirstFlag = TRUE;

  for (Index = 0; Index < ARRAY_SIZE (StateNames); Index++) {
    if ((DbState & StateNames[Index].Flag) != 0) {
      NameLen = AsciiStrLen (StateNames[Index].Name);

      // Account for separator (", ") if not the first flag
      if (!FirstFlag) {
        NameLen += 2;  // for ", "
      }

      if (Offset + NameLen >= BufSize) {
        return EFI_BUFFER_TOO_SMALL;
      }

      if (FirstFlag) {
        //
        // For the first flag, copy the name directly (initializes buffer).
        //
        AsciiStrCpyS (Buffer, BufSize, StateNames[Index].Name);
        FirstFlag = FALSE;
      } else {
        //
        // For subsequent flags, append with a separator.
        //
        AsciiStrCatS (Buffer, BufSize, ", ");
        AsciiStrCatS (Buffer, BufSize, StateNames[Index].Name);
      }

      Offset += NameLen;
    }
  }

  return EFI_SUCCESS;
}

/**
  Convert an EFI_STATUS value to a human-readable string for test reporting.

  @param[in]  Status  The EFI_STATUS to convert.

  @return A pointer to a constant string describing the status.
**/
STATIC
CONST CHAR8 *
StatusToString (
  IN EFI_STATUS  Status
  )
{
  switch (Status) {
    case EFI_SUCCESS:
      return "Approved";
    case EFI_ACCESS_DENIED:
      return "Denied";
    case EFI_SECURITY_VIOLATION:
      return "Security Violation";
    default:
      return "Unknown";
  }
}

/**
  Convert an IMAGE_TYPE value to a human-readable string for test reporting.

  @param[in]  ImageType  The IMAGE_TYPE to convert.

  @return A pointer to a constant string describing the image type.
**/
STATIC
CONST CHAR8 *
ImageTypeToString (
  IN IMAGE_TYPE  ImageType
  )
{
  switch (ImageType) {
    case IMAGE_TYPE_UNSIGNED:
      return "Unsigned";
    case IMAGE_TYPE_SIGNED:
      return "Signed";
    case IMAGE_TYPE_MULTI_SIGNED:
      return "Multi-Signed";
    case IMAGE_TYPE_SIGNED_CORRUPT_CERT:
      return "Signed (Corrupt Cert)";
    case IMAGE_TYPE_MULTI_SIGNED_CORRUPT_FIRST_CERT:
      return "Multi-Signed (Corrupt First Cert)";
    case IMAGE_TYPE_MULTI_SIGNED_CORRUPT_SECOND_CERT:
      return "Multi-Signed (Corrupt Second Cert)";
    case IMAGE_TYPE_SIGNED_TAMPERED:
      return "Signed (Tampered)";
    default:
      return "Unknown";
  }
}

/**
  Generate a scenario description from the image type, database states, and expected result.

  This function builds a concise scenario name in the format:
  "Image: <Image>, DB: [<DB state>], DBX: [<DBX state>], Expected: <Expected result>"

  @param[in]  ImageType      The IMAGE_TYPE the scenario loads.
  @param[in]  DbState        The DB_STATE_* flags for the `db` database.
  @param[in]  DbxState       The DB_STATE_* flags for the `dbx` database.
  @param[in]  ExpectedStatus The expected EFI_STATUS from LoadImage().
  @param[in]  UseV2Guids     TRUE when the scenario emits V2 GUIDs; appends a marker to the result.
  @param[out] Buffer         A caller-allocated string buffer to receive the result.
  @param[in]  BufSize        The size, in bytes, of Buffer.

  @retval EFI_SUCCESS        The description was written to Buffer.
  @retval EFI_BUFFER_TOO_SMALL  Buffer was too small for the description.
**/
STATIC
EFI_STATUS
GenerateScenarioDescription (
  IN  IMAGE_TYPE  ImageType,
  IN  UINT64      DbState,
  IN  UINT64      DbxState,
  IN  EFI_STATUS  ExpectedStatus,
  IN  BOOLEAN     UseV2Guids,
  OUT CHAR8       *Buffer,
  IN  UINTN       BufSize
  )
{
  CHAR8        DbString[128];
  CHAR8        DbxString[128];
  CONST CHAR8  *StatusString;
  EFI_STATUS   Status;

  if ((Buffer == NULL) || (BufSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  // Convert DB state to string
  Status = DbStateToString (DbState, DbString, sizeof (DbString));
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Convert DBX state to string
  Status = DbStateToString (DbxState, DbxString, sizeof (DbxString));
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Get status string
  StatusString = StatusToString (ExpectedStatus);

  // Build the final description with labeled Image/DB/DBX/Expected fields.
  Status = AsciiSPrint (
             Buffer,
             BufSize,
             "Image: %a, DB: [%a], DBX: [%a], Expected: %a%a",
             ImageTypeToString (ImageType),
             DbString,
             DbxString,
             StatusString,
             UseV2Guids ? " (V2 GUIDs)" : ""
             );

  return Status;
}

/**
  Corrupt a WIN_CERTIFICATE in a signed image so the platform aborts when it parses that entry.

  The certificate table is located from the Security data directory that
  PeCoffLoaderGetImageInfo() copies into the loader image context. The requested entry's
  dwLength is overwritten with an out-of-range value; the data directory that locates the
  table is left untouched.

  @param[in,out]  Image      The image buffer to mutate in place.
  @param[in]      ImageSize  Size, in bytes, of Image.
  @param[in]      CertIndex  Zero-based index of the WIN_CERTIFICATE to corrupt.
**/
STATIC
VOID
CorruptImageWinCertificate (
  IN OUT UINT8  *Image,
  IN     UINTN  ImageSize,
  IN     UINTN  CertIndex
  )
{
  PE_COFF_LOADER_IMAGE_CONTEXT  Context;
  WIN_CERTIFICATE               *WinCert;
  UINTN                         TableStart;
  UINTN                         TableEnd;
  UINTN                         Offset;
  UINTN                         Index;

  ZeroMem (&Context, sizeof (Context));
  Context.Handle    = Image;
  Context.ImageRead = PeCoffLoaderImageReadFromMemory;

  if (RETURN_ERROR (PeCoffLoaderGetImageInfo (&Context))) {
    return;
  }

  TableStart = Context.SecurityDataDirectory.VirtualAddress;
  TableEnd   = TableStart + Context.SecurityDataDirectory.Size;

  //
  // The certificate table must lie fully within the image.
  //
  if ((TableStart == 0) || (TableEnd < TableStart) || (TableEnd > ImageSize)) {
    return;
  }

  //
  // Walk the 8-byte-aligned WIN_CERTIFICATE list to the requested entry.
  //
  Offset  = TableStart;
  WinCert = NULL;
  for (Index = 0; Index <= CertIndex; Index++) {
    if ((Offset >= TableEnd) || ((TableEnd - Offset) < sizeof (WIN_CERTIFICATE))) {
      return;
    }

    WinCert = (WIN_CERTIFICATE *)(Image + Offset);
    if (Index == CertIndex) {
      break;
    }

    if ((WinCert->dwLength < sizeof (WIN_CERTIFICATE)) ||
        (WinCert->dwLength > (TableEnd - Offset)))
    {
      return;
    }

    Offset = ALIGN_VALUE (Offset + WinCert->dwLength, 8);
  }

  //
  // Overwrite the target entry's length with an out-of-range value so the platform aborts
  // when it validates this WIN_CERTIFICATE.
  //
  WinCert->dwLength = MAX_UINT32;
}

/**
  Tamper a signed image's body so its Authenticode hash no longer matches its signature.

  A single byte at the first offset past the image headers is flipped. That region is covered by
  the Authenticode hash but is neither a structural header field nor part of the WIN_CERTIFICATE
  table, so the image still parses and reaches signature validation while its recomputed
  Authenticode digest differs from both the embedded signature and any enrolled image digest.

  @param[in,out]  Image      The image buffer to mutate in place.
  @param[in]      ImageSize  Size, in bytes, of Image.
**/
STATIC
VOID
TamperImageBody (
  IN OUT UINT8  *Image,
  IN     UINTN  ImageSize
  )
{
  PE_COFF_LOADER_IMAGE_CONTEXT  Context;
  UINTN                         Offset;

  ZeroMem (&Context, sizeof (Context));
  Context.Handle    = Image;
  Context.ImageRead = PeCoffLoaderImageReadFromMemory;

  if (RETURN_ERROR (PeCoffLoaderGetImageInfo (&Context))) {
    return;
  }

  //
  // The first byte past the headers is section content: hashed by Authenticode, but not a header
  // field or part of the certificate table, so the image remains parseable after the flip.
  //
  Offset = Context.SizeOfHeaders;
  if ((Offset == 0) || (Offset >= ImageSize)) {
    return;
  }

  Image[Offset] ^= 0xFF;
}

/**
  Resolve a scenario's IMAGE_TYPE into a freshly allocated, writable image buffer.

  A copy is always returned so corrupt variants can be mutated in place. The caller owns the
  buffer and must release it with FreePool().

  @param[in]   ImageType  The image selector from the scenario.
  @param[out]  Image      Receives a pointer to the allocated image buffer.
  @param[out]  ImageSize  Receives the size, in bytes, of the image buffer.

  @retval EFI_SUCCESS            The image was resolved and copied.
  @retval EFI_INVALID_PARAMETER  ImageType was not a recognized value.
  @retval EFI_OUT_OF_RESOURCES   The image copy could not be allocated.
**/
STATIC
EFI_STATUS
GetImageForType (
  IN  IMAGE_TYPE  ImageType,
  OUT UINT8       **Image,
  OUT UINTN       *ImageSize
  )
{
  CONST UINT8  *Base;
  UINTN        BaseSize;
  UINT8        *Copy;

  switch (ImageType) {
    case IMAGE_TYPE_UNSIGNED:
      Base     = mUnsignedImage;
      BaseSize = mUnsignedImageSize;
      break;
    case IMAGE_TYPE_SIGNED:
    case IMAGE_TYPE_SIGNED_CORRUPT_CERT:
    case IMAGE_TYPE_SIGNED_TAMPERED:
      Base     = mSignedImage;
      BaseSize = mSignedImageSize;
      break;
    case IMAGE_TYPE_MULTI_SIGNED:
    case IMAGE_TYPE_MULTI_SIGNED_CORRUPT_FIRST_CERT:
    case IMAGE_TYPE_MULTI_SIGNED_CORRUPT_SECOND_CERT:
      Base     = mMultiSignedImage;
      BaseSize = mMultiSignedImageSize;
      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  Copy = AllocateCopyPool (BaseSize, Base);
  if (Copy == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if ((ImageType == IMAGE_TYPE_SIGNED_CORRUPT_CERT) ||
      (ImageType == IMAGE_TYPE_MULTI_SIGNED_CORRUPT_FIRST_CERT))
  {
    //
    // Corrupt the first WIN_CERTIFICATE (index 0): the signed image's only certificate, or the
    // multi-signed image's signer 1 certificate.
    //
    CorruptImageWinCertificate (Copy, BaseSize, 0);
  } else if (ImageType == IMAGE_TYPE_MULTI_SIGNED_CORRUPT_SECOND_CERT) {
    //
    // Corrupt the second WIN_CERTIFICATE (index 1): the multi-signed image's signer 2 certificate.
    //
    CorruptImageWinCertificate (Copy, BaseSize, 1);
  } else if (ImageType == IMAGE_TYPE_SIGNED_TAMPERED) {
    //
    // Tamper the image body so its Authenticode hash no longer matches the signature or any
    // enrolled image digest, while leaving the WIN_CERTIFICATE intact so validation still runs.
    //
    TamperImageBody (Copy, BaseSize);
  }

  *Image     = Copy;
  *ImageSize = BaseSize;
  return EFI_SUCCESS;
}

//
// Maps each V1 signature-type GUID this app emits to its V2 (EFI_SIGNATURE_V2_DATA) counterpart,
// used when a scenario requests V2 GUIDs. Types without a V2 form (e.g. SHA-1) are left unchanged.
//
STATIC CONST struct {
  CONST EFI_GUID    *V1;
  CONST EFI_GUID    *V2;
} mV1ToV2SignatureType[] = {
  { &gEfiCertSha256Guid,     &gEfiCertV2Sha256Guid     },
  { &gEfiCertSha384Guid,     &gEfiCertV2Sha384Guid     },
  { &gEfiCertSha512Guid,     &gEfiCertV2Sha512Guid     },
  { &gEfiCertX509Guid,       &gEfiCertV2X509Guid       },
  { &gEfiCertX509Sha256Guid, &gEfiCertV2X509Sha256Guid },
  { &gEfiCertX509Sha384Guid, &gEfiCertV2X509Sha384Guid },
  { &gEfiCertX509Sha512Guid, &gEfiCertV2X509Sha512Guid }
};

/**
  Map a V1 signature-type GUID to its V2 (EFI_SIGNATURE_V2_DATA) counterpart.

  @param[in]  V1Type  The V1 signature-type GUID.

  @return The matching V2 GUID, or V1Type unchanged if the type has no V2 form.
**/
STATIC
CONST EFI_GUID *
GetV2SignatureType (
  IN CONST EFI_GUID  *V1Type
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mV1ToV2SignatureType); Index++) {
    if (CompareGuid (V1Type, mV1ToV2SignatureType[Index].V1)) {
      return mV1ToV2SignatureType[Index].V2;
    }
  }

  return V1Type;
}

///
/// One signature list to emit while building a signature database.
///
typedef struct {
  CONST EFI_GUID    *Type;      ///< Signature type GUID (image digest or X.509 / TBS hash).
  CONST UINT8       *Data;      ///< Signature payload (digest bytes or certificate).
  UINTN             DataSize;   ///< Size, in bytes, of Data.
  UINTN             ExtraSize;  ///< Trailing bytes counted in SignatureSize but not copied (V1 TBS-hash EFI_TIME); left zeroed.
  BOOLEAN           Malformed;  ///< When TRUE, emit a deliberately corrupt EFI_SIGNATURE_LIST.
} SIG_LIST_SPEC;

/**
  Build a serialized set of EFI_SIGNATURE_LISTs from DB_STATE_* flags.

  Each set flag contributes one EFI_SIGNATURE_LIST holding a single EFI_SIGNATURE_DATA entry.

  The caller owns the returned buffer and must release it with FreeSignatureDatabase().

  @param[in]   StateFlags     Bitwise-OR of DB_STATE_* values.
  @param[in]   ImageType      The image whose digest to use for
                              DB_STATE_IMAGE_DIGEST.
  @param[in]   UseV2Guids     When TRUE, emit each entry with its V2 (EFI_CERT_V2_*) signature-type
                              GUID and the EFI_SIGNATURE_V2_DATA (ownerless) layout; when FALSE, use
                              the V1 GUIDs and the EFI_SIGNATURE_DATA (owner-prefixed) layout.
  @param[out]  Database       Receives the allocated database buffer, or NULL when
                              StateFlags is DB_STATE_EMPTY.
  @param[out]  DatabaseSize   Receives the size, in bytes, of the database.

  @retval EFI_SUCCESS            The database was built (possibly empty).
  @retval EFI_INVALID_PARAMETER  ImageType was invalid.
  @retval EFI_OUT_OF_RESOURCES   The database buffer could not be allocated.
**/
STATIC
EFI_STATUS
BuildSignatureDatabase (
  IN  UINT64      StateFlags,
  IN  IMAGE_TYPE  ImageType,
  IN  BOOLEAN     UseV2Guids,
  OUT UINT8       **Database,
  OUT UINTN       *DatabaseSize
  )
{
  //
  // Precomputed image digests, keyed by base image type (IMAGE_TYPE_UNSIGNED / _SIGNED /
  // _MULTI_SIGNED) then hash algorithm (SHA-256 / 384 / 512 / 1). SHA-1 is an unsupported
  // algorithm included only to prove the handler ignores it. Corrupt image variants are
  // normalized to their base type before indexing, so only the base types need rows; sizing to
  // exactly those rows also keeps the tables fully initialized (no implicit memset).
  //
  CONST UINT8  *DigestData[IMAGE_TYPE_MULTI_SIGNED + 1][4] = {
    { mUnsignedImageDigestSha256,    mUnsignedImageDigestSha384,    mUnsignedImageDigestSha512,    mUnsignedImageDigestSha1    },
    { mSignedImageDigestSha256,      mSignedImageDigestSha384,      mSignedImageDigestSha512,      mSignedImageDigestSha1      },
    { mMultiSignedImageDigestSha256, mMultiSignedImageDigestSha384, mMultiSignedImageDigestSha512, mMultiSignedImageDigestSha1 }
  };
  CONST UINTN  DigestDataSize[IMAGE_TYPE_MULTI_SIGNED + 1][4] = {
    { mUnsignedImageDigestSha256Size,    mUnsignedImageDigestSha384Size,    mUnsignedImageDigestSha512Size,    mUnsignedImageDigestSha1Size    },
    { mSignedImageDigestSha256Size,      mSignedImageDigestSha384Size,      mSignedImageDigestSha512Size,      mSignedImageDigestSha1Size      },
    { mMultiSignedImageDigestSha256Size, mMultiSignedImageDigestSha384Size, mMultiSignedImageDigestSha512Size, mMultiSignedImageDigestSha1Size }
  };
  CONST struct {
    UINT64            Flag;
    UINTN             HashIndex;
    CONST EFI_GUID    *Type;
  } DigestFlags[] = {
    { DB_STATE_IMAGE_DIGEST_SHA1,   3, &gEfiCertSha1Guid   },
    { DB_STATE_IMAGE_DIGEST_SHA256, 0, &gEfiCertSha256Guid },
    { DB_STATE_IMAGE_DIGEST_SHA384, 1, &gEfiCertSha384Guid },
    { DB_STATE_IMAGE_DIGEST_SHA512, 2, &gEfiCertSha512Guid }
  };
  CONST struct {
    UINT64         Flag;
    CONST UINT8    *Cert;
    UINTN          CertSize;
  } CertFlags[] = {
    { DB_STATE_SIGNER1_LEAF_CERT,          mLeafCert,             mLeafCertSize             },
    { DB_STATE_SIGNER1_INTERMEDIATE1_CERT, mIntermediate1Cert,    mIntermediate1CertSize    },
    { DB_STATE_SIGNER1_INTERMEDIATE2_CERT, mIntermediate2Cert,    mIntermediate2CertSize    },
    { DB_STATE_SIGNER1_ROOT_CERT,          mRootCert,             mRootCertSize             },
    { DB_STATE_SIGNER2_CERT,               mSigner2Cert,          mSigner2CertSize          },
    { DB_STATE_SIGNER2_UNRELATED_CERT,     mSigner2UnrelatedCert, mSigner2UnrelatedCertSize }
  };
  CONST struct {
    UINT64            Flag;
    CONST UINT8       *Hash;
    UINTN             HashSize;
    CONST EFI_GUID    *Type;
  } TbsFlags[] = {
    { DB_STATE_SIGNER1_LEAF_TBS_HASH_SHA256,          mLeafCertTbsHashSha256,             mLeafCertTbsHashSha256Size,             &gEfiCertX509Sha256Guid },
    { DB_STATE_SIGNER1_LEAF_TBS_HASH_SHA384,          mLeafCertTbsHashSha384,             mLeafCertTbsHashSha384Size,             &gEfiCertX509Sha384Guid },
    { DB_STATE_SIGNER1_LEAF_TBS_HASH_SHA512,          mLeafCertTbsHashSha512,             mLeafCertTbsHashSha512Size,             &gEfiCertX509Sha512Guid },
    { DB_STATE_SIGNER1_INTERMEDIATE1_TBS_HASH_SHA256, mIntermediate1CertTbsHashSha256,    mIntermediate1CertTbsHashSha256Size,    &gEfiCertX509Sha256Guid },
    { DB_STATE_SIGNER1_INTERMEDIATE1_TBS_HASH_SHA384, mIntermediate1CertTbsHashSha384,    mIntermediate1CertTbsHashSha384Size,    &gEfiCertX509Sha384Guid },
    { DB_STATE_SIGNER1_INTERMEDIATE1_TBS_HASH_SHA512, mIntermediate1CertTbsHashSha512,    mIntermediate1CertTbsHashSha512Size,    &gEfiCertX509Sha512Guid },
    { DB_STATE_SIGNER1_INTERMEDIATE2_TBS_HASH_SHA256, mIntermediate2CertTbsHashSha256,    mIntermediate2CertTbsHashSha256Size,    &gEfiCertX509Sha256Guid },
    { DB_STATE_SIGNER1_INTERMEDIATE2_TBS_HASH_SHA384, mIntermediate2CertTbsHashSha384,    mIntermediate2CertTbsHashSha384Size,    &gEfiCertX509Sha384Guid },
    { DB_STATE_SIGNER1_INTERMEDIATE2_TBS_HASH_SHA512, mIntermediate2CertTbsHashSha512,    mIntermediate2CertTbsHashSha512Size,    &gEfiCertX509Sha512Guid },
    { DB_STATE_SIGNER1_ROOT_TBS_HASH_SHA256,          mRootCertTbsHashSha256,             mRootCertTbsHashSha256Size,             &gEfiCertX509Sha256Guid },
    { DB_STATE_SIGNER1_ROOT_TBS_HASH_SHA384,          mRootCertTbsHashSha384,             mRootCertTbsHashSha384Size,             &gEfiCertX509Sha384Guid },
    { DB_STATE_SIGNER1_ROOT_TBS_HASH_SHA512,          mRootCertTbsHashSha512,             mRootCertTbsHashSha512Size,             &gEfiCertX509Sha512Guid },
    { DB_STATE_SIGNER2_TBS_HASH_SHA256,               mSigner2CertTbsHashSha256,          mSigner2CertTbsHashSha256Size,          &gEfiCertX509Sha256Guid },
    { DB_STATE_SIGNER2_TBS_HASH_SHA384,               mSigner2CertTbsHashSha384,          mSigner2CertTbsHashSha384Size,          &gEfiCertX509Sha384Guid },
    { DB_STATE_SIGNER2_TBS_HASH_SHA512,               mSigner2CertTbsHashSha512,          mSigner2CertTbsHashSha512Size,          &gEfiCertX509Sha512Guid },
    { DB_STATE_SIGNER2_UNRELATED_TBS_HASH_SHA256,     mSigner2UnrelatedCertTbsHashSha256, mSigner2UnrelatedCertTbsHashSha256Size, &gEfiCertX509Sha256Guid },
    { DB_STATE_SIGNER2_UNRELATED_TBS_HASH_SHA384,     mSigner2UnrelatedCertTbsHashSha384, mSigner2UnrelatedCertTbsHashSha384Size, &gEfiCertX509Sha384Guid },
    { DB_STATE_SIGNER2_UNRELATED_TBS_HASH_SHA512,     mSigner2UnrelatedCertTbsHashSha512, mSigner2UnrelatedCertTbsHashSha512Size, &gEfiCertX509Sha512Guid }
  };
  SIG_LIST_SPEC       Specs[ARRAY_SIZE (DigestFlags) + ARRAY_SIZE (CertFlags) + ARRAY_SIZE (TbsFlags) + 2];
  UINTN               Count;
  UINTN               Index;
  UINTN               TotalSize;
  UINTN               RealListSize;
  UINT8               *Buffer;
  UINT8               *Cursor;
  UINT8               *Payload;
  EFI_SIGNATURE_LIST  *List;
  UINTN               OwnerSize;

  *Database     = NULL;
  *DatabaseSize = 0;
  Count         = 0;

  if (StateFlags == DB_STATE_EMPTY) {
    return EFI_SUCCESS;
  }

  if (ImageType >= IMAGE_TYPE_MAX) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Corrupt-certificate variants reuse their base image's precomputed digests because corrupting a
  // WIN_CERTIFICATE does not change the Authenticode digest. The tampered variant also maps to the
  // base type so an enrolled image digest is the *original* one, which the tampered image's
  // recomputed digest no longer matches.
  //
  if ((ImageType == IMAGE_TYPE_SIGNED_CORRUPT_CERT) ||
      (ImageType == IMAGE_TYPE_SIGNED_TAMPERED))
  {
    ImageType = IMAGE_TYPE_SIGNED;
  } else if ((ImageType == IMAGE_TYPE_MULTI_SIGNED_CORRUPT_FIRST_CERT) ||
             (ImageType == IMAGE_TYPE_MULTI_SIGNED_CORRUPT_SECOND_CERT))
  {
    ImageType = IMAGE_TYPE_MULTI_SIGNED;
  }

  //
  // Zero every spec so the Malformed flag defaults to FALSE.
  //
  ZeroMem (Specs, sizeof (Specs));

  //
  // A deliberately-malformed signature list placed at the FRONT of the database, before every
  // otherwise-authorizing entry, so a parser that aborts on the corruption never reaches them.
  //
  if ((StateFlags & DB_STATE_CORRUPT_SIGNATURE_LIST_FIRST) != 0) {
    Specs[Count].Type      = &gEfiCertSha256Guid;
    Specs[Count].Data      = mCorruptSignaturePayload;
    Specs[Count].DataSize  = CORRUPT_SIGNATURE_PAYLOAD_SIZE;
    Specs[Count].Malformed = TRUE;
    Count++;
  }

  //
  // Image Authenticode digests (precomputed per algorithm; selected by ImageType).
  //
  for (Index = 0; Index < ARRAY_SIZE (DigestFlags); Index++) {
    if ((StateFlags & DigestFlags[Index].Flag) != 0) {
      Specs[Count].Type     = DigestFlags[Index].Type;
      Specs[Count].Data     = DigestData[ImageType][DigestFlags[Index].HashIndex];
      Specs[Count].DataSize = DigestDataSize[ImageType][DigestFlags[Index].HashIndex];
      Count++;
    }
  }

  //
  // X.509 certificates (algorithm-independent).
  //
  for (Index = 0; Index < ARRAY_SIZE (CertFlags); Index++) {
    if ((StateFlags & CertFlags[Index].Flag) != 0) {
      Specs[Count].Type     = &gEfiCertX509Guid;
      Specs[Count].Data     = CertFlags[Index].Cert;
      Specs[Count].DataSize = CertFlags[Index].CertSize;
      Count++;
    }
  }

  //
  // Certificate TBSCertificate hashes (precomputed per algorithm).
  //
  for (Index = 0; Index < ARRAY_SIZE (TbsFlags); Index++) {
    if ((StateFlags & TbsFlags[Index].Flag) != 0) {
      Specs[Count].Type     = TbsFlags[Index].Type;
      Specs[Count].Data     = TbsFlags[Index].Hash;
      Specs[Count].DataSize = TbsFlags[Index].HashSize;
      //
      // A V1 EFI_CERT_X509_SHA* entry appends an EFI_TIME (TimeOfRevocation) after the hash; the V2
      // (EFI_CERT_V2_X509_SHA*) layout omits it.
      //
      Specs[Count].ExtraSize = UseV2Guids ? 0 : sizeof (EFI_TIME);
      Count++;
    }
  }

  //
  // A deliberately-malformed signature list placed at the END of the database, after every
  // otherwise-authorizing entry, so a parser reaches (and may honor) those entries before it
  // aborts on the corruption.
  //
  if ((StateFlags & DB_STATE_CORRUPT_SIGNATURE_LIST_LAST) != 0) {
    Specs[Count].Type      = &gEfiCertSha256Guid;
    Specs[Count].Data      = mCorruptSignaturePayload;
    Specs[Count].DataSize  = CORRUPT_SIGNATURE_PAYLOAD_SIZE;
    Specs[Count].Malformed = TRUE;
    Count++;
  }

  //
  // V1 (EFI_SIGNATURE_DATA) entries prefix the payload with a 16-byte SignatureOwner; V2
  // (EFI_SIGNATURE_V2_DATA) entries omit it.
  //
  OwnerSize = UseV2Guids ? 0 : sizeof (EFI_GUID);

  TotalSize = 0;
  for (Index = 0; Index < Count; Index++) {
    TotalSize += sizeof (EFI_SIGNATURE_LIST) + OwnerSize + Specs[Index].DataSize + Specs[Index].ExtraSize;
  }

  Buffer = AllocateZeroPool (TotalSize);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Cursor = Buffer;
  for (Index = 0; Index < Count; Index++) {
    RealListSize = sizeof (EFI_SIGNATURE_LIST) + OwnerSize + Specs[Index].DataSize + Specs[Index].ExtraSize;

    List = (EFI_SIGNATURE_LIST *)Cursor;
    CopyGuid (&List->SignatureType, UseV2Guids ? GetV2SignatureType (Specs[Index].Type) : Specs[Index].Type);
    List->SignatureHeaderSize = 0;
    List->SignatureSize       = (UINT32)(OwnerSize + Specs[Index].DataSize + Specs[Index].ExtraSize);
    List->SignatureListSize   = (UINT32)RealListSize;

    //
    // The payload follows the optional SignatureOwner: write the owner GUID first for a V1 entry;
    // a V2 entry's payload starts at the entry.
    //
    Payload = (UINT8 *)(List + 1);
    if (!UseV2Guids) {
      CopyGuid ((EFI_GUID *)Payload, &mTestSignatureOwner);
    }

    CopyMem (Payload + OwnerSize, Specs[Index].Data, Specs[Index].DataSize);

    if (Specs[Index].Malformed) {
      //
      // Corrupt the size fields so the emitted list is internally inconsistent and claims to
      // run past the variable. The real bytes written above keep our cursor math correct.
      //
      List->SignatureSize     = 0;
      List->SignatureListSize = MAX_UINT32;
    }

    Cursor += RealListSize;
  }

  *Database     = Buffer;
  *DatabaseSize = TotalSize;
  return EFI_SUCCESS;
}

/**
  Release a database allocated by BuildSignatureDatabase().

  @param[in]  Database  The database buffer, which may be NULL.
**/
STATIC
VOID
FreeSignatureDatabase (
  IN UINT8  *Database
  )
{
  if (Database != NULL) {
    FreePool (Database);
  }
}

/**
  Unit test body that drives a single secure boot image validation scenario.

  See ImageValidationTestApp.h for the full contract.

  @param[in]  Context  A pointer to the SECURE_BOOT_IMAGE_TEST_SCENARIO that describes the
                       inputs and expected result.

  @retval  UNIT_TEST_PASSED             LoadImage() returned the expected status.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  LoadImage() returned an unexpected status.
**/
UNIT_TEST_STATUS
EFIAPI
RunImageValidationScenario (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SECURE_BOOT_IMAGE_TEST_SCENARIO  *Scenario;
  EFI_STATUS                       Status;
  EFI_HANDLE                       LoadedImageHandle;
  UINT8                            *Image;
  UINTN                            ImageSize;
  UINT8                            *Db;
  UINTN                            DbSize;
  UINT8                            *Dbx;
  UINTN                            DbxSize;

  Scenario          = (SECURE_BOOT_IMAGE_TEST_SCENARIO *)Context;
  LoadedImageHandle = NULL;
  Image             = NULL;
  Db                = NULL;
  Dbx               = NULL;
  UT_ASSERT_NOT_NULL (Scenario);

  //
  // Resolve the image to load and build the db / dbx databases this scenario describes.
  //
  UT_ASSERT_NOT_EFI_ERROR (GetImageForType (Scenario->ImageType, &Image, &ImageSize));
  UT_ASSERT_NOT_EFI_ERROR (BuildSignatureDatabase (Scenario->DbState, Scenario->ImageType, Scenario->UseV2Guids, &Db, &DbSize));
  UT_ASSERT_NOT_EFI_ERROR (BuildSignatureDatabase (Scenario->DbxState, Scenario->ImageType, Scenario->UseV2Guids, &Dbx, &DbxSize));

  //
  // Point the GetVariable() hook at the databases just built so the platform's Validation
  // handler sees the db / dbx this scenario wants.
  //
  mActiveDb      = Db;
  mActiveDbSize  = DbSize;
  mActiveDbx     = Dbx;
  mActiveDbxSize = DbxSize;

  //
  // Load the built-in image from memory. On a Secure Boot enabled platform the DXE core
  // invokes the registered image Validation handler, which consults the db / dbx served by
  // our GetVariable() hook. The status returned here is the result of that Validation.
  //
  Status = gBS->LoadImage (
                  FALSE,
                  gImageHandle,
                  (EFI_DEVICE_PATH_PROTOCOL *)&mTestImageDevicePath,
                  (VOID *)Image,
                  ImageSize,
                  &LoadedImageHandle
                  );

  //
  // Detach the databases from the hook before freeing them so a stray GetVariable() call
  // can never observe a dangling pointer.
  //
  mActiveDb      = NULL;
  mActiveDbSize  = 0;
  mActiveDbx     = NULL;
  mActiveDbxSize = 0;

  FreeSignatureDatabase (Db);
  FreeSignatureDatabase (Dbx);

  if (Image != NULL) {
    FreePool (Image);
  }

  //
  // LoadImage() may load an image yet still report EFI_SECURITY_VIOLATION (untrusted). In
  // every case where a handle was produced, unload it so the test leaves no image resident.
  //
  if (LoadedImageHandle != NULL) {
    gBS->UnloadImage (LoadedImageHandle);
  }

  UT_ASSERT_STATUS_EQUAL (Status, Scenario->ExpectedStatus);

  return UNIT_TEST_PASSED;
}

/**
  Initialize the unit test framework, register every image validation scenario into a single
  unit test suite, and run them.

  @retval  EFI_SUCCESS           All test cases were dispatched.
  @retval  EFI_OUT_OF_RESOURCES  Resources were unavailable to initialize the unit tests.
**/
EFI_STATUS
EFIAPI
UefiTestMain (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      SuiteHandle;
  UINTN                       ScenarioIndex;

  Framework = NULL;

  Print (L"%a v%a\n", UNIT_TEST_NAME, UNIT_TEST_VERSION);

  Status = InitUnitTestFramework (&Framework, UNIT_TEST_NAME, gEfiCallerBaseName, UNIT_TEST_VERSION);
  if (EFI_ERROR (Status)) {
    Print (L"Failed in InitUnitTestFramework. Status = %r\n", Status);
    goto EXIT;
  }

  //
  // Register every image validation scenario into a single unit test suite. Each scenario
  // carries its own full, self-describing hierarchical ID (see README.md "Test Catalog").
  //
  Status = CreateUnitTestSuite (
             &SuiteHandle,
             Framework,
             "Image Validation Scenarios",
             "SecurityPkg.ImageValidation",
             NULL,
             NULL
             );
  if (EFI_ERROR (Status)) {
    Print (L"Failed in CreateUnitTestSuite\n");
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  for (ScenarioIndex = 0; ScenarioIndex < mScenarioCount; ScenarioIndex++) {
    CHAR8        ScenarioDescription[384];
    CONST CHAR8  *TestId;
    EFI_STATUS   GenerateStatus;

    //
    // Auto-generate the human-readable scenario description from the image type, DB state,
    // DBX state, and expected status.
    //
    GenerateStatus = GenerateScenarioDescription (
                       mScenarios[ScenarioIndex].ImageType,
                       mScenarios[ScenarioIndex].DbState,
                       mScenarios[ScenarioIndex].DbxState,
                       mScenarios[ScenarioIndex].ExpectedStatus,
                       mScenarios[ScenarioIndex].UseV2Guids,
                       ScenarioDescription,
                       sizeof (ScenarioDescription)
                       );

    if (EFI_ERROR (GenerateStatus)) {
      Print (
        L"Failed to generate scenario description for scenario %u. Status=%r\n",
        (UINT32)ScenarioIndex,
        GenerateStatus
        );
      Status = GenerateStatus;
      goto EXIT;
    }

    //
    // The scenario's full ID is the test case name (shown as the class name in reports and
    // used in the machine-readable xUnit results), so the description itself leads with the
    // image type rather than repeating the ID.
    //
    TestId = mScenarios[ScenarioIndex].Id;

    AddTestCase (
      SuiteHandle,
      ScenarioDescription,
      (CHAR8 *)TestId,
      RunImageValidationScenario,
      NULL,
      NULL,
      (UNIT_TEST_CONTEXT)&mScenarios[ScenarioIndex]
      );
  }

  //
  // Install the GetVariable() hook for the duration of the run, then restore it so the
  // application leaves the platform exactly as it found it.
  //
  InstallGetVariableHook ();
  Status = RunAllTestSuites (Framework);
  RestoreGetVariableHook ();

EXIT:
  if (Framework != NULL) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}

/**
  Standard UEFI entry point for the image Validation unit test application.

  @param[in]  ImageHandle  The firmware allocated handle for the EFI image.
  @param[in]  SystemTable  A pointer to the EFI System Table.

  @retval  EFI_SUCCESS  The entry point executed successfully.
  @retval  other        An error occurred while running the unit tests.
**/
EFI_STATUS
EFIAPI
DxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return UefiTestMain ();
}
