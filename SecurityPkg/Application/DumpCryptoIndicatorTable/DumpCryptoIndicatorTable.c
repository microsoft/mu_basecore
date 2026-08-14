/** @file
  Dump the EFI Crypto Indicator Table (ECIT).

  A UEFI shell application that locates the ECIT (published by the collector as
  an EFI_CONFIGURATION_TABLE), validates its ACPI-style header and 8-bit
  checksum, and pretty-prints the header plus every entry: the feature (by
  friendly name and GUID) and its payload. Verification / firmware-update
  features carry a CSV OID string, printed one algorithm per line and annotated
  with the algorithm name where known; anything else is hex-dumped.

  Output is written both to the console (for interactive use) and to the debug
  log (so the dump is captured in automated/headless runs where the UEFI console
  is not otherwise recorded). One item per line keeps every message well under
  the per-message debug buffer (MAX_DEBUG_MESSAGE_LENGTH).

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Guid/CryptoIndicatorTable.h>

#define ECIT_RULE  "=============================================================="
#define ECIT_THIN  "--------------------------------------------------------------"

//
// Emit a line to both the console and the debug log so the dump is visible
// interactively and captured in headless runs. Each line is formatted into a
// bounded buffer first, then the buffer is emitted with a literal format ("%a")
// so DEBUG's format string is a plain literal (keeps the debug-macro checker
// happy) and nothing is truncated by the per-message debug buffer.
//
#define ECIT_DUMP(Fmt, ...)                                        \
  do {                                                             \
    CHAR8  EcitLine[160];                                          \
    AsciiSPrint (EcitLine, sizeof (EcitLine), Fmt, ##__VA_ARGS__); \
    AsciiPrint ("%a", EcitLine);                                   \
    DEBUG ((DEBUG_INFO, "DumpEcit: %a", EcitLine));                \
  } while (FALSE)

//
// Friendly names for the well-known feature identifiers.
//
typedef struct {
  CONST EFI_GUID    *Guid;
  CONST CHAR8       *Name;
} ECIT_FEATURE_NAME;

STATIC CONST ECIT_FEATURE_NAME  mFeatureNames[] = {
  { &gEfiEcitFeatureImageVerificationGuid,                "Secure Boot Image Verification"      },
  { &gEfiEcitFeatureSecureBootAuthorizationGuid,          "Secure Boot Authorization"           },
  { &gEfiEcitFeatureSecureBootServicingAuthorizationGuid, "Secure Boot Servicing Authorization" },
  { &gEfiEcitFeatureImageRevocationGuid,                  "Secure Boot Image Revocation"        },
  { &gEfiEcitFeatureAuthenticatedVariableGuid,            "Authenticated Variable Update"       },
  { &gEfiEcitFeatureSystemFirmwareUpdateGuid,             "System Firmware Update"              },
  { &gEfiEcitFeatureEsrtFirmwareUpdateGuid,               "ESRT Device Firmware Update"         },
};

//
// Friendly names for the signature-algorithm OIDs a verification feature may
// report. Unlisted OIDs are printed without an annotation.
//
typedef struct {
  CONST CHAR8    *Oid;
  CONST CHAR8    *Name;
} ECIT_OID_NAME;

STATIC CONST ECIT_OID_NAME  mOidNames[] = {
  { "1.2.840.113549.1.1.4",    "md5WithRSAEncryption"    },
  { "1.2.840.113549.1.1.5",    "sha1WithRSAEncryption"   },
  { "1.2.840.113549.1.1.11",   "sha256WithRSAEncryption" },
  { "1.2.840.113549.1.1.12",   "sha384WithRSAEncryption" },
  { "1.2.840.113549.1.1.13",   "sha512WithRSAEncryption" },
  { "1.2.840.113549.1.1.14",   "sha224WithRSAEncryption" },
  { "1.2.840.10045.4.1",       "ecdsa-with-SHA1"         },
  { "1.2.840.10045.4.3.1",     "ecdsa-with-SHA224"       },
  { "1.2.840.10045.4.3.2",     "ecdsa-with-SHA256"       },
  { "1.2.840.10045.4.3.3",     "ecdsa-with-SHA384"       },
  { "1.2.840.10045.4.3.4",     "ecdsa-with-SHA512"       },
  { "2.16.840.1.101.3.4.3.17", "id-ml-dsa-44"            },
  { "2.16.840.1.101.3.4.3.18", "id-ml-dsa-65"            },
  { "2.16.840.1.101.3.4.3.19", "id-ml-dsa-87"            },
};

/**
  Return a friendly name for a feature GUID, or NULL if unknown.
**/
STATIC
CONST CHAR8 *
FeatureName (
  IN CONST EFI_GUID  *Feature
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mFeatureNames); Index++) {
    if (CompareGuid (Feature, mFeatureNames[Index].Guid)) {
      return mFeatureNames[Index].Name;
    }
  }

  return NULL;
}

/**
  Return a friendly name for an algorithm OID string, or NULL if unknown.
**/
STATIC
CONST CHAR8 *
OidName (
  IN CONST CHAR8  *Oid
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mOidNames); Index++) {
    if (AsciiStrCmp (Oid, mOidNames[Index].Oid) == 0) {
      return mOidNames[Index].Name;
    }
  }

  return NULL;
}

/**
  Return TRUE if a feature's payload is a printable CSV OID string (as opposed to
  a binary blob), based on the well-known feature GUID.
**/
STATIC
BOOLEAN
IsCsvOidFeature (
  IN CONST EFI_GUID  *Feature
  )
{
  return (BOOLEAN)(
                   CompareGuid (Feature, &gEfiEcitFeatureImageVerificationGuid) ||
                   CompareGuid (Feature, &gEfiEcitFeatureAuthenticatedVariableGuid) ||
                   CompareGuid (Feature, &gEfiEcitFeatureSystemFirmwareUpdateGuid)
                   );
}

/**
  Compute the 8-bit checksum of a buffer (a valid ACPI table sums to zero).
**/
STATIC
UINT8
SumBytes (
  IN CONST UINT8  *Data,
  IN UINTN        Size
  )
{
  UINT8  Sum;
  UINTN  Index;

  Sum = 0;
  for (Index = 0; Index < Size; Index++) {
    Sum = (UINT8)(Sum + Data[Index]);
  }

  return Sum;
}

/**
  Print a single CSV OID payload, one algorithm per line, annotated with the
  algorithm name where known.

  @param[in]  Payload      The NUL-terminated CSV OID string.
  @param[in]  PayloadSize  Size of Payload in bytes (including the trailing NUL).
**/
STATIC
VOID
DumpCsvOids (
  IN CONST UINT8  *Payload,
  IN UINTN        PayloadSize
  )
{
  CONST CHAR8  *TokenStart;
  CONST CHAR8  *Ptr;
  UINTN        OidCount;

  TokenStart = (CONST CHAR8 *)Payload;
  OidCount   = 0;
  for (Ptr = TokenStart; ; Ptr++) {
    if ((*Ptr == ',') || (*Ptr == '\0')) {
      CHAR8        Oid[48];
      UINTN        Length;
      CONST CHAR8  *Name;

      Length = (UINTN)(Ptr - TokenStart);
      if (Length >= sizeof (Oid)) {
        Length = sizeof (Oid) - 1;
      }

      if (Length > 0) {
        CopyMem (Oid, TokenStart, Length);
        Oid[Length] = '\0';

        OidCount++;
        Name = OidName (Oid);
        if (Name != NULL) {
          ECIT_DUMP ("     %2u. %-24a %a\n", (UINT32)OidCount, Oid, Name);
        } else {
          ECIT_DUMP ("     %2u. %a\n", (UINT32)OidCount, Oid);
        }
      }

      if (*Ptr == '\0') {
        break;
      }

      TokenStart = Ptr + 1;
    }
  }

  if (OidCount == 0) {
    ECIT_DUMP ("     (none)\n");
  }
}

/**
  Hex-dump a binary payload, 16 bytes per line.
**/
STATIC
VOID
DumpHex (
  IN CONST UINT8  *Payload,
  IN UINTN        PayloadSize
  )
{
  UINTN  ByteIndex;

  for (ByteIndex = 0; ByteIndex < PayloadSize; ByteIndex += 16) {
    CHAR8  Line[64];
    UINTN  ColumnCount;
    UINTN  Column;
    UINTN  Offset;

    ColumnCount = MIN (16, PayloadSize - ByteIndex);
    Offset      = 0;
    for (Column = 0; Column < ColumnCount; Column++) {
      Offset += AsciiSPrint (
                  &Line[Offset],
                  sizeof (Line) - Offset,
                  "%02x ",
                  Payload[ByteIndex + Column]
                  );
    }

    ECIT_DUMP ("     %a\n", Line);
  }
}

/**
  Application entry point: find, validate, and pretty-print the ECIT.

  @param[in] ImageHandle  The image handle.
  @param[in] SystemTable  The system table.

  @retval EFI_SUCCESS    The table was found and dumped.
  @retval EFI_NOT_FOUND  No ECIT is installed.
**/
EFI_STATUS
EFIAPI
DumpCryptoIndicatorTableEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;
  EFI_CRYPTO_INDICATOR_TABLE  *Table;
  EFI_CRYPTO_INDICATOR_ENTRY  *Entry;
  UINT8                       *Cursor;
  UINT8                       *TableEnd;
  UINTN                       Index;

  Status = EfiGetSystemConfigurationTable (&gEfiCryptoIndicatorTableGuid, (VOID **)&Table);
  if (EFI_ERROR (Status) || (Table == NULL)) {
    ECIT_DUMP ("ECIT: not installed (%r)\n", Status);
    return EFI_NOT_FOUND;
  }

  if ((Table->Signature[0] != 'E') || (Table->Signature[1] != 'C') ||
      (Table->Signature[2] != 'I') || (Table->Signature[3] != 'T'))
  {
    ECIT_DUMP ("ECIT: bad signature\n");
    return EFI_NOT_FOUND;
  }

  //
  // Header.
  //
  ECIT_DUMP ("%a\n", ECIT_RULE);
  ECIT_DUMP (" EFI Crypto Indicator Table (ECIT)\n");
  ECIT_DUMP ("%a\n", ECIT_RULE);
  ECIT_DUMP (" Address  : 0x%p\n", (VOID *)Table);
  ECIT_DUMP (" Length   : %u bytes\n", Table->Length);
  ECIT_DUMP (" Version  : %u\n", Table->Version);
  ECIT_DUMP (
    " Checksum : %a\n",
    (SumBytes ((UINT8 *)Table, Table->Length) == 0) ? "OK" : "BAD"
    );
  ECIT_DUMP (" OEM ID   : %.6a\n", Table->OemId);
  ECIT_DUMP (" Entries  : %u\n", Table->NumberOfEntries);

  //
  // Entries. Walk them honoring EntryLength and staying within Length.
  //
  Cursor   = (UINT8 *)Table + sizeof (EFI_CRYPTO_INDICATOR_TABLE);
  TableEnd = (UINT8 *)Table + Table->Length;

  for (Index = 0; Index < Table->NumberOfEntries; Index++) {
    CONST CHAR8  *Name;
    UINT8        *Payload;
    UINTN        PayloadSize;

    ECIT_DUMP ("%a\n", ECIT_THIN);

    if ((Cursor + sizeof (EFI_CRYPTO_INDICATOR_ENTRY)) > TableEnd) {
      ECIT_DUMP (" [%u] <truncated>\n", (UINT32)Index);
      break;
    }

    Entry = (EFI_CRYPTO_INDICATOR_ENTRY *)Cursor;
    if ((Entry->EntryLength < sizeof (EFI_CRYPTO_INDICATOR_ENTRY)) ||
        ((Cursor + Entry->EntryLength) > TableEnd))
    {
      ECIT_DUMP (" [%u] <bad length %u>\n", (UINT32)Index, Entry->EntryLength);
      break;
    }

    Payload     = Cursor + sizeof (EFI_CRYPTO_INDICATOR_ENTRY);
    PayloadSize = Entry->EntryLength - sizeof (EFI_CRYPTO_INDICATOR_ENTRY);

    Name = FeatureName (&Entry->FeatureIdentifier);
    ECIT_DUMP (" [%u] %a\n", (UINT32)Index, (Name != NULL) ? Name : "Unknown feature");
    ECIT_DUMP ("     Feature : %g\n", &Entry->FeatureIdentifier);

    if (IsCsvOidFeature (&Entry->FeatureIdentifier) && (PayloadSize > 0) &&
        (Payload[PayloadSize - 1] == '\0'))
    {
      ECIT_DUMP ("     Payload : %u bytes, algorithm OIDs:\n", (UINT32)PayloadSize);
      DumpCsvOids (Payload, PayloadSize);
    } else {
      ECIT_DUMP ("     Payload : %u bytes (raw):\n", (UINT32)PayloadSize);
      DumpHex (Payload, PayloadSize);
    }

    Cursor += Entry->EntryLength;
  }

  ECIT_DUMP ("%a\n", ECIT_RULE);
  return EFI_SUCCESS;
}
