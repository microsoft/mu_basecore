# Pei Memory Buckets

Pei memory buckets consolidate Pei memory allocations to be contiguous to reduce fragmentation of the
memory map and is only used by the Pei Core.  Currently this is only used for EfiRuntimeServicesCode
EfiRuntimeServicesData, EfiACPIReclaimMemory and EfiACPIMemoryNVS but additional memory types can
be added.

New Fixed PCDs were added to set the number of pages for each memory type that are currently used.
The relevant PCDs are PcdPeiMemoryBucketRuntimeCode, PcdPeiMemoryBucketRuntimeData, PcdPeiMemoryBucketAcpiReclaimMemory
and PcdPeiMemoryBucketAcpiMemoryNvs.

## How to enable Pei Memory Buckets

All the added PCDs have a default value of 0 and if their values remain unedited then PEI memory buckets will not be
used and memory allocation will be the same as before.  Setting any of these values to be non-zero will have you using
the memory bucket infrastructure.  If you have any of these values set as a non-zero value and no PEI allocations for
the relevant memory types are made then no memory bucket structures will be allocated or used.

Be aware that the memory bucket sizes should be tuned match the number of pages each memory type uses in PEI to be the
most space efficient.  
