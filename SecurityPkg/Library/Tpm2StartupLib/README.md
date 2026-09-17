# TPM 2.0 Startup Library

`Tpm2StartupLib` separates TPM2 configuration, device access, startup, and PCR-bank
synchronization so platforms can perform each operation in the appropriate boot
phase.

The implementation is in [Tpm2StartupLib.c](Tpm2StartupLib.c), with the public API
in [Tpm2StartupLib.h](../../Include/Library/Tpm2StartupLib.h).

## Configuration, support, and active banks

These values have different responsibilities and must not be treated as interchangeable:

| Value | Meaning |
| --- | --- |
| `PcdTpmInstanceGuid` | Selects the TPM implementation, or indicates no TPM / TPM 1.2. |
| `PcdTcg2HashLibSupportMask` | Platform declaration of the hash algorithms firmware can support. |
| `PcdTpm2HashMask` | Platform intent: the preferred PCR-bank configuration. |
| `PcdTcg2HashAlgorithmBitmap` | Algorithms registered by the consuming module's hash libraries. |
| TpmHashAlgorithmBitmap | Algorithms the installed TPM reports as supported for PCR banks. |
| TpmActivePcrBanks | PCR banks currently allocated in the TPM. |

Algorithm masks use the following bits:

| Bit | Algorithm |
| --- | --- |
| 0 | SHA-1 |
| 1 | SHA-256 |
| 2 | SHA-384 |
| 3 | SHA-512 |
| 4 | SM3-256 |

The platform support and intent PCDs are configured as **FixedAtBuild**.
This makes them accessible in SEC without a dynamic PCD service. Declaring
multiple allowed types in [SecurityPkg.dec](../../SecurityPkg.dec) does not
itself select FixedAtBuild; the platform DSC must select the intended type.

`PcdTcg2HashAlgorithmBitmap` is dynamic because hash-library constructors
reset and populate it. `Tcg2Pei` and `Tcg2Dxe` validate that it equals the nonzero
platform support mask after registration. This validation belongs to the callers,
not to the startup library. Measurement code selects algorithms using the TPM's
active banks, not platform intent.

## Public API

### `Tpm2StartupIsTpmSupported()`

Returns a `BOOLEAN` describing configuration.

- `FALSE`: `PcdTpmInstanceGuid` selects no TPM or TPM 1.2.
- `TRUE`: another TPM instance GUID is selected, including platform-specific TPM2
  implementations such as TPM service over FF-A.

This is **not a hardware presence test**. A selected TPM may still be absent or
unreachable. Check support before entering TPM2 initialization or measurement
paths. An unsupported configuration is a normal skip, not a device failure.

### `Tpm2StartupRequest()`

Call only after `Tpm2StartupIsTpmSupported()` returns `TRUE`. In the current phase:

1. Check for `gTpmErrorHobGuid`. If present, return `EFI_DEVICE_ERROR` without
   requesting the device. This indicates a previous TPM device error.
2. Call `Tpm2RequestUseTpm()` to obtain device access/locality.
3. Return any device-access error unchanged; otherwise return `EFI_SUCCESS`.

### `Tpm2StartupInit(IsS3Resume, S3ErrorReport)`

Requires a successful request in the current phase. The caller decides whether
startup is required; the function does not inspect `PcdTpm2InitializationPolicy`.

- Normal boot: send `TPM2_Startup(TPM_SU_CLEAR)`.
- S3 resume: send `TPM2_Startup(TPM_SU_STATE)` to restore state.
- If state startup fails, attempt `TPM_SU_CLEAR`. If that succeeds and `S3ErrorReport`
is non-NULL, set `S3ErrorReport` to `TRUE`.

The return value is the final `Tpm2Startup()` status. A successful fallback means
the TPM started, **not that the saved PCR state was restored**.

Initializes the `S3ErrorReport` to `FALSE` if provided. This implementation only
sets it to `TRUE` on successful S3 fallback. `NULL` is accepted, but a platform that
supports S3 must not silently ignore a failed state restore.

### `Tpm2StartupSecuritySync()`

Ensures that firmware can measure into every active PCR bank. Requires successful
device access and a TPM that has completed startup, either in this or an earlier
phase.

The function queries TPM-supported and active banks, then validates:

- Platform support is nonzero.
- Platform intent is a subset of platform support. A zero platform-intent mask
  defaults to platform support for this operation.
- The platform and TPM support at least one common algorithm.

If active banks are nonempty and all are supported by the platform, it leaves them
unchanged. If no banks are active, or an active bank is outside platform support,
the target is platform intent intersected with TPM support. An empty target is rejected;
otherwise the function requests allocation and calls `ResetCold()` on success.

| Result | Meaning |
| --- | --- |
| `EFI_SUCCESS` | Active banks are compatible and no reallocation is required in the normal returning path. |
| `EFI_UNSUPPORTED` | Invalid platform configuration, no common support, or no usable intended bank when repair is required. |
| `EFI_DEVICE_ERROR` | Allocation failed, or the capability query returned this status. |

Configuration checks occur even when active banks would not require a change.
Several error paths also assert in assertion-enabled builds.

### `Tpm2StartupIntentSync(HashMask)`

Applies the caller's selected platform/user preference. Requires successful device access
and TPM startup before a nonzero intent is applied.

1. A zero input means **no intent**: return `EFI_SUCCESS` without querying the TPM
   or changing the active banks.
2. Requires nonzero platform support and rejects any requested algorithm outside it.
3. Queries TPM-supported and active banks.
4. If the original intent already matches active banks, leaves them unchanged.
5. Otherwise intersects intent with TPM support and rejects an empty result.
6. Compares the filtered intent with active banks again. If equal, returns success.
7. Allocates the filtered target and cold-reset if allocation succeeds.

The second comparison prevents repeated resets when some requested algorithms are
not supported by the TPM. For example, intent SHA-256 + SHA-384 with only SHA-256
supported; active banks are satisfied after filtering.

| Result | Meaning |
| --- | --- |
| `EFI_SUCCESS` | No intent was supplied, or the effective intent is already active. |
| `EFI_INVALID_PARAMETER` | Intent contains algorithms outside the nonempty platform support mask. |
| `EFI_UNSUPPORTED` | Platform support is empty, or no requested algorithm is supported by the TPM. |
| `EFI_DEVICE_ERROR` | Allocation failed, or the capability query returned this status. |

The entire input is checked against platform support **before** filtering against
TPM support. A platform-invalid bit is not silently discarded. An empty platform
support mask also triggers an assertion in assertion-enabled builds.
