# ImageSecureBootVerificationResultTableLib

Image Secure Boot Verification Result Table (IVRT) table defintion to fully describe images
evaluated by the secure boot image verification process. This table contains an entry for every
image evaluated by secure boot. This library contains the full definition of this table and
provides methods to both generate and parse this dynamically sized table.

This structure has two nested dynamically sized entries. The first level of this structure is a
dynamically sized list of `EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT` entries, which describe
the verification status of each image evaluated. If a particular image contains signatures, then
an additional nested dynamically sized lized of `EFI_SIGNATURE_VERIFICATION_RESULT` entries exists
for that given entry. See the table layout below.

## Table layout

```text
EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE
  Signature ('IVRT') | Version | Length | NumberOfImages
  +-- EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT
  |     Length | ImageStatus | NumberOfSignatures | NameLength |
  |     DevicePathLength | ImageDigestAlgorithm
  |     Name[]        (NameLength bytes, optional)
  |     DevicePath    (DevicePathLength bytes)
  |     +-- EFI_SIGNATURE_VERIFICATION_RESULT
  |     |     Length | SignatureIndex | Status | ThumbprintAlgorithm | Thumbprint[]
  |     +-- ...
  +-- ...
```

## Table Builder

`CreateImage` allocates an image record (name + device path up front), `AppendSignature` reallocates
it a signature larger for each evaluated signature, and `AppendImage` updates the now-known status and
returns a new, exactly-sized table (old + image). The image record and the old table are pool
allocations the caller frees with `FreePool`.

```c
VOID                                             *Image;
EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE  *NewTable;

// Allocate the image record, then record each evaluated signature.
ImageVerificationResultCreateImage (Name, DevicePath, DevicePathSize, &Image);
ImageVerificationResultAppendSignature (&Image, 0, SigStatus, ThumbAlgo, Thumb, ThumbSize);

// Append to the currently installed table (OldTable, or NULL for the first image).
// The now-known image status is supplied here.
ImageVerificationResultAppendImage (OldTable, Image, ImageStatus, DigestAlgo, &NewTable);

// ... install NewTable in place of OldTable, then free the old table and the image ...
FreePool (OldTable);
FreePool (Image);
```

## Table Iteration

The table iterator is used to walk both tiers of the table (each individual image verification
result and individual signature verification results per image verification). `NextImage` walks
the image list and resets the inner cursor to the new image while `NextSignature` walks the
current image's signatures:

```c
IMAGE_VERIFICATION_RESULT_ITERATOR               Iter;
const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT  *Image;
const EFI_SIGNATURE_VERIFICATION_RESULT          *Sig;

if (!ImageVerificationResultIteratorInit (&Iter, Table)) {
  DEBUG ((DEBUG_WARN, "IVRT truncated; iterating the valid prefix only\n"));
}

while ((Image = ImageVerificationResultIteratorNextImage (&Iter)) != NULL) {
  while ((Sig = ImageVerificationResultIteratorNextSignature (&Iter)) != NULL) {
    // ... a signature of the current Image ...
  }
}
```

Note that a malformed table can still be walked up to the portion that is malformed. This may
result in an iterator of length zero.
