// ------------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
// MU_CHANGE: WHOLE FILE - Add strcmp
//
// ------------------------------------------------------------------------------

int
strcmp (
  const char  *,
  const char  *
  );

#if defined (_MSC_VER)
  #pragma intrinsic(strcmp)
  #pragma function(strcmp)
#endif

int
strcmp (
  const char  *s1,
  const char  *s2
  )
{
  while ((*s1 != '\0') && (*s1 == *s2)) {
    s1++;
    s2++;
  }

  return *s1 - *s2;
}
