# Policy Library

The policy library is responsible for handling more robust features on top of
the basic policy store provided by the policy. The primary use case of this
library would be to use _verified policies_ stored in the policy store.

## Verified Policy Overview

Verified policy is a set of library functions and tools around policy that
provides a more type and version safe way of sharing data structures between
components in different code bases or firmware layers. Using the verified
policy functions in the library, combined with a per-data-structure generated
header, callers can set and read data from a policy with automatic checks and
guarantees to ensure the data being read correctly. This also provides useful
concepts such as default values allowing for structure-safe version mismatch.

## Verified Policy Components

The following are a few key components for utilizing verified policies.

__Policy Guid__ - As with all policies, a single instance of a policy should
have a unique GUID. This GUID is not associated with the policy structure and
only represents the data instance, not the data type or structure.

__Verified Policy Descriptor__ - The verified policy descriptor is a auto
generated structure which describes the expected data structure. This includes
a unique tag for the data, version info, size info, etc. The policy library will
use this information to check the policy data to ensure it matches the expected
type and required version.

__Attributes__ - Similar to all other policies, attributes can be used to
enforce access restrictions on the policy instance.

__Verified Policy Handle__ - To enforce the verified policy abstraction, the
caller will not be provided a direct pointer to the data. Instead they will be
provided a policy handle which should be used with the auto-generated accessors.
This allows for automatic checks to be done to ensure safe access of data.

## Verified Policy Functions

The policy library provides the following functions for accessing verified
policy.

___GetVerifiedPolicy___ - Retrieves a verified policy from the policy store,
checking that the data is the expected type and version. The returned handle
references a copy of the policy and so a set is required to store and changes.

___CreateVerifiedPolicy___ - Creates a new verified policy data instance. This
routine does not add the data to the policy store.

___SetVerifiedPolicy___ - Sets the provided verified policy data into the policy
store.

___CloseVerifiedPolicy___ - Closes the local handle to the verified policy,
freeing and resources.

___ReportVerifiedPolicyAccess___ - A API used to communicate access between the
generated accessors and the generic library. This routine should not be manually
called.

## Generating Verified Policy Headers

Details regarding generating verified policies are not yet documented here.
