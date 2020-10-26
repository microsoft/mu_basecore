/** @file
  NvmExpressDxe driver is used to manage non-volatile memory subsystem which follows
  NVM Express specification.

  Copyright (c) 2013 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "NvmExpress.h"
#include <Guid/NVMeEventGroup.h>

#define NVME_SHUTDOWN_PROCESS_TIMEOUT  45

//
// The number of NVME controllers managed by this driver, used by
// NvmeRegisterShutdownNotification() and NvmeUnregisterShutdownNotification().
//
UINTN  mNvmeControllerNumber = 0;

/**
  Read Nvm Express controller capability register.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param  Cap              The buffer used to store capability register content.

  @return EFI_SUCCESS      Successfully read the controller capability register content.
  @return EFI_DEVICE_ERROR Fail to read the controller capability register.

**/
EFI_STATUS
ReadNvmeControllerCapabilities (
  // MU_CHANGE [BEGIN] - Correct Cap parameter modifier
  IN  NVME_CONTROLLER_PRIVATE_DATA  *Private,
  OUT NVME_CAP                      *Cap
  // MU_CHANGE [END] - Correct Cap parameter modifier
  )
{
  EFI_PCI_IO_PROTOCOL  *PciIo;
  EFI_STATUS           Status;
  UINT64               Data;

  PciIo  = Private->PciIo;
  Status = PciIo->Mem.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        NVME_BAR,
                        NVME_CAP_OFFSET,
                        2,
                        &Data
                        );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  WriteUnaligned64 ((UINT64 *)Cap, Data);
  return EFI_SUCCESS;
}

/**
  Read Nvm Express controller configuration register.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param  Cc               The buffer used to store configuration register content.

  @return EFI_SUCCESS      Successfully read the controller configuration register content.
  @return EFI_DEVICE_ERROR Fail to read the controller configuration register.

**/
EFI_STATUS
ReadNvmeControllerConfiguration (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private,
  IN NVME_CC                       *Cc
  )
{
  EFI_PCI_IO_PROTOCOL  *PciIo;
  EFI_STATUS           Status;
  UINT32               Data;

  PciIo  = Private->PciIo;
  Status = PciIo->Mem.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        NVME_BAR,
                        NVME_CC_OFFSET,
                        1,
                        &Data
                        );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  WriteUnaligned32 ((UINT32 *)Cc, Data);
  return EFI_SUCCESS;
}

/**
  Write Nvm Express controller configuration register.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param  Cc               The buffer used to store the content to be written into configuration register.

  @return EFI_SUCCESS      Successfully write data into the controller configuration register.
  @return EFI_DEVICE_ERROR Fail to write data into the controller configuration register.

**/
EFI_STATUS
WriteNvmeControllerConfiguration (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private,
  IN NVME_CC                       *Cc
  )
{
  EFI_PCI_IO_PROTOCOL  *PciIo;
  EFI_STATUS           Status;
  UINT32               Data;

  PciIo  = Private->PciIo;
  Data   = ReadUnaligned32 ((UINT32 *)Cc);
  Status = PciIo->Mem.Write (
                        PciIo,
                        EfiPciIoWidthUint32,
                        NVME_BAR,
                        NVME_CC_OFFSET,
                        1,
                        &Data
                        );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, "Cc.En: %d\n", Cc->En));
  DEBUG ((DEBUG_INFO, "Cc.Css: %d\n", Cc->Css));
  DEBUG ((DEBUG_INFO, "Cc.Mps: %d\n", Cc->Mps));
  DEBUG ((DEBUG_INFO, "Cc.Ams: %d\n", Cc->Ams));
  DEBUG ((DEBUG_INFO, "Cc.Shn: %d\n", Cc->Shn));
  DEBUG ((DEBUG_INFO, "Cc.Iosqes: %d\n", Cc->Iosqes));
  DEBUG ((DEBUG_INFO, "Cc.Iocqes: %d\n", Cc->Iocqes));

  return EFI_SUCCESS;
}

/**
  Read Nvm Express controller status register.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param  Csts             The buffer used to store status register content.

  @return EFI_SUCCESS      Successfully read the controller status register content.
  @return EFI_DEVICE_ERROR Fail to read the controller status register.

**/
EFI_STATUS
ReadNvmeControllerStatus (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private,
  IN NVME_CSTS                     *Csts
  )
{
  EFI_PCI_IO_PROTOCOL  *PciIo;
  EFI_STATUS           Status;
  UINT32               Data;

  PciIo  = Private->PciIo;
  Status = PciIo->Mem.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        NVME_BAR,
                        NVME_CSTS_OFFSET,
                        1,
                        &Data
                        );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  WriteUnaligned32 ((UINT32 *)Csts, Data);
  return EFI_SUCCESS;
}

// MU_CHANGE [BEGIN] - Allocate IO Queue Buffer

/**
  Read Nvm Express admin queue attributes register.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param  Aqa              The buffer used to store the content to be read from admin queue attributes register.

  @return EFI_SUCCESS      Successfully read data from the admin queue attributes register.
  @return EFI_DEVICE_ERROR Fail to read data from the admin queue attributes register.

**/
EFI_STATUS
ReadNvmeAdminQueueAttributes (
  IN  NVME_CONTROLLER_PRIVATE_DATA  *Private,
  OUT NVME_AQA                      *Aqa
  )
{
  EFI_PCI_IO_PROTOCOL  *PciIo;
  EFI_STATUS           Status;
  UINT32               Data;

  PciIo  = Private->PciIo;
  Status = PciIo->Mem.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        NVME_BAR,
                        NVME_AQA_OFFSET,
                        1,
                        &Data
                        );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  WriteUnaligned32 ((UINT32 *)Aqa, Data);

  DEBUG ((DEBUG_INFO, "%a: Admin Submission Queue Size (Number of Entries): %d\n", __func__, Aqa->Asqs));
  DEBUG ((DEBUG_INFO, "%a: Admin Completion Queue Size (Number of Entries): %d\n", __func__, Aqa->Acqs));

  return EFI_SUCCESS;
}

// MU_CHANGE [END] - Allocate IO Queue Buffer

/**
  Write Nvm Express admin queue attributes register.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param  Aqa              The buffer used to store the content to be written into admin queue attributes register.

  @return EFI_SUCCESS      Successfully write data into the admin queue attributes register.
  @return EFI_DEVICE_ERROR Fail to write data into the admin queue attributes register.

**/
EFI_STATUS
WriteNvmeAdminQueueAttributes (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private,
  IN NVME_AQA                      *Aqa
  )
{
  EFI_PCI_IO_PROTOCOL  *PciIo;
  EFI_STATUS           Status;
  UINT32               Data;

  // MU_CHANGE [BEGIN] - Allocate IO Queue Buffer
  //
  // Save Aqa to Private data for later use.
  //
  Private->SqData[0].NumberOfEntries = Aqa->Asqs;
  Private->CqData[0].NumberOfEntries = Aqa->Acqs;
  // MU_CHANGE [END] - Allocate IO Queue Buffer

  PciIo  = Private->PciIo;
  Data   = ReadUnaligned32 ((UINT32 *)Aqa);
  Status = PciIo->Mem.Write (
                        PciIo,
                        EfiPciIoWidthUint32,
                        NVME_BAR,
                        NVME_AQA_OFFSET,
                        1,
                        &Data
                        );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, "Aqa.Asqs: %d\n", Aqa->Asqs));
  DEBUG ((DEBUG_INFO, "Aqa.Acqs: %d\n", Aqa->Acqs));

  return EFI_SUCCESS;
}

/**
  Write Nvm Express admin submission queue base address register.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param  Asq              The buffer used to store the content to be written into admin submission queue base address register.

  @return EFI_SUCCESS      Successfully write data into the admin submission queue base address register.
  @return EFI_DEVICE_ERROR Fail to write data into the admin submission queue base address register.

**/
EFI_STATUS
WriteNvmeAdminSubmissionQueueBaseAddress (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private,
  IN NVME_ASQ                      *Asq
  )
{
  EFI_PCI_IO_PROTOCOL  *PciIo;
  EFI_STATUS           Status;
  UINT64               Data;

  PciIo = Private->PciIo;
  Data  = ReadUnaligned64 ((UINT64 *)Asq);

  Status = PciIo->Mem.Write (
                        PciIo,
                        EfiPciIoWidthUint32,
                        NVME_BAR,
                        NVME_ASQ_OFFSET,
                        2,
                        &Data
                        );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, "Asq: %lx\n", *Asq));

  return EFI_SUCCESS;
}

/**
  Write Nvm Express admin completion queue base address register.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param  Acq              The buffer used to store the content to be written into admin completion queue base address register.

  @return EFI_SUCCESS      Successfully write data into the admin completion queue base address register.
  @return EFI_DEVICE_ERROR Fail to write data into the admin completion queue base address register.

**/
EFI_STATUS
WriteNvmeAdminCompletionQueueBaseAddress (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private,
  IN NVME_ACQ                      *Acq
  )
{
  EFI_PCI_IO_PROTOCOL  *PciIo;
  EFI_STATUS           Status;
  UINT64               Data;

  PciIo = Private->PciIo;
  Data  = ReadUnaligned64 ((UINT64 *)Acq);

  Status = PciIo->Mem.Write (
                        PciIo,
                        EfiPciIoWidthUint32,
                        NVME_BAR,
                        NVME_ACQ_OFFSET,
                        2,
                        &Data
                        );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, "Acq: %lxh\n", *Acq));

  return EFI_SUCCESS;
}

/**
  Disable the Nvm Express controller.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.

  @return EFI_SUCCESS      Successfully disable the controller.
  @return EFI_DEVICE_ERROR Fail to disable the controller.

**/
EFI_STATUS
NvmeDisableController (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private
  )
{
  NVME_CC     Cc;
  NVME_CSTS   Csts;
  EFI_STATUS  Status;
  UINT32      Index;
  UINT8       Timeout;

  //
  // Read Controller Configuration Register.
  //
  Status = ReadNvmeControllerConfiguration (Private, &Cc);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Cc.En = 0;

  //
  // Disable the controller.
  //
  Status = WriteNvmeControllerConfiguration (Private, &Cc);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Cap.To specifies max delay time in 500ms increments for Csts.Rdy to transition from 1 to 0 after
  // Cc.Enable transition from 1 to 0. Loop produces a 1 millisecond delay per itteration, up to 500 * Cap.To.
  //
  if (Private->Cap.To == 0) {
    Timeout = 1;
  } else {
    Timeout = Private->Cap.To;
  }

  for (Index = (Timeout * 500); Index != 0; --Index) {
    gBS->Stall (1000);

    //
    // Check if the controller is initialized
    //
    Status = ReadNvmeControllerStatus (Private, &Csts);

    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (Csts.Rdy == 0) {
      break;
    }
  }

  if (Index == 0) {
    Status = EFI_DEVICE_ERROR;
    REPORT_STATUS_CODE (
      (EFI_ERROR_CODE | EFI_ERROR_MAJOR),
      (EFI_IO_BUS_SCSI | EFI_IOB_EC_INTERFACE_ERROR)
      );
  }

  DEBUG ((DEBUG_INFO, "NVMe controller is disabled with status [%r].\n", Status));
  return Status;
}

// MU_CHANGE [BEGIN] - Allocate IO Queue Buffer

/**
  Enable the Nvm Express controller. Allocate and write the Controller Configuration data.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param  IoSqEs           The I/O Submission Queue Entry Size.
  @param  IoCqEs           The I/O Completion Queue Entry Size.

  @return EFI_SUCCESS      Successfully enable the controller.
  @return EFI_DEVICE_ERROR Fail to enable the controller.
  @return EFI_TIMEOUT      Fail to enable the controller in given time slot.

**/
EFI_STATUS
NvmeEnableController (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private,
  IN UINT8                         IoSqEs,
  IN UINT8                         IoCqEs
  )
{
  // MU_CHANGE [END] - Allocate IO Queue Buffer
  NVME_CC     Cc;
  NVME_CSTS   Csts;
  EFI_STATUS  Status;
  UINT32      Index;
  UINT8       Timeout;

  EfiEventGroupSignal (&gNVMeEnableStartEventGroupGuid);

  //
  // Enable the controller.
  // CC.AMS, CC.MPS and CC.CSS are all set to 0.
  //
  ZeroMem (&Cc, sizeof (NVME_CC));
  // MU_CHANGE [BEGIN] - Allocate IO Queue Buffer
  Cc.En     = 1;
  Cc.Iosqes = IoSqEs;
  Cc.Iocqes = IoCqEs;
  // MU_CHANGE [END] - Allocate IO Queue Buffer

  Status = WriteNvmeControllerConfiguration (Private, &Cc);
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  //
  // Cap.To specifies max delay time in 500ms increments for Csts.Rdy to set after
  // Cc.Enable. Loop produces a 1 millisecond delay per itteration, up to 500 * Cap.To.
  //
  if (Private->Cap.To == 0) {
    Timeout = 1;
  } else {
    Timeout = Private->Cap.To;
  }

  for (Index = (Timeout * 500); Index != 0; --Index) {
    gBS->Stall (1000);

    //
    // Check if the controller is initialized
    //
    Status = ReadNvmeControllerStatus (Private, &Csts);

    if (EFI_ERROR (Status)) {
      goto Cleanup;
    }

    if (Csts.Rdy) {
      break;
    }
  }

  if (Index == 0) {
    Status = EFI_TIMEOUT;
    REPORT_STATUS_CODE (
      (EFI_ERROR_CODE | EFI_ERROR_MAJOR),
      (EFI_IO_BUS_SCSI | EFI_IOB_EC_INTERFACE_ERROR)
      );
  }

  DEBUG ((DEBUG_INFO, "NVMe controller is enabled with status [%r].\n", Status));

Cleanup:
  EfiEventGroupSignal (&gNVMeEnableCompleteEventGroupGuid);
  return Status;
}

/**
  Get identify controller data.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param  Buffer           The buffer used to store the identify controller data.

  @return EFI_SUCCESS      Successfully get the identify controller data.
  @return EFI_DEVICE_ERROR Fail to get the identify controller data.

**/
EFI_STATUS
NvmeIdentifyController (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private,
  IN VOID                          *Buffer
  )
{
  EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET  CommandPacket;
  EFI_NVM_EXPRESS_COMMAND                   Command;
  EFI_NVM_EXPRESS_COMPLETION                Completion;
  EFI_STATUS                                Status;

  ZeroMem (&CommandPacket, sizeof (EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof (EFI_NVM_EXPRESS_COMMAND));
  ZeroMem (&Completion, sizeof (EFI_NVM_EXPRESS_COMPLETION));

  Command.Cdw0.Opcode = NVME_ADMIN_IDENTIFY_CMD;
  //
  // According to Nvm Express 1.1 spec Figure 38, When not used, the field shall be cleared to 0h.
  // For the Identify command, the Namespace Identifier is only used for the Namespace data structure.
  //
  Command.Nsid = 0;

  CommandPacket.NvmeCmd        = &Command;
  CommandPacket.NvmeCompletion = &Completion;
  CommandPacket.TransferBuffer = Buffer;
  CommandPacket.TransferLength = sizeof (NVME_ADMIN_CONTROLLER_DATA);
  CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
  CommandPacket.QueueType      = NVME_ADMIN_QUEUE;
  //
  // Set bit 0 (Cns bit) to 1 to identify a controller
  //
  Command.Cdw10 = 1;
  Command.Flags = CDW10_VALID;

  Status = Private->Passthru.PassThru (
                               &Private->Passthru,
                               NVME_CONTROLLER_ID,
                               &CommandPacket,
                               NULL
                               );

  return Status;
}

/**
  Get specified identify namespace data.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param  NamespaceId      The specified namespace identifier.
  @param  Buffer           The buffer used to store the identify namespace data.

  @return EFI_SUCCESS      Successfully get the identify namespace data.
  @return EFI_DEVICE_ERROR Fail to get the identify namespace data.

**/
EFI_STATUS
NvmeIdentifyNamespace (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private,
  IN UINT32                        NamespaceId,
  IN VOID                          *Buffer
  )
{
  EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET  CommandPacket;
  EFI_NVM_EXPRESS_COMMAND                   Command;
  EFI_NVM_EXPRESS_COMPLETION                Completion;
  EFI_STATUS                                Status;

  ZeroMem (&CommandPacket, sizeof (EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof (EFI_NVM_EXPRESS_COMMAND));
  ZeroMem (&Completion, sizeof (EFI_NVM_EXPRESS_COMPLETION));

  CommandPacket.NvmeCmd        = &Command;
  CommandPacket.NvmeCompletion = &Completion;

  Command.Cdw0.Opcode          = NVME_ADMIN_IDENTIFY_CMD;
  Command.Nsid                 = NamespaceId;
  CommandPacket.TransferBuffer = Buffer;
  CommandPacket.TransferLength = sizeof (NVME_ADMIN_NAMESPACE_DATA);
  CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
  CommandPacket.QueueType      = NVME_ADMIN_QUEUE;
  //
  // Set bit 0 (Cns bit) to 1 to identify a namespace
  //
  CommandPacket.NvmeCmd->Cdw10 = 0;
  CommandPacket.NvmeCmd->Flags = CDW10_VALID;

  Status = Private->Passthru.PassThru (
                               &Private->Passthru,
                               NamespaceId,
                               &CommandPacket,
                               NULL
                               );

  return Status;
}

// MU_CHANGE [BEGIN] - Request Number of Queues from Controller

/**
  Send the Set Features Command to the controller for the number of queues requested.
  Note that the number of queues allocated may be different from the number of queues requested.
  The number of data queue pairs allocated is returned and stored in the controller private data structure
  using the NumberOfIoQueuePairs field.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param  Ndqpr            The number of data queue pairs requested.

  @return EFI_SUCCESS      Successfully set the number of queues.
  @return EFI_DEVICE_ERROR Fail to set the number of queues.

**/
EFI_STATUS
NvmeSetFeaturesNumberOfQueues (
  IN OUT NVME_CONTROLLER_PRIVATE_DATA  *Private,
  IN UINT16                            Ndqpr
  )
{
  EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET  CommandPacket;
  EFI_NVM_EXPRESS_COMMAND                   Command;
  EFI_NVM_EXPRESS_COMPLETION                Completion;
  EFI_STATUS                                Status;
  NVME_ADMIN_SET_FEATURES_CDW10             SetFeatures;
  NVME_ADMIN_SET_FEATURES_NUM_QUEUES        NumberOfQueuesRequested;
  NVME_ADMIN_SET_FEATURES_NUM_QUEUES        NumberOfQueuesAllocated;

  Status = EFI_SUCCESS;

  if (Ndqpr == 0) {
    DEBUG ((DEBUG_ERROR, "Number of Data Queue Pairs Requested cannot be 0\n"));
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&CommandPacket, sizeof (EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof (EFI_NVM_EXPRESS_COMMAND));
  ZeroMem (&Completion, sizeof (EFI_NVM_EXPRESS_COMPLETION));
  ZeroMem (&SetFeatures, sizeof (NVME_ADMIN_SET_FEATURES));
  ZeroMem (&NumberOfQueuesRequested, sizeof (NVME_ADMIN_SET_FEATURES_NUM_QUEUES));
  ZeroMem (&NumberOfQueuesAllocated, sizeof (NVME_ADMIN_SET_FEATURES_NUM_QUEUES));

  CommandPacket.NvmeCmd        = &Command;
  CommandPacket.NvmeCompletion = &Completion;
  CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
  CommandPacket.QueueType      = NVME_ADMIN_QUEUE;
  Command.Nsid                 = 0; // NSID must be set to 0h or FFFFFFFFh for an admin command
  Command.Cdw0.Opcode          = NVME_ADMIN_SET_FEATURES_CMD;

  // Populate the Set Features Cdw10 and Cdw11 according to Nvm Express 1.3d Spec
  // Note we subtract 1 from the requested number of queues to get the 0-based value
  SetFeatures.Bits.Fid             = NVME_FEATURE_NUMBER_OF_QUEUES;
  NumberOfQueuesRequested.Bits.Ncq = Ndqpr - 1;
  NumberOfQueuesRequested.Bits.Nsq = Ndqpr - 1;
  CommandPacket.NvmeCmd->Cdw10     = SetFeatures.Uint32;
  CommandPacket.NvmeCmd->Cdw11     = NumberOfQueuesRequested.Uint32;

  CommandPacket.NvmeCmd->Flags = CDW10_VALID | CDW11_VALID;

  DEBUG ((DEBUG_INFO, "Number of Data Queue Pairs Requested: %d\n", Ndqpr));

  // Send the Set Features Command for Number of Queues
  Status = Private->Passthru.PassThru (
                               &Private->Passthru,
                               0,
                               &CommandPacket,
                               NULL
                               );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Set Features Command for Number of Queues failed with Status %r\n", Status));
    return Status;
  }

  //
  // Save the number of queues allocated, adding 1 to account for it being a 0-based value.
  // E.g. if 1 pair of data queues is allocated Nsq=0, Ncq=0, then NumberOfIoQueuePairs=1.
  // These numbers do not include the admin queues.
  //
  NumberOfQueuesAllocated.Uint32 = CommandPacket.NvmeCompletion->DW0;
  DEBUG ((DEBUG_INFO, "Number of Data Queue Pairs Allocated By Controller: %d, \n", (NumberOfQueuesAllocated.Bits.Nsq + 1)));
  if ((NumberOfQueuesAllocated.Bits.Nsq + 1) > Ndqpr) {
    // This driver at maximum supports 2 pairs of data queues. So we will take the minimum of the requested and allocated values.
    Private->NumberOfIoQueuePairs = Ndqpr;
  } else {
    Private->NumberOfIoQueuePairs = NumberOfQueuesAllocated.Bits.Nsq + 1;
  }

  DEBUG ((DEBUG_INFO, "Number of Data Queue Pairs Supported By Driver: %d, \n", Private->NumberOfIoQueuePairs));
  return Status;
}

// MU_CHANGE [END] - Request Number of Queues from Controller

/**
  Create io completion queue(s).

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.

  @return EFI_SUCCESS      Successfully create io completion queue.
  @return EFI_DEVICE_ERROR Fail to create io completion queue.

**/
EFI_STATUS
NvmeCreateIoCompletionQueue (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private
  )
{
  EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET  CommandPacket;
  EFI_NVM_EXPRESS_COMMAND                   Command;
  EFI_NVM_EXPRESS_COMPLETION                Completion;
  EFI_STATUS                                Status;
  NVME_ADMIN_CRIOCQ                         CrIoCq;
  UINT32                                    Index;
  UINT16                                    QueueSize;

  Status                 = EFI_SUCCESS;
  Private->CreateIoQueue = TRUE;

  // MU_CHANGE [BEGIN] - Request Number of Queues from Controller
  // Start from Index 1 because Index 0 is reserved for admin queue
  for (Index = 1; Index <= Private->NumberOfIoQueuePairs; Index++) {
    // MU_CHANGE [END] - Request Number of Queues from Controller

    ZeroMem (&CommandPacket, sizeof (EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
    ZeroMem (&Command, sizeof (EFI_NVM_EXPRESS_COMMAND));
    ZeroMem (&Completion, sizeof (EFI_NVM_EXPRESS_COMPLETION));
    ZeroMem (&CrIoCq, sizeof (NVME_ADMIN_CRIOCQ));

    CommandPacket.NvmeCmd        = &Command;
    CommandPacket.NvmeCompletion = &Completion;

    Command.Cdw0.Opcode          = NVME_ADMIN_CRIOCQ_CMD;
    CommandPacket.TransferBuffer = Private->CqBufferPciAddr[Index];
    CommandPacket.TransferLength = EFI_PAGE_SIZE;
    CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
    CommandPacket.QueueType      = NVME_ADMIN_QUEUE;

    // MU_CHANGE [BEGIN] - Use the Mqes value from the Cap register
    // MU_CHANGE [BEGIN] - Support alternative hardware queue sizes in NVME driver
    if (PcdGetBool (PcdSupportAlternativeQueueSize)) {
      QueueSize = MIN (NVME_ALTERNATIVE_MAX_QUEUE_SIZE, Private->Cap.Mqes);
    } else if (Index == 1) {
      // MU_CHANGE [END] - Support alternative hardware queue sizes in NVME driver
      QueueSize = MIN (NVME_CCQ_SIZE, Private->Cap.Mqes);
    } else {
      QueueSize = MIN (NVME_ASYNC_CCQ_SIZE, Private->Cap.Mqes);
    }

    // MU_CHANGE [END] - Use the Mqes value from the Cap register

    CrIoCq.Qid   = Index;
    CrIoCq.Qsize = QueueSize;
    CrIoCq.Pc    = 1;
    CopyMem (&CommandPacket.NvmeCmd->Cdw10, &CrIoCq, sizeof (NVME_ADMIN_CRIOCQ));
    CommandPacket.NvmeCmd->Flags = CDW10_VALID | CDW11_VALID;

    Status = Private->Passthru.PassThru (
                                 &Private->Passthru,
                                 0,
                                 &CommandPacket,
                                 NULL
                                 );
    if (EFI_ERROR (Status)) {
      // MU_CHANGE [BEGIN] - Request Number of Queues from Controller
      DEBUG ((DEBUG_ERROR, "%a: Create Completion Queue Command %d failed with Status %r\n", __func__, Index, Status));
      // MU_CHANGE [END] - Request Number of Queues from Controller
      break;
    }
  }

  Private->CreateIoQueue = FALSE;

  return Status;
}

/**
  Create io submission queue.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.

  @return EFI_SUCCESS      Successfully create io submission queue.
  @return EFI_DEVICE_ERROR Fail to create io submission queue.

**/
EFI_STATUS
NvmeCreateIoSubmissionQueue (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private
  )
{
  EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET  CommandPacket;
  EFI_NVM_EXPRESS_COMMAND                   Command;
  EFI_NVM_EXPRESS_COMPLETION                Completion;
  EFI_STATUS                                Status;
  NVME_ADMIN_CRIOSQ                         CrIoSq;
  UINT32                                    Index;
  UINT16                                    QueueSize;

  Status                 = EFI_SUCCESS;
  Private->CreateIoQueue = TRUE;

  // MU_CHANGE [BEGIN] - Request Number of Queues from Controller
  // Start from Index 1 because Index 0 is reserved for admin queue
  for (Index = 1; Index <= Private->NumberOfIoQueuePairs; Index++) {
    // MU_CHANGE [END] - Request Number of Queues from Controller
    ZeroMem (&CommandPacket, sizeof (EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
    ZeroMem (&Command, sizeof (EFI_NVM_EXPRESS_COMMAND));
    ZeroMem (&Completion, sizeof (EFI_NVM_EXPRESS_COMPLETION));
    ZeroMem (&CrIoSq, sizeof (NVME_ADMIN_CRIOSQ));

    CommandPacket.NvmeCmd        = &Command;
    CommandPacket.NvmeCompletion = &Completion;

    Command.Cdw0.Opcode          = NVME_ADMIN_CRIOSQ_CMD;
    CommandPacket.TransferBuffer = Private->SqBufferPciAddr[Index];
    CommandPacket.TransferLength = EFI_PAGE_SIZE;
    CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
    CommandPacket.QueueType      = NVME_ADMIN_QUEUE;

    // MU_CHANGE [BEGIN] - Use the Mqes value from the Cap register
    // MU_CHANGE [BEGIN] - Support alternative hardware queue sizes in NVME driver
    if (PcdGetBool (PcdSupportAlternativeQueueSize)) {
      QueueSize = MIN (NVME_ALTERNATIVE_MAX_QUEUE_SIZE, Private->Cap.Mqes);
    } else if (Index == 1) {
      // MU_CHANGE [END] - Support alternative hardware queue sizes in NVME driver
      QueueSize = MIN (NVME_CSQ_SIZE, Private->Cap.Mqes);
    } else {
      QueueSize = MIN (NVME_ASYNC_CSQ_SIZE, Private->Cap.Mqes);
    }

    // MU_CHANGE [END] - Use the Mqes value from the Cap register

    CrIoSq.Qid   = Index;
    CrIoSq.Qsize = QueueSize;
    CrIoSq.Pc    = 1;
    CrIoSq.Cqid  = Index;
    CrIoSq.Qprio = 0;
    CopyMem (&CommandPacket.NvmeCmd->Cdw10, &CrIoSq, sizeof (NVME_ADMIN_CRIOSQ));
    CommandPacket.NvmeCmd->Flags = CDW10_VALID | CDW11_VALID;

    Status = Private->Passthru.PassThru (
                                 &Private->Passthru,
                                 0,
                                 &CommandPacket,
                                 NULL
                                 );
    if (EFI_ERROR (Status)) {
      // MU_CHANGE [BEGIN] - Request Number of Queues from Controller
      DEBUG ((DEBUG_ERROR, "%a: Create Submission Queue Command %d failed with Status %r\n", __func__, Index, Status));
      // MU_CHANGE [END] - Request Number of Queues from Controller
      break;
    }
  }

  Private->CreateIoQueue = FALSE;

  return Status;
}

// MU_CHANGE [BEGIN] - Allocate IO Queue Buffer

/**
  Initialize the Nvm Express controller Data (IO) Queues

  @param[in] Private                 The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS                The NVM Express Controller IO Queues are initialized successfully.
  @retval Others                     A device error occurred while initializing the IO Queues.

**/
EFI_STATUS
NvmeControllerInitIoQueues (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private
  )
{
  UINTN       SqPageCount;
  UINTN       IoQueuePairPageCount;
  UINT32      Index;
  EFI_STATUS  Status;

  // Offset completion queue with submission queue size
  SqPageCount = NVME_SQ_SIZE_IN_PAGES (Private, 1);

  // Calculate the number of pages required for the data queues
  IoQueuePairPageCount = SqPageCount + NVME_CQ_SIZE_IN_PAGES (Private, 1);

  //
  // Address of Data I/O submission & completion queue(s).
  // We are using the same table of buffer pointers that the admin queues are in, so we start the table from Index + 1, but we have a separate
  // buffer so we start at the beginning of that buffer.
  //
  ZeroMem (Private->IoQueueBuffer, EFI_PAGES_TO_SIZE (IoQueuePairPageCount) * Private->NumberOfIoQueuePairs);
  for (Index = 0; Index < Private->NumberOfIoQueuePairs; Index++) {
    Private->SqBuffer[Index + 1]        = (NVME_SQ *)(UINTN)(Private->IoQueueBuffer + EFI_PAGES_TO_SIZE (Index * IoQueuePairPageCount));
    Private->SqBufferPciAddr[Index + 1] = (NVME_SQ *)(UINTN)(Private->IoQueueBufferPciAddr + EFI_PAGES_TO_SIZE (Index * IoQueuePairPageCount));
    Private->CqBuffer[Index + 1]        = (NVME_CQ *)(UINTN)(Private->IoQueueBuffer + EFI_PAGES_TO_SIZE (Index * IoQueuePairPageCount + SqPageCount));
    Private->CqBufferPciAddr[Index + 1] = (NVME_CQ *)(UINTN)(Private->IoQueueBufferPciAddr + EFI_PAGES_TO_SIZE (Index * IoQueuePairPageCount + SqPageCount));

    DEBUG ((DEBUG_INFO, "Data IO   Submission Queue (SqBuffer[%d]) = [%016X]\n", Index + 1, Private->SqBuffer[Index + 1]));
    DEBUG ((DEBUG_INFO, "Data IO   Completion Queue (CqBuffer[%d]) = [%016X]\n", Index + 1, Private->CqBuffer[Index + 1]));
  }

  //
  // Create I/O completion queue(s).
  //
  Status = NvmeCreateIoCompletionQueue (Private);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Create I/O Submission queue(s).
  //
  Status = NvmeCreateIoSubmissionQueue (Private);

  return Status;
}

// MU_CHANGE [END] - Allocate IO Queue Buffer

// MU_CHANGE [BEGIN] - Allocate IO Queue Buffer

/**
  Initialize the Nvm Express controller Admin Queues

  @param[in] Private                 The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS                The NVM Express Controller is initialized successfully.
  @retval Others                     A device error occurred while initializing the controller.

**/
EFI_STATUS
NvmeControllerInitAdminQueues (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private
  )
{
  NVME_ASQ    Asq;
  NVME_ACQ    Acq;
  UINTN       AsqPageCount;
  UINTN       AsqSize;
  UINTN       AdminQueuePairPageCount;
  EFI_STATUS  Status;

  // Offset completion queue with submission queue size
  AsqPageCount = NVME_SQ_SIZE_IN_PAGES (Private, 0);
  AsqSize      = EFI_PAGES_TO_SIZE (AsqPageCount);

  //
  // Address of admin submission queue.
  //
  // MU_CHANGE - Remove Page Mask
  Asq = (UINT64)(UINTN)(Private->BufferPciAddr);

  //
  // Address of admin completion queue.
  //
  // MU_CHANGE - Remove Page Mask
  Acq = (UINT64)(UINTN)(Private->BufferPciAddr + AsqSize);

  // Calculate the number of pages required for the admin queues
  AdminQueuePairPageCount = AsqPageCount + NVME_CQ_SIZE_IN_PAGES (Private, 0);

  //
  // Address of Admin I/O submission & completion queues.
  //
  ZeroMem (Private->Buffer, EFI_PAGES_TO_SIZE (AdminQueuePairPageCount));
  Private->SqBuffer[0]        = (NVME_SQ *)(UINTN)(Private->Buffer);
  Private->SqBufferPciAddr[0] = (NVME_SQ *)(UINTN)(Private->BufferPciAddr);
  Private->CqBuffer[0]        = (NVME_CQ *)(UINTN)(Private->Buffer + AsqSize);
  Private->CqBufferPciAddr[0] = (NVME_CQ *)(UINTN)(Private->BufferPciAddr + AsqSize);

  DEBUG ((DEBUG_INFO, "Private->Buffer = [%016X]\n", (UINT64)(UINTN)Private->Buffer));
  DEBUG ((DEBUG_INFO, "Admin     Submission Queue size (Number of Entries) = [%08X]\n", Private->SqData[0].NumberOfEntries));
  DEBUG ((DEBUG_INFO, "Admin     Completion Queue size (Number of Entries) = [%08X]\n", Private->CqData[0].NumberOfEntries));
  DEBUG ((DEBUG_INFO, "Admin     Submission Queue (SqBuffer[0]) = [%016X]\n", Private->SqBuffer[0]));
  DEBUG ((DEBUG_INFO, "Admin     Completion Queue (CqBuffer[0]) = [%016X]\n", Private->CqBuffer[0]));

  //
  // Program admin submission queue address.
  //
  Status = WriteNvmeAdminSubmissionQueueBaseAddress (Private, &Asq);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Program admin completion queue address.
  //
  Status = WriteNvmeAdminCompletionQueueBaseAddress (Private, &Acq);

  return Status;
}

// MU_CHANGE [END] - Allocate IO Queue Buffer

/**
  Initialize the Nvm Express controller.

  @param[in] Private                 The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS                The NVM Express Controller is initialized successfully.
  @retval Others                     A device error occurred while initializing the controller.

**/
EFI_STATUS
NvmeControllerInit (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private
  )
{
  // MU_CHANGE [BEGIN] - Allocate IO Queue Buffer
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  UINT64               Supports;
  UINT16               VidDid[2];  // MU_CHANGE - Improve NVMe controller init robustness
  // NVME_ASQ             Asq;
  // NVME_ACQ             Acq;
  UINT8                 Sn[21];
  UINT8                 Mn[41];
  UINTN                 Bytes;
  UINT32                Index;
  EFI_PHYSICAL_ADDRESS  MappedAddr;
  NVME_AQA              Aqa;
  UINTN                 AdminQueuePairPageCount;
  UINTN                 IoQueuePairPageCount;

  // MU_CHANGE [END] - Allocate IO Queue Buffer

  // MU_CHANGE [BEGIN] - Improve NVMe controller init robustness
  PciIo = Private->PciIo;

  //
  // Verify the controller is still accessible
  //
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint16,
                        PCI_VENDOR_ID_OFFSET,
                        ARRAY_SIZE (VidDid),
                        VidDid
                        );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return EFI_DEVICE_ERROR;
  }

  if ((VidDid[0] == NVME_INVALID_VID_DID) || (VidDid[1] == NVME_INVALID_VID_DID)) {
    return EFI_DEVICE_ERROR;
  }

  // MU_CHANGE [END] - Improve NVMe controller init robustness

  //
  // Enable this controller.
  //
  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationSupported,
                    0,
                    &Supports
                    );

  if (!EFI_ERROR (Status)) {
    Supports &= (UINT64)EFI_PCI_DEVICE_ENABLE;
    Status    = PciIo->Attributes (
                         PciIo,
                         EfiPciIoAttributeOperationEnable,
                         Supports,
                         NULL
                         );
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "NvmeControllerInit: failed to enable controller\n"));
    return Status;
  }

  //
  // Read the Controller Capabilities register and verify that the NVM command set is supported
  //
  Status = ReadNvmeControllerCapabilities (Private, &Private->Cap);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((Private->Cap.Css & BIT0) == 0) {
    DEBUG ((DEBUG_INFO, "NvmeControllerInit: the controller doesn't support NVMe command set\n"));
    return EFI_UNSUPPORTED;
  }

  //
  // Currently the driver only supports 4k page size.
  //
  // MU_CHANGE [BEGIN] - Improve NVMe controller init robustness

  // Currently, this means Cap.Mpsmin must be zero for an EFI_PAGE_SHIFT size of 12.
  // ASSERT ((Private->Cap.Mpsmin + 12) <= EFI_PAGE_SHIFT);
  if ((Private->Cap.Mpsmin + 12) > EFI_PAGE_SHIFT) {
    DEBUG ((DEBUG_ERROR, "NvmeControllerInit: Mpsmin is larger than expected (0x%02x).\n", Private->Cap.Mpsmin));
    return EFI_DEVICE_ERROR;
  }

  // MU_CHANGE [END] - Improve NVMe controller init robustness

  // MU_CHANGE [BEGIN] - Allocate IO Queue Buffer

  // Private->Cid[0]        = 0;
  // Private->Cid[1]        = 0;
  // Private->Cid[2]        = 0;
  // Private->Pt[0]         = 0;
  // Private->Pt[1]         = 0;
  // Private->Pt[2]         = 0;
  // Private->SqTdbl[0].Sqt = 0;
  // Private->SqTdbl[1].Sqt = 0;
  // Private->SqTdbl[2].Sqt = 0;
  // Private->CqHdbl[0].Cqh = 0;
  // Private->CqHdbl[1].Cqh = 0;
  // Private->CqHdbl[2].Cqh = 0;
  // Private->AsyncSqHead   = 0;

  for (Index = 0; Index < NVME_MAX_QUEUES; Index++) {
    Private->Cid[Index]        = 0;
    Private->Pt[Index]         = 0;
    Private->SqTdbl[Index].Sqt = 0;
    Private->CqHdbl[Index].Cqh = 0;
  }

  Private->AsyncSqHead = 0;

  //
  // set number of entries admin submission & completion queues.
  //
  // MU_CHANGE [BEGIN] - Support alternative hardware queue sizes in NVME driver
  Aqa.Asqs  = PcdGetBool (PcdSupportAlternativeQueueSize) ? MIN (NVME_ALTERNATIVE_MAX_QUEUE_SIZE, Private->Cap.Mqes) : MIN (NVME_ASQ_SIZE, Private->Cap.Mqes);
  Aqa.Rsvd1 = 0;
  Aqa.Acqs  = PcdGetBool (PcdSupportAlternativeQueueSize) ? MIN (NVME_ALTERNATIVE_MAX_QUEUE_SIZE, Private->Cap.Mqes) : MIN (NVME_ACQ_SIZE, Private->Cap.Mqes);
  Aqa.Rsvd2 = 0;
  // MU_CHANGE [END] - Support alternative hardware queue sizes in NVME driver

  //
  // Set admin queue entry size to default
  // Note we are using the spec-defined minimum SQES and CQES here.
  //
  Private->SqData[0].EntrySize       = NVME_IOSQES_MIN;
  Private->SqData[0].NumberOfEntries = Aqa.Asqs;
  Private->CqData[0].EntrySize       = NVME_IOCQES_MIN;
  Private->CqData[0].NumberOfEntries = Aqa.Acqs;

  // Calculate the number of pages required for the admin queues
  AdminQueuePairPageCount = NVME_SQ_SIZE_IN_PAGES (Private, 0) + NVME_CQ_SIZE_IN_PAGES (Private, 0);

  //
  // Default:
  // 6 x 4kB aligned buffers will be carved out of this buffer.
  // 1st 4kB boundary is the start of the admin submission queue.
  // 2nd 4kB boundary is the start of the admin completion queue.
  // 3rd 4kB boundary is the start of I/O submission queue #1.
  // 4th 4kB boundary is the start of I/O completion queue #1.
  // 5th 4kB boundary is the start of I/O submission queue #2.
  // 6th 4kB boundary is the start of I/O completion queue #2.
  //
  // Allocate 6 pages of memory, then map it for bus master read and write.
  //
  Status = PciIo->AllocateBuffer (
                    PciIo,
                    AllocateAnyPages,
                    EfiBootServicesData,
                    AdminQueuePairPageCount,
                    (VOID **)&Private->Buffer,
                    0
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Bytes  = EFI_PAGES_TO_SIZE (AdminQueuePairPageCount);
  Status = PciIo->Map (
                    PciIo,
                    EfiPciIoOperationBusMasterCommonBuffer,
                    Private->Buffer,
                    &Bytes,
                    &MappedAddr,
                    &Private->Mapping
                    );
  if (EFI_ERROR (Status) || (Bytes != EFI_PAGES_TO_SIZE (AdminQueuePairPageCount))) {
    return EFI_DEVICE_ERROR;
  }

  Private->BufferPciAddr = (UINT8 *)(UINTN)MappedAddr;

  // Disable the controller to wait for CSTS.RDY to become 0
  // NVMe Base Specification 2.0e, Section 3.5 Controller Initialization
  // MU_CHANGE [END] - Allocate IO Queue Buffer
  Status = NvmeDisableController (Private);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  // MU_CHANGE [BEGIN] - Allocate IO Queue Buffer
  //
  // Address of admin submission queue.
  //
  // MU_CHANGE - Remove the page mask since the buffer is allocated using AllocatePages
  // Asq = (UINT64)(UINTN)(Private->BufferPciAddr);

  //
  // Address of admin completion queue.
  //
  // MU_CHANGE - Remove the page mask since the buffer is allocated using AllocatePages
  // Acq = (UINT64)(UINTN)(Private->BufferPciAddr + EFI_PAGE_SIZE);

  //
  // Address of I/O submission & completion queue.
  //
  // ZeroMem (Private->Buffer, EFI_PAGES_TO_SIZE (6));
  // Private->SqBuffer[0]        = (NVME_SQ *)(UINTN)(Private->Buffer);
  // Private->SqBufferPciAddr[0] = (NVME_SQ *)(UINTN)(Private->BufferPciAddr);
  // Private->CqBuffer[0]        = (NVME_CQ *)(UINTN)(Private->Buffer + 1 * EFI_PAGE_SIZE);
  // Private->CqBufferPciAddr[0] = (NVME_CQ *)(UINTN)(Private->BufferPciAddr + 1 * EFI_PAGE_SIZE);
  // Private->SqBuffer[1]        = (NVME_SQ *)(UINTN)(Private->Buffer + 2 * EFI_PAGE_SIZE);
  // Private->SqBufferPciAddr[1] = (NVME_SQ *)(UINTN)(Private->BufferPciAddr + 2 * EFI_PAGE_SIZE);
  // Private->CqBuffer[1]        = (NVME_CQ *)(UINTN)(Private->Buffer + 3 * EFI_PAGE_SIZE);
  // Private->CqBufferPciAddr[1] = (NVME_CQ *)(UINTN)(Private->BufferPciAddr + 3 * EFI_PAGE_SIZE);
  // Private->SqBuffer[2]        = (NVME_SQ *)(UINTN)(Private->Buffer + 4 * EFI_PAGE_SIZE);
  // Private->SqBufferPciAddr[2] = (NVME_SQ *)(UINTN)(Private->BufferPciAddr + 4 * EFI_PAGE_SIZE);
  // Private->CqBuffer[2]        = (NVME_CQ *)(UINTN)(Private->Buffer + 5 * EFI_PAGE_SIZE);
  // Private->CqBufferPciAddr[2] = (NVME_CQ *)(UINTN)(Private->BufferPciAddr + 5 * EFI_PAGE_SIZE);

  // DEBUG ((DEBUG_INFO, "Admin     Submission Queue size (Aqa.Asqs) = [%08X]\n", Aqa.Asqs));
  // DEBUG ((DEBUG_INFO, "Admin     Completion Queue size (Aqa.Acqs) = [%08X]\n", Aqa.Acqs));
  // DEBUG ((DEBUG_INFO, "Admin     Submission Queue (SqBuffer[0]) = [%016X]\n", Private->SqBuffer[0]));
  // DEBUG ((DEBUG_INFO, "Admin     Completion Queue (CqBuffer[0]) = [%016X]\n", Private->CqBuffer[0]));
  // DEBUG ((DEBUG_INFO, "Sync  I/O Submission Queue (SqBuffer[1]) = [%016X]\n", Private->SqBuffer[1]));
  // DEBUG ((DEBUG_INFO, "Sync  I/O Completion Queue (CqBuffer[1]) = [%016X]\n", Private->CqBuffer[1]));
  // DEBUG ((DEBUG_INFO, "Async I/O Submission Queue (SqBuffer[2]) = [%016X]\n", Private->SqBuffer[2]));
  // DEBUG ((DEBUG_INFO, "Async I/O Completion Queue (CqBuffer[2]) = [%016X]\n", Private->CqBuffer[2]));

  //
  // Program admin queue attributes.
  //
  Status = WriteNvmeAdminQueueAttributes (Private, &Aqa);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Program admin submission queue address.
  //
  // Status = WriteNvmeAdminSubmissionQueueBaseAddress (Private, &Asq);

  //
  // Program admin completion queue address.
  //
  // Status = WriteNvmeAdminCompletionQueueBaseAddress (Private, &Acq);

  Status = NvmeControllerInitAdminQueues (Private);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = NvmeEnableController (Private, Private->SqData[0].EntrySize, Private->CqData[0].EntrySize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // MU_CHANGE [END] - Allocate IO Queue Buffer

  //
  // Allocate buffer for Identify Controller data
  //
  if (Private->ControllerData == NULL) {
    Private->ControllerData = (NVME_ADMIN_CONTROLLER_DATA *)AllocateZeroPool (sizeof (NVME_ADMIN_CONTROLLER_DATA));

    if (Private->ControllerData == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  //
  // Get current Identify Controller Data
  //
  Status = NvmeIdentifyController (Private, Private->ControllerData);

  if (EFI_ERROR (Status)) {
    FreePool (Private->ControllerData);
    Private->ControllerData = NULL;
    return EFI_NOT_FOUND;
  }

  //
  // Dump NvmExpress Identify Controller Data
  //
  // Serial Number and Model Number are not null-terminated strings, but will be printed as ones.
  // So here we add a null terminator to the end of their arrays.
  CopyMem (Sn, Private->ControllerData->Sn, sizeof (Private->ControllerData->Sn));
  Sn[20] = 0;
  CopyMem (Mn, Private->ControllerData->Mn, sizeof (Private->ControllerData->Mn));
  Mn[40] = 0;
  DEBUG ((DEBUG_INFO, " == NVME IDENTIFY CONTROLLER DATA ==\n"));
  DEBUG ((DEBUG_INFO, "    PCI VID   : 0x%x\n", Private->ControllerData->Vid));
  DEBUG ((DEBUG_INFO, "    PCI SSVID : 0x%x\n", Private->ControllerData->Ssvid));
  DEBUG ((DEBUG_INFO, "    SN        : %a\n", Sn));
  DEBUG ((DEBUG_INFO, "    MN        : %a\n", Mn));
  DEBUG ((DEBUG_INFO, "    FR        : 0x%x\n", *((UINT64 *)Private->ControllerData->Fr)));
  DEBUG ((DEBUG_INFO, "    TNVMCAP (high 8-byte) : 0x%lx\n", *((UINT64 *)(Private->ControllerData->Tnvmcap + 8))));
  DEBUG ((DEBUG_INFO, "    TNVMCAP (low 8-byte)  : 0x%lx\n", *((UINT64 *)Private->ControllerData->Tnvmcap)));
  DEBUG ((DEBUG_INFO, "    RAB       : 0x%x\n", Private->ControllerData->Rab));
  DEBUG ((DEBUG_INFO, "    IEEE      : 0x%x\n", *(UINT32 *)Private->ControllerData->Ieee_oui));
  DEBUG ((DEBUG_INFO, "    AERL      : 0x%x\n", Private->ControllerData->Aerl));
  DEBUG ((DEBUG_INFO, "    SQES      : 0x%x\n", Private->ControllerData->Sqes));
  DEBUG ((DEBUG_INFO, "    CQES      : 0x%x\n", Private->ControllerData->Cqes));
  DEBUG ((DEBUG_INFO, "    NN        : 0x%x\n", Private->ControllerData->Nn));

  // MU_CHANGE [BEGIN] - Request Number of Queues from Controller
  //
  // Send Set Features Command to request the maximum number of data queues.
  // The controller is free to allocate a different number of queues from the number requested.
  // The number of queues allocated is returned and stored in the controller private data structure
  // using the NumberOfIoQueuePairs field.
  //
  Status = NvmeSetFeaturesNumberOfQueues (Private, NVME_MAX_QUEUES - 1);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  // MU_CHANGE [END] - Request Number of Queues from Controller

  // MU_CHANGE [BEGIN] - Allocate IO Queue Buffer
  //
  // Allocate Data Queues - note we are assuming the queue entry sizes are the same as the admin queue entry sizes for the sake of memory allocation.
  // The identify controller data tells us in SQES and CQES what the controller's minimum and maximum queue entry sizes are. We haven't used this before since we
  // use the spec-defined minimum queue entry sizes.
  // We are also allocating based on the admin defined queue sizes in number of entries.
  // Some scenarios may use different queue sizes, currently we only see the case where the driver needs IO queue sizes <= admin queue sizes. So this allocation should be sufficient.
  // We may want to explore a more dynamic allocation in the future.
  //
  for (Index = 1; Index <= Private->NumberOfIoQueuePairs; Index++) {
    Private->SqData[Index].NumberOfEntries = Private->SqData[0].NumberOfEntries;
    Private->CqData[Index].NumberOfEntries = Private->CqData[0].NumberOfEntries;
    Private->SqData[Index].EntrySize       = Private->SqData[0].EntrySize;
    Private->CqData[Index].EntrySize       = Private->CqData[0].EntrySize;
  }

  // Using the first data queue size for the number of pages required for the data queues
  IoQueuePairPageCount = NVME_SQ_SIZE_IN_PAGES (Private, 1) + NVME_CQ_SIZE_IN_PAGES (Private, 1);

  Status = PciIo->AllocateBuffer (
                    PciIo,
                    AllocateAnyPages,
                    EfiBootServicesData,
                    IoQueuePairPageCount * Private->NumberOfIoQueuePairs,
                    (VOID **)&Private->IoQueueBuffer,
                    0
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Bytes  = EFI_PAGES_TO_SIZE (IoQueuePairPageCount * Private->NumberOfIoQueuePairs);
  Status = PciIo->Map (
                    PciIo,
                    EfiPciIoOperationBusMasterCommonBuffer,
                    Private->IoQueueBuffer,
                    &Bytes,
                    &MappedAddr,
                    &Private->IoQueueMapping
                    );

  if (EFI_ERROR (Status) || (Bytes != EFI_PAGES_TO_SIZE (IoQueuePairPageCount * Private->NumberOfIoQueuePairs))) {
    return EFI_DEVICE_ERROR;
  }

  Private->IoQueueBufferPciAddr = (UINT8 *)(UINTN)MappedAddr;

  Status = NvmeControllerInitIoQueues (Private);
  // MU_CHANGE [END] - Allocate IO Queue Buffer

  return Status;
}

// MU_CHANGE [BEGIN] - Allocate IO Queue Buffer

/**
  Reset the Nvm Express controller.

  @param[in] Private                 The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS                The NVM Express Controller is reset successfully.
  @retval Others                     A device error occurred while resetting the controller.

**/
EFI_STATUS
NvmeControllerReset (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  UINT32               Index;
  UINT16               VidDid[2];
  UINT8                Sn[21];
  UINT8                Mn[41];
  NVME_AQA             Aqa;
  UINT32               NdqpBeforeReset;

  DEBUG ((DEBUG_INFO, "%a: Begin Controller Reset\n", __func__));

  PciIo = Private->PciIo;

  //
  // Verify the controller is still accessible
  //
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint16,
                        PCI_VENDOR_ID_OFFSET,
                        ARRAY_SIZE (VidDid),
                        VidDid
                        );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return EFI_DEVICE_ERROR;
  }

  if ((VidDid[0] == NVME_INVALID_VID_DID) || (VidDid[1] == NVME_INVALID_VID_DID)) {
    return EFI_DEVICE_ERROR;
  }

  for (Index = 0; Index < NVME_MAX_QUEUES; Index++) {
    Private->Cid[Index]        = 0;
    Private->Pt[Index]         = 0;
    Private->SqTdbl[Index].Sqt = 0;
    Private->CqHdbl[Index].Cqh = 0;
  }

  Private->AsyncSqHead = 0;

  Status = NvmeDisableController (Private);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ReadNvmeAdminQueueAttributes (Private, &Aqa);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((Private->SqData[0].NumberOfEntries != Aqa.Asqs) || (Private->CqData[0].NumberOfEntries != Aqa.Acqs)) {
    // By spec these values should not change between resets, so we'll fail out here.
    DEBUG ((DEBUG_ERROR, "%a: Admin Submission Queue Size (Number of Entries) mismatch: %d != %d\n", __func__, Private->SqData[0].NumberOfEntries, Aqa.Asqs));
    ASSERT (FALSE);
    return EFI_DEVICE_ERROR;
  }

  Status = NvmeControllerInitAdminQueues (Private);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = NvmeEnableController (Private, Private->SqData[0].EntrySize, Private->CqData[0].EntrySize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Allocate buffer for Identify Controller data
  //
  if (Private->ControllerData == NULL) {
    Private->ControllerData = (NVME_ADMIN_CONTROLLER_DATA *)AllocateZeroPool (sizeof (NVME_ADMIN_CONTROLLER_DATA));

    if (Private->ControllerData == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  //
  // Get current Identify Controller Data
  //
  Status = NvmeIdentifyController (Private, Private->ControllerData);

  if (EFI_ERROR (Status)) {
    FreePool (Private->ControllerData);
    Private->ControllerData = NULL;
    return EFI_NOT_FOUND;
  }

  //
  // Dump NvmExpress Identify Controller Data
  //
  CopyMem (Sn, Private->ControllerData->Sn, sizeof (Private->ControllerData->Sn));
  // Serial Number and Model Number are not null-terminated strings, but will be printed as ones.
  // So here we add a null terminator to the end of their arrays.
  Sn[20] = 0;
  CopyMem (Mn, Private->ControllerData->Mn, sizeof (Private->ControllerData->Mn));
  Mn[40] = 0;
  DEBUG ((DEBUG_INFO, " == NVME IDENTIFY CONTROLLER DATA ==\n"));
  DEBUG ((DEBUG_INFO, "    PCI VID   : 0x%x\n", Private->ControllerData->Vid));
  DEBUG ((DEBUG_INFO, "    PCI SSVID : 0x%x\n", Private->ControllerData->Ssvid));
  DEBUG ((DEBUG_INFO, "    SN        : %a\n", Sn));
  DEBUG ((DEBUG_INFO, "    MN        : %a\n", Mn));
  DEBUG ((DEBUG_INFO, "    FR        : 0x%x\n", *((UINT64 *)Private->ControllerData->Fr)));
  DEBUG ((DEBUG_INFO, "    TNVMCAP (high 8-byte) : 0x%lx\n", *((UINT64 *)(Private->ControllerData->Tnvmcap + 8))));
  DEBUG ((DEBUG_INFO, "    TNVMCAP (low 8-byte)  : 0x%lx\n", *((UINT64 *)Private->ControllerData->Tnvmcap)));
  DEBUG ((DEBUG_INFO, "    RAB       : 0x%x\n", Private->ControllerData->Rab));
  DEBUG ((DEBUG_INFO, "    IEEE      : 0x%x\n", *(UINT32 *)Private->ControllerData->Ieee_oui));
  DEBUG ((DEBUG_INFO, "    AERL      : 0x%x\n", Private->ControllerData->Aerl));
  DEBUG ((DEBUG_INFO, "    SQES      : 0x%x\n", Private->ControllerData->Sqes));
  DEBUG ((DEBUG_INFO, "    CQES      : 0x%x\n", Private->ControllerData->Cqes));
  DEBUG ((DEBUG_INFO, "    NN        : 0x%x\n", Private->ControllerData->Nn));

  //
  // Send Set Features Command to request the maximum number of data queues.
  // The controller is free to allocate a different number of queues from the number requested.
  // The number of queues allocated is returned and stored in the controller private data structure
  // using the NumberOfIoQueuePairs field.
  //
  NdqpBeforeReset = Private->NumberOfIoQueuePairs;
  Status          = NvmeSetFeaturesNumberOfQueues (Private, NVME_MAX_QUEUES - 1);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (NdqpBeforeReset != Private->NumberOfIoQueuePairs) {
    // This is a controller reset, so the number of data queues should not have changed.
    // If the number of queues has changed, then the driver must be re-initialized.
    DEBUG ((DEBUG_ERROR, "%a: Number of Data Queue Pairs mismatch: %d != %d\n", __func__, NdqpBeforeReset, Private->NumberOfIoQueuePairs));
    ASSERT (FALSE);
    return EFI_DEVICE_ERROR;
  }

  //
  // Create I/O completion queue(s).
  //
  Status = NvmeControllerInitIoQueues (Private);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return Status;
}

// MU_CHANGE [END] - Allocate IO Queue Buffer

/**
 This routine is called to properly shutdown the Nvm Express controller per NVMe spec.

  @param[in]  ResetType         The type of reset to perform.
  @param[in]  ResetStatus       The status code for the reset.
  @param[in]  DataSize          The size, in bytes, of ResetData.
  @param[in]  ResetData         For a ResetType of EfiResetCold, EfiResetWarm, or
                                EfiResetShutdown the data buffer starts with a Null-terminated
                                string, optionally followed by additional binary data.
                                The string is a description that the caller may use to further
                                indicate the reason for the system reset.
                                For a ResetType of EfiResetPlatformSpecific the data buffer
                                also starts with a Null-terminated string that is followed
                                by an EFI_GUID that describes the specific type of reset to perform.
**/
VOID
EFIAPI
NvmeShutdownAllControllers (
  IN EFI_RESET_TYPE  ResetType,
  IN EFI_STATUS      ResetStatus,
  IN UINTN           DataSize,
  IN VOID            *ResetData OPTIONAL
  )
{
  EFI_STATUS                           Status;
  EFI_HANDLE                           *Handles;
  UINTN                                HandleCount;
  UINTN                                HandleIndex;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY  *OpenInfos;
  UINTN                                OpenInfoCount;
  UINTN                                OpenInfoIndex;
  EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL   *NvmePassThru;
  NVME_CC                              Cc;
  NVME_CSTS                            Csts;
  UINTN                                Index;
  NVME_CONTROLLER_PRIVATE_DATA         *Private;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    HandleCount = 0;
  }

  for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) {
    Status = gBS->OpenProtocolInformation (
                    Handles[HandleIndex],
                    &gEfiPciIoProtocolGuid,
                    &OpenInfos,
                    &OpenInfoCount
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    for (OpenInfoIndex = 0; OpenInfoIndex < OpenInfoCount; OpenInfoIndex++) {
      //
      // Find all the NVME controller managed by this driver.
      // gImageHandle equals to DriverBinding handle for this driver.
      //
      if (((OpenInfos[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) != 0) &&
          (OpenInfos[OpenInfoIndex].AgentHandle == gImageHandle))
      {
        Status = gBS->OpenProtocol (
                        OpenInfos[OpenInfoIndex].ControllerHandle,
                        &gEfiNvmExpressPassThruProtocolGuid,
                        (VOID **)&NvmePassThru,
                        NULL,
                        NULL,
                        EFI_OPEN_PROTOCOL_GET_PROTOCOL
                        );
        if (EFI_ERROR (Status)) {
          continue;
        }

        Private = NVME_CONTROLLER_PRIVATE_DATA_FROM_PASS_THRU (NvmePassThru);

        //
        // Read Controller Configuration Register.
        //
        Status = ReadNvmeControllerConfiguration (Private, &Cc);
        if (EFI_ERROR (Status)) {
          continue;
        }

        //
        // The host should set the Shutdown Notification (CC.SHN) field to 01b
        // to indicate a normal shutdown operation.
        //
        Cc.Shn = NVME_CC_SHN_NORMAL_SHUTDOWN;
        Status = WriteNvmeControllerConfiguration (Private, &Cc);
        if (EFI_ERROR (Status)) {
          continue;
        }

        //
        // The controller indicates when shutdown processing is completed by updating the
        // Shutdown Status (CSTS.SHST) field to 10b.
        // Wait up to 45 seconds (break down to 4500 x 10ms) for the shutdown to complete.
        //
        for (Index = 0; Index < NVME_SHUTDOWN_PROCESS_TIMEOUT * 100; Index++) {
          Status = ReadNvmeControllerStatus (Private, &Csts);
          if (!EFI_ERROR (Status) && (Csts.Shst == NVME_CSTS_SHST_SHUTDOWN_COMPLETED)) {
            DEBUG ((DEBUG_INFO, "NvmeShutdownController: shutdown processing is completed after %dms.\n", Index * 10));
            break;
          }

          //
          // Stall for 10ms
          //
          gBS->Stall (10 * 1000);
        }

        if (Index == NVME_SHUTDOWN_PROCESS_TIMEOUT * 100) {
          DEBUG ((DEBUG_ERROR, "NvmeShutdownController: shutdown processing is timed out\n"));
        }
      }
    }
  }
}

/**
  Register the shutdown notification through the ResetNotification protocol.

  Register the shutdown notification when mNvmeControllerNumber increased from 0 to 1.
**/
VOID
NvmeRegisterShutdownNotification (
  VOID
  )
{
  EFI_STATUS                       Status;
  EFI_RESET_NOTIFICATION_PROTOCOL  *ResetNotify;

  mNvmeControllerNumber++;
  if (mNvmeControllerNumber == 1) {
    Status = gBS->LocateProtocol (&gEfiResetNotificationProtocolGuid, NULL, (VOID **)&ResetNotify);
    if (!EFI_ERROR (Status)) {
      Status = ResetNotify->RegisterResetNotify (ResetNotify, NvmeShutdownAllControllers);
      ASSERT_EFI_ERROR (Status);
    } else {
      DEBUG ((DEBUG_WARN, "NVME: ResetNotification absent! Shutdown notification cannot be performed!\n"));
    }
  }
}

/**
  Unregister the shutdown notification through the ResetNotification protocol.

  Unregister the shutdown notification when mNvmeControllerNumber decreased from 1 to 0.
**/
VOID
NvmeUnregisterShutdownNotification (
  VOID
  )
{
  EFI_STATUS                       Status;
  EFI_RESET_NOTIFICATION_PROTOCOL  *ResetNotify;

  mNvmeControllerNumber--;
  if (mNvmeControllerNumber == 0) {
    Status = gBS->LocateProtocol (&gEfiResetNotificationProtocolGuid, NULL, (VOID **)&ResetNotify);
    if (!EFI_ERROR (Status)) {
      Status = ResetNotify->UnregisterResetNotify (ResetNotify, NvmeShutdownAllControllers);
      ASSERT_EFI_ERROR (Status);
    }
  }
}
