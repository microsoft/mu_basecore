# ImageValidationTestApp

## Overview

`ImageValidationTestApp` is a UEFI shell test application that validates image authentication
behavior before loading an image. Its purpose is to ensure the platform image validation handler
(invoked by `LoadImage`) meets UEFI specification Secure Boot requirements based on the provided
inputs:

- Image type (unsigned, signed, multi-signed)
- `db` (allowed signatures/hashes)
- `dbx` (revoked signatures/hashes)

## Test Framework

A simple framework was created so that each test scenario specifies the image type, `db`/`dbx`
contents, and the expected output. The framework itself follows the same flow for each test
scenario:

1. Build synthetic `db` and `dbx` databases from scenario bit flags.
2. Hook `GetVariable` so reads the following variables return scenario controlled values:
    - `db`
    - `dbx`
    - `SecureBoot`
3. Call `LoadImage` on the scenario-selected built-in image buffer.
4. Compare the returned `EFI_STATUS` to the scenario's expected result.

The `GetVariable` hook exists only for the duration of test execution and is
restored afterward.

## Supported Image Types

The test supports three image classes:

1. `IMAGE_TYPE_UNSIGNED`
    - Unsigned PE/COFF image.

2. `IMAGE_TYPE_SIGNED`
    - Single PKCS#7 Authenticode signature (`WIN_CERTIFICATE`).
    - Signature chain is four signer 1 certificates deep:
    - `SIGNER1_ROOT`
    - `SIGNER1_INTERMEDIATE1`
    - `SIGNER1_INTERMEDIATE2`
    - `SIGNER1_LEAF`

3. `IMAGE_TYPE_MULTI_SIGNED`
    - Two PKCS#7 Authenticode signatures.
    - Signature 1 is the same signer 1 chain as `IMAGE_TYPE_SIGNED`.
    - Signature 2 is `SIGNER2` alone (a single self-signed certificate), plus a few
      unrelated certificates embedded in the signature that are not part of its chain.

The following variants reuse the images above but are modified on a private copy at load time.
Items 4–6 damage a `WIN_CERTIFICATE` to exercise malformed-input handling; item 7 tampers the
image body to exercise content-integrity enforcement:

4. `IMAGE_TYPE_SIGNED_CORRUPT_CERT`
    - The signed image with its single `WIN_CERTIFICATE` damaged — its `dwLength` set out of
      range — so the platform aborts when it parses that entry. The Security data directory
      that locates the table is left intact.

5. `IMAGE_TYPE_MULTI_SIGNED_CORRUPT_FIRST_CERT`
    - The multi-signed image with its first (signer 1) `WIN_CERTIFICATE` damaged the same way,
      so the platform aborts before it can reach the second signature.

6. `IMAGE_TYPE_MULTI_SIGNED_CORRUPT_SECOND_CERT`
    - The multi-signed image with its second (signer 2) `WIN_CERTIFICATE` damaged the same way,
      so the platform aborts only after it has already parsed the intact first signature.

7. `IMAGE_TYPE_SIGNED_TAMPERED`
    - The signed image with a single body byte flipped after signing, so its Authenticode hash no
      longer matches the embedded signature or the original image digest. The `WIN_CERTIFICATE` is
      left intact, so the image still parses and reaches signature validation.

## DB / DBX Scenario Flags

Scenario inputs use `DB_STATE_*` bit flags. These flags describe what is added
to the generated `db` or `dbx` EFI signature database for that scenario.

`db` and `dbx` use the same flags. The meaning of a flag is identical in both;
only whether it allows (`db`) or revokes (`dbx`) changes behavior.

### Flag Definitions

Flags can be OR-combined to place multiple signature lists into one database, and hash
algorithms can be mixed freely (for example a SHA-256 digest alongside a SHA-512 TBS hash).

1. `DB_STATE_EMPTY`
    - `GetVariable` returns `EFI_NOT_FOUND`

2. `DB_STATE_IMAGE_DIGEST_SHA256` / `_SHA384` / `_SHA512`
    - Add the image's Authenticode digest for that algorithm

3. `DB_STATE_SIGNER1_LEAF_CERT`
    - Add signer 1 leaf X.509 certificate (`EFI_CERT_X509_GUID`).

4. `DB_STATE_SIGNER1_INTERMEDIATE1_CERT`
    - Add signer 1 intermediate 1 (root-signed) X.509 certificate.

5. `DB_STATE_SIGNER1_INTERMEDIATE2_CERT`
    - Add signer 1 intermediate 2 (leaf-signing) X.509 certificate.

6. `DB_STATE_SIGNER1_ROOT_CERT`
    - Add signer 1 root X.509 certificate.

7. `DB_STATE_SIGNER2_CERT`
    - Add signer 2 X.509 certificate (used by multi-signed image scenarios).

8. `DB_STATE_SIGNER2_UNRELATED_CERT`
    - Add the X.509 certificate embedded in signer 2's signature but not part of its chain
      (multi-signed scenarios).

9. `DB_STATE_SIGNER1_LEAF_TBS_HASH_SHA256` / `_SHA384` / `_SHA512`
    - Add the signer 1 leaf certificate's TBSCertificate hash for that algorithm

10. `DB_STATE_SIGNER1_INTERMEDIATE1_TBS_HASH_SHA256` / `_SHA384` / `_SHA512`
    - Add the signer 1 intermediate 1 certificate's TBSCertificate hash for that algorithm.

11. `DB_STATE_SIGNER1_INTERMEDIATE2_TBS_HASH_SHA256` / `_SHA384` / `_SHA512`
    - Add the signer 1 intermediate 2 certificate's TBSCertificate hash for that algorithm.

12. `DB_STATE_SIGNER1_ROOT_TBS_HASH_SHA256` / `_SHA384` / `_SHA512`
    - Add the signer 1 root certificate's TBSCertificate hash for that algorithm.

13. `DB_STATE_SIGNER2_TBS_HASH_SHA256` / `_SHA384` / `_SHA512`
    - Add the signer 2 certificate's TBSCertificate hash for that algorithm
      (multi-signed scenarios).

14. `DB_STATE_SIGNER2_UNRELATED_TBS_HASH_SHA256` / `_SHA384` / `_SHA512`
    - Add the TBSCertificate hash (for that algorithm) of a certificate embedded in
      signer 2's signature but not part of its chain (multi-signed scenarios).

15. `DB_STATE_CORRUPT_SIGNATURE_LIST_FIRST` / `DB_STATE_CORRUPT_SIGNATURE_LIST_LAST`
    - Add a deliberately malformed `EFI_SIGNATURE_LIST` (invalid size fields) to the database
      to exercise handling of a corrupt `db` / `dbx`. `_FIRST` places it before all other
      entries; `_LAST` places it after all other entries.

## Test Catalog

This catalog is the human-readable companion to the tests defined in `Scenarios.c`. The
**[Top-Level Groups](#top-level-groups)** table gives the high-level test groups, and each
group's section below opens with a table of its second-level behaviors. The full per-ID index is
the **[All Tests](#all-tests)** table at the bottom of this document. To look up a test, find its
ID and follow the link to the section that documents it, which covers the specifics such as the
exact `db` / `dbx` contents. In VS Code you can also use the Markdown outline (Ctrl+Shift+O) to
jump between sections.

Test IDs follow the format `<group>.<behavior>.<scenario>` with an optional trailing run letter —
for example `2.4.1` or `2.4.1a`:

- **First value — top-level group** (`1`–`7`): the broad category under test, listed in the
  [Top-Level Groups](#top-level-groups) table (for example `2` is Signed Image Validation).
- **Second value — second-level behavior**: a specific behavior within that group, listed in the
  group's own table (for example `2.4` is Chain Revocation Relativeness).
- **Third value — scenario**: one concrete case of that behavior (for example `2.4.1` is a root
  anchor in `db` revoked by a root revoker in `dbx`).
- **Trailing letter — run** (`a`, `b`, `c`, ...): one run of a scenario that repeats over several
  variations, such as the hash algorithm or the certificate / TBS-hash form (for example
  `2.4.1a`–`2.4.1d`). A scenario with only one run has no letter.

To skip straight to a flat index of every test with its expected result, jump to
[All Tests](#all-tests).

### Top-Level Groups

| ID | Group | Description |
| -- | ----- | ----------- |
| [`1`](#1-unsigned-image-validation) | Unsigned Image Validation | Tests for images with no Authenticode signature. |
| [`2`](#2-signed-image-validation) | Signed Image Validation | Tests for images with a single Authenticode signature (signer 1 chain: root → intermediate 1 → intermediate 2 → leaf). |
| [`3`](#3-multi-signed-image-validation) | Multi-Signed Image Validation | Tests for images carrying two signatures (the signer 1 chain plus signer 2). |
| [`4`](#4-unrelated-certificate-handling) | Unrelated Certificate Handling | Tests that a certificate riding along in a signature but not part of its chain neither authorizes (`db`) nor revokes (`dbx`) the image. |
| [`5`](#5-database-parsing-errors) | Database Parsing Errors | Tests that `db` / `dbx` parsing aborts at a malformed entry: entries before it are honored, entries after are unreachable, and a corrupt `dbx` denies. |
| [`6`](#6-image-certificate-parsing-errors) | Image Certificate Parsing Errors | Tests that image `WIN_CERTIFICATE` parsing aborts at a malformed entry, while an image digest in `db` still authorizes independently. |
| [`7`](#7-tampered-image-rejection) | Tampered Image Rejection | Tests that an image whose body was modified after signing is always denied, regardless of `db` / `dbx` contents. |
| [`8`](#8-sha-1-rejection) | SHA-1 Rejection | Tests that SHA-1 is not a supported hash algorithm: a SHA-1 image digest in `db` cannot authorize an image, and one in `dbx` cannot revoke it. |
| [`9`](#9-v2-signature-type-guids) | V2 Signature-Type GUIDs | Tests that the handler accepts the V2 signature types (`EFI_CERT_V2_*`, `EFI_SIGNATURE_V2_DATA` layout) for image digests, X.509 certificates, and TBS-cert hashes, in both `db` and `dbx`. |

### 1. Unsigned Image Validation

**Image type:** `IMAGE_TYPE_UNSIGNED`.

An unsigned image carries no signature.

| ID | Group | Description |
| -- | ----- | ----------- |
| [`1.1`](#11-empty-database-denial) | Empty Database Denial | An unsigned image with an empty `db` is always denied. |
| [`1.2`](#12-revoked-digest-denial) | Revoked-Digest Denial | An unsigned image is denied by its digest in `dbx` even when `db` would authorize it. |
| [`1.3`](#13-digest-approval) | Digest Approval | An unsigned image is authorized by its digest in `db` and `dbx` is empty. |

#### 1.1 Empty Database Denial

- **Purpose:** An unsigned image with an empty `db` is always denied.
- **Image type:** Unsigned (`IMAGE_TYPE_UNSIGNED`).
- **db:** Empty (`DB_STATE_EMPTY`).
- **dbx:** Empty (`DB_STATE_EMPTY`).
- **Expected result:** `EFI_ACCESS_DENIED`.
- **Rationale:** With nothing in `db` to authorize the image, it must be denied; an empty `dbx`
  does not imply approval.

#### 1.2 Revoked-Digest Denial

- **Purpose:** An unsigned image is denied when its digest is listed in `dbx`, even though an
  authorizing digest in `db` approves the same image.
- **Image type:** Unsigned (`IMAGE_TYPE_UNSIGNED`).
- **db:** The image's Authenticode digest, which authorizes the image (`DB_STATE_IMAGE_DIGEST_*`).
- **dbx:** The same image Authenticode digest, which revokes it (`DB_STATE_IMAGE_DIGEST_*`).
- **Expected result:** `EFI_ACCESS_DENIED` for every run.
- **Rationale:** `dbx` revocation takes precedence over `db` authority, so when the image's
  digest is present in both, the `dbx` match denies the image even though the `db` match would
  approve it.

Each run places the image digest for one hash algorithm in both `db` and `dbx`:

| Run | Hash algorithm |
| --- | -------------- |
| `1.2.1a` | SHA-256 |
| `1.2.1b` | SHA-384 |
| `1.2.1c` | SHA-512 |

#### 1.3 Digest Approval

- **Purpose:** An unsigned image is approved when its digest is listed in `db` and the `dbx` is
  empty, for each hash algorithm.
- **Image type:** Unsigned (`IMAGE_TYPE_UNSIGNED`).
- **db:** The image's Authenticode digest, which authorizes the image (`DB_STATE_IMAGE_DIGEST_*`).
- **dbx:** Empty (`DB_STATE_EMPTY`).
- **Expected result:** `EFI_SUCCESS` for every run.
- **Rationale:** With the image's digest present in `db` and no revocation in `dbx`, the image
  is authorized; this is the positive counterpart to 1.2's revocation test.

Each run places the image digest for one hash algorithm in `db` (with an empty `dbx`):

| Run | Hash algorithm |
| --- | -------------- |
| `1.3.1a` | SHA-256 |
| `1.3.1b` | SHA-384 |
| `1.3.1c` | SHA-512 |

### 2. Signed Image Validation

**Image type:** `IMAGE_TYPE_SIGNED`.

A singly-signed image carries one Authenticode signature whose chain is four certificates
deep: root CA → intermediate 1 CA → intermediate 2 CA → leaf.

| ID | Group | Description |
| -- | ----- | ----------- |
| [`2.1`](#21-empty-database-denial) | Empty Database Denial | A signed image with an empty `db` is always denied. |
| [`2.2`](#22-revoked-digest-denial) | Revoked-Digest Denial | A signed image is denied by its image digest in `dbx`, however `db` would authorize it. |
| [`2.3`](#23-database-approval) | Database Approval | A signed image is approved by an image digest, chain certificate, or certificate TBS hash in `db` (empty `dbx`). |
| [`2.4`](#24-chain-revocation-relativeness) | Chain Revocation Relativeness | A `dbx` chain entry revokes only when at the `db` anchor's position or closer to the signer. |
| [`2.5`](#25-image-digest-overrides-signature-revocation) | Image-Digest Overrides Signature Revocation | A revoked signature is still approved when the image digest is in `db`. |

#### 2.1 Empty Database Denial

- **Purpose:** A signed image with an empty `db` is always denied.
- **Image type:** Signed (`IMAGE_TYPE_SIGNED`).
- **db:** Empty (`DB_STATE_EMPTY`).
- **dbx:** Empty (`DB_STATE_EMPTY`).
- **Expected result:** `EFI_ACCESS_DENIED`.
- **Rationale:** An empty `db` provides no trust anchor for the signature, so the image is
  denied; an empty `dbx` does not imply approval.

#### 2.2 Revoked-Digest Denial

- **Purpose:** A signed image is denied whenever its Authenticode digest is listed in `dbx`, no
  matter how `db` would otherwise authorize the image (image digest, chain certificate, or
  certificate TBS hash).
- **Image type:** Signed (`IMAGE_TYPE_SIGNED`).
- **db:** An authorizing entry — image digest, chain certificate, or certificate TBS hash.
- **dbx:** The image's Authenticode digest, which revokes the image.
- **Expected result:** `EFI_ACCESS_DENIED` for every run.
- **Rationale:** `dbx` revocation of the image digest takes precedence over any `db`
  authorization, so the image is denied regardless of what authorizes it in `db`.

The third ID segment selects the authorizing entry placed in `db`; the image digest is always
present in `dbx`:

| ID | `db` authorizing entry |
| -- | ---------------------- |
| `2.2.1` | Image Authenticode digest |
| `2.2.2` | Signer 1 chain certificate |
| `2.2.3` | Signer 1 certificate TBS hash |

`2.2.1` places the image digest in both `db` and `dbx`, one case per hash algorithm:

| Run | Image digest (`db` and `dbx`) |
| --- | ----------------------------- |
| `2.2.1a` | SHA-256 |
| `2.2.1b` | SHA-384 |
| `2.2.1c` | SHA-512 |

`2.2.2` places a chain certificate in `db` and the SHA-256 image digest in `dbx`
(certificates are algorithm-independent):

| Run | `db` certificate |
| --- | ---------------- |
| `2.2.2a` | Leaf |
| `2.2.2b` | Intermediate 1 |
| `2.2.2c` | Root |

`2.2.3` places a certificate TBS hash in `db` and the SHA-256 image digest in `dbx`:

| Run | `db` certificate | `db` TBS hash algorithm |
| --- | ---------------- | ----------------------- |
| `2.2.3a` | Leaf | SHA-256 |
| `2.2.3b` | Leaf | SHA-384 |
| `2.2.3c` | Leaf | SHA-512 |
| `2.2.3d` | Intermediate 1 | SHA-256 |
| `2.2.3e` | Intermediate 1 | SHA-384 |
| `2.2.3f` | Intermediate 1 | SHA-512 |
| `2.2.3g` | Root | SHA-256 |
| `2.2.3h` | Root | SHA-384 |
| `2.2.3i` | Root | SHA-512 |

#### 2.3 Database Approval

- **Purpose:** A signed image is approved whenever `db` holds an authority that covers it — its
  image digest, a certificate in its signing chain, or a chain certificate's TBS hash — and the
  `dbx` is empty.
- **Image type:** Signed (`IMAGE_TYPE_SIGNED`).
- **dbx:** Empty (`DB_STATE_EMPTY`).
- **Expected result:** `EFI_SUCCESS` for every run.
- **Rationale:** With no revocation in `dbx`, a single authorizing entry in `db` is sufficient to
  approve the image, across every authority type and hash algorithm.

The third ID segment selects the authority type placed in `db`:

| ID | `db` authorizing entry |
| -- | ---------------------- |
| `2.3.1` | Image Authenticode digest |
| `2.3.2` | Signer 1 chain certificate |
| `2.3.3` | Signer 1 certificate TBS hash |

`2.3.1` places the image digest in `db`, one run per hash algorithm:

| Run | Image digest |
| --- | ------------ |
| `2.3.1a` | SHA-256 |
| `2.3.1b` | SHA-384 |
| `2.3.1c` | SHA-512 |

`2.3.2` places one signer 1 chain certificate in `db` (certificates are algorithm-independent):

| Run | `db` certificate |
| --- | ---------------- |
| `2.3.2a` | Leaf |
| `2.3.2b` | Intermediate 1 |
| `2.3.2c` | Intermediate 2 |
| `2.3.2d` | Root |

`2.3.3` places one signer 1 chain certificate's TBS hash in `db`, for each certificate and hash
algorithm:

| Run | `db` certificate | TBS hash algorithm |
| --- | ---------------- | ------------------ |
| `2.3.3a` | Leaf | SHA-256 |
| `2.3.3b` | Leaf | SHA-384 |
| `2.3.3c` | Leaf | SHA-512 |
| `2.3.3d` | Intermediate 1 | SHA-256 |
| `2.3.3e` | Intermediate 1 | SHA-384 |
| `2.3.3f` | Intermediate 1 | SHA-512 |
| `2.3.3g` | Intermediate 2 | SHA-256 |
| `2.3.3h` | Intermediate 2 | SHA-384 |
| `2.3.3i` | Intermediate 2 | SHA-512 |
| `2.3.3j` | Root | SHA-256 |
| `2.3.3k` | Root | SHA-384 |
| `2.3.3l` | Root | SHA-512 |

#### 2.4 Chain Revocation Relativeness

- **Purpose:** When `db` authorizes a signed image through a certificate in the signer chain
  (as an X.509 certificate or as a TBS hash), a `dbx` entry revokes the image only when it names
  a certificate at the same chain position as the `db` anchor or closer to the signer. A `dbx`
  entry naming a certificate closer to the root than the `db` anchor does not revoke, because it
  sits above the trust anchor and is not part of the trusted path.
- **Image type:** Signed (`IMAGE_TYPE_SIGNED`).
- **Expected result:** per the matrix below. (A revoking image digest in `dbx` always denies and
  is covered by 2.2, so `dbx` here holds only a chain certificate or chain TBS hash.)
- **Rationale:** Authorizing through a chain certificate anchors trust at that certificate, so
  the trusted path runs from the anchor down to the leaf. Revoking any certificate on that path
  (the anchor itself or one closer to the signer) breaks it; revoking a certificate above the
  anchor does not, because that certificate is no longer relied upon once trust is anchored
  lower.

The signer 1 chain, ordered from the root toward the signer, is root → intermediate 1 →
intermediate 2 → leaf. With the `db` anchor as the row and the `dbx` certificate as the column,
`D` denotes `EFI_ACCESS_DENIED` (revoked) and `A` denotes `EFI_SUCCESS` (approved):

| `db` anchor ↓ / `dbx` → | Root | Intermediate 1 | Intermediate 2 | Leaf |
| ----------------------- | ---- | -------------- | -------------- | ---- |
| Root | D | D | D | D |
| Intermediate 1 | A | D | D | D |
| Intermediate 2 | A | A | D | D |
| Leaf | A | A | A | D |

Each numbered scenario `2.4.1`–`2.4.16` fixes one (`db` anchor, `dbx` revoker) position pair
from the matrix above, and its four runs `a`–`d` cover every combination of the two
representation forms — full X.509 certificate and SHA-256 TBS hash — for the anchor and the
revoker. All four runs of a scenario share the same expected result, because revocation
depends only on chain position, not on the form used to express the anchor or the revoker:

| Run | `db` anchor form | `dbx` revoker form |
| --- | ---------------- | ------------------ |
| `a` | X.509 certificate | X.509 certificate |
| `b` | X.509 certificate | SHA-256 TBS hash |
| `c` | SHA-256 TBS hash | X.509 certificate |
| `d` | SHA-256 TBS hash | SHA-256 TBS hash |

The position pair fixed by each scenario, and the expected result shared by its four runs:

| ID | `db` anchor | `dbx` revoker | Expected (all runs) |
| -- | ----------- | ------------- | ------------------- |
| `2.4.1` | Root | Root | `EFI_ACCESS_DENIED` |
| `2.4.2` | Root | Intermediate 1 | `EFI_ACCESS_DENIED` |
| `2.4.3` | Root | Intermediate 2 | `EFI_ACCESS_DENIED` |
| `2.4.4` | Root | Leaf | `EFI_ACCESS_DENIED` |
| `2.4.5` | Intermediate 1 | Root | `EFI_SUCCESS` |
| `2.4.6` | Intermediate 1 | Intermediate 1 | `EFI_ACCESS_DENIED` |
| `2.4.7` | Intermediate 1 | Intermediate 2 | `EFI_ACCESS_DENIED` |
| `2.4.8` | Intermediate 1 | Leaf | `EFI_ACCESS_DENIED` |
| `2.4.9` | Intermediate 2 | Root | `EFI_SUCCESS` |
| `2.4.10` | Intermediate 2 | Intermediate 1 | `EFI_SUCCESS` |
| `2.4.11` | Intermediate 2 | Intermediate 2 | `EFI_ACCESS_DENIED` |
| `2.4.12` | Intermediate 2 | Leaf | `EFI_ACCESS_DENIED` |
| `2.4.13` | Leaf | Root | `EFI_SUCCESS` |
| `2.4.14` | Leaf | Intermediate 1 | `EFI_SUCCESS` |
| `2.4.15` | Leaf | Intermediate 2 | `EFI_SUCCESS` |
| `2.4.16` | Leaf | Leaf | `EFI_ACCESS_DENIED` |

`2.4.17` places two chain anchors in `db` at once — intermediate 1 and intermediate 2 — while
`dbx` revokes only intermediate 1. Certificates are written into `db` in chain order (leaf,
intermediate 1, intermediate 2, root), so the revoked intermediate 1 anchor is evaluated first;
authorization must not stop at that revoked anchor but continue to the intermediate 2 anchor,
which the intermediate 1 entry in `dbx` (above it) does not revoke, so the image is approved. The
four runs cover both representation forms for the `db` anchors and the `dbx` revoker:

| ID | `db` anchors | `dbx` revoker | Expected |
| -- | ------------ | ------------- | -------- |
| `2.4.17a` | Intermediate 1 + intermediate 2 certificates | Intermediate 1 certificate | `EFI_SUCCESS` |
| `2.4.17b` | Intermediate 1 + intermediate 2 certificates | Intermediate 1 TBS hash | `EFI_SUCCESS` |
| `2.4.17c` | Intermediate 1 + intermediate 2 TBS hashes | Intermediate 1 certificate | `EFI_SUCCESS` |
| `2.4.17d` | Intermediate 1 + intermediate 2 TBS hashes | Intermediate 1 TBS hash | `EFI_SUCCESS` |

#### 2.5 Image-Digest Overrides Signature Revocation

- **Purpose:** A signed image whose signing certificate is revoked by `dbx` is still approved
  when the image's own Authenticode digest is enrolled in `db`. The image-digest authority is
  evaluated after the per-signature step and is independent of the revoked signature.
- **Image type:** Signed (`IMAGE_TYPE_SIGNED`).
- **db:** The image's SHA-256 Authenticode digest (`DB_STATE_IMAGE_DIGEST_SHA256`).
- **dbx:** The signer 1 leaf certificate (`DB_STATE_SIGNER1_LEAF_CERT`), which revokes the
  signature.
- **Expected result:** `EFI_SUCCESS`.
- **Rationale:** Validation rejects the revoked *signature* but continues to the image-digest
  check, where a digest match in `db` authorizes the image on its own. This is the counterpart to
  an image digest in `dbx` (2.2), which is always fatal: a revoked certificate is survivable
  through a `db` image digest, whereas a revoked image digest is not.

### 3. Multi-Signed Image Validation

**Image type:** `IMAGE_TYPE_MULTI_SIGNED`.

A multi-signed image carries two signatures: the signer 1 chain
(root → intermediate 1 → intermediate 2 → leaf) and signer 2. Signer 2's signature also
embeds unrelated certificates that are not part of its chain.

| ID | Group | Description |
| -- | ----- | ----------- |
| [`3.1`](#31-empty-database-denial) | Empty Database Denial | A multi-signed image with an empty `db` is always denied. |
| [`3.2`](#32-revoked-digest-denial) | Revoked-Digest Denial | A multi-signed image is denied by its image digest in `dbx`, however `db` would authorize it. |
| [`3.3`](#33-two-signature-evaluation) | Two-Signature Evaluation | How the two signatures' per-signature results combine into a single image decision. |

#### 3.1 Empty Database Denial

- **Purpose:** A multi-signed image with an empty `db` is always denied.
- **Image type:** Multi-Signed (`IMAGE_TYPE_MULTI_SIGNED`).
- **db:** Empty (`DB_STATE_EMPTY`).
- **dbx:** Empty (`DB_STATE_EMPTY`).
- **Expected result:** `EFI_ACCESS_DENIED`.
- **Rationale:** An empty `db` provides no trust anchor for either signature, so the image is
  denied; an empty `dbx` does not imply approval.

#### 3.2 Revoked-Digest Denial

- **Purpose:** A multi-signed image is denied whenever its Authenticode digest is listed in
  `dbx`, no matter how `db` would otherwise authorize the image (image digest, signer 2
  certificate, or signer 2 certificate TBS hash).
- **Image type:** Multi-Signed (`IMAGE_TYPE_MULTI_SIGNED`).
- **db:** An authorizing entry — image digest, signer 2 certificate, or signer 2 certificate
  TBS hash.
- **dbx:** The image's Authenticode digest, which revokes the image.
- **Expected result:** `EFI_ACCESS_DENIED` for every run.
- **Rationale:** `dbx` revocation of the image digest takes precedence over any `db`
  authorization, so the image is denied regardless of what authorizes it in `db`.

The third ID segment selects the authorizing entry placed in `db`; the image digest is always
present in `dbx`:

| ID | `db` authorizing entry |
| -- | ---------------------- |
| `3.2.1` | Image Authenticode digest |
| `3.2.2` | Signer 2 certificate |
| `3.2.3` | Signer 2 certificate TBS hash |

`3.2.1` places the image digest in both `db` and `dbx`, one case per hash algorithm:

| Run | Image digest (`db` and `dbx`) |
| --- | ----------------------------- |
| `3.2.1a` | SHA-256 |
| `3.2.1b` | SHA-384 |
| `3.2.1c` | SHA-512 |

`3.2.2` places the signer 2 certificate in `db` and the SHA-256 image digest in `dbx`. Signer 2
is a single self-signed certificate, so this test has one case.

`3.2.3` places a signer 2 certificate TBS hash in `db` and the SHA-256 image digest in `dbx`,
one case per hash algorithm:

| Run | Signer 2 TBS hash algorithm |
| --- | --------------------------- |
| `3.2.3a` | SHA-256 |
| `3.2.3b` | SHA-384 |
| `3.2.3c` | SHA-512 |

#### 3.3 Two-Signature Evaluation

- **Purpose:** Exercise the behavior that only exists with two `WIN_CERTIFICATE`s: how the
  per-signature results combine into a single image decision. Validation returns on the first
  authorizing signature, records-and-continues past a revoked one, and denies only once no
  signature authorizes. Authorizing or revoking a single signature is itself covered by Group 2.
- **Image type:** Multi-Signed (`IMAGE_TYPE_MULTI_SIGNED`).
- **db / dbx:** Only signer entries (no image digests), so the two-signature loop alone decides.
  Signer 1 authority is its leaf certificate (`DB_STATE_SIGNER1_LEAF_CERT`); signer 2 authority is
  its certificate (`DB_STATE_SIGNER2_CERT`); a revoker is the same certificate placed in `dbx`.
- **Expected result:** per the table below.
- **Rationale:** A multi-signed image is authorized when any one of its signatures is authorized
  by `db` and that signature is not revoked by `dbx`; a *different* signature being revoked does
  not by itself deny the image.

Signer 1 is the first certificate (its chain), signer 2 is the second. "Authorized" means the
signer is in `db`; "revoked" means it is in `dbx`; "absent" means it is in neither:

| ID | Signer 1 | Signer 2 | Expected | Behavior exercised |
| -- | -------- | -------- | -------- | ------------------ |
| `3.3.1` | Authorized | absent | `EFI_SUCCESS` | the first signature authorizes; a second signature does not interfere |
| `3.3.2` | absent | Authorized | `EFI_SUCCESS` | the loop advances and authorizes via the second signature |
| `3.3.3` | Revoked | Authorized | `EFI_SUCCESS` | a revoked signature does not stop evaluation of the next one |
| `3.3.4` | Authorized | Revoked | `EFI_SUCCESS` | the first authorization returns before the second (revoked) signature is consulted |
| `3.3.5` | Revoked | absent | `EFI_ACCESS_DENIED` | a revoked signature with no authorization denies |
| `3.3.6` | absent | Revoked | `EFI_ACCESS_DENIED` | no authorization with a revoked signature denies |
| `3.3.7` | Revoked | Revoked | `EFI_ACCESS_DENIED` | both signatures revoked denies |

### 4. Unrelated Certificate Handling

These tests confirm that a certificate that merely rides along inside a signature — embedded in
the PKCS#7 but not part of the signer's chain, so it signs nothing — is neither a valid
authority in `db` nor a valid revocation in `dbx`. Only a certificate that actually anchors a
signature's chain may authorize or revoke the image.

| ID | Group | Description |
| -- | ----- | ----------- |
| [`4.1`](#41-unrelated-certificate-non-authorization) | Unrelated Certificate Non-Authorization | A certificate outside the signer's chain in `db` does not authorize the image. |
| [`4.2`](#42-unrelated-certificate-non-revocation) | Unrelated Certificate Non-Revocation | A certificate outside the signer's chain in `dbx` does not revoke an image that `db` authorizes. |

#### 4.1 Unrelated Certificate Non-Authorization

- **Purpose:** A certificate embedded in a signature but not part of its signing chain
  does not authorize the image, whether it is listed in `db` as a certificate or as a TBS hash.
- **dbx:** Empty (`DB_STATE_EMPTY`).
- **Expected result:** `EFI_ACCESS_DENIED` for every run.
- **Rationale:** Trust in `db` must anchor only a certificate that actually validates a
  signature's chain. A certificate outside that chain is carried inside the signature but signs
  nothing, so trusting it — by certificate or by TBS hash — must not approve the image; with no
  other authority in `db`, the image is denied.

`4.1.1` places the unrelated certificate itself in `db` (a single case):

| ID | `db` authorizing entry | Expected |
| -- | ---------------------- | -------- |
| `4.1.1` | Certificate outside the signer's chain | `EFI_ACCESS_DENIED` |

`4.1.2` places the unrelated certificate's TBS hash in `db`, one run per hash algorithm:

| Run | Unrelated-cert TBS hash algorithm |
| --- | --------------------------------- |
| `4.1.2a` | SHA-256 |
| `4.1.2b` | SHA-384 |
| `4.1.2c` | SHA-512 |

#### 4.2 Unrelated Certificate Non-Revocation

- **Purpose:** A certificate embedded in a signature but not part of its signing chain
  does not *revoke* an image that `db` authorizes, whether it is listed in `dbx` as a
  certificate or as a TBS hash.
- **db:** A certificate that actually authorizes the image (`DB_STATE_SIGNER2_CERT`).
- **Expected result:** `EFI_SUCCESS` for every run.
- **Rationale:** A `dbx` revocation must target a certificate that actually anchors a signature's
  chain. A certificate outside that chain merely rides along inside the signature and signs
  nothing, so its presence in `dbx` — by certificate or by TBS hash — must not revoke an image
  that a real authority in `db` approves. This is the `dbx` counterpart to 4.1.

`4.2.1` places the unrelated certificate itself in `dbx` (a single case):

| ID | `db` (authorizes) | `dbx` (must not revoke) | Expected |
| -- | ----------------- | ----------------------- | -------- |
| `4.2.1` | Authorizing certificate | Certificate outside the signer's chain | `EFI_SUCCESS` |

`4.2.2` places the unrelated certificate's TBS hash in `dbx`, one run per hash algorithm:

| Run | Unrelated-cert TBS hash algorithm (in `dbx`) |
| --- | -------------------------------------------- |
| `4.2.2a` | SHA-256 |
| `4.2.2b` | SHA-384 |
| `4.2.2c` | SHA-512 |

### 5. Database Parsing Errors

These tests exercise a `db` / `dbx` that carries a deliberately malformed `EFI_SIGNATURE_LIST`.
The model under test is that database parsing *aborts at the malformed entry*: entries that
appear before it are parsed and honored, while the malformed entry and anything after it are
unreachable. Two placement flags control where the malformed list sits relative to an
otherwise-authorizing entry — `DB_STATE_CORRUPT_SIGNATURE_LIST_FIRST` (before) and
`DB_STATE_CORRUPT_SIGNATURE_LIST_LAST` (after).

This yields three behaviors, tested for every image type:

- **Approver before the corruption (`db`)** — the authorizing entry is parsed before the
  malformed list is reached, so the image is *approved*.
- **Approver after the corruption (`db`)** — parsing aborts at the malformed list before the
  authorizing entry is reached, so the image is *denied*.
- **Corrupt `dbx`** — a revocation cannot be ruled out when the `dbx` cannot be fully parsed,
  so the image is *denied* even though `db` would approve it.

| ID | Group | Description |
| -- | ----- | ----------- |
| [`5.1`](#51-unsigned-database-corruption) | Unsigned Database Corruption | Malformed `db` / `dbx` handling for an unsigned image. |
| [`5.2`](#52-signed-database-corruption) | Signed Database Corruption | Malformed `db` / `dbx` handling for a signed image. |
| [`5.3`](#53-multi-signed-database-corruption) | Multi-Signed Database Corruption | Malformed `db` / `dbx` handling for a multi-signed image. |

#### 5.1 Unsigned Database Corruption

- **Image type:** Unsigned (`IMAGE_TYPE_UNSIGNED`); authorized in `db` by its image digest.

| ID | `db` | `dbx` | Expected |
| -- | ---- | ----- | -------- |
| `5.1.1` | Authorizing digest, then malformed list (`_LAST`) | Empty | `EFI_SUCCESS` |
| `5.1.2` | Malformed list (`_FIRST`), then authorizing digest | Empty | `EFI_ACCESS_DENIED` |
| `5.1.3` | Authorizing digest | Malformed list | `EFI_ACCESS_DENIED` |

- `5.1.1` — the authorizing digest is parsed before the malformed list, so the image is
  approved before parsing would abort.
- `5.1.2` — the malformed list comes first, so parsing aborts before the digest is reached and
  the image is denied.
- `5.1.3` — the `db` would authorize the image, but the `dbx` is malformed; a revocation cannot
  be ruled out, so the image is denied.

#### 5.2 Signed Database Corruption

- **Image type:** Signed (`IMAGE_TYPE_SIGNED`). A signed image can be authorized by an image
  digest, a signing certificate, or a certificate TBS hash, so the "approver before corruption"
  case exercises each authority type.

`5.2.1` places an authorizing entry before the malformed list (`_LAST`) with an empty `dbx`;
each run is approved because the authority is parsed first:

| Run | `db` authorizing entry (before the malformed list) | Expected |
| --- | -------------------------------------------------- | -------- |
| `5.2.1a` | Image digest | `EFI_SUCCESS` |
| `5.2.1b` | Signer 1 leaf certificate | `EFI_SUCCESS` |
| `5.2.1c` | Signer 1 leaf TBS hash | `EFI_SUCCESS` |

| ID | `db` | `dbx` | Expected |
| -- | ---- | ----- | -------- |
| `5.2.2` | Malformed list (`_FIRST`), then authorizing digest | Empty | `EFI_ACCESS_DENIED` |
| `5.2.3` | Authorizing digest | Malformed list | `EFI_ACCESS_DENIED` |

- `5.2.2` — the malformed list comes first, so parsing aborts before any authority is reached
  and the image is denied.
- `5.2.3` — `db` would authorize the image, but the malformed `dbx` means a revocation cannot
  be ruled out, so the image is denied.

#### 5.3 Multi-Signed Database Corruption

- **Image type:** Multi-Signed (`IMAGE_TYPE_MULTI_SIGNED`). Either of the two signatures alone
  would authorize the image, so each signature's authority is tested independently — signer 1's
  leaf certificate (first signature) and signer 2's certificate (second signature) — placed
  before or after the malformed list.

`5.3.1` places a single signature's authority *before* the malformed list (`_LAST`); `5.3.2`
places it *after* the malformed list (`_FIRST`). The `dbx` is empty in both:

| ID | `db` authorizing entry | Placement | Expected |
| -- | ---------------------- | --------- | -------- |
| `5.3.1a` | Signer 1 leaf certificate | Before corruption | `EFI_SUCCESS` |
| `5.3.1b` | Signer 2 certificate | Before corruption | `EFI_SUCCESS` |
| `5.3.2a` | Signer 1 leaf certificate | After corruption | `EFI_ACCESS_DENIED` |
| `5.3.2b` | Signer 2 certificate | After corruption | `EFI_ACCESS_DENIED` |

`5.3.3` keeps both signatures' approvals in a valid `db` but corrupts the `dbx`:

| ID | `db` | `dbx` | Expected |
| -- | ---- | ----- | -------- |
| `5.3.3` | Both-signature approvals | Malformed list | `EFI_ACCESS_DENIED` |

- `5.3.1a` / `5.3.1b` — either signature's authority, parsed before the malformed list,
  approves the image on its own.
- `5.3.2a` / `5.3.2b` — the malformed list comes first, so parsing aborts before either
  authority is reached and the image is denied.
- `5.3.3` — `db` approves both signatures, but the malformed `dbx` means a revocation cannot be
  ruled out, so the image is denied.

### 6. Image Certificate Parsing Errors

These tests exercise an image whose embedded `WIN_CERTIFICATE` (Authenticode signature) table
carries a malformed entry. The model under test is that certificate-table parsing *aborts at
the malformed entry*: signatures before it can still be parsed and honored, while the malformed
entry and any signature after it are unreachable. Independently, an image digest in `db`
authorizes the image without parsing any signature at all.

The malformed image variants damage a private copy of the image at load time by setting a
`WIN_CERTIFICATE.dwLength` out of range; the Security data directory that locates the table is
left intact, so the table is still found but cannot be walked past the damaged entry. The
variant selects which certificate is damaged: `IMAGE_TYPE_SIGNED_CORRUPT_CERT` (the signed
image's only certificate), `IMAGE_TYPE_MULTI_SIGNED_CORRUPT_FIRST_CERT` (the multi-signed
image's first / signer 1 certificate), or `IMAGE_TYPE_MULTI_SIGNED_CORRUPT_SECOND_CERT` (its
second / signer 2 certificate).

| ID | Group | Description |
| -- | ----- | ----------- |
| [`6.1`](#61-signed-image-certificate-corruption) | Signed Image Certificate Corruption | Malformed `WIN_CERTIFICATE` in a signed image; an image digest in `db` still authorizes. |
| [`6.2`](#62-multi-signed-first-certificate-corruption) | Multi-Signed First-Certificate Corruption | A malformed first signature aborts parsing before the intact second is reached. |
| [`6.3`](#63-multi-signed-second-certificate-corruption) | Multi-Signed Second-Certificate Corruption | The intact first signature is parsed before the malformed second. |

#### 6.1 Signed Image Certificate Corruption

- **Image type:** Signed, corrupt certificate (`IMAGE_TYPE_SIGNED_CORRUPT_CERT`); empty `dbx`.
- **Behavior:** The image's only signature cannot be parsed. An image digest in `db` still
  authorizes it (the hash path never parses the signature), but a signature-path authority — a
  certificate or a certificate TBS hash — cannot be matched, so the image is denied.

| ID | `db` authorizing entry | Expected |
| -- | ---------------------- | -------- |
| `6.1.1` | Image digest (SHA-256) | `EFI_SUCCESS` |
| `6.1.2a` | Signer 1 leaf certificate | `EFI_ACCESS_DENIED` |
| `6.1.2b` | Signer 1 leaf TBS hash (SHA-256) | `EFI_ACCESS_DENIED` |

- `6.1.1` — the image digest in `db` authorizes the image without ever parsing the malformed
  signature, so it is approved.
- `6.1.2a` / `6.1.2b` — the only authority in `db` requires parsing the signature to match the
  signing certificate (or its TBS hash); because the certificate cannot be parsed, no match is
  possible and the image is denied.

#### 6.2 Multi-Signed First-Certificate Corruption

- **Image type:** Multi-signed, first certificate corrupt
  (`IMAGE_TYPE_MULTI_SIGNED_CORRUPT_FIRST_CERT`); empty `dbx`.
- **Behavior:** The first (signer 1) `WIN_CERTIFICATE` is malformed, so parsing aborts before
  the intact second (signer 2) signature can be reached.

| ID | `db` authorizing entry | Expected |
| -- | ---------------------- | -------- |
| `6.2.1` | Signer 2 certificate | `EFI_ACCESS_DENIED` |

- `6.2.1` — `db` authorizes signer 2, whose signature is intact, but the malformed first
  certificate stops parsing before signer 2's signature is reached, so the image is denied.

#### 6.3 Multi-Signed Second-Certificate Corruption

- **Image type:** Multi-signed, second certificate corrupt
  (`IMAGE_TYPE_MULTI_SIGNED_CORRUPT_SECOND_CERT`); empty `dbx`.
- **Behavior:** The first (signer 1) `WIN_CERTIFICATE` is intact and is parsed first; the second
  (signer 2) certificate is malformed. Whether the image is approved depends on which signature
  `db` authorizes.

| ID | `db` authorizing entry | Expected |
| -- | ---------------------- | -------- |
| `6.3.1` | Signer 1 leaf certificate | `EFI_SUCCESS` |
| `6.3.2` | Signer 2 certificate | `EFI_ACCESS_DENIED` |

- `6.3.1` — `db` authorizes signer 1, whose signature is intact and parsed before the malformed
  second certificate, so the image is approved.
- `6.3.2` — `db` authorizes only signer 2, but its certificate is the malformed one; parsing
  aborts there without an approval, so the image is denied.

### 7. Tampered Image Rejection

**Image type:** `IMAGE_TYPE_SIGNED_TAMPERED`.

A tampered image is a signed image whose body was modified after signing, so its Authenticode
hash no longer matches the embedded signature or the digest enrolled from the original image. The
`WIN_CERTIFICATE` is left intact, so the image still parses and reaches signature validation.

| ID | Group | Description |
| -- | ----- | ----------- |
| [`7.1`](#71-tampered-signed-image) | Tampered Signed Image | A tampered image is denied regardless of how `db` would authorize the original. |

#### 7.1 Tampered Signed Image

- **Purpose:** A tampered image is denied no matter how `db` would try to authorize the original
  image — by a signer certificate, a signer certificate TBS hash, the original image digest, or
  nothing at all.
- **Image type:** Tampered signed (`IMAGE_TYPE_SIGNED_TAMPERED`).
- **db:** One authority per run (see the table); `dbx` is empty.
- **Expected result:** `EFI_ACCESS_DENIED` for every run.
- **Rationale:** Tampering changes the image's Authenticode hash, so the signature no longer
  verifies and the recomputed image digest no longer matches the enrolled one. No `db` authority
  should approve it.

| ID | `db` authority | Expected | Notes |
| -- | -------------- | -------- | ----- |
| `7.1.1` | Signer 1 leaf certificate | `EFI_ACCESS_DENIED` | `AuthenticodeVerify` fails against the changed hash |
| `7.1.2` | Signer 1 leaf certificate TBS hash | `EFI_ACCESS_DENIED` | **fails today** — the TBS-hash authorization path matches an embedded certificate without verifying the signature, so it wrongly approves the tampered image |
| `7.1.3` | Original image SHA-256 digest | `EFI_ACCESS_DENIED` | the tampered image hashes to a different digest, so `db` does not match |
| `7.1.4` | Empty | `EFI_ACCESS_DENIED` | nothing authorizes the image |

### 8. SHA-1 Rejection

**Image type:** `IMAGE_TYPE_UNSIGNED` and `IMAGE_TYPE_SIGNED`.

SHA-1 is not a supported image-hash algorithm (the handler recognizes only SHA-256/384/512). A
`db` or `dbx` entry that carries a SHA-1 image digest (`EFI_CERT_SHA1_GUID`) is therefore inert:
it can neither authorize nor revoke an image, and it does not disrupt processing of the supported
entries around it.

| ID | Group | Description |
| -- | ----- | ----------- |
| [`8.1`](#81-sha-1-digest-cannot-approve) | SHA-1 Digest Cannot Approve | A SHA-1 image digest in `db` does not authorize an image. |
| [`8.2`](#82-sha-1-digest-cannot-revoke) | SHA-1 Digest Cannot Revoke | A SHA-1 image digest in `dbx` does not revoke an otherwise-authorized image. |
| [`8.3`](#83-sha-1-digest-ignored-alongside-a-supported-digest) | SHA-1 Digest Ignored Alongside a Supported Digest | A SHA-1 digest emitted before a supported digest is skipped, and the supported digest is still honored. |

#### 8.1 SHA-1 Digest Cannot Approve

- **Purpose:** A SHA-1 image digest in `db` cannot authorize an image, because SHA-1 is not a
  supported image-hash algorithm.
- **db:** The image's SHA-1 Authenticode digest (`DB_STATE_IMAGE_DIGEST_SHA1`).
- **dbx:** Empty (`DB_STATE_EMPTY`).
- **Expected result:** `EFI_ACCESS_DENIED` for every run.
- **Rationale:** The handler never evaluates SHA-1 digests, so the `db` entry matches nothing and
  the image has no authority; contrast 1.3.1, where a supported-algorithm digest approves. The
  signed run additionally confirms the signature is not consulted as an authority here (it is
  absent from `db`).

| Run | Image type |
| --- | ---------- |
| `8.1.1` | Unsigned (`IMAGE_TYPE_UNSIGNED`) |
| `8.1.2` | Signed (`IMAGE_TYPE_SIGNED`) |

#### 8.2 SHA-1 Digest Cannot Revoke

- **Purpose:** A SHA-1 image digest in `dbx` cannot revoke an image that `db` authorizes through a
  supported authority.
- **dbx:** The image's SHA-1 Authenticode digest (`DB_STATE_IMAGE_DIGEST_SHA1`).
- **Expected result:** `EFI_SUCCESS` for every run.
- **Rationale:** The handler never evaluates SHA-1 digests, so the `dbx` entry matches nothing and
  cannot revoke; contrast 1.2.1, where a supported-algorithm `dbx` digest denies.

| Run | Image type | `db` authority |
| --- | ---------- | -------------- |
| `8.2.1` | Unsigned (`IMAGE_TYPE_UNSIGNED`) | Image SHA-256 digest (`DB_STATE_IMAGE_DIGEST_SHA256`) |
| `8.2.2` | Signed (`IMAGE_TYPE_SIGNED`) | Signer 1 leaf certificate (`DB_STATE_SIGNER1_LEAF_CERT`) |

#### 8.3 SHA-1 Digest Ignored Alongside a Supported Digest

- **Purpose:** A SHA-1 digest emitted *before* a supported digest in the same database is skipped
  without aborting the walk, and the supported digest that follows is still honored. This
  distinguishes an unsupported (well-formed) entry, which is skipped, from a malformed entry, which
  aborts the walk (see group 5).
- **Image type:** Unsigned (`IMAGE_TYPE_UNSIGNED`).
- **Expected result:** per the table below.
- **Rationale:** The database builder emits the SHA-1 image digest ahead of the supported digests,
  so each run places a SHA-1 entry in front of a SHA-256 entry. Because the handler skips the
  unrecognized SHA-1 entry and continues, the SHA-256 entry behind it is still evaluated —
  approving in `db` and revoking in `dbx`.

| ID | `db` | `dbx` | Expected | Proves |
| -- | ---- | ----- | -------- | ------ |
| `8.3.1` | SHA-1 + SHA-256 image digests | Empty | `EFI_SUCCESS` | leading SHA-1 in `db` is skipped; the SHA-256 digest still approves |
| `8.3.2` | SHA-256 image digest | SHA-1 + SHA-256 image digests | `EFI_ACCESS_DENIED` | leading SHA-1 in `dbx` is skipped; the SHA-256 digest still revokes |

### 9. V2 Signature-Type GUIDs

**Image type:** `IMAGE_TYPE_UNSIGNED` and `IMAGE_TYPE_SIGNED`.

The V2 signature types (`EFI_CERT_V2_*`) carry the same payloads as their V1 counterparts but use
the `EFI_SIGNATURE_V2_DATA` layout, which drops the 16-byte `SignatureOwner` from every entry.
These scenarios are declared with `TEST_SCENARIO_V2`, which makes the database builder emit each
entry with its V2 GUID and no owner prefix; the image, flags, and expected results otherwise mirror
their V1 baselines. The group is intentionally focused - one case for each of the three ways a V2
GUID is used (image digest, full X.509 certificate, X.509 TBS-cert hash), in both the authorizing
(`db`) and revoking (`dbx`) roles - rather than an exhaustive matrix. The image-digest cases run all
three hash sizes (SHA-256/384/512), because that path maps each V2 image-hash GUID to its base GUID
before hashing.

| ID | Group | Description |
| -- | ----- | ----------- |
| [`9.1`](#91-v2-database-approval) | V2 Database Approval | A V2 `db` entry authorizes the image (empty `dbx`). |
| [`9.2`](#92-v2-database-revocation) | V2 Database Revocation | A V2 `dbx` entry revokes an image that a V2 `db` entry would otherwise authorize. |

#### 9.1 V2 Database Approval

- **Purpose:** Each form of V2 `db` entry authorizes the image, exactly as its V1 counterpart does.
- **dbx:** Empty (`DB_STATE_EMPTY`).
- **Expected result:** `EFI_SUCCESS` for every run.
- **Rationale:** Proves the handler parses the ownerless V2 layout and honors the V2 GUIDs for the
  image-digest, full-certificate, and TBS-cert-hash authorities; mirrors 1.3.1a-c / 2.3.2a / 2.3.3a.

| Run | Image type | `db` authority (V2) |
| --- | ---------- | ------------------- |
| `9.1.1a` | Unsigned (`IMAGE_TYPE_UNSIGNED`) | Image SHA-256 digest (`DB_STATE_IMAGE_DIGEST_SHA256`) |
| `9.1.1b` | Unsigned (`IMAGE_TYPE_UNSIGNED`) | Image SHA-384 digest (`DB_STATE_IMAGE_DIGEST_SHA384`) |
| `9.1.1c` | Unsigned (`IMAGE_TYPE_UNSIGNED`) | Image SHA-512 digest (`DB_STATE_IMAGE_DIGEST_SHA512`) |
| `9.1.2` | Signed (`IMAGE_TYPE_SIGNED`) | Signer 1 leaf certificate (`DB_STATE_SIGNER1_LEAF_CERT`) |
| `9.1.3` | Signed (`IMAGE_TYPE_SIGNED`) | Signer 1 leaf TBS hash, SHA-256 (`DB_STATE_SIGNER1_LEAF_TBS_HASH_SHA256`) |

#### 9.2 V2 Database Revocation

- **Purpose:** Each form of V2 `dbx` entry revokes an image that a V2 `db` entry would otherwise
  authorize.
- **Image type:** Signed (`IMAGE_TYPE_SIGNED`).
- **db:** Signer 1 leaf certificate (`DB_STATE_SIGNER1_LEAF_CERT`), which alone would authorize.
- **Expected result:** `EFI_ACCESS_DENIED` for every run.
- **Rationale:** Proves the handler honors the V2 GUIDs for revocation in each form - image-digest
  revocation, exact-certificate chain revocation, and TBS-cert-hash chain revocation; mirrors
  2.2.2a / 2.4.16a / 2.4.16b.

| Run | `dbx` revoking entry (V2) |
| --- | ------------------------- |
| `9.2.1a` | Image SHA-256 digest (`DB_STATE_IMAGE_DIGEST_SHA256`) |
| `9.2.1b` | Image SHA-384 digest (`DB_STATE_IMAGE_DIGEST_SHA384`) |
| `9.2.1c` | Image SHA-512 digest (`DB_STATE_IMAGE_DIGEST_SHA512`) |
| `9.2.2` | Signer 1 leaf certificate (`DB_STATE_SIGNER1_LEAF_CERT`) |
| `9.2.3` | Signer 1 leaf TBS hash, SHA-256 (`DB_STATE_SIGNER1_LEAF_TBS_HASH_SHA256`) |

### Adding or Documenting a Test

To add or document a test, keep the code and this catalog in sync:

1. **Pick the ID.** Choose the group (`1`/`2`/`3`/`4`/`5`/`6`/`7`/`8`/`9`), the behavior, and the variant — for
   example `2.4.1`. When a test runs several cases, append a run letter to each case
   (`2.4.1a`, `2.4.1b`, ...); a single-case test uses no letter.
2. **Define the scenario in `Scenarios.c`.** Use `TEST_SCENARIO` and pass the full ID as the
   first argument:

   ```c
   TEST_SCENARIO (
     "2.4.1",
     IMAGE_TYPE_SIGNED,
     DB_STATE_SIGNER1_LEAF_CERT,
     DB_STATE_SIGNER1_ROOT_CERT,
     EFI_ACCESS_DENIED
     ),
   ```

3. **Add it to the test table.** In the [All Tests](#all-tests) table in the
   [Test Catalog](#test-catalog), add a row for the test's `<group>.<behavior>.<variant>` ID
   (linked to the section that documents it) with a short behavior description. Individual run
   letters are documented in the test's section, not the catalog.
4. **Document the behavior.** Add or update the section that the test ID links to, describing
   what the test proves and the inputs it uses. Use this template:

   ```markdown
   #### <id> <short title>

   - **Purpose:** <what behavior this proves>
   - **Image type:** <Unsigned | Signed | Multi-Signed> (`IMAGE_TYPE_*`)
   - **db:** <DB_STATE_* flags and meaning>
   - **dbx:** <DB_STATE_* flags and meaning>
   - **Expected result:** `<EFI_STATUS>`
   - **Rationale:** <why that result is correct>
   ```

5. **Keep IDs stable.** Once published, do not renumber a test; failures and reports
   reference its ID.

## Notes

- Scenario definitions live in `Scenarios.c`.
- Test data blobs (images, digests, certificates, and each certificate's precomputed
  TBSCertificate hashes) are generated by `GenTestData.py`; see that script's header comment
  or run `python3 GenTestData.py --help` to regenerate them. Each generated `TestData.c` also
  points back to the script.
- Certificate TBSCertificate hashes are precomputed into `TestData.c` rather than hashed at
  runtime, so the application links no cryptography library and is platform/binary independent.
- Scenario data structures, `DB_STATE_*` definitions, and the `TEST_SCENARIO` / `TEST_SCENARIO_V2`
  macros are in `ImageValidationTestApp.h`. `TEST_SCENARIO_V2` emits the same entries using the V2
  signature-type GUIDs and the ownerless `EFI_SIGNATURE_V2_DATA` layout.
- Runtime test behavior, database synthesis, and test-ID composition are in
  `ImageValidationTestApp.c`.

## All Tests

| Test ID | Expected | Description |
| ------- | -------- | ----------- |
| [`1.1.1`](#11-empty-database-denial) | `EFI_ACCESS_DENIED` | Unsigned image with empty `db` is always denied |
| [`1.2.1`](#12-revoked-digest-denial) | `EFI_ACCESS_DENIED` | Unsigned image denied by a `dbx` digest even when an authorizing `db` digest approves it |
| [`1.3.1`](#13-digest-approval) | `EFI_SUCCESS` | Unsigned image approved by a `db` digest when the `dbx` is empty |
| [`2.1.1`](#21-empty-database-denial) | `EFI_ACCESS_DENIED` | Signed image with empty `db` is always denied |
| [`2.2.1`](#22-revoked-digest-denial) | `EFI_ACCESS_DENIED` | Signed image denied by a `dbx` image digest even when an authorizing `db` digest approves it |
| [`2.2.2`](#22-revoked-digest-denial) | `EFI_ACCESS_DENIED` | Signed image denied by a `dbx` image digest even when an authorizing `db` certificate approves it |
| [`2.2.3`](#22-revoked-digest-denial) | `EFI_ACCESS_DENIED` | Signed image denied by a `dbx` image digest even when an authorizing `db` TBS hash approves it |
| [`2.3.1`](#23-database-approval) | `EFI_SUCCESS` | Signed image approved by its image digest in `db` (empty `dbx`) |
| [`2.3.2`](#23-database-approval) | `EFI_SUCCESS` | Signed image approved by a signer 1 chain certificate in `db` (empty `dbx`) |
| [`2.3.3`](#23-database-approval) | `EFI_SUCCESS` | Signed image approved by a signer 1 certificate TBS hash in `db` (empty `dbx`) |
| [`2.4.1`](#24-chain-revocation-relativeness) | `EFI_ACCESS_DENIED` | Signed image with a root anchor in `db` revoked by a root revoker in `dbx` |
| [`2.4.2`](#24-chain-revocation-relativeness) | `EFI_ACCESS_DENIED` | Signed image with a root anchor in `db` revoked by an intermediate 1 revoker in `dbx` |
| [`2.4.3`](#24-chain-revocation-relativeness) | `EFI_ACCESS_DENIED` | Signed image with a root anchor in `db` revoked by an intermediate 2 revoker in `dbx` |
| [`2.4.4`](#24-chain-revocation-relativeness) | `EFI_ACCESS_DENIED` | Signed image with a root anchor in `db` revoked by a leaf revoker in `dbx` |
| [`2.4.5`](#24-chain-revocation-relativeness) | `EFI_SUCCESS` | Signed image with an intermediate 1 anchor in `db` not revoked by a root revoker in `dbx` (revoker above anchor) |
| [`2.4.6`](#24-chain-revocation-relativeness) | `EFI_ACCESS_DENIED` | Signed image with an intermediate 1 anchor in `db` revoked by an intermediate 1 revoker in `dbx` |
| [`2.4.7`](#24-chain-revocation-relativeness) | `EFI_ACCESS_DENIED` | Signed image with an intermediate 1 anchor in `db` revoked by an intermediate 2 revoker in `dbx` |
| [`2.4.8`](#24-chain-revocation-relativeness) | `EFI_ACCESS_DENIED` | Signed image with an intermediate 1 anchor in `db` revoked by a leaf revoker in `dbx` |
| [`2.4.9`](#24-chain-revocation-relativeness) | `EFI_SUCCESS` | Signed image with an intermediate 2 anchor in `db` not revoked by a root revoker in `dbx` (revoker above anchor) |
| [`2.4.10`](#24-chain-revocation-relativeness) | `EFI_SUCCESS` | Signed image with an intermediate 2 anchor in `db` not revoked by an intermediate 1 revoker in `dbx` (revoker above anchor) |
| [`2.4.11`](#24-chain-revocation-relativeness) | `EFI_ACCESS_DENIED` | Signed image with an intermediate 2 anchor in `db` revoked by an intermediate 2 revoker in `dbx` |
| [`2.4.12`](#24-chain-revocation-relativeness) | `EFI_ACCESS_DENIED` | Signed image with an intermediate 2 anchor in `db` revoked by a leaf revoker in `dbx` |
| [`2.4.13`](#24-chain-revocation-relativeness) | `EFI_SUCCESS` | Signed image with a leaf anchor in `db` not revoked by a root revoker in `dbx` (revoker above anchor) |
| [`2.4.14`](#24-chain-revocation-relativeness) | `EFI_SUCCESS` | Signed image with a leaf anchor in `db` not revoked by an intermediate 1 revoker in `dbx` (revoker above anchor) |
| [`2.4.15`](#24-chain-revocation-relativeness) | `EFI_SUCCESS` | Signed image with a leaf anchor in `db` not revoked by an intermediate 2 revoker in `dbx` (revoker above anchor) |
| [`2.4.16`](#24-chain-revocation-relativeness) | `EFI_ACCESS_DENIED` | Signed image with a leaf anchor in `db` revoked by a leaf revoker in `dbx` |
| [`2.4.17`](#24-chain-revocation-relativeness) | `EFI_SUCCESS` | Signed image with two `db` chain anchors (intermediate 1 and 2) approved via intermediate 2 when only intermediate 1 is revoked in `dbx` |
| [`2.5.1`](#25-image-digest-overrides-signature-revocation) | `EFI_SUCCESS` | Signed image whose signer is revoked in `dbx` is still approved by its image digest in `db` |
| [`3.1.1`](#31-empty-database-denial) | `EFI_ACCESS_DENIED` | Multi-signed image with empty `db` is always denied |
| [`3.2.1`](#32-revoked-digest-denial) | `EFI_ACCESS_DENIED` | Multi-signed image denied by a `dbx` image digest even when an authorizing `db` digest approves it |
| [`3.2.2`](#32-revoked-digest-denial) | `EFI_ACCESS_DENIED` | Multi-signed image denied by a `dbx` image digest even when an authorizing `db` certificate approves it |
| [`3.2.3`](#32-revoked-digest-denial) | `EFI_ACCESS_DENIED` | Multi-signed image denied by a `dbx` image digest even when an authorizing `db` TBS hash approves it |
| [`3.3.1`](#33-two-signature-evaluation) | `EFI_SUCCESS` | Multi-signed image authorized by its first signature; a second signature present in the image does not interfere |
| [`3.3.2`](#33-two-signature-evaluation) | `EFI_SUCCESS` | Multi-signed image authorized by its second signature when the first is not found in `db` |
| [`3.3.3`](#33-two-signature-evaluation) | `EFI_SUCCESS` | Multi-signed image approved when its first signature is revoked but its second is authorized |
| [`3.3.4`](#33-two-signature-evaluation) | `EFI_SUCCESS` | Multi-signed image approved by its first signature even though its second would be revoked (never consulted) |
| [`3.3.5`](#33-two-signature-evaluation) | `EFI_ACCESS_DENIED` | Multi-signed image denied when its first signature is revoked and its second is not authorized |
| [`3.3.6`](#33-two-signature-evaluation) | `EFI_ACCESS_DENIED` | Multi-signed image denied when its first signature is not authorized and its second is revoked |
| [`3.3.7`](#33-two-signature-evaluation) | `EFI_ACCESS_DENIED` | Multi-signed image denied when both signatures are revoked |
| [`4.1.1`](#41-unrelated-certificate-non-authorization) | `EFI_ACCESS_DENIED` | Image denied when only a certificate outside the signer's chain is in `db` |
| [`4.1.2`](#41-unrelated-certificate-non-authorization) | `EFI_ACCESS_DENIED` | Image denied when only the TBS hash of a certificate outside the signer's chain is in `db` |
| [`4.2.1`](#42-unrelated-certificate-non-revocation) | `EFI_SUCCESS` | Image (authorized by `db`) not revoked when a certificate outside the signer's chain is in `dbx` |
| [`4.2.2`](#42-unrelated-certificate-non-revocation) | `EFI_SUCCESS` | Image (authorized by `db`) not revoked when the TBS hash of a certificate outside the signer's chain is in `dbx` |
| [`5.1.1`](#51-unsigned-database-corruption) | `EFI_SUCCESS` | Unsigned image approved when an authorizing `db` digest precedes a malformed signature list |
| [`5.1.2`](#51-unsigned-database-corruption) | `EFI_ACCESS_DENIED` | Unsigned image denied when a malformed `db` signature list precedes the authorizing digest |
| [`5.1.3`](#51-unsigned-database-corruption) | `EFI_ACCESS_DENIED` | Unsigned image denied when the `dbx` is corrupt, so revocation cannot be ruled out |
| [`5.2.1`](#52-signed-database-corruption) | `EFI_SUCCESS` | Signed image approved when an authorizing `db` entry precedes a malformed signature list |
| [`5.2.2`](#52-signed-database-corruption) | `EFI_ACCESS_DENIED` | Signed image denied when a malformed `db` signature list precedes the authorizing entry |
| [`5.2.3`](#52-signed-database-corruption) | `EFI_ACCESS_DENIED` | Signed image denied when the `dbx` is corrupt, so revocation cannot be ruled out |
| [`5.3.1`](#53-multi-signed-database-corruption) | `EFI_SUCCESS` | Multi-signed image approved when either signature's authority precedes a malformed `db` signature list |
| [`5.3.2`](#53-multi-signed-database-corruption) | `EFI_ACCESS_DENIED` | Multi-signed image denied when a malformed `db` signature list precedes either signature's authority |
| [`5.3.3`](#53-multi-signed-database-corruption) | `EFI_ACCESS_DENIED` | Multi-signed image denied when the `dbx` is corrupt, even though `db` approves both signatures |
| [`6.1.1`](#61-signed-image-certificate-corruption) | `EFI_SUCCESS` | Signed image with a malformed `WIN_CERTIFICATE` approved by an image digest in `db` |
| [`6.1.2`](#61-signed-image-certificate-corruption) | `EFI_ACCESS_DENIED` | Signed image with a malformed `WIN_CERTIFICATE` denied when only a signature-path authority is in `db` |
| [`6.2.1`](#62-multi-signed-first-certificate-corruption) | `EFI_ACCESS_DENIED` | Multi-signed image with a malformed first `WIN_CERTIFICATE` denied even though `db` authorizes the unreachable second signature |
| [`6.3.1`](#63-multi-signed-second-certificate-corruption) | `EFI_SUCCESS` | Multi-signed image with a malformed second `WIN_CERTIFICATE` approved because `db` authorizes the intact first signature |
| [`6.3.2`](#63-multi-signed-second-certificate-corruption) | `EFI_ACCESS_DENIED` | Multi-signed image with a malformed second `WIN_CERTIFICATE` denied when only the unreachable second signature's authority is in `db` |
| [`7.1.1`](#71-tampered-signed-image) | `EFI_ACCESS_DENIED` | Tampered signed image denied when a signer certificate is in `db` |
| [`7.1.2`](#71-tampered-signed-image) | `EFI_ACCESS_DENIED` | Tampered signed image denied when a signer certificate TBS hash is in `db` |
| [`7.1.3`](#71-tampered-signed-image) | `EFI_ACCESS_DENIED` | Tampered signed image denied when the original image digest is in `db` |
| [`7.1.4`](#71-tampered-signed-image) | `EFI_ACCESS_DENIED` | Tampered signed image denied when `db` is empty |
| [`8.1.1`](#81-sha-1-digest-cannot-approve) | `EFI_ACCESS_DENIED` | Unsigned image not authorized by a SHA-1 image digest in `db` (SHA-1 unsupported) |
| [`8.1.2`](#81-sha-1-digest-cannot-approve) | `EFI_ACCESS_DENIED` | Signed image not authorized by a SHA-1 image digest in `db` (SHA-1 unsupported) |
| [`8.2.1`](#82-sha-1-digest-cannot-revoke) | `EFI_SUCCESS` | Unsigned image (authorized by a SHA-256 `db` digest) not revoked by a SHA-1 image digest in `dbx` (SHA-1 unsupported) |
| [`8.2.2`](#82-sha-1-digest-cannot-revoke) | `EFI_SUCCESS` | Signed image (authorized by a `db` certificate) not revoked by a SHA-1 image digest in `dbx` (SHA-1 unsupported) |
| [`8.3.1`](#83-sha-1-digest-ignored-alongside-a-supported-digest) | `EFI_SUCCESS` | Unsigned image approved by a SHA-256 `db` digest that follows a skipped SHA-1 `db` digest |
| [`8.3.2`](#83-sha-1-digest-ignored-alongside-a-supported-digest) | `EFI_ACCESS_DENIED` | Unsigned image revoked by a SHA-256 `dbx` digest that follows a skipped SHA-1 `dbx` digest |
| [`9.1.1`](#91-v2-database-approval) | `EFI_SUCCESS` | Unsigned image approved by a V2 image digest in `db` |
| [`9.1.2`](#91-v2-database-approval) | `EFI_SUCCESS` | Signed image approved by a V2 signer 1 leaf certificate in `db` |
| [`9.1.3`](#91-v2-database-approval) | `EFI_SUCCESS` | Signed image approved by a V2 signer 1 leaf TBS hash (SHA-256) in `db` |
| [`9.2.1`](#92-v2-database-revocation) | `EFI_ACCESS_DENIED` | Signed image revoked by a V2 image digest in `dbx` over an authorizing V2 `db` certificate |
| [`9.2.2`](#92-v2-database-revocation) | `EFI_ACCESS_DENIED` | Signed image revoked by a V2 signer 1 leaf certificate in `dbx` over an authorizing V2 `db` certificate |
| [`9.2.3`](#92-v2-database-revocation) | `EFI_ACCESS_DENIED` | Signed image revoked by a V2 signer 1 leaf TBS hash (SHA-256) in `dbx` over an authorizing V2 `db` certificate |
