/**

This library implements the necessary functions 
to log hardware and software security locks for post-processing

Copyright (c) 2018, Microsoft Corporation

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

#ifndef __SECURITY_LOCK_LIB_H__
#define __SECURITY_LOCK_LIB_H__


#define SECURITY_LOCK_REPORT_EVENT(LockMessage,LockType) \
      SecurityLockReportEvent(&gEfiCallerIdGuid,__FUNCTION__,LockMessage,LockType); \

/**

Enum to hold the various lock types for use in post-processing

**/
typedef enum {
    SOFTWARE_LOCK = 0,
    HARDWARE_LOCK,
} LOCK_TYPE;


/**
  Function for security Lock event logging and reporting

  @param[in] Module                   GUID of calling module
  @param[in] Function                 Name of calling function
  @param[in] LockEventText            Event text explaining what is locked
  @param[in] LockType                 Enumerated lock type for differentiation
                                    
**/
VOID
EFIAPI
SecurityLockReportEvent (
  IN GUID           *Module,
  IN CONST CHAR8    *Function,
  IN CONST CHAR8    *LockEventText,
  IN LOCK_TYPE      LockType
  );

#endif