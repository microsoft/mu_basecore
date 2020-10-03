/** @file -- TempPreUefiEventLogLib.c
NOTE: This is a temporary shim to resolve a build error while a more permanent solution is tested!

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>

/**
  Create the EventLog entries.
**/
VOID
CreateTcg2PreUefiEventLogEntries (
  VOID
  )
{
  DEBUG(( DEBUG_ERROR, __FUNCTION__" - This is just a shim and should not be used in real builds!!\n" ));
  // WE NEED TO FINISH TESTING THE ACTUAL SOLUTION!
  ASSERT( FALSE );
  return;
}
