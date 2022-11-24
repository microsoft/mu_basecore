/** @file
  Implements sample policy for DXE environment.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>

#include <Protocol/Policy.h>
#include "SamplePolicy.h"

//
// Guids used to store sample policies. For production scenarios this should be
// defined in appropriate .dec file.
//

EFI_GUID  mSampleGuidPeiToDxe = POLICY_SAMPLE_PEI_TO_DXE_GUID;

//
// Global to store the protocol.
//
POLICY_PROTOCOL  *mPolicyProtocol;

/**
  A routine to retrieve the sample policy created by the PEI sample module.

  @param[in,out]  PolicyGuid        The GUID to use for the sample policy.

  @retval   EFI_SUCCESS             Successfully ran sample policy code.
  @retval   EFI_PROTOCOL_ERROR      Unexpected status returned by policy interface.
  @retval   other                   Failure status returned by policy interface.
**/
EFI_STATUS
DxeSampleGetPeiPolicy (
  IN EFI_GUID  *PolicyGuid
  )
{
  EFI_STATUS     Status;
  SAMPLE_POLICY  Policy;
  UINT16         PolicySize;
  UINT64         Attributes;

  // Set the policy size to 0 to indicate a null policy pointer.
  PolicySize = 0;

  // First check the size. This would usually be done for policies of a dynamic
  // or changing size. Attributes may be retrieved at this time if desired.
  Status = mPolicyProtocol->GetPolicy (PolicyGuid, NULL, NULL, NULL, &PolicySize);
  if ((Status != EFI_BUFFER_TOO_SMALL) || (PolicySize != sizeof (Policy))) {
    ASSERT (FALSE);
    return EFI_PROTOCOL_ERROR;
  }

  // Retrieve the actual policy.
  Status = mPolicyProtocol->GetPolicy (PolicyGuid, NULL, &Attributes, &Policy, &PolicySize);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Verify all the details are correct.
  ASSERT (PolicySize == sizeof (Policy));
  ASSERT (Attributes == 0);
  ASSERT (Policy.Signature == SAMPLE_POLICY_SIGNATURE);
  ASSERT (Policy.Revision == SAMPLE_POLICY_REVISION);
  ASSERT (Policy.Value == SAMPLE_POLICY_VALUE);

  return Status;
}

/**
  DXE policy driver entry point. Initialized the policy store from the HOB list
  and install the DXE policy protocol.

  @param[in]  ImageHandle     The firmware allocated handle for the EFI image.
  @param[in]  SystemTable     UNUSED.

  @retval   EFI_SUCCESS       Policy store initialized and protocol installed.
  @retval   other             Sample routines returned a failure.
**/
EFI_STATUS
EFIAPI
DxePolicySampleEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  // Get the policy protocol.
  Status = gBS->LocateProtocol (
                  &gPolicyProtocolGuid,
                  NULL,
                  (VOID **)&mPolicyProtocol
                  );

  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = DxeSampleGetPeiPolicy (&mSampleGuidPeiToDxe);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  return Status;
}
