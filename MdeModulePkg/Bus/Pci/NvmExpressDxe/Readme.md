# NvmExpressDxe

## Overview

NvmExpressDxe is a UEFI driver that manages NVM Express (NVMe) non-volatile memory subsystems
connected over PCI. It follows the UEFI Driver Model and the NVM Express specification to
discover, initialize, and provide block-level access to NVMe storage devices during the UEFI
boot phase.

## What This Module Does

### Driver Binding

The driver implements the standard UEFI Driver Binding Protocol (`Supported`, `Start`, `Stop`)
to attach to PCI devices with class code Mass Storage / NVM (0x01/0x08) and NVMHCI programming
interface (0x02).

### Controller Initialization

During `Start`, the driver:

1. Opens the `EFI_PCI_IO_PROTOCOL` on the controller handle.
2. Reads the NVMe Controller Capabilities register (`CAP`) and validates NVM command set support.
3. Allocates DMA-accessible buffers for admin submission/completion queues.
4. Disables the controller, programs the Admin Queue Attributes (`AQA`), Admin Submission Queue
   Base Address (`ASQ`), and Admin Completion Queue Base Address (`ACQ`), then re-enables the
   controller.
5. Sends Identify Controller to retrieve controller metadata (serial number, model, capabilities).
6. Uses the Set Features command (Number of Queues) to negotiate I/O queue pairs with the
   controller.
7. Allocates DMA-accessible buffers for I/O submission/completion queues and creates the I/O
   queue pairs via Create I/O Completion Queue and Create I/O Submission Queue admin commands.
8. Enumerates NVMe namespaces and creates child handles for each discovered namespace.

### Protocols Produced (per controller)

| Protocol | Purpose |
|---|---|
| `EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL` | Raw NVMe command passthrough for admin and I/O commands. Installed on the controller handle. |
| `EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL` | Declares the EFI specification version the driver supports. Installed on the driver image handle at entry point. |

### Protocols Produced (per namespace)

| Protocol | Purpose |
|---|---|
| `EFI_BLOCK_IO_PROTOCOL` | Synchronous block read/write/flush/reset operations. |
| `EFI_BLOCK_IO2_PROTOCOL` | Asynchronous (non-blocking) block I/O operations. Only installed when the controller allocates more than one I/O queue pair. |
| `EFI_DISK_INFO_PROTOCOL` | Exposes NVMe Identify Namespace data for disk information queries. |
| `EFI_STORAGE_SECURITY_COMMAND_PROTOCOL` | Security Send/Receive commands (if the controller supports OACS bit 0). |
| `MEDIA_SANITIZE_PROTOCOL` | Media Clear, Purge, and Format operations mapped to NVMe Format NVM and Sanitize admin commands per NIST SP 800-88 guidelines. |

### Protocols Consumed

| Protocol | Purpose |
|---|---|
| `EFI_PCI_IO_PROTOCOL` | PCI BAR memory access, DMA buffer allocation, and bus master mapping. |
| `EFI_DEVICE_PATH_PROTOCOL` | Device path construction for namespace child handles. |
| `EFI_RESET_NOTIFICATION_PROTOCOL` | Registers a shutdown callback to gracefully shut down all NVMe controllers before platform reset. |

### Asynchronous I/O

The driver uses a periodic timer event (`NVME_HC_ASYNC_TIMER`, 1 ms) to poll the async I/O
completion queue and process completed asynchronous requests. The `BlockIo2` protocol is only
installed when the controller has allocated at least two I/O queue pairs (one for blocking, one
for async).

### Controller Reset

On command timeout, the driver performs a full controller reset: disable, re-program admin queues,
re-enable, re-identify, re-negotiate queue count, and re-create I/O queues—all while preserving
allocated DMA buffers.

### Shutdown Notification

The driver registers with `EFI_RESET_NOTIFICATION_PROTOCOL` to issue NVMe shutdown notifications
(CC.SHN) to all managed controllers before a platform reset, ensuring data integrity.

## MU_CHANGE Summary

This section documents all Microsoft (Project Mu) changes made to the upstream EDK2 NvmExpressDxe
driver. Each change is tagged in the source with `// MU_CHANGE` comments.

### 1. Allocate IO Queue Buffer

**Tag:** `MU_CHANGE - Allocate IO Queue Buffer`

**Files:** `NvmExpress.h`, `NvmExpress.c`, `NvmExpressHci.h`, `NvmExpressHci.c`, `NvmExpressBlockIo.c`, `NvmExpressPassthru.c`

**What changed:**

The upstream driver allocates a single flat 6-page DMA buffer at `DriverBindingStart` time and
carves fixed 4 KiB regions out of it for all six queues (admin SQ, admin CQ, I/O SQ #1, I/O CQ #1,
I/O SQ #2, I/O CQ #2). This MU change replaces that approach with a split allocation model:

- **Admin queues** are allocated separately based on the actual admin queue entry size and count
  derived from controller capabilities (via `NVME_SQ_SIZE_IN_PAGES` / `NVME_CQ_SIZE_IN_PAGES`
  macros).
- **I/O queues** are allocated in a separate DMA buffer (`IoQueueBuffer` / `IoQueueBufferPciAddr`)
  whose size is computed dynamically based on the negotiated number of I/O queue pairs and their
  entry sizes.
- A new structure `NVME_QUEUE_SIZE_DATA` (with `NumberOfEntries` and `EntrySize` fields) is added
  to track per-queue sizing metadata in the controller private data.
- Queue buffer page counts are computed using macros (`NVME_SQ_SIZE_IN_PAGES`,
  `NVME_CQ_SIZE_IN_PAGES`) that use the actual entry size (as a power of 2) rather than assuming
  fixed 4 KiB pages.
- New functions are introduced:
  - `NvmeControllerInitAdminQueues()` — initializes admin queue buffer pointers and programs ASQ/ACQ.
  - `NvmeControllerInitIoQueues()` — initializes I/O queue buffer pointers and creates I/O CQ/SQ.
  - `NvmExpressDriverCleanUpQueues()` — unmaps and frees both admin and I/O queue DMA buffers.
  - `NvmeControllerReset()` — performs a full controller reset reusing existing buffer allocations.
  - `ReadNvmeAdminQueueAttributes()` — reads the AQA register for validation during reset.
- The `NvmeEnableController()` function now accepts `IoSqEs` and `IoCqEs` parameters to program
  the correct queue entry sizes into `CC.IOSQES` and `CC.IOCQES`.
- Page mask operations on queue base addresses are removed since buffers allocated via
  `AllocatePages` are already page-aligned.

**Why it's needed:**

The fixed 6-page allocation is insufficient when queue sizes exceed 1 entry (i.e., when using
alternative queue sizes like 255 entries). The dynamic allocation supports variable queue entry
counts and entry sizes, allows the admin and I/O queues to be managed independently, and enables
proper cleanup and reset without leaking DMA memory. Separating the I/O queue buffer also means
the driver can scale the allocation based on how many queue pairs the controller actually grants.

---

### 2. Request Number of Queues from Controller

**Tag:** `MU_CHANGE - Request Number of Queues from Controller`

**Files:** `NvmExpress.h`, `NvmExpress.c`, `NvmExpressHci.c`

**What changed:**

- A new function `NvmeSetFeaturesNumberOfQueues()` is added. It sends the NVMe Set Features
  command (Feature ID: Number of Queues) to request the desired number of I/O queue pairs from
  the controller. The controller may allocate fewer pairs than requested; the driver stores the
  actual granted count in `Private->NumberOfIoQueuePairs`.
- The maximum number of queue pairs the driver requests is defined by `NVME_MAX_QUEUES` (3 total:
  1 admin + 2 I/O), meaning the driver requests up to 2 I/O queue pairs.
- The `NVME_SUPPORT_BLOCKIO2()` macro checks whether the controller allocated more than 1 I/O
  queue pair. If not, the `BlockIo2` protocol and async timer event are **not** installed.
- `NvmeCreateIoCompletionQueue()` and `NvmeCreateIoSubmissionQueue()` loop from index 1 to
  `NumberOfIoQueuePairs` instead of using hardcoded indices.
- `BlockIo2` protocol installation/uninstallation in `EnumerateNvmeDevNamespace()` and
  `UnregisterNvmeNamespace()` is made conditional on `NVME_SUPPORT_BLOCKIO2()`.

**Why it's needed:**

The upstream driver assumes every controller supports exactly two I/O queue pairs. Some NVMe
controllers (especially embedded or resource-constrained ones) may only support a single I/O
queue pair. By querying the controller via Set Features and gracefully degrading (skipping
`BlockIo2` when only one queue pair is available), the driver avoids failures on controllers that
cannot satisfy a two-queue-pair request and correctly reflects the controller's actual capabilities.

---

### 3. Support Alternative Hardware Queue Sizes in NVME Driver

**Tag:** `MU_CHANGE - Support alternative hardware queue sizes in NVME driver`

**Files:** `NvmExpress.h`, `NvmExpress.c`, `NvmExpressDxe.inf`, `NvmExpressHci.c`, `NvmExpressPassthru.c`

**What changed:**

- A PCD `PcdSupportAlternativeQueueSize` (Boolean) is consumed. When `TRUE`, the driver uses a
  maximum queue size of 255 entries (`NVME_ALTERNATIVE_MAX_QUEUE_SIZE`) instead of the default
  sizes (1 for sync, 63/255 for async).
- Queue creation (`NvmeCreateIoCompletionQueue`, `NvmeCreateIoSubmissionQueue`) uses
  `MIN(NVME_ALTERNATIVE_MAX_QUEUE_SIZE, Cap.Mqes)` for all queues when the PCD is enabled.
- Admin queue sizes (`AQA.ASQS`, `AQA.ACQS`) also use the alternative size when the PCD is set.
- Passthrough command handling (`NvmExpressPassThru`) adjusts queue head/tail pointer arithmetic
  to use modular wrap-around (instead of XOR toggle) when the alternative queue size is active,
  supporting queue depths greater than 1.
- The async task list processor (`ProcessAsyncTaskList`) similarly uses the alternative queue size
  for completion queue head management.

**Why it's needed:**

Some NVMe hardware implementations require a minimum queue depth greater than 1 (e.g., 255
entries). The upstream driver defaults to queue sizes of 1 for synchronous I/O and uses XOR-based
head/tail toggling which only works with 2-entry queues (0 and 1). When hardware requires deeper
queues, this feature enables proper modular arithmetic for queue management and allocates
appropriately sized buffers. This is controlled by a PCD so platforms that don't need it retain the
original behavior.

---

### 4. NVMe Namespace Filtering

**Tag:** `MU_CHANGE - NVMe namespace filtering`

**Files:** `NvmExpress.h`, `NvmExpress.c`, `NvmExpressDxe.inf`

**What changed:**

- A PCD `PcdNvmeNamespaceFilterId` (UINT32) is consumed. When `0` (default), all namespaces are
  enumerated as in the upstream behavior. When non-zero, namespace discovery is restricted to the
  single namespace whose NSID matches the PCD value.
- `DiscoverAllNamespaces()` takes a new `FilterNsId` parameter. When non-zero, `GetNextNamespace`
  is iterated and only the matching NSID is enumerated; the loop then exits.
- In the `RemainingDevicePath` path, if filtering is enabled and the requested `NamespaceId` does
  not match `FilterNsId`, the namespace is skipped.

**Why it's needed:**

In some platform or test configurations, it is desirable to restrict which NVMe namespaces are
exposed to the UEFI environment. For example, a system with a multi-namespace NVMe device may
only want a specific boot namespace available during boot to reduce enumeration time, limit
attack surface, or avoid exposing non-boot partitions. This PCD-controlled filter provides that
capability without modifying the driver at build time.

---

### 5. Use the Mqes Value from the Cap Register

**Tag:** `MU_CHANGE - Use the Mqes value from the Cap register`

**Files:** `NvmExpressHci.c`

**What changed:**

- When creating I/O completion and submission queues, the queue size is clamped to
  `MIN(requested_size, Cap.Mqes)` instead of using the requested size directly.

**Why it's needed:**

The NVMe `CAP.MQES` field reports the maximum queue entries supported by the controller. If the
driver requests a queue larger than what the controller supports, the behavior is undefined or the
command fails. By clamping queue sizes to `MQES`, the driver respects the controller's hardware
limits and avoids creating oversized queues.

---

### 6. Correct Cap Parameter Modifier

**Tag:** `MU_CHANGE - Correct Cap parameter modifier`

**Files:** `NvmExpressHci.c`

**What changed:**

- The `ReadNvmeControllerCapabilities()` function signature is corrected so that the `Cap`
  parameter uses the `OUT` modifier instead of `IN`, reflecting that the function writes to (not
  reads from) this parameter.

**Why it's needed:**

A correctness fix. The `Cap` parameter is an output of `ReadNvmeControllerCapabilities()`—the
function reads the hardware register and writes the result into the caller's buffer. Marking it
`IN` was semantically incorrect and could mislead static analysis tools or code reviewers.

---

### 7. Improve NVMe Controller Init Robustness

**Tag:** `MU_CHANGE - Improve NVMe controller init robustness`

**Files:** `NvmExpressHci.c`

**What changed:**

- At the start of `NvmeControllerInit()` (and `NvmeControllerReset()`), the driver reads the PCI
  Vendor ID and Device ID. If either returns `0xFFFF` (`NVME_INVALID_VID_DID`), the function
  returns `EFI_DEVICE_ERROR` immediately.
- The assertion on `Cap.Mpsmin` is replaced with a conditional check and `EFI_DEVICE_ERROR` return,
  so the driver fails gracefully instead of asserting if the controller reports an unsupported
  minimum page size.

**Why it's needed:**

If an NVMe controller has been surprise-removed (hot-unplug), is behind a failed PCIe link, or
is otherwise inaccessible, PCI config reads return all-ones (`0xFFFF`). Without this check, the
driver would proceed to access invalid MMIO space, potentially causing system hangs or crashes.
The `Mpsmin` check change prevents a hard assert in production builds if the controller reports an
unexpected minimum memory page size, instead returning a clean error.

---

### 8. Remove Page Mask

**Tag:** `MU_CHANGE - Remove Page Mask` / `MU_CHANGE - Remove the page mask since the buffer is allocated using AllocatePages`

**Files:** `NvmExpressHci.c`

**What changed:**

- The page-alignment mask operations (`& ~(EFI_PAGE_SIZE - 1)`) on the admin submission queue
  (ASQ) and admin completion queue (ACQ) base addresses are removed.

**Why it's needed:**

Since the admin queue buffers are allocated using `PciIo->AllocateBuffer()` with
`AllocateAnyPages`, the returned addresses are already guaranteed to be page-aligned. Applying a
page mask is redundant and was removed for clarity. This is part of the broader Allocate IO Queue
Buffer refactoring.
