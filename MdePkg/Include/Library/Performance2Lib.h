/**
Provides services to log the execution times.

Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017, Microsoft Corporation

All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#ifndef __PERFORMANCE_LIB_2_H__
#define __PERFORMANCE_LIB_2_H__

//
// Performance library propery mask bits
//
//#define PERFORMANCE_LIBRARY_PROPERTY_MEASUREMENT_ENABLED  0x00000001

//
// Performance library verbosity levels
//
#define PERF_VERBOSITY_LOW      1
#define PERF_VERBOSITY_STANDARD 2
#define PERF_VERBOSITY_HIGH     3

//
// Performance library core functionality enable bits
//
#define PERF_CORE_FUNC_ENTRYPOINT       BIT0
#define PERF_CORE_FUNC_LOADIMAGE        BIT1
#define PERF_CORE_FUNC_BINDING_SUPPORT  BIT2
#define PERF_CORE_FUNC_BINDING_START    BIT3
#define PERF_CORE_FUNC_BINDING_STOP     BIT4

//
// Progress IDs
//
#define PERF_EVENT_ID                 0x0
#define PERF_ENTRYPOINT_BEGIN_ID      0x1
#define PERF_ENTRYPOINT_END_ID        0x2
#define PERF_LOADIMAGE_BEGIN_ID       0x3
#define PERF_LOADIMAGE_END_ID         0x4
#define PERF_BINDING_SUPPORT_BEGIN_ID 0x5
#define PERF_BINDING_SUPPORT_END_ID   0x6
#define PERF_BINDING_START_BEGIN_ID   0x7
#define PERF_BINDING_START_END_ID     0x8
#define PERF_BINDING_STOP_BEGIN_ID    0x9
#define PERF_BINDING_STOP_END_ID      0xA
#define PERF_EVENTSIGNAL_BEGIN_ID     0xB
#define PERF_EVENTSIGNAL_END_ID       0xC
#define PERF_CALLBACK_BEGIN_ID        0xD
#define PERF_CALLBACK_END_ID          0xE
#define PERF_FUNCTION_BEGIN_ID        0xF
#define PERF_FUNCTION_END_ID          0x10
#define PERF_INMODULE_BEGIN_ID        0x11
#define PERF_INMODULE_END_ID          0x12
#define PERF_CROSSMODULE_BEGIN_ID     0x13
#define PERF_CROSSMODULE_END_ID       0x14

//
// Perf macros
//

//
// Macros for instrumenting the Core (PEI, DXE, SMM). Below macros are toggled by a bitmask returned from
// GetPerformanceCoreFunctionalityMask - see function comment header below for details.
//
#define PERF_ENTRYPOINT_BEGIN(ModuleHandle) \
  do { \
    if (GetPerformanceCoreFunctionalityMask () & PERF_CORE_FUNC_ENTRYPOINT) { \
      LogPerformanceMeasurement (ModuleHandle, NULL, NULL, 0, PERF_ENTRYPOINT_BEGIN_ID); \
    } \
  } while (FALSE)

#define PERF_ENTRYPOINT_END(ModuleHandle) \
  do { \
    if (GetPerformanceCoreFunctionalityMask () & PERF_CORE_FUNC_ENTRYPOINT) { \
      LogPerformanceMeasurement (ModuleHandle, NULL, NULL, 0, PERF_ENTRYPOINT_END_ID); \
    } \
  } while (FALSE)

#define PERF_LOADIMAGE_BEGIN() \
  do { \
    if (GetPerformanceCoreFunctionalityMask () & PERF_CORE_FUNC_LOADIMAGE) { \
      LogPerformanceMeasurement (NULL, NULL, NULL, 0, PERF_LOADIMAGE_BEGIN_ID); \
    } \
  } while (FALSE)

#define PERF_LOADIMAGE_END(ModuleHandle) \
  do { \
    if (GetPerformanceCoreFunctionalityMask () & PERF_CORE_FUNC_LOADIMAGE) { \
      LogPerformanceMeasurement (ModuleHandle, NULL, NULL, 0, PERF_LOADIMAGE_END_ID); \
    } \
  } while (FALSE)

#define PERF_BINDING_SUPPORT_BEGIN(ModuleHandle, ControllerHandle) \
  do { \
    if (GetPerformanceCoreFunctionalityMask () & PERF_CORE_FUNC_BINDING_SUPPORT) { \
      LogPerformanceMeasurement (ModuleHandle, NULL, NULL, (UINT64)ControllerHandle, PERF_BINDING_SUPPORT_BEGIN_ID); \
    } \
  } while (FALSE)

#define PERF_BINDING_SUPPORT_END(ModuleHandle, ControllerHandle) \
  do { \
    if (GetPerformanceCoreFunctionalityMask () & PERF_CORE_FUNC_BINDING_SUPPORT) { \
      LogPerformanceMeasurement (ModuleHandle, NULL, NULL, (UINT64)ControllerHandle, PERF_BINDING_SUPPORT_END_ID); \
    } \
  } while (FALSE)

#define PERF_BINDING_START_BEGIN(ModuleHandle, ControllerHandle) \
  do { \
    if (GetPerformanceCoreFunctionalityMask () & PERF_CORE_FUNC_BINDING_START) { \
      LogPerformanceMeasurement (ModuleHandle, NULL, NULL, (UINT64)ControllerHandle, PERF_BINDING_START_BEGIN_ID); \
    } \
  } while (FALSE)

#define PERF_BINDING_START_END(ModuleHandle, ControllerHandle) \
  do { \
    if (GetPerformanceCoreFunctionalityMask () & PERF_CORE_FUNC_BINDING_START) { \
      LogPerformanceMeasurement (ModuleHandle, NULL, NULL, (UINT64)ControllerHandle, PERF_BINDING_START_END_ID); \
    } \
  } while (FALSE)

#define PERF_BINDING_STOP_BEGIN(ModuleHandle, ControllerHandle) \
  do { \
    if (GetPerformanceCoreFunctionalityMask () & PERF_CORE_FUNC_BINDING_STOP) { \
      LogPerformanceMeasurement (ModuleHandle, NULL, NULL, (UINT64)ControllerHandle, PERF_BINDING_STOP_BEGIN_ID); \
    } \
  } while (FALSE)

#define PERF_BINDING_STOP_END(ModuleHandle, ControllerHandle) \
  do { \
    if (GetPerformanceCoreFunctionalityMask () & PERF_CORE_FUNC_BINDING_STOP) { \
      LogPerformanceMeasurement (ModuleHandle, NULL, NULL, (UINT64)ControllerHandle, PERF_BINDING_STOP_END_ID); \
    } \
  } while (FALSE)

//
// Macros for instrumenting any module. Below macros accept a verbosity level returned by
// GetPerformanceVerbosityLevel - see function comment header below for details.
//
#define PERF_EVENT(Verbosity, EventString) \
  do { \
    if (GetPerformanceVerbosityLevel () >= Verbosity) { \
      LogPerformanceMeasurement (&gEfiCallerIdGuid, NULL, EventString, 0, PERF_EVENT_ID); \
    } \
  } while (FALSE)

#define PERF_EVENTSIGNAL_BEGIN(Verbosity, EventGuid) \
  do { \
    if (GetPerformanceVerbosityLevel () >= Verbosity) { \
      LogPerformanceMeasurement (&gEfiCallerIdGuid, EventGuid, __FUNCTION__, 0, PERF_EVENTSIGNAL_BEGIN_ID); \
    } \
  } while (FALSE)

#define PERF_EVENTSIGNAL_END(Verbosity, EventGuid) \
  do { \
    if (GetPerformanceVerbosityLevel () >= Verbosity) { \
      LogPerformanceMeasurement (&gEfiCallerIdGuid, EventGuid, __FUNCTION__, 0, PERF_EVENTSIGNAL_END_ID); \
    } \
  } while (FALSE)

#define PERF_CALLBACK_BEGIN(Verbosity, TriggerGuid) \
  do { \
    if (GetPerformanceVerbosityLevel () >= Verbosity) { \
      LogPerformanceMeasurement (&gEfiCallerIdGuid, TriggerGuid, __FUNCTION__, 0, PERF_CALLBACK_BEGIN_ID); \
    } \
  } while (FALSE)

#define PERF_CALLBACK_END(Verbosity, TriggerGuid) \
  do { \
    if (GetPerformanceVerbosityLevel () >= Verbosity) { \
      LogPerformanceMeasurement (&gEfiCallerIdGuid, TriggerGuid, __FUNCTION__, 0, PERF_CALLBACK_END_ID); \
    } \
  } while (FALSE)

#define PERF_FUNCTION_BEGIN(Verbosity) \
  do { \
    if (GetPerformanceVerbosityLevel () >= Verbosity) { \
      LogPerformanceMeasurement (&gEfiCallerIdGuid, NULL, __FUNCTION__, 0, PERF_FUNCTION_BEGIN_ID); \
    } \
  } while (FALSE)

#define PERF_FUNCTION_END(Verbosity) \
  do { \
    if (GetPerformanceVerbosityLevel () >= Verbosity) { \
      LogPerformanceMeasurement (&gEfiCallerIdGuid, NULL, __FUNCTION__, 0, PERF_FUNCTION_END_ID); \
    } \
  } while (FALSE)

#define PERF_INMODULE_BEGIN(Verbosity, MeasurementString) \
  do { \
    if (GetPerformanceVerbosityLevel () >= Verbosity) { \
      LogPerformanceMeasurement (&gEfiCallerIdGuid, NULL, MeasurementString, 0, PERF_INMODULE_BEGIN_ID); \
    } \
  } while (FALSE)

#define PERF_INMODULE_END(Verbosity, MeasurementString) \
  do { \
    if (GetPerformanceVerbosityLevel () >= Verbosity) { \
      LogPerformanceMeasurement (&gEfiCallerIdGuid, NULL, MeasurementString, 0, PERF_INMODULE_END_ID); \
    } \
  } while (FALSE)

#define PERF_CROSSMODULE_BEGIN(Verbosity, MeasurementString) \
  do { \
    if (GetPerformanceVerbosityLevel () >= Verbosity) { \
      LogPerformanceMeasurement (&gEfiCallerIdGuid, NULL, MeasurementString, 0, PERF_CROSSMODULE_BEGIN_ID); \
    } \
  } while (FALSE)

#define PERF_CROSSMODULE_END(Verbosity, MeasurementString) \
  do { \
    if (GetPerformanceVerbosityLevel () >= Verbosity) { \
      LogPerformanceMeasurement (&gEfiCallerIdGuid, NULL, MeasurementString, 0, PERF_CROSSMODULE_END_ID); \
    } \
  } while (FALSE)

/**
  Macro that marks the beginning of performance measurement source code.

  If the verbosity level obtained from PcdPerformance2LibraryVerbosityLevel is non-zero,
  then this macro marks the beginning of source code that is included in a module.
  Otherwise, the source lines between PERF_CODE_BEGIN() and PERF_CODE_END() are not included in a module.

**/
#define PERF_CODE_BEGIN()  do { if (GetPerformanceVerbosityLevel ()) { UINT8  __PerformanceCodeLocal

/**
  Macro that marks the end of performance measurement source code.

  If the verbosity level obtained from PcdPerformance2LibraryVerbosityLevel is non-zero,
  then this macro marks the end of source code that is included in a module.
  Otherwise, the source lines between PERF_CODE_BEGIN() and PERF_CODE_END() are not included in a module.

**/
#define PERF_CODE_END()    __PerformanceCodeLocal = 0; __PerformanceCodeLocal++; } } while (FALSE)

/**
  Macro that declares a section of performance measurement source code.

  If the verbosity level obtained from PcdPerformance2LibraryVerbosityLevel is non-zero,
  then the source code specified by Expression is included in a module.
  Otherwise, the source specified by Expression is not included in a module.

  @param  Expression - Performance measurement source code to include in a module.

**/
#define PERF_CODE(Expression)  \
  PERF_CODE_BEGIN ();          \
  Expression                   \
  PERF_CODE_END ()

/**
  Returns performance logging verbosity obtained from
  PcdPerformance2LibraryVerbosityLevel.

  @retval - 0 - Performance logging turned off.
  @retval - 1 - Low verbosity performance logging.
  @retval - 2 - Standard verbosity performance logging.
  @retval - 3 - High verbosity performance logging.
**/
UINT8
EFIAPI
GetPerformanceVerbosityLevel (
  VOID
  );

/**
  Returns performnce library's core functionality mask with bits defined as:
    BIT0 - 1 - Enable Entrypoint Logging.
           0 - Disable Entrypoint Logging.
    BIT1 - 1 - Enable Load Image logging.
           0 - Disable Load Image Logging.
    BIT2 - 1 - Enable Binding Support logging.
           0 - Disable Binding Support Logging.
    BIT3 - 1 - Enable Binding Start logging.
           0 - Disable Binding Start Logging.
    BIT4 - 1 - Enable Binding Stop logging.
           0 - Disable Binding Stop Logging.
    Bits 7-5 - Reserved
  The mask is obtained from PcdPerformance2LibraryCoreFunctionalityMask.

  @retval - Performnce library's core functionality mask.
**/
UINT8
EFIAPI
GetPerformanceCoreFunctionalityMask (
  VOID
  );

/**
  Create performance record with event description and a timestamp.

  @param CallerIdentifier  - Image handle or pointer to caller ID GUID
  @param Guid              - Pointer to a GUID
  @param String            - Pointer to a string describing the measurement
  @param Address           - Pointer to a location in memory relevant to the measurement
  @param PerfId            - Performance identifier describing the type of measurement

  @retval RETURN_SUCCESS           - Successfully created performance record
  @retval RETURN_OUT_OF_RESOURCES  - Ran out of space to store the records
  @retval RETURN_INVALID_PARAMETER - Invalid parameter passed to function - NULL
                                     pointer or invalid PerfId

**/
RETURN_STATUS
EFIAPI
LogPerformanceMeasurement (
  IN CONST VOID   *CallerIdentifier,  OPTIONAL
  IN CONST VOID   *Guid,    OPTIONAL
  IN CONST CHAR8  *String,  OPTIONAL
  IN UINT64       Address,  OPTIONAL
  IN UINT16       PerfId
  );

//
// No implementation for GetPerformanceMeasurement
//

#endif
