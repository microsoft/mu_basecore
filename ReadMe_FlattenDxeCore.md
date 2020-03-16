# Flatten DXE CORE project

## Overview

The accompanying Power Point presentation (see FlattenDXECore.pptx) describes the Flatten DXE_CORE project.  Basically, it is converting some separately loaded components (ie CpuDxe) into DXE_CORE Libraries in order to have full DXE functionality after DxeMain init.

## Phase 1 - Getting started

Phase 1 is limited to converting UefiCpuPkg\CpuDxe to a DXE_CORE library.  Eventually, it will be moved to UefiCpuPkg\Library\CpuDxeLib.  It is left where it is only to view the changes made to CpuDxe to convert it to a library.

## Notes

In this first pass, there is still a delay between the possible use of the Cpu Architectural Protocol and the instantiation of the Cpu Architectural Protocol. 