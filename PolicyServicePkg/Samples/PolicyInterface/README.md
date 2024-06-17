
# Policy Sample

This directory contains sample PEI and DXE modules to demonstrate a basic use of
the policy service. The [PEI module](PolicySamplePei.c) demonstrates all the
interfaces available, and creates a policy to be made available to DXE. The
[DXE module](PolicySampleDxe.c) demonstrates retrieving this policy passed from
PEI to DXE.

In this example the policy is a simple C struct, but a policy may be in whatever
data format the provider/suppliers wish.
