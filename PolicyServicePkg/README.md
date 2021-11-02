# Policy Service

Documents the interface, design, and implementation of the Policy service split
across PEI and DXE modules as well as any supporting libraries or best
practices.

## Overview

The generalized policy service, implemented in DXE and PEI, provides interfaces
for components to publish and consume generic policies in the UEFI environment.
This policy takes the form of generic data and it is up to the producer and
consumer to agree upon the contents and format of that policy. This document
makes recommendations as to best practices to sustainably format policy data,
but the service is agnostic to the format or meaning of the policy.

Policy is a data block containing configurations or settings relating to the
\silicon, platform, or feature state of the system. This information can
originate from the platform code, user settings, or be determined at runtime.
A policy may be entirely defined by a single component or it can be built and
transformed by several components each further customizing or locking down the
system.

## Policy Management Process

Policies are uniquely defined by a GUID. This GUID should only be used for a
specific data type and purpose. Only one policy may be published under a
specific GUID at any given time. Duplicates will overwrite the original policy
unless the policy has been finalized via the policy attributes. Policies are
managed through a simple Get/Set/Remove interface detailed below.

### Attributes

Policies can be can be set with attribute flags allowing the policy provider to
specify on how the policy is handled. For example, this can be used to finalize
the policy making it read-only or can be used to limit access to the policy to
a phase of boot. See the PolicyInterface header definitions for full list of
attributes.

### Policy Notification Protocols & PPIs

When a policy is set or updated a PPI or Protocol will be installed with the
GUID of the policy. Consumers may use this GUID to either set Protocol/PPI
notifications, or create a DEPEX dependency so that the consumer is not
dispatched until the policy is made available. The protocol/PPI will not
contain any useful interface and consumers are expected to use the protocol
interface to retrieve the policy data after being notified or dispatched.

## Policy Interface

Both the PEI and DXE implementation provide the following interfaces.

### _SetPolicy_

Creates a new or overwrites and existing policy. Policies can only be
overwritten if the policy has not be finalized. The policy will be copied from
the provided buffer to an internal store, so all further edits must be done
though additional calls to _SetPolicy_.

### _GetPolicy_

Returns a copy of the policy for the provided policy GUID and its current
attributes. The caller is responsible for allocating the buffer the policy is
copied into.

### _RemovePolicy_

Removes a policy from the policy list, freeing it when possible.

## Data Format

Policies data format is up to the producer to determine. Recommendations about
more robust and maintainable data formats to be made at a later time.
