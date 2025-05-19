# IoMmuLibNull

IoMmuLib is a library that provides a simplified interface for IOMMU (Input/Output Memory Management Unit) operations.
This library abstracts the complexity of directly interfacing with the IOMMU protocol.
Eliminates the need for PCD (Platform Configuration Database) dependencies and manual protocol location.
On NULL implementations such as this one, all functions return `EFI_SUCCESS` with no depex on the IOMMU protocol.

## Overview

The library is a NULL implementation of the IoMmuLib. All library functions return `EFI_SUCCESS`.
This can be used on platforms where IOMMU is not supported. For more details see readme in IoMmuLib.
