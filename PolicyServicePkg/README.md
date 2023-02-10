# Policy Service

Documents the interface, design, and implementation of the Policy service split
across PEI and DXE modules as well as any supporting libraries or best
practices.

## Overview

The generalized policy service, implemented in DXE and PEI, provides interfaces
for components to publish and consume generic policies between components in the
UEFI environment. This policy takes the form of generic data and it is up to the
producer and consumer to agree upon the contents and format of that policy. This
document makes recommendations as to best practices to sustainably format policy
data, but the service is agnostic to the format or meaning of the policy.

Policy is a data block containing configurations or settings relating to the
silicon, platform, or feature state of the system. This information can
originate from PCD entries, platform/silicon configuration code, user settings,
or otherwise determined at runtime. A policy may be entirely defined by a single
component or it can be built and transformed by several components each further
customizing or locking down the system.

## Background

In UEFI there is a need to share these Policy data blocks across components
or phases. This could be specific device configuration, settings data,
platform information, etc. The Policy service is intended to give an easy,
abstracted, and extensible interface for accomplishing this data publishing.

### Previous Technologies

There are several existing mechanisms that can be used to share configuration
and data with the rest of the UEFI environment, but currently these mechanisms
either don't perfectly align with the Policy Service's use case or are not
suitable for adoption across all platforms. Below is a comparison to some of the
currently existing methods for sharing data blocks in UEFI.

_NVRAM Variables_ - Because NVRAM will be implemented at the platform level,
leveraging across silicon or other base code causes a inverse dependency.

_Hand Off Blocks_ - While handoff blocks can be used to share data, they only
solve the particular case from where the data is sourced from PEI. HOBs do not
provide any robust data management features. Additionally, HOBs can be easily
misused, for example by storing direct references to the HOB across memory
initialization.

_Config Blocks_ - Config blocks serve a similar use case as the Policy service,
but is not currently in a position to be used across the UEFI ecosystem. Config
Blocks lack some of the features such as data management abilities, or
features such as the ability to dispatch and notify on data updates.
Additionally Config Blocks lack public documentation to be widely adopted.

_Platform Configuration Database_ - PCDs, in particular dynamic PCDs, can also
be used to share data across components. However, PCDs are more focused and
prescriptive in the data type and use case while also lacking some of the data
management features and extensibility. Due to it's dependence on the build
system PCDs can also be problematic when integrating pre-compiled binary
components. The Policy Service is not intended to replace PCDs, but to provide a
more light weight and simple method for components to share data.

## Data Format

The Policy service only implements a generic blob storage and should not be
directly used in many scenarios. For managed configuration and data format, see
the [Policy Library ReadMe](./Library/README.md).

## Policy Management Process

Policies are uniquely defined by a GUID. This GUID should only be used for a
specific data type and purpose. Only one policy may be published under a
specific GUID at any given time. Duplicates will overwrite the original policy
unless the policy has been finalized via the policy attributes. Policies are
managed through a simple Get/Set/Remove interface detailed below. After their
creation, policies are universally available to all components and supported
environments unless otherwise specified in the policy attributes.

### Policy Components

The policy will be accessed by two types of components: producers and consumers.
There may be some components that are both consumers and producers by altering
an existing policy, possibly using notifications to alter the policy as soon as
it is made available.

    Producer - Creates original policy
      |--->[Consumer/Producer - Alters policy]
           |--->Consumer 1 - Reads altered policy
           |--->Consumer 2 - Reads altered Policy

The original policy will often be created in PEI to be available as soon as
possible, and may be consumed in PEI or may not be consumed until DXE.
Implementors of a new policy should consider what component is the origin of the
the data, what components may need to edit the data, and what components will
eventually use the data. In the sample code, the [PEI sample module](Samples\PolicyInterface\PolicySamplePei.c)
is the producer and the [DXE sample module](Samples\PolicyInterface\PolicySampleDxe.c)
is the consumer for the example policy shared from PEI to DXE.

Producers may also finalize the policy to prevent future accessors from writing
to the data. Finalizing will prevent specialization of a policy and so should be
done sparingly in silicon or other low-level components.

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

## Policy Service Implementation

The policy interfaces use a pass-by-copy scheme to ensure that the producers and
consumers cannot directly edit the policy data and all interactions are done in
transactions. This also allows for attributes such as _finalized_ to be strongly
enforced. Internally the PEI and DXE phase implementation have their own method
for storing policy data.

### PEI Phase

Policies created during PEI are immediately stored into a HOB. All policy HOBs
are given the same well-known GUID, gPolicyHobGuid, with a POLICY_HOB_HEADER
header to track it's metadata. When a given policy is requested the PEIM will
search the HOBs with the well-known GUID to find the matching policy header. If
a policy is removed or made obsolete with a larger version of the policy, the
header will be set with the removed flag and will no longer be evaluated by the
service.

When a policy is created or updated in the PEI phase, a NULL PPI will be
installed, or reinstalled, with the GUID of the policy. This PPI is intended to
allow for notification and dispatch of consumers when the policy becomes
available.

### DXE Phase

During it's initialization, the DXE driver will process the HOB list to discover
any valid policies that may exist. These HOB policy blocks will then be added to
the DXE driver's linked list structure of the active policies on the system. Any
policies created or updated in the DXE phase will be allocated in pool memory
and freed when removed or expanded.

Like the PEIM, the DXE driver will install/reinstall a NULL protocol with the
given policies GUID when it is created or updated to allow for notification and
dispatch on the policy availability.

## YAML Based Policy Definition

This section provides an overview of YAML based policy definition and how platform can
integrate them.

### YAML Definition for Policy Structures

When used, the YAML based policy definition is treated as the ground truth of policy structure
and default data. The YAML parser is largely inheritted from [Intel's slim bootloader](https://github.com/slimbootloader/slimbootloader).
Thus, the YAML syntax follows the specification defined in [slim bootloader](https://slimbootloader.github.io/specs/config.html#configuration-description-yaml-explained)
as well.

Such YAML definition will be used to generate header files and the field accessors for platform consumption.

### MU Added Rules

In addition to aforementioned YAML specification from slim bootloader, a few extra rules were added to the existing
specification to facilitate the adaptation of policy specific usage. These rules will be enforced by a Pre-Build
plugin, more details in its [implementation section](#pre-build-plugin).

1. Each policy definition group must include a `POLICY_HEADER_TMPL` section, as provided in this template [here](CommonPolicy/Template_PolicyHeader.yaml).
This section should include a 64-bit signature, an expected major version, an maximally expected minor version and
a size of such structure. This data will mainly be used as metadata instead of policy data. Platforms could `!include`
the provided template for easier inclusion.

1. For each non-header fields defined in the YAML policy file, developers could optionally add a `minver` field, which
denotes at which minor version this field is added. If not added, this field will be treated as 0 for default value.

1. Under the same major value, all new minor fields should only be appended after the fields with lower minor version
values, otherwise the build will break.

1. This YAML definition is not created to support UI configuration features, thus no UI related configuration fields
will be recognized in the context of policy YAML definition.

### Field Accessors

For each fields defined in YAML structures, 4 accessor functions will be created. These functions will cover the
functionality of setting this field to target value or default value, get current or default value from policy handle.

All autogen functions will be created under the naming scheme of `SET_POLICY_STRUCTURE_NAME_Field_Name`,
`SET_POLICY_STRUCTURE_NAME_Field_Name_default`, `GET_POLICY_STRUCTURE_NAME_Field_Name` and
`GET_POLICY_STRUCTURE_NAME_Field_Name_default`.

The internal implementation of these functions are dependent on `PolicyLib`, specifically the verified policy related
functionalities.

In order to simplify the usage of policy initialization, a function of `SET_POLICY_STRUCTURE_NAME_default` is created.
This function could be invoked for a platform to initialize the newly created policy handle.

### Pre-Build Plugin

A pre-build plugin is created to enforce rules indicated in the previous [section](#field-accessors).

This plugin requires 3 build environment variable to execute properly:

- `BUILD_OUTPUT_BASE`: This is used to create a temporary folder to contain intermediate files
- `UPDATE_SETTINGS`: Setting this to `false` to disable this plugin
- `POLICY_REPORT_FOLDER`: This optional variable can be used to indicate where the plugin should output the report.
- `POLICY_IGNORE_PATHS`: This optional variable can be used by platform to specify which directories or files the autogen
should ignore. Each entry should be relative UEFI path separated by colons (';').
If not supplied this report will be save to the same folder of `ACTIVE_PLATFORM`.

A policy report is the collateral output after codebase analyzing:

- During pre-build, this plugin will search through all the included package paths
for files ending with `_policy_def.yaml` or `_policy_def.yml`. Each policy component
should have its own `*_policy_def.yaml` file. i.e. `USB_policy_def.yaml` for USB
policies and `PCI_policy_def.yaml` for PCI policies.

- For each discovered policy definition, the plugin will compute a hash value of
each defined minor version with the structure and its content. This value will
be compared to the value from previously output report, if this report does not
exist, the plugin will create one.

- If the any of the rules does not meet, or any field change is detected, the build
will halt and notify the developers to fix the unexpected and update the report file.

- The plugin will also generate the header file from this YAML definition automatically,
which includes the header structure and its accessors.

### Platform Integration

For a platform to integrate the changes, the following needs to be satisfied:

#### Silicon Packages

- Define and create component policy defintions in YAML.

- Use autogen accessors and `PolicyLib` to consume policy values.

#### Platform Packages

- Use autogen accessors and `PolicyLib` to publish and/or override policy values.

- Include `POLICY_REPORT.xml` for version tracking purpose.

- Add `PolicyLib` instances to platform descriptor file.
