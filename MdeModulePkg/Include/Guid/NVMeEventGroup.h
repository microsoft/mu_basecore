/**

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __NVME_EVENT_GROUP_GUID__
#define __NVME_EVENT_GROUP_GUID__

// gNVMeEnableStartEventGroupGuid is used to signal the start of enabling the NVMe controller
extern EFI_GUID  gNVMeEnableStartEventGroupGuid;
// gNVMeEnableCompleteEventGroupGuid is used to signal that the NVMe controller enable has finished
extern EFI_GUID  gNVMeEnableCompleteEventGroupGuid;

#endif
