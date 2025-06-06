/** @file Smmuv3Util.c

    This file contains util functions for the SMMU driver.
    All functions are derived from the SMMU spec: <https://developer.arm.com/documentation/ihi0070/latest/>

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include "SmmuV3.h"

/**
  Decode the address width from the given address size type.

  @param [in]  AddressSizeType  The address size type.

  @return The decoded address width. 0 if the address size type is invalid.
**/
UINT32
SmmuV3DecodeAddressWidth (
  IN UINT32  AddressSizeType
  )
{
  UINT32  Length;

  switch (AddressSizeType) {
    case SmmuAddressSize32Bit:
      Length = 32;
      break;
    case SmmuAddressSize36Bit:
      Length = 36;
      break;
    case SmmuAddressSize40Bit:
      Length = 40;
      break;
    case SmmuAddressSize42Bit:
      Length = 42;
      break;
    case SmmuAddressSize44Bit:
      Length = 44;
      break;
    case SmmuAddressSize48Bit:
      Length = 48;
      break;
    case SmmuAddressSize52Bit:
      Length = 52;
      break;
    default:
      DEBUG ((DEBUG_ERROR, "%a: Invalid Address Size Type: 0x%lx\n", __func__, AddressSizeType));
      Length = 0;
      break;
  }

  return Length;
}

/**
  Encode the address width to the corresponding address size type.

  @param [in]  AddressWidth  The address width.

  @return The encoded address size type. 0 if the address width is invalid.
**/
UINT8
SmmuV3EncodeAddressWidth (
  IN UINT32  AddressWidth
  )
{
  UINT8  Encoding;

  switch (AddressWidth) {
    case 32:
      Encoding = SmmuAddressSize32Bit;
      break;
    case 36:
      Encoding = SmmuAddressSize36Bit;
      break;
    case 40:
      Encoding = SmmuAddressSize40Bit;
      break;
    case 42:
      Encoding = SmmuAddressSize42Bit;
      break;
    case 44:
      Encoding = SmmuAddressSize44Bit;
      break;
    case 48:
      Encoding = SmmuAddressSize48Bit;
      break;
    case 52:
      Encoding = SmmuAddressSize52Bit;
      break;
    default:
      DEBUG ((DEBUG_ERROR, "%a: Invalid Address Width: 0x%lx\n", __func__, AddressWidth));
      Encoding = 0;
      break;
  }

  return Encoding;
}

/**
  Set the translation starting level for SMMUv3 page tables.
  Only 3 and 4 level paging are supported.

  @param [in]  SmmuInfo           Pointer to the SMMU_INFO structure.
  @param [in]  OutputAddressWidth  The output address width.
  @param [out] S2Sl0              The starting level for stage 2 translation.

  @retval EFI_SUCCESS              Success.
  @retval EFI_INVALID_PARAMETER    Invalid parameter.
**/
EFI_STATUS
SmmuV3SetTranslationStartingLevel (
  IN SMMU_INFO  *SmmuInfo,
  IN UINT32     OutputAddressWidth,
  OUT UINT64    *S2Sl0
  )
{
  if ((OutputAddressWidth > PAGE_TABLE_OUTPUT_ADDRESS_WIDTH_MAX) || (OutputAddressWidth < PAGE_TABLE_OUTPUT_ADDRESS_WIDTH_MIN)) {
    DEBUG ((DEBUG_ERROR, "%a: OutputAddressWidth %d not supported.\n", __func__, OutputAddressWidth));
    return EFI_INVALID_PARAMETER;
  }

  // Per the Arm ARM VMSA spec, >= 44 bits of address width requires 4 level paging.
  // Otherwise, 3 level paging is used.
  if (OutputAddressWidth >= PAGE_TABLE_4_LEVEL_OUTPUT_ADDRESS_WIDTH_MIN) {
    SmmuInfo->TranslationStartingLevel = 0; // 4 level paging
    *S2Sl0                             = 0x2;
  } else {
    SmmuInfo->TranslationStartingLevel = 1; // 3 level paging
    *S2Sl0                             = 0x1;
    // If the output address width is greater than PAGE_TABLE_CONCATENATED_PAGES_BITS_CUTOFF, the page table root must be concatenated.
    SmmuInfo->PageTableRootConcatenated = (OutputAddressWidth > PAGE_TABLE_CONCATENATED_PAGES_BITS_CUTOFF);
  }

  return EFI_SUCCESS;
}

/**
  Read a 32-bit value from the specified SMMU register.

  @param [in]  SmmuBase   The base address of the SMMU.
  @param [in]  Register   The offset of the register.

  @return The 32-bit value read from the register. 0 if the SMMU base address is invalid.
**/
UINT32
SmmuV3ReadRegister32 (
  IN UINT64  SmmuBase,
  IN UINT64  Register
  )
{
  if (SmmuBase == 0) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid SMMU base address\n", __func__));
    return 0;
  }

  return MmioRead32 (SmmuBase + Register);
}

/**
  Read a 64-bit value from the specified SMMU register.

  @param [in]  SmmuBase   The base address of the SMMU.
  @param [in]  Register   The offset of the register.

  @return The 64-bit value read from the register. 0 if the SMMU base address is invalid.
**/
UINT64
SmmuV3ReadRegister64 (
  IN UINT64  SmmuBase,
  IN UINT64  Register
  )
{
  if (SmmuBase == 0) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid SMMU base address\n", __func__));
    return 0;
  }

  return MmioRead64 (SmmuBase + Register);
}

/**
  Write a 32-bit value to the specified SMMU register.

  @param [in]  SmmuBase   The base address of the SMMU.
  @param [in]  Register   The offset of the register.
  @param [in]  Value      The 32-bit value to write.

  @return The 32-bit value written to the register, or 0 if the SMMU base address is invalid.
**/
UINT32
SmmuV3WriteRegister32 (
  IN UINT64  SmmuBase,
  IN UINT64  Register,
  IN UINT32  Value
  )
{
  if (SmmuBase == 0) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid SMMU base address\n", __func__));
    return 0;
  }

  return MmioWrite32 (SmmuBase + Register, Value);
}

/**
  Write a 64-bit value to the specified SMMU register.

  @param [in]  SmmuBase   The base address of the SMMU.
  @param [in]  Register   The offset of the register.
  @param [in]  Value      The 64-bit value to write.

  @return The 64-bit value written to the register, or 0 if the SMMU base address is invalid.
**/
UINT64
SmmuV3WriteRegister64 (
  IN UINT64  SmmuBase,
  IN UINT64  Register,
  IN UINT64  Value
  )
{
  if (SmmuBase == 0) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid SMMU base address\n", __func__));
    return 0;
  }

  return MmioWrite64 (SmmuBase + Register, Value);
}

/**
  Disable interrupts for the SMMUv3.

  @param [in]  SmmuBase          The base address of the SMMU.
  @param [in]  ClearStaleErrors  Whether to clear stale errors.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid Parameters.
  @retval EFI_TIMEOUT            Timeout.
**/
EFI_STATUS
SmmuV3DisableInterrupts (
  IN UINT64   SmmuBase,
  IN BOOLEAN  ClearStaleErrors
  )
{
  EFI_STATUS       Status;
  SMMUV3_IRQ_CTRL  IrqControl;
  SMMUV3_GERROR    GlobalErrors;

  if (SmmuBase == 0) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid SMMU base address\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  IrqControl.AsUINT32 = SmmuV3ReadRegister32 (SmmuBase, SMMU_IRQ_CTRL);
  if ((IrqControl.AsUINT32 & SMMUV3_IRQ_CTRL_GLOBAL_PRIQ_EVTQ_EN_MASK) != 0) {
    IrqControl.AsUINT32 &= ~SMMUV3_IRQ_CTRL_GLOBAL_PRIQ_EVTQ_EN_MASK;
    SmmuV3WriteRegister32 (SmmuBase, SMMU_IRQ_CTRL, IrqControl.AsUINT32);
    Status = SmmuV3Poll (SmmuBase, SMMU_IRQ_CTRLACK, SMMUV3_IRQ_CTRL_GLOBAL_PRIQ_EVTQ_EN_MASK, 0);
    if (Status != EFI_SUCCESS) {
      DEBUG ((DEBUG_ERROR, "%a: Error polling register: 0x%lx\n", __func__, SmmuBase + SMMU_IRQ_CTRLACK));
      return Status;
    }
  }

  if (ClearStaleErrors != FALSE) {
    GlobalErrors.AsUINT32 = SmmuV3ReadRegister32 (SmmuBase, SMMU_GERROR);
    GlobalErrors.AsUINT32 = GlobalErrors.AsUINT32 & SMMUV3_GERROR_VALID_MASK;
    SmmuV3WriteRegister32 (SmmuBase, SMMU_GERROR, GlobalErrors.AsUINT32);
  }

  return EFI_SUCCESS;
}

/**
  Enable interrupts for the SMMUv3.

  @param [in]  SmmuBase  The base address of the SMMU.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid Parameters.
  @retval EFI_TIMEOUT            Timeout.
**/
EFI_STATUS
SmmuV3EnableInterrupts (
  IN UINT64  SmmuBase
  )
{
  EFI_STATUS       Status;
  SMMUV3_IRQ_CTRL  IrqControl;

  if (SmmuBase == 0) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid SMMU base address\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  IrqControl.AsUINT32         = SmmuV3ReadRegister32 (SmmuBase, SMMU_IRQ_CTRL);
  IrqControl.AsUINT32        &= ~SMMUV3_IRQ_CTRL_GLOBAL_PRIQ_EVTQ_EN_MASK;
  IrqControl.GlobalErrorIrqEn = 1;
  IrqControl.EventqIrqEn      = 1;
  SmmuV3WriteRegister32 (SmmuBase, SMMU_IRQ_CTRL, IrqControl.AsUINT32);
  Status = SmmuV3Poll (SmmuBase, SMMU_IRQ_CTRLACK, 0x5, 0x5);
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "%a: Error polling register: 0x%lx\n", __func__, SmmuBase + SMMU_IRQ_CTRLACK));
  }

  return Status;
}

/**
  Disable translation for the SMMUv3.

  @param [in]  SmmuBase  The base address of the SMMU.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid Parameters.
  @retval EFI_TIMEOUT            Timeout.
**/
EFI_STATUS
SmmuV3DisableTranslation (
  IN UINT64  SmmuBase
  )
{
  SMMUV3_CR0  Cr0;
  EFI_STATUS  Status;

  if (SmmuBase == 0) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid SMMU base address\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Cr0.AsUINT32 = SmmuV3ReadRegister32 (SmmuBase, SMMU_CR0);
  if ((Cr0.AsUINT32 & SMMUV3_CR0_SMMU_CMDQ_EVTQ_PRIQ_EN_MASK) != 0) {
    Cr0.AsUINT32 = Cr0.AsUINT32 & ~SMMUV3_CR0_SMMU_CMDQ_EVTQ_PRIQ_EN_MASK;
    SmmuV3WriteRegister32 (SmmuBase, SMMU_CR0, Cr0.AsUINT32);
    Status = SmmuV3Poll (SmmuBase, SMMU_CR0ACK, SMMUV3_CR0_SMMU_CMDQ_EVTQ_PRIQ_EN_MASK, 0);
    if (Status != EFI_SUCCESS) {
      DEBUG ((DEBUG_ERROR, "%a: Error polling register: 0x%lx\n", __func__, SmmuBase + SMMU_CR0ACK));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Set the Smmu in ABORT mode and stop DMA.

  @param [in]  SmmuReg    Base address of the SMMUv3.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid Parameters.
  @retval EFI_TIMEOUT            Timeout.
**/
EFI_STATUS
SmmuV3GlobalAbort (
  IN  UINT64  SmmuBase
  )
{
  EFI_STATUS  Status;
  UINT32      RegVal;

  if (SmmuBase == 0) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid SMMU base address\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  // Attribute update has completed when SMMU_(S)_GBPA.Update bit is 0.
  Status = SmmuV3Poll (SmmuBase, SMMU_GBPA, SMMU_GBPA_UPDATE, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // SMMU_(S)_CR0 resets to zero with all streams bypassing the SMMU,
  // so just abort all incoming transactions.
  RegVal = SmmuV3ReadRegister32 (SmmuBase, SMMU_GBPA);

  // Set the SMMU_GBPA.ABORT and SMMU_GBPA.UPDATE.
  RegVal |= (SMMU_GBPA_ABORT | SMMU_GBPA_UPDATE);

  SmmuV3WriteRegister32 (SmmuBase, SMMU_GBPA, RegVal);

  // Attribute update has completed when SMMU_(S)_GBPA.Update bit is 0.
  Status = SmmuV3Poll (SmmuBase, SMMU_GBPA, SMMU_GBPA_UPDATE, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Sanity check to see if abort is set
  Status = SmmuV3Poll (SmmuBase, SMMU_GBPA, SMMU_GBPA_ABORT, SMMU_GBPA_ABORT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Set all streams to bypass the SMMU.

  @param [in]  SmmuReg    Base address of the SMMUv3.

  @retval EFI_SUCCESS            Success.
  @retval EFI_TIMEOUT            Timeout.
  @retval EFI_INVALID_PARAMETER  Invalid Parameters.
**/
EFI_STATUS
SmmuV3SetGlobalBypass (
  IN UINT64  SmmuBase
  )
{
  EFI_STATUS  Status;
  UINT32      RegVal;

  if (SmmuBase == 0) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid SMMU base address\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  // Attribute update has completed when SMMU_(S)_GBPA.Update bit is 0.
  Status = SmmuV3Poll (SmmuBase, SMMU_GBPA, SMMU_GBPA_UPDATE, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // SMMU_(S)_CR0 resets to zero with all streams bypassing the SMMU
  RegVal = SmmuV3ReadRegister32 (SmmuBase, SMMU_GBPA);

  // TF-A configures the SMMUv3 to abort all incoming transactions.
  // Clear the SMMU_GBPA.ABORT to allow Non-secure streams to bypass
  // the SMMU.
  RegVal &= ~SMMU_GBPA_ABORT;
  RegVal |= SMMU_GBPA_UPDATE;

  SmmuV3WriteRegister32 (SmmuBase, SMMU_GBPA, RegVal);

  Status = SmmuV3Poll (SmmuBase, SMMU_GBPA, SMMU_GBPA_UPDATE, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Poll the SMMU register and test the value based on the mask.

  @param [in]  SmmuBase   Base address of the SMMU.
  @param [in]  SmmuReg    The SMMU register to poll.
  @param [in]  Mask       Mask of register bits to monitor.
  @param [in]  Value      Expected value.

  @retval EFI_SUCCESS            Success.
  @retval EFI_TIMEOUT            Timeout.
  @retval EFI_INVALID_PARAMETER  Invalid Parameters.
**/
EFI_STATUS
SmmuV3Poll (
  IN UINT64  SmmuBase,
  IN UINT64  SmmuReg,
  IN UINT32  Mask,
  IN UINT32  Value
  )
{
  UINT32  RegVal;
  UINTN   Count;

  if (SmmuBase == 0) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid SMMU base address\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  // Set 0.1ms timeout value.
  Count = 10;
  do {
    RegVal = SmmuV3ReadRegister32 (SmmuBase, SmmuReg);
    if ((RegVal & Mask) == Value) {
      return EFI_SUCCESS;
    }

    MicroSecondDelay (10);
  } while ((--Count) > 0);

  DEBUG ((
    DEBUG_ERROR,
    "%a: Timeout polling SMMUv3 register @%p Read value 0x%x "
    "expected 0x%x\n",
    __func__,
    SmmuReg,
    RegVal,
    ((Value == 0) ? (RegVal & ~Mask) : (RegVal | Mask))
    ));

  return EFI_TIMEOUT;
}

/**
  Consume the event queue for errors and retrieve the fault record.
  Clears the outputted FaultRecord if the queue is empty.

  @param [in]  SmmuInfo     Pointer to the SMMU_INFO structure.
  @param [out] FaultRecord  Pointer to the fault record structure.
  @param [out] IsEmpty      Flag to indicate if the queue is empty.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid Parameters.
**/
EFI_STATUS
SmmuV3ConsumeEventQueueForErrors (
  IN SMMU_INFO             *SmmuInfo,
  OUT SMMUV3_FAULT_RECORD  *FaultRecord,
  OUT BOOLEAN              *IsEmpty
  )
{
  SMMUV3_EVENTQ_CONS   Consumer;
  UINT32               ConsumerIndex;
  UINT32               ConsumerWrap;
  SMMUV3_FAULT_RECORD  *NextFault;
  SMMUV3_EVENTQ_PROD   Producer;
  UINT32               ProducerIndex;
  UINT32               ProducerWrap;
  BOOLEAN              QueueEmpty;
  UINT32               QueueMask;
  UINT32               TotalQueueEntries;
  UINT32               WrapMask;

  if ((SmmuInfo == NULL) || ((FaultRecord == NULL) || (IsEmpty == NULL))) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameters\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  TotalQueueEntries = SMMUV3_COUNT_FROM_LOG2 (SmmuInfo->EventQueueLog2Size);
  WrapMask          = TotalQueueEntries;
  QueueMask         = TotalQueueEntries - 1;

  Producer.AsUINT32 = SmmuV3ReadRegister32 (SmmuInfo->SmmuBase + SMMUV3_PAGE_1_OFFSET, SMMU_EVENTQ_PROD);
  Consumer.AsUINT32 = SmmuV3ReadRegister32 (SmmuInfo->SmmuBase + SMMUV3_PAGE_1_OFFSET, SMMU_EVENTQ_CONS);

  ProducerIndex = Producer.WriteIndex & QueueMask;
  ProducerWrap  = Producer.WriteIndex & WrapMask;
  ConsumerIndex = Consumer.ReadIndex & QueueMask;
  ConsumerWrap  = Consumer.ReadIndex & WrapMask;
  QueueEmpty    = SMMUV3_IS_QUEUE_EMPTY (
                    ProducerIndex,
                    ProducerWrap,
                    ConsumerIndex,
                    ConsumerWrap
                    );

  if (QueueEmpty != FALSE) {
    *IsEmpty = TRUE;
    goto End;
  }

  *IsEmpty  = FALSE;
  NextFault = (SMMUV3_FAULT_RECORD *)SmmuInfo->EventQueue + ConsumerIndex;
  CopyMem (FaultRecord, NextFault, SMMUV3_EVENT_QUEUE_ENTRY_SIZE);

  ConsumerIndex += 1;
  if (ConsumerIndex == TotalQueueEntries) {
    ConsumerIndex = 0;
    ConsumerWrap  = ConsumerWrap ^ WrapMask;
  }

  Consumer.ReadIndex = ConsumerIndex | ConsumerWrap;

  ArmDataSynchronizationBarrier ();

  SmmuV3WriteRegister32 (SmmuInfo->SmmuBase + SMMUV3_PAGE_1_OFFSET, SMMU_EVENTQ_CONS, Consumer.AsUINT32);

End:
  return EFI_SUCCESS;
}

/**
  Dump the page table entries for a given virtual address.
  Dumps PTE's for all levels regardless of the starting level chosen for translation.

  @param [in]  SmmuInfo        Pointer to the SMMU_INFO structure.
  @param [in]  VirtualAddress  The virtual address to dump.
  @param [in]  Root            Pointer to the root page table.

  @retval None.
**/
VOID
SmmuV3DumpPageTableEntries (
  IN SMMU_INFO   *SmmuInfo,
  IN UINT64      VirtualAddress,
  IN PAGE_TABLE  *Root
  )
{
  UINTN       Index;
  UINT8       Level;
  PAGE_TABLE  *Current;

  Current = Root;

  for (Level = SmmuInfo->TranslationStartingLevel; Level < PAGE_TABLE_DEPTH; Level++) {
    Index = PAGE_TABLE_INDEX (VirtualAddress, Level, SmmuInfo->OutputAddressWidth, SmmuInfo->TranslationStartingLevel, SmmuInfo->PageTableRootConcatenated);
    if (Current->Entries[Index] == 0) {
      DEBUG ((DEBUG_ERROR, "%a: Invalid entry at level %d, index %d\n", __func__, Level, Index));
      break;
    }

    DEBUG ((DEBUG_INFO, "%a: VirtualAddress = %llx Level = %d Current->Entries[%d] = 0x%llx\n", __func__, VirtualAddress, Level, Index, Current->Entries[Index]));
    Current = (PAGE_TABLE *)((UINTN)Current->Entries[Index] & ~0xFFF);
  }
}

/**
  Log the errors if found from the SMMUv3. Prints Event Queue entries and GError register.
  Does nothing if no errors found.

  @param [in]  SmmuInfo  Pointer to the SMMU_INFO structure.

  @retval EFI_SUCCESS            No SMMU errors found.
  @retval EFI_INVALID_PARAMETER  Invalid Parameters.
  @retval EFI_DEVICE_ERROR       SMMU error found.
**/
EFI_STATUS
SmmuV3LogErrors (
  IN SMMU_INFO  *SmmuInfo
  )
{
  SMMUV3_GERROR        GError;
  SMMUV3_FAULT_RECORD  FaultRecord;
  UINTN                Index;
  BOOLEAN              IsEmpty;
  EFI_STATUS           Status;

  if (SmmuInfo == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameters\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  do {
    // Only consumes one entry at a time, so we loop until empty
    Status = SmmuV3ConsumeEventQueueForErrors (SmmuInfo, &FaultRecord, &IsEmpty);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Error consuming event queue\n", __func__));
      return Status;
    }

    if (IsEmpty == FALSE) {
      Status = EFI_DEVICE_ERROR;
      DEBUG ((DEBUG_ERROR, "%a: %llx FaultRecord:\n", __func__, SmmuInfo->SmmuBase));
      for (Index = 0; Index < sizeof (FaultRecord.Fault) / sizeof (FaultRecord.Fault[0]); Index++) {
        DEBUG ((DEBUG_ERROR, "0x%llx\n", FaultRecord.Fault[Index]));
      }

      // Dump PTE's if translation related fault
      if (((FaultRecord.Fault[0] & 0xFF) == 0x10) || ((FaultRecord.Fault[0] & 0xFF) == 0x11) ||
          ((FaultRecord.Fault[0] & 0xFF) == 0x12) || ((FaultRecord.Fault[0] & 0xFF) == 0x13))
      {
        SmmuV3DumpPageTableEntries (SmmuInfo, FaultRecord.Fault[2], SmmuInfo->PageTableRoot);
      }
    }
  } while (IsEmpty == FALSE);

  GError.AsUINT32 = SmmuV3ReadRegister32 (SmmuInfo->SmmuBase, SMMU_GERROR);
  if (GError.AsUINT32 != 0) {
    Status = EFI_DEVICE_ERROR;
    DEBUG ((DEBUG_ERROR, "%a: %llx GError: 0x%lx\n", __func__, SmmuInfo->SmmuBase, GError.AsUINT32));
  }

  return Status;
}

/**
  Write commands to the SMMUv3 command queue.

  @param [in]  SmmuInfo       Pointer to the SMMU_INFO structure.
  @param [in]  StartingIndex  The starting index in the command queue.
  @param [in]  CommandCount   The number of commands to write.
  @param [in]  Commands       Pointer to the commands to write.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid Parameters.
**/
STATIC
EFI_STATUS
SmmuV3WriteCommands (
  IN SMMU_INFO           *SmmuInfo,
  IN UINT32              StartingIndex,
  IN UINT32              CommandCount,
  IN SMMUV3_CMD_GENERIC  *Commands
  )
{
  UINT32              Index;
  UINT32              ProducerIndex;
  UINT32              QueueMask;
  UINT32              WrapMask;
  SMMUV3_CMD_GENERIC  *CommandQueue;

  if ((SmmuInfo == NULL) || (Commands == NULL) || (CommandCount == 0)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameters\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  WrapMask     = (1UL << SmmuInfo->CommandQueueLog2Size);
  QueueMask    = WrapMask - 1;
  CommandQueue = (SMMUV3_CMD_GENERIC *)SmmuInfo->CommandQueue;
  for (Index = 0; Index < CommandCount; Index += 1) {
    ProducerIndex               = (UINT32)((StartingIndex + Index) & QueueMask);
    CommandQueue[ProducerIndex] = Commands[Index];
  }

  // This DSB ensures that all commands written to the command queue before this point will be visible
  // before we update the producer index register to actually trigger processing of the commands.
  ArmDataSynchronizationBarrier ();

  return EFI_SUCCESS;
}

/**
  Update the cached consumer index for the SMMUv3 command queue.

  @param [in]  SmmuInfo            Pointer to the SMMU_INFO structure.
  @param [in]  QueueMask           The queue mask.
  @param [in]  WrapMask            The wrap mask.
  @param [in]  TotalQueueEntries   The total number of entries in the queue.
  @param [out] ConsumerIndexOut    Pointer to store the consumer index.
  @param [out] ConsumerWrapOut     Pointer to store the consumer wrap.
**/
VOID
SmmuV3CmdQueueUpdateCachedConsumer (
  IN  SMMU_INFO  *SmmuInfo,
  IN  UINT32     QueueMask,
  IN  UINT32     WrapMask,
  IN  UINT32     TotalQueueEntries,
  OUT UINT32     *ConsumerIndexOut,
  OUT UINT32     *ConsumerWrapOut
  )
{
  SMMUV3_CMDQ_CONS  Consumer;
  UINT32            ConsumerIndex;
  UINT32            ConsumerWrap;
  UINT64            CachedConsumerWrap;

  if ((SmmuInfo == NULL) || (ConsumerIndexOut == NULL) || (ConsumerWrapOut == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameters\n", __func__));
    ASSERT (FALSE);
    return;
  }

  Consumer.AsUINT32 = SmmuV3ReadRegister32 (SmmuInfo->SmmuBase, SMMU_CMDQ_CONS);
  ConsumerIndex     = Consumer.ReadIndex & QueueMask;
  ConsumerWrap      = Consumer.ReadIndex & WrapMask;
  *ConsumerIndexOut = ConsumerIndex;
  *ConsumerWrapOut  = ConsumerWrap;

  CachedConsumerWrap       = SmmuInfo->CachedConsumer & WrapMask;
  SmmuInfo->CachedConsumer = (SmmuInfo->CachedConsumer & ~QueueMask) | ConsumerIndex;

  if (CachedConsumerWrap != ConsumerWrap) {
    SmmuInfo->CachedConsumer += TotalQueueEntries;
  }
}

/**
  Send a SMMUV3_CMD_GENERIC command to the SMMUv3.

  @param [in]  SmmuInfo  Pointer to the SMMU_INFO structure.
  @param [in]  Command   Pointer to the command to send.

  @retval EFI_SUCCESS            Success.
  @retval EFI_TIMEOUT            Timeout.
  @retval EFI_INVALID_PARAMETER  Invalid Parameters.
**/
EFI_STATUS
SmmuV3SendCommand (
  IN SMMU_INFO           *SmmuInfo,
  IN SMMUV3_CMD_GENERIC  *Command
  )
{
  UINT32            QueueMask;
  UINT32            WrapMask;
  UINT32            TotalQueueEntries;
  UINT32            ProducerIndex;
  UINT32            ConsumerIndex;
  UINT32            ProducerWrap;
  UINT32            ConsumerWrap;
  SMMUV3_CMDQ_PROD  Producer;
  EFI_STATUS        Status;
  EFI_TPL           OldTpl;
  UINT64            NewProducer;

  if ((SmmuInfo == NULL) || (Command == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameters\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  TotalQueueEntries = SMMUV3_COUNT_FROM_LOG2 (SmmuInfo->CommandQueueLog2Size);
  WrapMask          = TotalQueueEntries;
  QueueMask         = WrapMask - 1;

  // We need to synchronize the the entire command queue write and producer update with the TPL locks.
  // As a result we don't just lock SmmuV3CmdQueueUpdateCachedConsumer but the entire process of writing the command
  // and updating the producer index.
  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  // Loop until there is space in the command queue
  do {
    Producer.AsUINT32 = SmmuV3ReadRegister32 (SmmuInfo->SmmuBase, SMMU_CMDQ_PROD);
    ProducerWrap      = Producer.WriteIndex & WrapMask;
    ProducerIndex     = Producer.WriteIndex & QueueMask;

    SmmuV3CmdQueueUpdateCachedConsumer (SmmuInfo, QueueMask, WrapMask, TotalQueueEntries, &ConsumerIndex, &ConsumerWrap);
  } while (SMMUV3_IS_QUEUE_FULL (ProducerIndex, ProducerWrap, ConsumerIndex, ConsumerWrap) != FALSE);

  Status = SmmuV3WriteCommands (SmmuInfo, ProducerIndex, 1, Command);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Error writing command to queue\n", __func__));
    gBS->RestoreTPL (OldTpl);
    return Status;
  }

  SmmuInfo->CachedProducer += 1;
  NewProducer               = SmmuInfo->CachedProducer;
  SmmuV3WriteRegister32 (SmmuInfo->SmmuBase, SMMU_CMDQ_PROD, (UINT32)(NewProducer & (WrapMask | QueueMask)));

  gBS->RestoreTPL (OldTpl);

  // Loop until the command is consumed
  do {
    // SmmuV3CmdQueueUpdateCachedConsumer needs to be within the scope of this lock because we want to make sure we have
    // the synchronized view of the consumer index when checking against the current local producer index.
    OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
    SmmuV3CmdQueueUpdateCachedConsumer (SmmuInfo, QueueMask, WrapMask, TotalQueueEntries, &ConsumerIndex, &ConsumerWrap);
    // Only prints errors if Event Queue is not empty and GError != 0. Returns EFI_SUCCESS if no errors found.
    Status = SmmuV3LogErrors (SmmuInfo);
    gBS->RestoreTPL (OldTpl);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Error logged from SMMUv3 during command queue operation.\n", __func__));
      break;
    }
  } while (SmmuInfo->CachedConsumer < NewProducer);

  return Status;
}

/**
  Invalidate all TLB entries in the SMMUv3.

  @param [in]  SmmuInfo  Pointer to the SMMU_INFO structure.

  @retval EFI_SUCCESS            Success.
  @retval EFI_TIMEOUT            Timeout.
  @retval EFI_INVALID_PARAMETER  Invalid Parameters.
**/
EFI_STATUS
SmmuV3TLBInvalidateAll (
  IN SMMU_INFO  *SmmuInfo
  )
{
  SMMUV3_CMD_GENERIC  Command;
  EFI_STATUS          Status;

  if (SmmuInfo == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameters\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  // Invalidate TLBI Commands
  SMMUV3_BUILD_CMD_TLBI_NSNH_ALL (&Command);
  Status = SmmuV3SendCommand (SmmuInfo, &Command);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: CMD_TLBI_NSNH_ALL failed.\n", __func__));
    return Status;
  }

  // Issue a CMD_SYNC command to guarantee that any previously issued TLB
  // invalidations (CMD_TLBI_*) are completed (SMMUv3.2 spec section 4.6.3).
  SMMUV3_BUILD_CMD_SYNC_NO_INTERRUPT (&Command);
  Status = SmmuV3SendCommand (SmmuInfo, &Command);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: CMD_SYNC_NO_INTERRUPT failed.\n", __func__));
    return Status;
  }

  ArmDataSynchronizationBarrier ();

  return Status;
}

/**
  Invalidate TLB entries for specified InputAddress for Stage 2 of SmmuV3.

  @param [in]  SmmuInfo      Pointer to the SMMU_INFO structure.
  @param [in]  InputAddress  The input address to invalidate.

  @retval EFI_SUCCESS            Success.
  @retval EFI_TIMEOUT            Timeout.
  @retval EFI_INVALID_PARAMETER  Invalid Parameters.
**/
EFI_STATUS
SmmuV3TLBInvalidateAddress (
  IN SMMU_INFO  *SmmuInfo,
  IN UINT64     InputAddress
  )
{
  SMMUV3_CMD_GENERIC  Command;
  EFI_STATUS          Status;

  if (SmmuInfo == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameters\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  // Invalidate with TLBI_S2_IPA Commands
  SMMUV3_BUILD_CMD_TLBI_S2_IPA (&Command, SMMUV3_STREAM_TABLE_ENTRY_S2VMID, InputAddress);
  Status = SmmuV3SendCommand (SmmuInfo, &Command);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: CMD_TLBI_S2_IPA failed.\n", __func__));
    return Status;
  }

  // Issue a CMD_SYNC command to guarantee that any previously issued TLB
  // invalidations (CMD_TLBI_*) are completed (SMMUv3.2 spec section 4.6.3).
  SMMUV3_BUILD_CMD_SYNC_NO_INTERRUPT (&Command);
  Status = SmmuV3SendCommand (SmmuInfo, &Command);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: CMD_SYNC_NO_INTERRUPT failed.\n", __func__));
    return Status;
  }

  ArmDataSynchronizationBarrier ();

  return Status;
}

/**
 * Get SMMUV3 node information from the IORT table.
 *
 * @param [in]  IortTable      Pointer to the IORT table.
 * @param [out] SmmuInfoArray  Pointer to the array of SMMU_INFO structures.
 * @param [out] SmmuNodePtrs   Pointer to the array of SMMU node pointers.
 *
 * @retval EFI_SUCCESS            Success.
 * @retval EFI_INVALID_PARAMETER  Invalid Parameters.
 *
 */
EFI_STATUS
SmmuV3GetNodeInfo (
  IN  VOID       *IortTable,
  OUT SMMU_INFO  *SmmuInfoArray,
  OUT VOID       **SmmuNodePtrs
  )
{
  EFI_ACPI_6_0_IO_REMAPPING_TABLE       *Iort;
  EFI_ACPI_6_0_IO_REMAPPING_NODE        *Node;
  EFI_ACPI_6_0_IO_REMAPPING_SMMU3_NODE  *SmmuNode;
  UINT32                                SmmuIndex;
  UINT32                                Count;

  if ((IortTable == NULL) || (SmmuInfoArray == NULL) || (SmmuNodePtrs == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameters\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Iort      = (EFI_ACPI_6_0_IO_REMAPPING_TABLE *)IortTable;
  Node      = (EFI_ACPI_6_0_IO_REMAPPING_NODE *)((UINT8 *)Iort + Iort->NodeOffset);
  SmmuIndex = 0;

  for (Count = 0; Count < Iort->NumNodes; Count++) {
    if (Node->Type == EFI_ACPI_IORT_TYPE_SMMUv3) {
      InitializeListHead (&SmmuInfoArray[SmmuIndex].RmrNodeList);
      SmmuNode                                     = (EFI_ACPI_6_0_IO_REMAPPING_SMMU3_NODE *)Node;
      SmmuInfoArray[SmmuIndex].SmmuBase            = SmmuNode->Base;
      SmmuInfoArray[SmmuIndex].Flags               = SmmuNode->Flags;
      SmmuInfoArray[SmmuIndex].StreamTableEntryMax = 0;    // Initialize max stream ID to 0
      SmmuInfoArray[SmmuIndex].EBSBehaviorAbort    = TRUE; // Initialize EBS behavior to Abort by default
      SmmuNodePtrs[SmmuIndex]                      = (VOID *)SmmuNode;
      SmmuIndex++;
    }

    // Move to the next node
    Node = (EFI_ACPI_6_0_IO_REMAPPING_NODE *)((UINT8 *)Node + Node->Length);
  }

  return EFI_SUCCESS;
}

/**
 * Get the number of SMMUV3 nodes in the IORT table.
 *
 * @param [in]  IortTable      Pointer to the IORT table.
 * @param [out] SmmuNodeCount  Pointer to store the number of SMMU nodes found.
 *
 * @retval EFI_SUCCESS            Success.
 * @retval EFI_INVALID_PARAMETER  Invalid Parameters.
 * @retval EFI_NOT_FOUND          IORT table not found.
 */
EFI_STATUS
SmmuV3NodeCount (
  IN  VOID    *IortTable,
  OUT UINT32  *SmmuNodeCount
  )
{
  EFI_ACPI_6_0_IO_REMAPPING_TABLE  *Iort;
  EFI_ACPI_6_0_IO_REMAPPING_NODE   *Node;
  UINT32                           Counter;

  if ((IortTable == NULL) || (SmmuNodeCount == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameters\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  // Cast the void* to the proper IORT structure
  Iort = (EFI_ACPI_6_0_IO_REMAPPING_TABLE *)IortTable;
  if (Iort == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: NULL IORT table\n", __func__));
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_VERBOSE, "%a: IORT contains %d nodes\n", __func__, Iort->NumNodes));

  // First pass: count SMMU nodes
  *SmmuNodeCount = 0;
  Node           = (EFI_ACPI_6_0_IO_REMAPPING_NODE *)((UINT8 *)Iort + Iort->NodeOffset);

  for (Counter = 0; Counter < Iort->NumNodes; Counter++) {
    if (Node->Type == EFI_ACPI_IORT_TYPE_SMMUv3) {
      (*SmmuNodeCount)++;
    }

    // Move to the next node
    Node = (EFI_ACPI_6_0_IO_REMAPPING_NODE *)((UINT8 *)Node + Node->Length);
  }

  DEBUG ((DEBUG_VERBOSE, "%a: Found %d SMMU nodes\n", __func__, *SmmuNodeCount));

  return EFI_SUCCESS;
}

/*
* Get the max stream ID for each SMMU.
*
* @param [in]  IortTable      Pointer to the IORT table.
* @param [in]  SmmuNodePtrs   Pointer to the array of SMMU node pointers.
* @param [in]  SmmuNodeCount  Number of SMMU nodes.
* @param [out] SmmuInfoArray  Pointer to the array of SMMU_INFO structures.
*
*
* @retval EFI_SUCCESS            Success.
* @retval EFI_INVALID_PARAMETER  Invalid Parameters.
* @retval EFI_NOT_FOUND          SMMU node not found.
*/
EFI_STATUS
SmmuV3GetMaxStreamIds (
  IN  VOID       *IortTable,
  IN  VOID       **SmmuNodePtrs,
  IN  UINT32     SmmuNodeCount,
  OUT SMMU_INFO  *SmmuInfoArray
  )
{
  EFI_ACPI_6_0_IO_REMAPPING_TABLE     *Iort;
  EFI_ACPI_6_0_IO_REMAPPING_NODE      *Node;
  EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE  *IdMapping;
  VOID                                *OutputNode;
  UINT32                              ByteOffset;
  BOOLEAN                             Found;
  UINT32                              Count;
  UINT32                              IdMappingIndex;
  UINT32                              SmmuIndex;
  UINT32                              CurMaxMappingStreamId;

  if ((IortTable == NULL) || (SmmuNodePtrs == NULL) || (SmmuInfoArray == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameters\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Iort = (EFI_ACPI_6_0_IO_REMAPPING_TABLE *)IortTable;
  Node = (EFI_ACPI_6_0_IO_REMAPPING_NODE *)((UINT8 *)Iort + Iort->NodeOffset);

  for (Count = 0; Count < Iort->NumNodes; Count++) {
    if (((Node->Type == EFI_ACPI_IORT_TYPE_ROOT_COMPLEX) || (Node->Type == EFI_ACPI_IORT_TYPE_NAMED_COMP)) && (Node->NumIdMappings > 0)) {
      // Get the ID mapping array
      IdMapping = (EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE *)((UINT8 *)Node + Node->IdReference);

      for (IdMappingIndex = 0; IdMappingIndex < Node->NumIdMappings; IdMappingIndex++) {
        // Calculate the absolute offset of the output reference
        ByteOffset = IdMapping[IdMappingIndex].OutputReference;
        OutputNode = (VOID *)((UINT8 *)Iort + ByteOffset);

        // Check if the output reference points to an SMMU node
        Found = FALSE;
        for (SmmuIndex = 0; SmmuIndex < SmmuNodeCount; SmmuIndex++) {
          if (OutputNode == SmmuNodePtrs[SmmuIndex]) {
            // This ID mapping references an SMMU node
            // Calculate the max Stream ID for this mapping: OutputBase + NumIds
            CurMaxMappingStreamId = IdMapping[IdMappingIndex].OutputBase + IdMapping[IdMappingIndex].NumIds;

            // Update MaxStreamId if this mapping has a higher value
            if (CurMaxMappingStreamId > SmmuInfoArray[SmmuIndex].StreamTableEntryMax) {
              SmmuInfoArray[SmmuIndex].StreamTableEntryMax = CurMaxMappingStreamId;
              DEBUG ((
                DEBUG_VERBOSE,
                "%a: Updated MaxStreamId for SMMU[0x%llx] to 0x%x (from mapping: InputBase=0x%x, NumIds=0x%x, OutputBase=0x%x)\n",
                __func__,
                SmmuInfoArray[SmmuIndex].SmmuBase,
                SmmuInfoArray[SmmuIndex].StreamTableEntryMax,
                IdMapping[IdMappingIndex].InputBase,
                IdMapping[IdMappingIndex].NumIds,
                IdMapping[IdMappingIndex].OutputBase
                ));
            }

            Found = TRUE;
            break;
          }
        }

        if (!Found) {
          DEBUG ((
            DEBUG_ERROR,
            "%a: ID mapping references a non-SMMU node (offset: 0x%x)\n",
            __func__,
            ByteOffset
            ));
          return EFI_NOT_FOUND;
        }
      }
    }

    // Move to the next node
    Node = (EFI_ACPI_6_0_IO_REMAPPING_NODE *)((UINT8 *)Node + Node->Length);
  }

  return EFI_SUCCESS;
}

/**
 * Add a new RMR node to the SMMU_INFO structure's RMR node list.
 *
 * @param [in] SmmuInfo  Pointer to the SMMU_INFO structure.
 * @param [in] RmrNode   Pointer to the RMR node to add.
 *
 * @return EFI_SUCCESS on success, or EFI_OUT_OF_RESOURCES on failure.
 */
EFI_STATUS
SmmuV3AddRmrNodeToList (
  IN SMMU_INFO                           *SmmuInfo,
  IN EFI_ACPI_6_0_IO_REMAPPING_RMR_NODE  *RmrNode
  )
{
  RMR_NODE_INFO  *Item;

  Item = AllocateZeroPool (sizeof (RMR_NODE_INFO));
  if (Item == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate memory for RMR_NODE_INFO\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  Item->RmrNode = RmrNode;
  InsertTailList (&SmmuInfo->RmrNodeList, &Item->Link);
  return EFI_SUCCESS;
}

/**
 * Collect RMR Node information for each SMMU and add it to the RmrNodeList
 *
 * @param [in]  IortTable      Pointer to the IORT table.
 * @param [in]  SmmuNodePtrs   Pointer to the array of SMMU node pointers.
 * @param [in]  SmmuNodeCount  Number of SMMU nodes.
 * @param [out] SmmuInfoArray  Pointer to the array of SMMU_INFO structures.
 *
 * @retval EFI_SUCCESS            Success.
 * @retval EFI_INVALID_PARAMETER  Invalid Parameters.
 * @retval EFI_NOT_FOUND          SMMU node not found.
 */
EFI_STATUS
SmmuV3GetRMRNodeInfo (
  IN  VOID       *IortTable,
  IN  VOID       **SmmuNodePtrs,
  IN  UINT32     SmmuNodeCount,
  OUT SMMU_INFO  *SmmuInfoArray
  )
{
  EFI_ACPI_6_0_IO_REMAPPING_TABLE     *Iort;
  EFI_ACPI_6_0_IO_REMAPPING_NODE      *Node;
  EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE  *IdMapping;
  VOID                                *OutputNode;
  UINT32                              ByteOffset;
  UINT32                              SmmuIndex;
  UINT32                              IdMappingIndex;
  UINT32                              Count;
  BOOLEAN                             Found;
  EFI_STATUS                          Status;

  if ((IortTable == NULL) || (SmmuNodePtrs == NULL) || (SmmuInfoArray == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameters\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Iort = (EFI_ACPI_6_0_IO_REMAPPING_TABLE *)IortTable;
  Node = (EFI_ACPI_6_0_IO_REMAPPING_NODE *)((UINT8 *)Iort + Iort->NodeOffset);

  for (Count = 0; Count < Iort->NumNodes; Count++) {
    if (Node->Type == EFI_ACPI_IORT_TYPE_RMR) {
      if (Node->NumIdMappings > 0) {
        // Get the ID mapping array
        IdMapping = (EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE *)((UINT8 *)Node + Node->IdReference);

        for (IdMappingIndex = 0; IdMappingIndex < Node->NumIdMappings; IdMappingIndex++) {
          // Calculate the absolute offset of the output reference
          ByteOffset = IdMapping[IdMappingIndex].OutputReference;
          OutputNode = (VOID *)((UINT8 *)Iort + ByteOffset);

          // Check if the output reference points to an SMMU node
          Found = FALSE;
          for (SmmuIndex = 0; SmmuIndex < SmmuNodeCount; SmmuIndex++) {
            if (OutputNode == SmmuNodePtrs[SmmuIndex]) {
              // This ID mapping references an SMMU node
              // If RMR Node store the RMR node pointer for this SMMU
              Status = SmmuV3AddRmrNodeToList (&SmmuInfoArray[SmmuIndex], (EFI_ACPI_6_0_IO_REMAPPING_RMR_NODE *)Node);
              if (EFI_ERROR (Status)) {
                DEBUG ((DEBUG_ERROR, "%a: Failed to add RMR node to list for SMMU[0x%llx]\n", __func__, SmmuInfoArray[SmmuIndex].SmmuBase));
                return Status;
              }

              Found = TRUE;
              break;
            }
          }

          if (!Found) {
            DEBUG ((
              DEBUG_ERROR,
              "%a: ID mapping references a non-SMMU node (offset: 0x%x)\n",
              __func__,
              ByteOffset
              ));
            return EFI_NOT_FOUND;
          }
        }
      }
    }

    // Move to the next node
    Node = (EFI_ACPI_6_0_IO_REMAPPING_NODE *)((UINT8 *)Node + Node->Length);
  }

  return EFI_SUCCESS;
}

/**
 * Add RMR mappings for each SMMU node in the SmmuInfo structure.
 * This function iterates through the RMR nodes and updates the page table
 * for each memory range described in the RMR node.
 *
 * @param [in] SmmuInfo  Pointer to the SMMU_INFO structure.
 *
 * @retval EFI_SUCCESS            Success.
 * @retval EFI_INVALID_PARAMETER  Invalid Parameters.
 * @retval Other                  RMR mapping update failure.
 */
EFI_STATUS
SmmuV3AddRMRMapping (
  IN SMMU_INFO  *SmmuInfo
  )
{
  EFI_ACPI_6_0_IO_REMAPPING_MEM_RANGE_DESC  *IortMemRangeDesc;
  UINT32                                    NumMemRangeDesc;
  LIST_ENTRY                                *Entry;
  RMR_NODE_INFO                             *Item;
  EFI_STATUS                                Status;

  if (SmmuInfo == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameters\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Entry = GetFirstNode (&SmmuInfo->RmrNodeList);
  while (!IsNull (&SmmuInfo->RmrNodeList, Entry)) {
    Item  = BASE_CR (Entry, RMR_NODE_INFO, Link);
    Entry = GetNextNode (&SmmuInfo->RmrNodeList, Entry);

    if ((Item != NULL) && (Item->RmrNode != NULL)) {
      for (NumMemRangeDesc = 0; NumMemRangeDesc < Item->RmrNode->NumMemRangeDesc; NumMemRangeDesc++) {
        IortMemRangeDesc = (EFI_ACPI_6_0_IO_REMAPPING_MEM_RANGE_DESC *)((UINT8 *)Item->RmrNode + Item->RmrNode->MemRangeDescRef);
        if ((IortMemRangeDesc[NumMemRangeDesc].Base > 0) && (IortMemRangeDesc[NumMemRangeDesc].Length > 0)) {
          SmmuInfo->EBSBehaviorAbort = FALSE; // At least one RMR mapping exists, set EBS behavior to bypass
          DEBUG ((
            DEBUG_INFO,
            "%a: Adding RMR mapping for SMMU[0x%llx]: Base=0x%llx, Length=0x%llx\n",
            __func__,
            SmmuInfo->SmmuBase,
            IortMemRangeDesc[NumMemRangeDesc].Base,
            IortMemRangeDesc[NumMemRangeDesc].Length
            ));
          Status = UpdatePageTable (
                     SmmuInfo->PageTableRoot,
                     IortMemRangeDesc[NumMemRangeDesc].Base,
                     IortMemRangeDesc[NumMemRangeDesc].Length,
                     PAGE_TABLE_READ_WRITE_FROM_IOMMU_ACCESS ((EDKII_IOMMU_ACCESS_READ | EDKII_IOMMU_ACCESS_WRITE)),
                     TRUE
                     );
          if (EFI_ERROR (Status)) {
            DEBUG ((DEBUG_ERROR, "%a: Failed to update RMR mapping.\n", __func__));
            return Status;
          }
        }
      }

      RemoveEntryList (&Item->Link);
      FreePool (Item);
    }
  }

  return EFI_SUCCESS;
}

/**
 * Parse IORT table and extract SMMU information
 *
 * @param[in]  IortTable    Pointer to the IORT table
 * @param[out] SmmuInfo     Pointer to store the array of SMMU_INFO structures
 * @param[out] SmmuCount    Pointer to store the number of SMMU nodes found
 *
 * @return EFI_SUCCESS on success
 * @return EFI_INVALID_PARAMETER if any parameter is NULL
 * @return EFI_OUT_OF_RESOURCES if memory allocation fails
 * @return EFI_NOT_FOUND if no SMMU nodes are found
 * @return EFI_UNSUPPORTED if the IORT table is not supported
 */
EFI_STATUS
SmmuV3ParseIort (
  IN  VOID       *IortTable,
  OUT SMMU_INFO  **SmmuInfo,
  OUT UINT32     *SmmuCount
  )
{
  EFI_STATUS                       Status;
  EFI_ACPI_6_0_IO_REMAPPING_TABLE  *Iort;
  SMMU_INFO                        *SmmuInfoArray;
  VOID                             **SmmuNodePtrs;
  UINT32                           SmmuNodeCount;

  if ((IortTable == NULL) || (SmmuInfo == NULL) || (SmmuCount == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameters\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  SmmuInfoArray = NULL;
  SmmuNodePtrs  = NULL;

  // Cast the void* to the IORT structure
  Iort = (EFI_ACPI_6_0_IO_REMAPPING_TABLE *)IortTable;

  // Verify IORT signature
  if (Iort->Header.Signature != EFI_ACPI_6_0_IO_REMAPPING_TABLE_SIGNATURE) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid IORT signature: 0x%08X, expected: 0x%08X\n",
      __func__,
      Iort->Header.Signature,
      EFI_ACPI_6_0_IO_REMAPPING_TABLE_SIGNATURE
      ));
    return EFI_UNSUPPORTED;
  }

  if ((Iort->Header.Revision != EFI_ACPI_IO_REMAPPING_TABLE_REVISION_00) && (Iort->Header.Revision != EFI_ACPI_IO_REMAPPING_TABLE_REVISION_06)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Unsupported IORT revision: %d, expected: [%d, %d]\n",
      __func__,
      Iort->Header.Revision,
      EFI_ACPI_IO_REMAPPING_TABLE_REVISION_00,
      EFI_ACPI_IO_REMAPPING_TABLE_REVISION_06
      ));
    return EFI_UNSUPPORTED;
  }

  // First pass: get the number of SMMU nodes
  Status = SmmuV3NodeCount (IortTable, &SmmuNodeCount);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get IORT node count\n", __func__));
    return Status;
  }

  if (SmmuNodeCount == 0) {
    *SmmuCount = 0;
    *SmmuInfo  = NULL;
    return EFI_NOT_FOUND;
  }

  // Allocate memory for SMMU info array
  SmmuInfoArray = AllocateZeroPool (SmmuNodeCount * sizeof (SMMU_INFO));
  if (SmmuInfoArray == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate memory for SMMU info array\n", __func__));
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  // Allocate memory for SMMU node pointers (for output reference lookup)
  SmmuNodePtrs = AllocateZeroPool (SmmuNodeCount * sizeof (VOID *));
  if (SmmuNodePtrs == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate memory for SMMU node pointers\n", __func__));
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  // Second pass: collect SMMU information
  Status = SmmuV3GetNodeInfo (IortTable, SmmuInfoArray, SmmuNodePtrs);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get SMMU node info\n", __func__));
    goto Error;
  }

  // Third pass: calculate max Stream ID for each SMMU node
  Status = SmmuV3GetMaxStreamIds (IortTable, SmmuNodePtrs, SmmuNodeCount, SmmuInfoArray);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get max Stream ID for SMMU nodes\n", __func__));
    goto Error;
  }

  // Fourth pass: collect per Stream ID range info like CCA, CPM, DACS for each RC/NamedComp node
  Status = SmmuV3GetRMRNodeInfo (IortTable, SmmuNodePtrs, SmmuNodeCount, SmmuInfoArray);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get Stream ID info for SMMU nodes\n", __func__));
    goto Error;
  }

  FreePool (SmmuNodePtrs);
  *SmmuInfo  = SmmuInfoArray;
  *SmmuCount = SmmuNodeCount;
  return Status;

Error:
  if (SmmuInfoArray != NULL) {
    FreePool (SmmuInfoArray);
  }

  if (SmmuNodePtrs != NULL) {
    FreePool (SmmuNodePtrs);
  }

  return Status;
}
