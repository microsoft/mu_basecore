/**

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

This library provides helper functions to create performance timing events.

**/

#include "CorePerformance2Lib.h"
#include <Library/BaseLib.h>

/**
  Copies the string from Source into Destination and updates Length with the
  size of the string.

  @param Destination - destination of the string copy
  @param Source      - pointer to the source string which will get copied
  @param Length      - pointer to a length variable to be updated

**/
VOID
CopyStringIntoPerfRecordAndUpdateLength (
  IN OUT CHAR8  *Destination,
  IN     CONST CHAR8  *Source,
  IN OUT UINT8  *Length
  )
{
  UINTN StringSize;

  if (Length == NULL || *Length > MAX_PERF_RECORD_SIZE) {
    return;
  }

  StringSize = AsciiStrnSizeS(Source, MAX_PERF_RECORD_SIZE - *Length);
  AsciiStrCpyS(Destination, StringSize, Source);
  *Length += (UINT8)StringSize;

  return;
} // CopyStringIntoPerfRecordAndUpdateLength