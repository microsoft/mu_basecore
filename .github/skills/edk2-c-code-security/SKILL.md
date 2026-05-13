---
name: EDK2 C Code Security Guidelines
description: Comprehensive security guidelines for EDK2 firmware C code, including secure coding practices, secure code review principles, and firmware-specific security requirements. Follow these instructions to prevent vulnerabilities and ensure robust security in EDK2 firmware development.
applyTo: '**/*.{c,h,inf,dsc,dec,fdf}'
---
MANDATORY: Completely understand and follow this instructions file. Failure to do so will cost significant time and
effort.

# EDK2 Security Instructions

This file provides security-specific guidelines for EDK2 firmware C code. For foundational
EDK2 concepts (file formats, build system, module types, coding standards, and common APIs),
refer to [edk2-project-general.instructions.md](../edk2-project-general/SKILL.md).

## Secure Coding Guidelines for EDK2 Firmware

> **Reference**: [EDK II Secure Coding Guide](https://github.com/tianocore-docs/EDK_II_Secure_Coding_Guide)

These secure coding guidelines provide specific requirements for writing secure firmware code in EDK2. Follow these mandatory rules to prevent security vulnerabilities during development.

### General Secure Coding Principles

#### Input Validation
- **#GENERAL.1**: Code in trusted region MUST check any data from untrusted regions (PE images, capsules, SMM comm buffers, MMIO BARs)
- **#GENERAL.2**: MUST avoid buffer overflows - never access beyond buffer size fields
- **#GENERAL.3**: MUST avoid integer overflow - use addition vs subtraction, division vs multiplication for validation
- **#GENERAL.4**: MUST avoid untrusted data overlap with trusted regions
- **#GENERAL.5**: MUST check untrusted data in ALL possible code paths

#### Input Processing Rules
- **#GENERAL.INPUT.1**: Check for valid input and reject everything else
- **#GENERAL.INPUT.2**: Perform sanity checks: type, length, range, format validation
- **#GENERAL.INPUT.3**: Use canonical representation - fully qualified pathnames
- **#GENERAL.INPUT.4**: Beware character encoding and escape characters
- **#GENERAL.INPUT.5**: Validate as deep as possible to prevent errors from code changes
- **#GENERAL.INPUT.6**: Careful of boundary conditions (off-by-one) and conditionals

#### Buffer Management
- **#GENERAL.BUFFER.1**: Check buffer sizes, copies, and indices (especially sizeof)
- **#GENERAL.BUFFER.2**: Check appropriate buffer size with globally defined maximums
- **#GENERAL.BUFFER.3**: Check for NULL pointer dereferences
- **#GENERAL.BUFFER.4**: Check NULL for NIL-terminated strings

#### Arithmetic Safety
- **#GENERAL.ARITH.1**: Check data type limitations (integer underflow/overflow)
- **#GENERAL.ARITH.2**: Properly cast numeric variables in string manipulation
- **#GENERAL.ARITH.3**: SHOULD use SafeInt library for external input integers

#### Failure Handling
- **#GENERAL.FAIL.1**: Fail secure - fail closed when checks fail
- **#GENERAL.FAIL.2**: Don't provide hints to attackers in error messages

### ASSERT Usage Guidelines

**Critical Rules for ASSERT**:
- **#ASSERT.1**: ASSERT MUST be used for things that NEVER occur - use error handling for things that MIGHT occur
- **#ASSERT.Variable.1**: GetVariable with NV+RT without AU/RO MUST NOT ASSERT
- **#ASSERT.Variable.2**: SetVariable with NV attribute MUST NOT ASSERT
- **#ASSERT.Resource.1**: Memory allocation MUST NOT ASSERT after EndOfDxe
- **#ASSERT.SMM.1**: SMI handlers MUST NOT use ASSERT for external input after EndOfDxe
- **#ASSERT.NETWORK.1**: Network drivers MUST NOT use ASSERT for remote packets
- **#ASSERT.SHELL.1**: Shell MUST NOT use ASSERT for resource requests or user input

### Deprecated API Replacement

**#DEPRECATEDAPI.1**: MUST NOT use deprecated APIs - use secure replacements:

| **Deprecated API** | **Secure Replacement** |
|-------------------|------------------------|
| `StrCpy` | `StrCpyS` |
| `StrnCpy` | `StrnCpyS` |
| `StrCat` | `StrCatS` |
| `StrnCat` | `StrnCatS` |
| `AsciiStrCpy` | `AsciiStrCpyS` |
| `AsciiStrnCpy` | `AsciiStrnCpyS` |
| `AsciiStrCat` | `AsciiStrCatS` |
| `AsciiStrnCat` | `AsciiStrnCatS` |
| `UnicodeStrToAsciiStr` | `UnicodeStrToAsciiStrS` |
| `AsciiStrToUnicodeStr` | `AsciiStrToUnicodeStrS` |
| `GetVariable` | `GetVariable2` |
| `GetEfiGlobalVariable` | `GetEfiGlobalVariable2` |

### Race Condition Prevention

- **#RACECONDITION.1**: Careful of Time-of-Check/Time-of-Use attacks - copy untrusted data to trusted region before processing
- **#RACECONDITION.2**: Careful of BSP/AP race conditions - keep security critical sections short and simple

### Environment Security

#### Memory Protection
- **#ENVIRONMENT.RUNTIME.1**: Runtime modules MUST be built with 4K alignment for OS protection
- **#ENVIRONMENT.NX.1**: Code regions SHOULD be ReadOnly, Data regions SHOULD be NonExecutable
- **#ENVIRONMENT.NX.2**: Unallocated memory SHOULD be non-present or NonExecutable
- **#ENVIRONMENT.STACK.1**: Stack SHOULD be set to NX
- **#ENVIRONMENT.STACK.2**: Stack Guard SHOULD be enabled
- **#ENVIRONMENT.HEAP.1**: Heap SHOULD be NX for data
- **#ENVIRONMENT.HEAP.2**: MAY use heap guard for debugging

#### Advanced Protections
- **#ENVIRONMENT.ASLR.1-3**: Address Space Layout Randomization guidelines
- **#ENVIRONMENT.CONTROLFLOW.1**: Control Flow Guard for ROP/JOP protection

### Cryptography Requirements

#### Algorithm Selection
- **#CRYPTO.1**: SHOULD NOT use deprecated crypto algorithms
- **#CRYPTO.2**: SHOULD NOT implement custom crypto algorithms
- **#CRYPTO.3**: MUST follow cryptographic standards exactly
- **#CRYPTO.HASH.1**: SHOULD use SHA256 or stronger (NOT SHA1, MD4, MD5)
- **#CRYPTO.SYM.1**: SHOULD use AES or stronger
- **#CRYPTO.ASYM.1**: SHOULD use RSA or ECC equivalent or stronger

#### Key Management
- **#CRYPTO.SYM.2**: Symmetric keys MUST NOT be saved in flash as plain text
- **#CRYPTO.ASYM.2**: Private keys MUST NOT be saved in flash as plain text
- **#CRYPTO.RANDOM.1**: SHOULD use approved random number generator

### Password and Secret Handling

#### Password Security
- **#PASSWORD.1**: Password plaintext MUST NOT be saved to variables (use SALT+HASH)
- **#PASSWORD.2**: Password updates MUST be in secure environment (SMM or before EndOfDxe)
- **#PASSWORD.3**: MUST meet password criteria (strength, update, algorithm, retry, etc.)
- **#PASSWORD.5**: MUST clear passwords from memory after use
- **#PASSWORD.6**: MUST NOT hardcode passwords in code
- **#PASSWORD.7**: MUST compare all characters of password strings (constant-time)
- **#PASSWORD.8**: MUST add salt to resist rainbow table attacks
- **#PASSWORD.9**: MUST add sufficient iterations to slow hash calculation

#### Secret Management
- **#SECRET.1**: Secrets MUST NOT be saved as plain text in variables/disk
- **#SECRET.2**: MUST clear secrets from memory after use (global, stack, heap)
- **#SECRET.3**: MUST NOT hardcode secrets in code
- **#SECRET.4**: MUST compare entire secret data before completion
- **#SECRET.5**: Secret length MUST resist brute force attacks

### Firmware-Specific Security Guidelines

#### Flash Protection
- **#FLASH.1**: Platform MUST lock flash no later than EndOfDxe
- **#FLASH.2**: Flash lock MUST happen in ALL boot modes (normal, S3, capsule, recovery)

#### Flash Updates
- **#FLASH.UPDATE.1-7**: Comprehensive flash update security requirements including integrity checking, version validation, trusted execution environment

#### Variable Security
- **#VARIABLE.1**: MUST lock critical variables before EndOfDxe
- **#VARIABLE.2**: MUST use same lock policy in normal boot and S4
- **#VARIABLE.SET.1-2**: Error handling for variable operations
- **#VARIABLE.GET.1-2**: Error handling and validation for variable access
- **#VARIABLE.ATTRIB.1-2**: Conservative RT and NV attribute usage
- **#VARIABLE.CHECK.1-3**: Enable HII, PCD, and UEFI variable checks
- **#VARIABLE.CONFIDENTIALITY.1-2**: Encryption requirements for confidential variables

#### S3 Resume Security
- **#S3.1**: S3 BootScript MUST be saved in secure place (lockbox)
- **#S3.2**: S3 image dispatch MUST be saved securely
- **#S3.3**: S3 CPU data MUST be saved in SMM
- **#S3.4**: S3 configuration MUST be saved securely

#### Secure Boot Requirements
- **#SECUREBOOT.1**: MUST NOT disable secure boot without authentication
- **#SECUREBOOT.2**: MUST verify ALL images in secure boot path
- **#SECUREBOOT.3-4**: Firmware update/recovery images must be signed
- **#SECUREBOOT.5**: Verification in ALL boot paths
- **#SECUREBOOT.UEFI.1-3**: UEFI secure boot implementation requirements
- **#SECUREBOOT.Key.1**: Public keys MUST be in hardware/boot block/auth variables

#### Hardware and DMA Security
- **#HARDWARE.1**: MUST verify untrusted hardware inputs (SPD, USB descriptors, etc.)
- **#DMA.1**: Device DMA MUST be disabled by default
- **#DMA.2**: Enable DMA only when needed, disable after transaction
- **#DMA.3-5**: IOMMU configuration and DMA region isolation. DMA must not occur to the stack.
- **#NETWORK.1-2**: Network packet validation at all layers
- **#NETWORK.TLS.1-2**: Certificate storage requirements
- **#NETWORK.WIFI.1**: WiFi password protection

#### Silicon Register Security
- **#SILICON.1**: Lockable registers MUST be locked before EndOfDxe
- **#SILICON.2**: Register locks MUST work in all boot paths

### Side-Channel Attack Mitigation

- **#SIDECHANNEL.1**: Use `SpeculationBarrier()` after untrusted data validation in SMM
- **#SIDECHANNEL.2**: Use `StuffRsb` before RSM to mitigate Branch Target Injection
- **#MDS.1**: Rendezvous all logical processors in SMM for MDS mitigation

### Example: Secure Input Validation

```c
// SECURE: Proper input validation with overflow checks
EFI_STATUS
SecureProcessBuffer (
  IN VOID   *InputBuffer,
  IN UINTN  BufferSize,
  IN UINTN  ElementCount,
  IN UINTN  ElementSize
  )
{
  UINTN  RequiredSize;

  // Validate input parameters
  if (InputBuffer == NULL || BufferSize == 0) {
    return EFI_INVALID_PARAMETER;
  }

  // Check for integer overflow in multiplication
  if (ElementCount > MAX_UINTN / ElementSize) {
    return EFI_INVALID_PARAMETER;
  }

  RequiredSize = ElementCount * ElementSize;

  // Validate buffer size
  if (BufferSize < RequiredSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  // Copy to trusted region before processing
  VOID *TrustedBuffer = AllocatePool (RequiredSize);
  if (TrustedBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (TrustedBuffer, InputBuffer, RequiredSize);

  // Process data from trusted region only
  ProcessTrustedData (TrustedBuffer, RequiredSize);

  // Clear sensitive data
  ZeroMem (TrustedBuffer, RequiredSize);
  FreePool (TrustedBuffer);

  return EFI_SUCCESS;
}
```

### Example: Secure Password Handling

```c
// SECURE: Proper password comparison and cleanup
EFI_STATUS
SecurePasswordVerify (
  IN CHAR8  *InputPassword,
  IN UINTN  InputPasswordSize
  )
{
  CHAR8       StoredPasswordHash[32];
  CHAR8       ComputedHash[32];
  CHAR8       Salt[16];
  CHAR8       *LocalPassword;
  UINTN       Index;
  UINT8       Result;
  EFI_STATUS  Status;

  if ((InputPassword == NULL) || (InputPasswordSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  // Allocate buffer for local copy of password
  LocalPassword = AllocateZeroPool (InputPasswordSize);
  if (LocalPassword == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Result = 0;

  // Copy input password to local buffer
  CopyMem (LocalPassword, InputPassword, InputPasswordSize);

  // Get stored hash and salt
  GetStoredPasswordHash (StoredPasswordHash, Salt);

  // Compute hash of local password with salt
  ComputePasswordHash (LocalPassword, Salt, ComputedHash);

  // Constant-time comparison
  for (Index = 0; Index < sizeof (StoredPasswordHash); Index++) {
    Result |= (StoredPasswordHash[Index] ^ ComputedHash[Index]);
  }

  Status = (Result == 0) ? EFI_SUCCESS : EFI_ACCESS_DENIED;

  // Clear sensitive data
  ZeroMem (ComputedHash, sizeof (ComputedHash));
  ZeroMem (LocalPassword, InputPasswordSize);
  FreePool (LocalPassword);

  return Status;
}
```

These secure coding guidelines provide the foundation for writing secure EDK2 firmware code that resists common attack vectors and follows industry best practices.

## Security Guidelines for Firmware Code

> **Reference**: [EDK II Secure Code Review Guide](https://github.com/tianocore-docs/EDK_II_Secure_Code_Review_Guide)

Secure code review is critical for firmware development due to the privileged execution context and attack surface exposure. Unlike typical software reviews focused on quality, secure code reviews focus on confidentiality, integrity, and availability (C.I.A.) aspects.

### General Security Review Principles

#### 1. Prerequisites for Reviewers
- **Understand Threat Model**: Know the security architecture, assets, objectives, adversaries, and mitigations
- **Know Secure Design Principles**: Understand EDK II secure coding best practices
- **Review Priority**: Focus on high-risk code areas first

#### 2. Code Review Priority (High to Low)
1. **Old legacy code** - Higher vulnerability likelihood
2. **Code running by default** - Broader attack surface
3. **Code in elevated context** - SMM, PEI, early DXE
4. **C/C++/Assembly code** - Memory safety vulnerabilities
5. **Code with vulnerability history** - Known problem areas
6. **Code handling sensitive data** - Cryptographic material, secrets
7. **Complex code** - Higher error probability
8. **Frequently changing code** - Introduction of new vulnerabilities

#### 3. Common Vulnerability Patterns to Review
- **Integer arithmetic vulnerabilities** - Overflow, underflow, wraparound
- **Buffer overrun vulnerabilities** - Stack/heap buffer overflows
- **Cryptographic vulnerabilities** - Weak algorithms, improper key handling
- **Logic errors** - Off-by-one, incorrect operators (>, >=, ||, &&)
- **Input validation failures** - Insufficient boundary checks
- **Pointer validation issues** - NULL, dangling, out-of-bounds pointers

### Firmware-Specific Security Categories

Based on analysis of firmware vulnerabilities, focus reviews on these 8 critical categories:

#### 1. External Input Validation
**Risk**: Attacker-controlled data causing overflow, injection, or privilege escalation

**Common External Inputs**:
- UEFI capsule images
- Boot logo files (BMP, JPEG)
- File system partition contents
- Read/write UEFI variables
- SMM communication buffers
- Network packets (AMT, PXE)
- ACPI tables

**Review Checklist**:
- [ ] What external inputs does the code process?
- [ ] How is input size/bounds validation performed?
- [ ] Are validation checks performed on ALL possible code paths?
- [ ] What happens when validation fails? (proper error handling)
- [ ] For SMM: Is `SmmIsBufferOutsideSmmValid()` used for communication buffers?
- [ ] For variables: How are read/write variables consumed and validated?
- [ ] Are ASSERT macros used for validation? (Should use proper error returns)

**Example Vulnerability Pattern - Integer Overflow**:
```c
// VULNERABLE: Multiplication can overflow
BltBufferSize = BmpHeader->PixelWidth * BmpHeader->PixelHeight * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
*GopBlt = AllocatePool(BltBufferSize);  // May allocate tiny buffer due to overflow

// SECURE: Check for overflow before multiplication
if (BmpHeader->PixelWidth > MAX_UINT32 / sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) / BmpHeader->PixelHeight) {
  return EFI_INVALID_PARAMETER;
}
```

#### 2. Race Conditions
**Risk**: Time-of-check-time-of-use (TOCTOU) vulnerabilities, resource corruption

**Review Checklist**:
- [ ] What critical resources are accessed?
- [ ] Can BSP and AP cores access the same resource simultaneously?
- [ ] Does trusted code access untrusted region resources?
- [ ] Are proper synchronization mechanisms used (locks, atomics)?

#### 3. Hardware Input Validation
**Risk**: Malicious or corrupted hardware providing unexpected data

**Review Checklist**:
- [ ] What hardware inputs are processed (MMIO, registers, DMA)?
- [ ] How are hardware input values validated?
- [ ] Are MMIO BAR addresses properly validated before access?
- [ ] Are hardware timeouts and error conditions handled?

#### 4. Secret Handling
**Risk**: Information disclosure, credential compromise

**Review Checklist**:
- [ ] Where are secrets stored (keys, passwords, tokens)?
- [ ] How are secrets cleared after use (stack, heap, global data)?
- [ ] Are secrets stored in variables or configuration?
- [ ] Are default/hardcoded passwords used?
- [ ] Does password comparison use constant-time algorithms?
- [ ] Are side-channel attack mitigations implemented?

**Example Secure Secret Clearing**:
```c
// Clear sensitive data from all locations
ZeroMem(Password, PasswordSize);           // Clear variable
ZeroMem(PasswordBuffer, BufferSize);       // Clear communication buffer
ZeroMem(&LocalPassword, sizeof(LocalPassword)); // Clear stack
```

#### 5. Register Lock Security
**Risk**: Security register bypass, configuration tampering

**Review Checklist**:
- [ ] Which security registers need locking?
- [ ] When are registers locked (boot phase, timing)?
- [ ] Are register locks controlled by policy or variables?
- [ ] Can register locks be bypassed?
- [ ] Are registers locked in all boot paths (normal, S3, recovery, capsule)?
- [ ] Are registers locked in manufacturing/debug modes?

#### 6. Secure Configuration
**Risk**: Security policy bypass, insecure defaults

**Review Checklist**:
- [ ] Are security policies controlled by variables or PCDs?
- [ ] What are the default security configurations?
- [ ] How does security behave in different modes (S3, recovery, debug, manufacturing)?
- [ ] Can configuration be tampered with by unauthorized parties?

#### 7. Replay/Rollback Protection
**Risk**: Downgrade attacks, replay of old vulnerable firmware

**Review Checklist**:
- [ ] Are LSV (Lowest Supported Version) or SVN (Security Version Number) used?
- [ ] Where are version numbers stored and protected?
- [ ] How are timestamps, nonces, or monotonic counters implemented?
- [ ] Can version checks be bypassed?

#### 8. Cryptography Implementation
**Risk**: Weak encryption, algorithm vulnerabilities, key management issues

**Review Checklist**:
- [ ] Are signature verification algorithms properly implemented?
- [ ] Are deprecated/weak algorithms avoided (MD5, SHA1, RC4)?
- [ ] Is CRC/checksum used where cryptographic hash is needed?
- [ ] Are appropriate algorithms chosen (hash vs HMAC, symmetric vs asymmetric)?
- [ ] How are cryptographic keys deployed, stored, and destroyed?
- [ ] Are root keys vs session keys used appropriately?
- [ ] Is key material properly protected from disclosure?

### Security Review Summary Checklist

Use this comprehensive checklist during security-focused code reviews:

| **Category** | **Key Review Questions** |
|--------------|--------------------------|
| **External Input** | What external inputs exist? How are they validated? Are checks on all paths? What happens on validation failure? Is SMM communication buffer validated? How are variables consumed? |
| **Race Conditions** | What critical resources exist? Can BSP/AP access same resource? Does trusted code access untrusted regions? |
| **Hardware Input** | What hardware inputs exist? How are they validated? Are MMIO BARs validated? |
| **Secret Handling** | Where are secrets stored? How are they cleared? Are they in variables? Default/hardcoded credentials? Constant-time comparison? Side-channel protection? |
| **Register Lock** | What registers need locking? When are they locked? Policy-controlled? Locked in all boot modes? |
| **Secure Configuration** | Variable/PCD policy control? Default configuration? Behavior in special modes? |
| **Replay/Rollback** | LSV/SVN usage? Where stored? Timestamp/nonce/counter usage? |
| **Cryptography** | Algorithm choice? Deprecated algorithms avoided? Proper key management? Root vs session keys? |

### Security Tools and Analysis
- **Static Analysis**: Use available code analysis tools for vulnerability detection
- **Dynamic Testing**: Test with malformed inputs, boundary conditions, error scenarios
- **Threat Modeling**: Understand attack vectors and security assumptions
- **Penetration Testing**: Validate security controls with adversarial testing

### Security Development Lifecycle Integration
- **Design Review**: Security architecture review before implementation
- **Implementation Review**: Line-by-line security-focused code review
- **Testing**: Security-specific test cases and fuzzing
- **Response**: Incident response plan for discovered vulnerabilities

This guide covers secure EDK2 firmware development, including secure coding practices and
security review guidelines. For trusted boot chain and TPM measurement topics, refer to
[edk2-trusted-boot.instructions.md](../edk2-trusted-boot/SKILL.md).
