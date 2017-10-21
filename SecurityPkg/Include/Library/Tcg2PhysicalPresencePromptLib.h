/** @file -- Tcg2PhysicalPresencePromptLib.h
This library abstracts the action of prompting the user so that it may be overridden in a platform-specific way.
Rather than just printing to the screen.

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

// MS_CHANGE: Entire file created.

#ifndef _TCG2_PHYSICAL_PRESENCE_PROMPT_LIB_H_
#define _TCG2_PHYSICAL_PRESENCE_PROMPT_LIB_H_

/**
  Simple function to inform any callers of whether the lib is ready to present a prompt.
  Since the prompt itself only returns TRUE or FALSE, make sure all other technical requirements
  are out of the way.

  @retval     EFI_SUCCESS       Prompt is ready.
  @retval     EFI_NOT_READY     Prompt does not have sufficient resources at this time.
  @retval     EFI_DEVICE_ERROR  Library failed to prepare resources.

**/
EFI_STATUS
EFIAPI
IsPromptReady (
  VOID
  );


/**
  This function will take in a prompt string to present to the user in a
  OK/Cancel dialog box and return TRUE if the user actively pressed OK. Returns
  FALSE on Cancel or any errors.

  @param[in]  PromptString  The string that should occupy the body of the prompt.

  @retval     TRUE    User confirmed action.
  @retval     FALSE   User rejected action or a failure occurred.

**/
BOOLEAN
EFIAPI
PromptForUserConfirmation (
  IN  CHAR16    *PromptString
  );

#endif // _TCG2_PHYSICAL_PRESENCE_PROMPT_LIB_H_
