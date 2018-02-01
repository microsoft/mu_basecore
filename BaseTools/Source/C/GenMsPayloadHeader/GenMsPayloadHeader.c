/** @file

Copyright (c) 2014, Microsoft Corporation. All rights reserved.<BR>

Module Name:

  GenMsPayloadHeader.c

Abstract:

  This contains all code necessary to build the GenMsPayloadHeader utility.       
  This code creates a MS defined payload header structure and writes the file out. 

**/

//
// File included in build
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <Common/UefiBaseTypes.h>
#include <Common/MsFmpPayloadHeaderV1.h>

#include "ParseInf.h"


#include "CommonLib.h"
#include "EfiUtilityMsgs.h"


//
// Utility Name
//
#define UTILITY_NAME  "GenMsPayloadHeader"

//
// Utility version information
//
#define UTILITY_MAJOR_VERSION 0
#define UTILITY_MINOR_VERSION 1

#define MAXDEP 5

STATIC
VOID 
Version (
  VOID
)
/*++

Routine Description:

  Displays the standard utility information to SDTOUT

Arguments:

  None

Returns:

  None

--*/
{
  fprintf (stdout, "%s Version %d.%d %s \n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION, __BUILD_VERSION);
}

STATIC
VOID 
Usage (
  VOID
  )
/*++

Routine Description:

  Displays the utility usage syntax to STDOUT

Arguments:

  None

Returns:

  None

--*/
{
  //
  // Summary usage
  //
  fprintf (stdout, "\nUsage: %s [options]\n\n", UTILITY_NAME);
  
  //
  // Copyright declaration
  // 
  fprintf (stdout, "Copyright (c) 2014, Microsoft Corporation. All rights reserved.\n\n");

  //
  // Details Option
  //
  fprintf (stdout, "Options:\n");
  fprintf (stdout, "  -o FileName, --outputfile FileName\n File is the header output plus payload.\n");
  fprintf(stdout, " -p PayloadFileName, --payload PayloadFileName\n File is payload input.\n");
  fprintf (stdout, "  --version 32 bit hex version \n");
  fprintf (stdout, "  --lsv 32 bit hex lowest supported version \n");  
  fprintf (stdout, "  --dep Guid Index Version Flags, FMP Guid, Descriptor Index, Version and flags for the dependency for this payload.\n");
 
  fprintf (stdout, "  -v, --verbose         Turn on verbose output with informational messages.\n");
  fprintf (stdout, "  -q, --quiet           Disable all messages except key message and fatal error\n");
  fprintf (stdout, "  -d, --debug level     Enable debug messages, at input debug level.\n");
  fprintf (stdout, "  --version             Show program's version number and exit.\n");
  fprintf (stdout, "  -h, --help            Show this help message and exit.\n");
}


int
main (
  IN int   argc,
  IN char  **argv
  )
/*++

Routine Description:



Arguments:

  
Returns:

  EFI_SUCCESS            No error conditions detected.
  EFI_INVALID_PARAMETER  One or more of the input parameters is invalid.
  EFI_OUT_OF_RESOURCES   A resource required by the utility was unavailable.  
                         Most commonly this will be memory allocation 
                         or file creation.
  EFI_LOAD_ERROR         GenFvImage.lib could not be loaded.
  EFI_ABORTED            Error executing the GenFvImage lib.

--*/
{
  EFI_STATUS            Status;
  UINTN					Index;
  UINTN					PayloadSize;
  CHAR8                 *OutFileName;
  FILE                  *FpFile;
  UINT64                LogLevel, TempNumber;
  FW_DEPENDENCY 		Dependency[MAXDEP];
  MS_FMP_PAYLOAD_HEADER Header;
  UINT8					Dependencies;
  CHAR8					*PayloadFileName;
  UINT8					*PayloadData;

  OutFileName   = NULL;
  FpFile        = NULL;
  PayloadFileName = NULL;
  LogLevel      = 0;
  TempNumber    = 0;
  Index = 0;
  Dependencies = 0;
  Status        = EFI_SUCCESS;
  PayloadData = NULL;
  PayloadSize = 0;

  SetUtilityName (UTILITY_NAME);
  
  if (argc == 1) {
    Error (NULL, 0, 1001, "Missing options", "No input options specified.");
    Usage ();
    return STATUS_ERROR;
  }

  //
  // Parse command line
  //
  argc --;
  argv ++;

  if ((stricmp (argv[0], "-h") == 0) || (stricmp (argv[0], "--help") == 0)) {
    Version ();
    Usage ();
    return STATUS_SUCCESS;    
  }

  if (stricmp (argv[0], "--version") == 0) {
    Version ();
    return STATUS_SUCCESS;    
  }

  while (argc > 0) {
    if ((stricmp (argv[0], "-o") == 0) || (stricmp (argv[0], "--outputfile") == 0)) {
      OutFileName = argv[1];
      if (OutFileName == NULL) {
        Error (NULL, 0, 1003, "Invalid option value", "Output file can't be null");
        return STATUS_ERROR;
      }
      argc -= 2;
      argv += 2;
      continue; 
    }

	if ((stricmp(argv[0], "-p") == 0) || (stricmp(argv[0], "--payload") == 0)) {
		PayloadFileName = argv[1];
		if (PayloadFileName == NULL) {
			Error(NULL, 0, 1003, "Invalid option value", "Payload file can't be null");
			return STATUS_ERROR;
		}
		argc -= 2;
		argv += 2;
		continue;
	}

	//
	// Parse Dependency parameter
	// --dep  <guid> <index> <ver> <flag>
	// 
    if ((stricmp (argv[0], "--dep") == 0)) {
	  if (argc < 4) {
		Error(NULL, 0, 1003, "Invalid number of arguments", "Option %s should have %d arguments but only found %d", argv[0], 4, argc-1);
		return STATUS_ERROR;
	  }
	  if (MAXDEP == Dependencies) {
		  Error(NULL, 0, 1003, "Invalid option value", "Too Many Dependencies");
	  }
	  
	  //GUID
	  Status = StringToGuid(argv[1], &(Dependency[Dependencies].FmpInstance));
	  if (EFI_ERROR(Status)) {
		  Error(NULL, 0, 1003, "Invalid option value", "%s %s %s %s %s", argv[0], argv[1], argv[2], argv[3], argv[4]);
		  return STATUS_ERROR;
	  }
	  
	  //INDEX
	  Status = AsciiStringToUint64(argv[2], TRUE, &TempNumber);
	  if (EFI_ERROR(Status) || (TempNumber >= 0x100)) {
		  Error(NULL, 0, 1003, "Invalid option value", "%s %s %s %s %s", argv[0], argv[1], argv[2], argv[3], argv[4]);
		  return STATUS_ERROR;
	  }
	  Dependency[Dependencies].ImageIndex = (UINT8)TempNumber;
	  
	  // Version 
	  Status = AsciiStringToUint64(argv[3], TRUE, &TempNumber);
	  if (EFI_ERROR(Status) || (TempNumber >= 0x100000000)) {
		  Error(NULL, 0, 1003, "Invalid option value", "%s %s %s %s %s", argv[0], argv[1], argv[2], argv[3], argv[4]);
		  return STATUS_ERROR;
	  }
	  Dependency[Dependencies].MiniumVersionInSystem = (UINT32)TempNumber;
	  
	  //Flags
	  Status = AsciiStringToUint64(argv[4], TRUE, &TempNumber); 
	  if (EFI_ERROR(Status) || (TempNumber >= 0x10000))  {
		  Error(NULL, 0, 1003, "Invalid option value", "%s %s %s %s %s", argv[0], argv[1], argv[2], argv[3], argv[4]);
		  return STATUS_ERROR;
	  }
	  Dependency[Dependencies].Flags  = (UINT16)TempNumber;
      
	  argc -= 5;
      argv += 5;
	  Dependencies++;
      continue; 
    }
	
	if((stricmp (argv[0], "--lsv") == 0)) {
		Status = AsciiStringToUint64(argv[1], TRUE, &TempNumber);
		if (EFI_ERROR(Status) || (TempNumber >= 0x100000000)) {
		    Error(NULL, 0, 1003, "Invalid option value", "%s %s", argv[0], argv[1]);
			return STATUS_ERROR;
		}
		Header.LowestSupportedVersion = (UINT32)TempNumber;
		argc-=2;
		argv+=2;
		continue;
	}
	
	if((stricmp (argv[0], "--version") == 0)) {
		Status = AsciiStringToUint64(argv[1], TRUE, &TempNumber);
		if (EFI_ERROR(Status) || (TempNumber >= 0x100000000)) {
		    Error(NULL, 0, 1003, "Invalid option value", "%s %s", argv[0], argv[1]);
			return STATUS_ERROR;
		}
		Header.FwVersion = (UINT32)TempNumber;
		argc-=2;
		argv+=2;
		continue;
	}

    if ((stricmp (argv[0], "-v") == 0) || (stricmp (argv[0], "--verbose") == 0)) {
      SetPrintLevel (VERBOSE_LOG_LEVEL);
      VerboseMsg ("Verbose output Mode Set!");
      argc --;
      argv ++;
      continue;
    }

    if ((stricmp (argv[0], "-q") == 0) || (stricmp (argv[0], "--quiet") == 0)) {
      SetPrintLevel (KEY_LOG_LEVEL);
      KeyMsg ("Quiet output Mode Set!");
      argc --;
      argv ++;
      continue;
    }

    if ((stricmp (argv[0], "-d") == 0) || (stricmp (argv[0], "--debug") == 0)) {
      Status = AsciiStringToUint64 (argv[1], FALSE, &LogLevel);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[0], argv[1]);
        return STATUS_ERROR;
      }
      if (LogLevel > 9) {
        Error (NULL, 0, 1003, "Invalid option value", "Debug Level range is 0-9, current input level is %d", (int) LogLevel);
        return STATUS_ERROR;
      }
      SetPrintLevel (LogLevel);
      DebugMsg (NULL, 0, 9, "Debug Mode Set", "Debug Output Mode Level %s is set!", argv[1]);
      argc -= 2;
      argv += 2;
      continue;
    }

    //
    // Don't recognize the parameter.
    //
    Error (NULL, 0, 1000, "Unknown option", "%s", argv[0]);
    return STATUS_ERROR;
  }

  VerboseMsg ("%s tool start.", UTILITY_NAME);

  // Make sure payload file set
  if (PayloadFileName != NULL)
  {
	  VerboseMsg("The payload file name is %s", PayloadFileName);
  }
  else 
  {
	  Error(NULL, 0, 1003, "Invalid option value", "Payload file can't be null");
	  return STATUS_ERROR;
  }

  if (OutFileName != NULL) {
    VerboseMsg ("the output file name is %s", OutFileName);
  }
  else 
  {
	  Error(NULL, 0, 1003, "Invalid option value", "Output file can't be null");
	  return STATUS_ERROR;
  }



  //read the payload file

  FpFile = fopen(PayloadFileName, "rb");

  if (FpFile == NULL) {
	  Error(NULL, 0, 0001, "Error opening file", PayloadFileName);
	  return STATUS_ERROR;
  }
  //get size and allocate memory buffer
  fseek(FpFile, 0, SEEK_END);
  PayloadSize = ftell(FpFile);
  fseek(FpFile, 0, SEEK_SET);  //go back to beginning
  PayloadData = malloc(PayloadSize);
  VerboseMsg("Payload File Size is: 0x%x", PayloadSize);
  

  if (PayloadData == NULL) {
	  Error(NULL, 0, 0001, "Error Allocating Memory to read payload file", "");
	  return STATUS_ERROR;
  }
  else
  {
	  UINTN readsize = 0;


	  //read it
	  while (readsize < PayloadSize)
	  {
		  readsize += fread((PayloadData + readsize), sizeof(UINT8), (PayloadSize - readsize), FpFile);
	  }
	  fclose(FpFile);

	  //all data read into PayloadData
	  VerboseMsg("Read 0x%x Bytes of Payload Data", readsize);
  }

  
  //set signature
  Header.Identifier = MS_PAYLOAD_HEADER_IDENTIFIER;
  
  //calculate the size of total header
  Header.HeaderSize = sizeof(Header) + (sizeof(FW_DEPENDENCY) * Dependencies);

  //Start writing output
  FpFile = fopen(OutFileName, "wb");

  if (FpFile == NULL) {
	  Error(NULL, 0, 0001, "Error opening output file", OutFileName);
	  return STATUS_ERROR;
  }

  //write out header
  VerboseMsg("Writing out MS Payload Header");
  fwrite(&Header, sizeof(Header), 1, FpFile);
  
  for(Index = 0; Index < Dependencies; Index++) 
  {
    VerboseMsg("Write out a dependency on version 0x%x", Dependency[Index].MiniumVersionInSystem);
	fwrite(&Dependency[Index], sizeof(FW_DEPENDENCY), 1, FpFile);
  }

  //write out payload
  VerboseMsg("Writing out Payload File Data");
  fwrite(PayloadData, PayloadSize, sizeof(UINT8), FpFile);
  fflush(FpFile);
  fclose(FpFile);
  
  return GetUtilityStatus ();
}
