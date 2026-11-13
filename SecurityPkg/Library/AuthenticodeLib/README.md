# AuthenticodeLib

`AuthenticodeLib` provides functionality to extract the Authenticode image
and locate certificate tables within a PE/COFF image.

The public interface is declared in
[`AuthenticodeLib.h`](../../Include/Library/AuthenticodeLib.h).

## PE/COFF file layout

The diagram below shows a conventional **unloaded PE image**. The Authenticode
image is generated from an unloaded image. Both PE32 and PE32+ use this general
organization, although their optional-header field offsets differ.

```text
Low file offset
    +---------------------------------------------------+
    | DOS header and DOS stub                           | Included
    |   e_lfanew points to the PE signature             |
    +---------------------------------------------------+
    | PE signature ("PE\0\0")                           | Included
    +---------------------------------------------------+
    | COFF file header                                  | Included
    |   NumberOfSections, SizeOfOptionalHeader, ...     |
    +---------------------------------------------------+
    | Optional header: fields before CheckSum           | Included
    +---------------------------------------------------+
    | CheckSum (4 bytes)                                | EXCLUDED
    +---------------------------------------------------+
    | Optional header: remaining fields and data        | Included
    | directories before the Certificate Table entry    |
    +---------------------------------------------------+
    | Certificate Table data-directory entry (8 bytes)  | EXCLUDED
    |   File offset (4 bytes) + table size (4 bytes)    |
    +---------------------------------------------------+
    | Remaining optional-header data directories        | Included
    +---------------------------------------------------+
    | Section headers                                   | Included
    +---------------------------------------------------+
    | Header padding through SizeOfHeaders              | Included
    +---------------------------------------------------+
    | Raw section data, e.g. .text                      | Included
    +---------------------------------------------------+
    | File padding or gaps between sections             | Included
    +---------------------------------------------------+
    | Raw section data, e.g. .data / .rdata / .reloc    | Included
    +---------------------------------------------------+
    | Extra file data before the certificate table      | Included
    +---------------------------------------------------+
    | Attribute-certificate table                       | EXCLUDED
    |   WIN_CERTIFICATE entries and table padding       |
    +---------------------------------------------------+
High file offset / end of file
```

The Certificate Table is data-directory index
`EFI_IMAGE_DIRECTORY_ENTRY_SECURITY`. Unlike most PE data directories, its
`VirtualAddress` field is a **file offset**, not a relative virtual address
(RVA). It locates the attribute-certificate table; the directory entry and
the table it points to are separate regions.

The attribute-certificate table is not an ordinary PE section. It contains
variable-length `WIN_CERTIFICATE` records aligned to 8-byte boundaries.
An Authenticode signature is carried in this table.

## What is excluded?

| Region | Bytes omitted | Reason |
| :----- | :------------ | :----- |
| Optional-header `CheckSum` | 4 bytes | The PE checksum is not part of the Authenticode image. |
| Certificate Table data-directory entry | 8 bytes, when the entry exists | The certificate table's location and size are not part of the Authenticode image. |
| Attribute-certificate table | The size recorded in the Certificate Table directory | Embedded certificates and signatures are separate from the Authenticode image. |

These bytes are **omitted**, not replaced with zeros. Other optional-header
fields, the section headers, and header padding remain in the byte stream.
Sections are not excluded based on their names: for example, `.reloc` is
included when it has raw file data.

For an unsigned image, the certificate-table size is normally zero, so
there is no certificate-table payload to remove. The 8-byte directory entry
is still omitted if it exists in the optional header. If
`NumberOfRvaAndSizes` does not reach the Certificate Table index, there is
no such entry to omit.

## Assembling the Authenticode image

`BuildAuthenticodeImage()` returns a contiguous buffer containing the
Authenticode image. This buffer is not a loadable PE image.

```text
    +---------------------------------------------------+
    | Headers before CheckSum                           |
    +---------------------------------------------------+
    | Headers after CheckSum, before Certificate Table  |
    | directory entry                                   |
    +---------------------------------------------------+
    | Headers after that entry, through SizeOfHeaders   |
    +---------------------------------------------------+
    | Section raw data in ascending PointerToRawData    |
    | order, including file gaps between sections       |
    +---------------------------------------------------+
    | Remaining file data, excluding certificate table  |
    +---------------------------------------------------+
```

The implementation separates this work into three private helpers:

1. `GetAuthenticodeImageLayout()` locates the header fields and section table
   after `PeCoffLib` has validated the image.
2. `AppendAuthenticodeHeaders()` copies the headers while skipping the
   checksum and, when present, the Certificate Table directory entry.
3. `AppendSections()` sorts nonempty section ranges by `PointerToRawData`
   and appends their raw data and intervening file gaps in order. Sections
   with no raw data contribute no section bytes.
4. `AppendRemainingImageData()` appends remaining file data up to the
   certificate table, or to the end of an unsigned image.

The function rejects overlapping section ranges and sections that extend
into the headers or certificate table. When a certificate table is present,
it must begin on an 8-byte boundary, be at least `sizeof (WIN_CERTIFICATE)`
bytes long, and end at the end of the file. Other layouts return
`EFI_VOLUME_CORRUPTED` rather than producing an ambiguous Authenticode image.
PE header and section fields are copied into aligned local structures before
being read, and offsets and lengths are checked against the supplied buffer.

File-alignment padding within `SizeOfRawData` is included. Zero-filled
virtual memory beyond a section's raw file data is not added: the input is
the original file, not the loaded image. TE images are unsupported by
`BuildAuthenticodeImage()`.

## Buffer ownership and certificate lookup

- `BuildAuthenticodeImage()` allocates the assembled buffer. The caller must
  release it with `FreePool()` when it is no longer needed.
- `GetWinCertificates()` returns a borrowed byte pointer into the original
  image buffer and the certificate-table length. It returns `NULL` and length
  zero for an image with no certificate table. Do not free this pointer;
  keep the original image buffer alive while using it. The pointer may be
  unaligned even if the table's file offset is aligned. The function checks
  the table range, file-offset alignment, and minimum header size.
- Certificate records have variable lengths. Walk them using `dwLength`
  and 8-byte alignment, not array indexing. Copy record headers into aligned
  storage before accessing their fields, and validate each record against
  the returned length. Locating the table does not validate its placement,
  record structure, or signatures.

## Tests

[`GoogleTest/AuthenticodeLibGoogleTest.cpp`](GoogleTest/AuthenticodeLibGoogleTest.cpp)
exercises certificate-table lookup and byte-for-byte assembly of signed and
unsigned PE32/PE32+ images, including section ordering, gaps, omitted header
fields, trailing data, unaligned buffers, truncated images, and malformed
section and certificate ranges.

[`GoogleTest/AuthenticodeLibFaultGoogleTest.cpp`](GoogleTest/AuthenticodeLibFaultGoogleTest.cpp)
uses test-only adapters and mocked dependencies to exercise internal buffer
boundaries, read callbacks, allocation failures, and checked-arithmetic
failures.

## References

- [Microsoft PE format](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format),
  particularly the Optional Header Data Directories and Attribute
  Certificate Table sections.
