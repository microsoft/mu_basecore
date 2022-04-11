/** @file
  Implements sample policy for PEI environment.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>

#include <Ppi/Policy.h>
#include "SamplePolicy.h"

//
// Guids used to store sample policies. For production scenarios this should be
// defined in appropriate .dec file.
//

EFI_GUID  mSamplePolicyGuid1 = {
  0xbb7b94d5, 0x105c, 0x41a6, { 0xb4, 0xe4, 0xa1, 0x58, 0x2e, 0xde, 0xf0, 0x7e }
};

EFI_GUID  mSampleGuidPeiToDxe = POLICY_SAMPLE_PEI_TO_DXE_GUID;

//
// Global to simplify examples.
//

POLICY_PPI  *mPolicyPpi;

/**
  A routine to sample the different Policy PPI functions. This routine will not
  leave any active policies.

  @param[in,out]  PolicyGuid        The GUID to use for the sample policy.

  @retval   EFI_SUCCESS             Successfully ran sample policy code.
  @retval   EFI_PROTOCOL_ERROR      Unexpected status returned by policy interface.
  @retval   other                   Failure status returned by policy interface.
**/
EFI_STATUS
PeiSampleSetGetRemovePolicy (
  IN EFI_GUID  *PolicyGuid
  )
{
  SAMPLE_POLICY  Policy;
  SAMPLE_POLICY  ResultPolicy;
  UINT64         Attributes;
  EFI_STATUS     Status;
  UINT16         PolicySize;

  PolicySize = sizeof (ResultPolicy);

  // Since the policy does not yet exist, it should fail.
  Status = mPolicyPpi->GetPolicy (PolicyGuid, &Attributes, &ResultPolicy, &PolicySize);
  if (Status != EFI_NOT_FOUND) {
    DEBUG ((DEBUG_ERROR, "%a: Unexpected return code: %r\n", __FUNCTION__, Status));
    ASSERT (FALSE);
    return EFI_PROTOCOL_ERROR;
  }

  // Creating the policy for the first time.
  Status = mPolicyPpi->SetPolicy (PolicyGuid, 0, &Policy, sizeof (Policy));
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // After this the policy should be retrievable. This will be stored in the
  // provided buffer.
  Status = mPolicyPpi->GetPolicy (PolicyGuid, &Attributes, &ResultPolicy, &PolicySize);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // The policy can now be removed.
  Status = mPolicyPpi->RemovePolicy (PolicyGuid);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Trying to get the policy now should fail.
  Status = mPolicyPpi->GetPolicy (PolicyGuid, &Attributes, &ResultPolicy, &PolicySize);
  if (Status != EFI_NOT_FOUND) {
    DEBUG ((DEBUG_ERROR, "%a: Unexpected return code: %r\n", __FUNCTION__, Status));
    ASSERT (FALSE);
    return EFI_PROTOCOL_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Creates a policy that will be available to the rest of PEI and DXE.

  @param[in,out]  PolicyGuid        The GUID to use for the sample policy.

  @retval   EFI_SUCCESS             Successfully created policy.
  @retval   other                   Failure status returned by policy interface.
**/
EFI_STATUS
PeiSampleCreatePolicy (
  IN EFI_GUID  *PolicyGuid
  )
{
  EFI_STATUS     Status;
  SAMPLE_POLICY  Policy;

  // Set sample values to be checked later.
  Policy.Signature = SAMPLE_POLICY_SIGNATURE;
  Policy.Revision  = SAMPLE_POLICY_REVISION;
  Policy.Value     = SAMPLE_POLICY_VALUE;

  // Creating the policy for the first time.
  Status = mPolicyPpi->SetPolicy (PolicyGuid, 0, &Policy, sizeof (Policy));
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Entry point for the policy PEIM. Installs the policy service PPI.

  @param[in]    FileHandle      UNUSED.
  @param[in]    PeiServices     UNUSED.

  @retval   EFI_SUCCESS             Successfully ran sample policy code.
  @retval   other                   Failure status returned by policy interface.
**/
EFI_STATUS
EFIAPI
PeiPolicySampleEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS  Status;

  // Retrieve the Policy PPI.
  Status = PeiServicesLocatePpi (
             &gPeiPolicyPpiGuid,
             0,
             NULL,
             (VOID **)&mPolicyPpi
             );

  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Run a simple set, get, and remove sample.
  Status = PeiSampleSetGetRemovePolicy (&mSamplePolicyGuid1);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Now create a policy that will be kept available for DXE.
  Status = PeiSampleCreatePolicy (&mSampleGuidPeiToDxe);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  return Status;
}
