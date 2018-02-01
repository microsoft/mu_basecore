/** @file

Copyright (c) 2014, Microsoft Corporation. All rights reserved.<BR>

Module Name:

  GenFmpCap.c

Abstract:

  This contains all code necessary to build the GenFmpCap.exe utility.       
  This code creates Firmware Managemenat Protocol compliant capsules

**/

//
// File included in build
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <Common/UefiBaseTypes.h>
#include <Common/UefiFmpCapsule.h>

#include "ParseInf.h"


#include "CommonLib.h"
#include "EfiUtilityMsgs.h"


//
// Utility Name
//
#define UTILITY_NAME  "GenFmpCap"

//
// Utility version information
//
#define UTILITY_MAJOR_VERSION 0
#define UTILITY_MINOR_VERSION 1

#define MaxPE 10
#define MaxDE 10


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
  fprintf (stdout, "  -o FileName, --outputfile FileName\n File is the FMP compliant UEFI Capsule body to be created.\n");
  fprintf (stdout, "  -p PayloadFileName Guid Index VendorCodeSize, --payloadfile FileName Guid Index VendorCodeSize\n File is a capsule payload file.\n");
  fprintf (stdout, "  --driverfile DriverFileName\n File is a driver file to add to the capsule. \n");
  fprintf (stdout, "  --dumpinfo FMP capsule payload\n File is the payload of a capsule in FMP format. \n");
  fprintf (stdout, "  --dumpcap UefiCapFile \n File is the UEFI capsule file with payload in FMP format. \n");
 
  fprintf (stdout, "  -v, --verbose         Turn on verbose output with informational messages.\n");
  fprintf (stdout, "  -q, --quiet           Disable all messages except key message and fatal error\n");
  fprintf (stdout, "  -d, --debug level     Enable debug messages, at input debug level.\n");
  fprintf (stdout, "  --version             Show program's version number and exit.\n");
  fprintf (stdout, "  -h, --help            Show this help message and exit.\n");
}


typedef struct {
	CHAR8*		FileName;
	EFI_GUID	Guid;
	UINT8		  Index;
	UINT32		VendorCodeSize;
  UINT64    HardwareInstance;
	UINT32		DataSize;
	UINT8*		FileData;
} PayloadFileDesc;

typedef struct {
	CHAR8*		FileName;
	UINT32		DataSize;
	UINT8*		FileData;
} DriverFileDesc;


//
EFI_STATUS
ParseAndPrintFmpInfo(
	IN UINT8*  Data
)
{
	EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER*	fm_cap_img_header;
	EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER*			fm_cap_header;
	UINTN Index = 0;
	UINT64 Offset = 0;


	if (Data == NULL)
	{
		VerboseMsg("ParseAndPrintFmpInfo - Data can not be NULL!");
		return STATUS_ERROR;
	}

	fm_cap_header = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER*)Data;

	if (fm_cap_header->Version != EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER_INIT_VERSION) {
		Error(NULL, 0, 1001, "Unknown Version Type", "This tool does not support this version. ");
		return STATUS_ERROR;
	}

	fprintf(stdout, "\n\nDumping  EFI_FIRMWARE_MANAGEMENT PROTOCOL FORMATTED CAPSULE INFORMATION\n");
	fprintf(stdout, "-------------- EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER ----------------------\n");
	fprintf(stdout, "Version:               0x%x\n", fm_cap_header->Version);
	fprintf(stdout, "Embedded Driver Count: 0x%x\n", fm_cap_header->EmbeddedDriverCount);
	fprintf(stdout, "Payload Item Count:    0x%x\n", fm_cap_header->PayloadItemCount);

	for (Index = 0; Index < fm_cap_header->EmbeddedDriverCount; Index++)
	{
		Offset = *((UINT64 *)(Data + sizeof(EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER) + (Index * 8)));
		fprintf(stdout, "Embedded Driver %d at Offset: 0x%x\n", Index, Offset);
		VerboseMsg("ParseAndPrintFmpInfo - Finished Processing Driver Item %d!\n", Index);

		//TODO: dump something interesting about the driver.  GUID or name or something. 
	}

	for (Index = 0; Index < fm_cap_header->PayloadItemCount; Index++)
	{
		Offset = *((UINT64 *)(Data + sizeof(EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER) + ((fm_cap_header->EmbeddedDriverCount + Index) * 8)));
		fprintf(stdout, "Payload Item %d at Offset: 0x%x\n", Index, Offset);
		fm_cap_img_header = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER*)(Data + Offset);
		fprintf(stdout, "-------------- EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER ----------------------\n");
		
		//check for unknown version
		if (fm_cap_img_header->Version != EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER_INIT_VERSION) {
			fprintf(stdout, "Unknown/Unsupported Version 0x%x\n", fm_cap_img_header->Version);
			continue;
		}
		fprintf(stdout, "  Version:               0x%x\n", fm_cap_img_header->Version);
		fprintf(stdout, "  Guid:               ");
		PrintGuid(&fm_cap_img_header->UpdateImageTypeId);
		fprintf(stdout, "\n");

		fprintf(stdout, "  Index:                 0x%x\n", fm_cap_img_header->UpdateImageIndex);
		fprintf(stdout, "  UpdateImageSize:       0x%x\n", fm_cap_img_header->UpdateImageSize);
		fprintf(stdout, "  UpdateVendorCodeSize:  0x%x\n", fm_cap_img_header->UpdateVendorCodeSize);
    fprintf(stdout, "  HardwareInstance:      0x%X\n", fm_cap_img_header->UpdateHardwareInstance);
		VerboseMsg("ParseAndPrintFmpInfo - Finished Processing Payload Item %d!\n", Index);
	}
	fprintf(stdout, "----------------------------------------------------------------------------\n\n\n");
	return STATUS_SUCCESS;
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
  DriverFileDesc        DriverFileList[MaxDE];
  PayloadFileDesc       PayloadFileList[MaxPE];
  UINTN					PayloadFreeIndex;
  UINTN					DriverFreeIndex;
  UINTN					Index;
  CHAR8                 *OutFileName;
  CHAR8					*InputFmpPayloadFileName;  //just the fmp payload
  CHAR8		            *InputCapsuleFileName;		//UEFI capsule
  FILE                  *FpFile;
  UINT64                LogLevel, TempNumber;
  UINT64				Offset;

  EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER* pfm_cap_img_header;
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER fm_cap_header;
  int tread = 0;
  int readsize = 0;


  OutFileName   = NULL;
  FpFile        = NULL;
  InputFmpPayloadFileName = NULL;
  InputCapsuleFileName = NULL;
  LogLevel      = 0;
  TempNumber    = 0;
  Index = 0;
  PayloadFreeIndex = 0;
  DriverFreeIndex = 0;
  Status        = EFI_SUCCESS;
  pfm_cap_img_header = NULL;

  SetUtilityName (UTILITY_NAME);
  
  if (argc == 1) {
    Error (NULL, 0, 1001, "Missing options", "No input options specified.");
    Usage ();
    return STATUS_ERROR;
  }

  //
  // Init global data to Zero
  //

   
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

	//
	// Parse Driver file parameter
	// --driverfile <driver file name>
	//
	if ((stricmp(argv[0], "--driverfile") == 0)) {
		if (MaxDE == DriverFreeIndex) {
			Error(NULL, 0, 1003, "Invalid option value", "Too Many Driver Files");
		}

		DriverFileList[DriverFreeIndex].FileName = argv[1];
		if (DriverFileList[DriverFreeIndex].FileName == NULL) {
			Error(NULL, 0, 1003, "Invalid option value", "Driver file can't be null");
			return STATUS_ERROR;
		}
		argc -= 2;
		argv += 2;
		DriverFreeIndex++;
		continue;
	}

	//
	// Parse the dumpinfo file parameter
	// --dumpinfo <Capsule payload file>
	//
	if ((stricmp(argv[0], "--dumpinfo") == 0)) {

		InputFmpPayloadFileName = argv[1];
		if (InputFmpPayloadFileName == NULL) {
			Error(NULL, 0, 1003, "Invalid option value", "DumpInfo file can't be null");
			return STATUS_ERROR;
		}
		argc -= 2;
		argv += 2;
		continue;
	}

	//  
	// Parse Payload parameter
	// -p <filename> <guid> <index> <vendorcodesize>
	// 
  //TODO:  add input parameter suppport for <HardwareInstance>.  right now just use 0  

    if ((stricmp (argv[0], "-p") == 0) || (stricmp (argv[0], "--payloadfile") == 0)) {
	  if (argc < 4) {
		Error(NULL, 0, 1003, "Invalid number of arguments", "Option %s should have %d arguments but only found %d", argv[0], 4, argc-1);
		return STATUS_ERROR;
	  }
	  if (MaxPE == PayloadFreeIndex) {
		  Error(NULL, 0, 1003, "Invalid option value", "Too Many Payload Files");
	  }

	  PayloadFileList[PayloadFreeIndex].FileName = argv[1];

    // Parse the guid
	  Status = StringToGuid(argv[2], &(PayloadFileList[PayloadFreeIndex].Guid));
	  if (EFI_ERROR(Status)) {
		  Error(NULL, 0, 1003, "Invalid option value", "%s %s %s %s %s", argv[0], argv[1], argv[2], argv[3], argv[4]);
		  return STATUS_ERROR;
	  }

    //parse the Index
	  Status = AsciiStringToUint64(argv[3], FALSE, &TempNumber);
	  if (EFI_ERROR(Status) || (TempNumber > 0xF)) {
		  Error(NULL, 0, 1003, "Invalid option value", "%s %s %s %s %s", argv[0], argv[1], argv[2], argv[3], argv[4]);
		  return STATUS_ERROR;
	  }
	  PayloadFileList[PayloadFreeIndex].Index = (UINT8)TempNumber;

    //Parse the VendorCode Size
	  Status = AsciiStringToUint64(argv[4], FALSE, &TempNumber); 
	  if (EFI_ERROR(Status)) {
		  Error(NULL, 0, 1003, "Invalid option value", "%s %s %s %s %s", argv[0], argv[1], argv[2], argv[3], argv[4]);
		  return STATUS_ERROR;
	  }
	  PayloadFileList[PayloadFreeIndex].VendorCodeSize = (UINT32)TempNumber;

    //Parse the Hardware Instance  
    //TODO:  hard code to 0 for now as we don't need this function yet
    /*
    Status = AsciiStringToUint64(argv[5], FALSE, &TempNumber);
    if (EFI_ERROR(Status)) {
      Error(NULL, 0, 1003, "Invalid option value", "%s %s %s %s %s", argv[0], argv[1], argv[2], argv[3], argv[4]);
      return STATUS_ERROR;
    }
    */
    TempNumber = 0;
    PayloadFileList[PayloadFreeIndex].HardwareInstance = TempNumber;

      argc -= 5;
      argv += 5;
	  PayloadFreeIndex++;
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

  //
  // User requesting to dump the 
  //
  if (InputFmpPayloadFileName != NULL)
  {
	  int tr = 0;
	  int rs = 0;
	  UINT8 *Data = NULL;

	  FpFile = fopen(InputFmpPayloadFileName, "rb");
	  if (FpFile == NULL) {
		  Error(NULL, 0, 0001, "Error opening file", InputFmpPayloadFileName);
		  return STATUS_ERROR;
	  }
	  fseek(FpFile, 0, SEEK_END);
	  tr = ftell(FpFile);  //ammount of data to read
	  fseek(FpFile, 0, SEEK_SET);
	  Data = malloc(tr);
	  if (Data == NULL)
	  {
		  Error(NULL, 0, 0001, "Error allocating Memory", InputFmpPayloadFileName);
		  return STATUS_ERROR;
	  }
	  while (rs < tr)
	  {
		  rs += fread((Data + rs), sizeof(UINT8), (tr - rs), FpFile);
	  }
	  fclose(FpFile);

	  
	  Status = ParseAndPrintFmpInfo(Data);
	  if (EFI_ERROR(Status))
	  {
		  Error(NULL, 0, 0001, "Error Parsing FMP Info Data", InputFmpPayloadFileName);
		  return STATUS_ERROR;
	  }
  }

	  
  
  if (OutFileName != NULL) {
    VerboseMsg ("the output file name is %s", OutFileName);
  }

  //Read all payload files
  for (Index = 0; Index < PayloadFreeIndex; Index++)
  {
	FpFile = fopen(PayloadFileList[Index].FileName, "rb");
	
	if (FpFile == NULL) {
		Error(NULL, 0, 0001, "Error opening file", PayloadFileList[Index].FileName);
		return STATUS_ERROR;
	}
	fseek(FpFile, 0, SEEK_END);
	tread = ftell(FpFile);  //ammount of data to read
	fseek(FpFile, 0, SEEK_SET);

	PayloadFileList[Index].DataSize = tread + sizeof(EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER); //add header size
	PayloadFileList[Index].FileData = malloc(PayloadFileList[Index].DataSize);

	if (PayloadFileList[Index].FileData == NULL) {
		Error(NULL, 0, 0001, "Error Allocating Memory to read payload file", PayloadFileList[Index].FileName);
		return STATUS_ERROR;
	}

	//setup the EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER
	pfm_cap_img_header = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER *)PayloadFileList[Index].FileData;
	pfm_cap_img_header->Version = EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER_INIT_VERSION;
	pfm_cap_img_header->UpdateImageIndex = PayloadFileList[Index].Index;
	pfm_cap_img_header->UpdateImageSize = tread - PayloadFileList[Index].VendorCodeSize;
	pfm_cap_img_header->UpdateVendorCodeSize = PayloadFileList[Index].VendorCodeSize;
  pfm_cap_img_header->UpdateHardwareInstance = PayloadFileList[Index].HardwareInstance;
	CopyMem(&pfm_cap_img_header->UpdateImageTypeId, &PayloadFileList[Index].Guid, sizeof(EFI_GUID));

	
	//read it
	//tread is the filesize
	//readsize is amount of data read from file
	//FiledData = header + file
	//DataSize = Header + file (file contains security stuff + image + vendor code)  
	readsize = 0;
	while (readsize < tread )
	{
		readsize += fread((PayloadFileList[Index].FileData + sizeof(EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER) + readsize), sizeof(UINT8), (tread - readsize), FpFile);
	}
	fclose(FpFile);
	VerboseMsg("Finished Reading %d bytes from File %s.  PayloadFile Index %d complete.", tread, PayloadFileList[Index].FileName, Index);

  } //close loop for all payload files

  //Read all driver files
  for (Index = 0; Index < DriverFreeIndex; Index++)
  {
	  int tread = 0;
	  int readsize = 0;
	  FpFile = fopen(DriverFileList[Index].FileName, "rb");

	  if (FpFile == NULL) {
		  Error(NULL, 0, 0001, "Error opening file", DriverFileList[Index].FileName);
		  return STATUS_ERROR;
	  }
	  fseek(FpFile, 0, SEEK_END);
	  DriverFileList[Index].DataSize = ftell(FpFile);
	  fseek(FpFile, 0, SEEK_SET);

	  DriverFileList[Index].FileData = malloc(DriverFileList[Index].DataSize);

	  if (DriverFileList[Index].FileData == NULL) {
		  Error(NULL, 0, 0001, "Error Allocating Memory to read driver file", DriverFileList[Index].FileName);
		  return STATUS_ERROR;
	  }

	  tread = DriverFileList[Index].DataSize;

	  //read it
	  readsize = 0;
	  while (readsize < tread)
	  {
		  readsize += fread((DriverFileList[Index].FileData + readsize), sizeof(UINT8), (tread - readsize), FpFile);
	  }
	  fclose(FpFile);

	  VerboseMsg("Finished Reading %d bytes from File %s.  DriverFile Index %d complete.", DriverFileList[Index].DataSize, DriverFileList[Index].FileName, Index);
  } //close loop for all driver files


  if ((DriverFreeIndex == 0) && (PayloadFreeIndex == 0))
  {
	  Error(NULL, 0, 0001, "No Files for capsule", "Exiting");
	  return STATUS_ERROR;
  }
  //Start

  FpFile = fopen(OutFileName, "wb");

  if (FpFile == NULL) {
	  Error(NULL, 0, 0001, "Error opening output file", OutFileName);
	  return STATUS_ERROR;
  }

  fm_cap_header.Version = EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER_INIT_VERSION;
  fm_cap_header.EmbeddedDriverCount = (UINT16)DriverFreeIndex;
  fm_cap_header.PayloadItemCount = (UINT16)PayloadFreeIndex;

  //write out header
  fwrite(&fm_cap_header, sizeof(EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER), 1, FpFile);

  VerboseMsg("Writing out Firmware Management Capsule Header");

  //base offset past header
  Offset = sizeof(EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER) + ( sizeof(UINT64) * (fm_cap_header.EmbeddedDriverCount + fm_cap_header.PayloadItemCount));

  for (Index = 0; Index < DriverFreeIndex; Index++) {
	  fwrite(&Offset, sizeof(UINT64), 1, FpFile);
	  VerboseMsg("Writing out an offset 0x%x for Driver Item  %d", Offset, Index);
	  Offset += DriverFileList[Index].DataSize;
  }
  for (Index = 0; Index < PayloadFreeIndex; Index++) {
	  fwrite(&Offset, sizeof(UINT64), 1, FpFile);
	  VerboseMsg("Writing out an offset 0x%x for Payload Item  %d", Offset, Index);
	  Offset += PayloadFileList[Index].DataSize;
  }

  //now write out the real files
  for (Index = 0; Index < DriverFreeIndex; Index++) {
	  tread = DriverFileList[Index].DataSize;
	  readsize = 0;
	  while (readsize < tread)
	  {
		  readsize += fwrite((DriverFileList[Index].FileData + readsize), sizeof(UINT8), (tread - readsize), FpFile);
	  }
	  VerboseMsg("Finished Writing out an Driver File %s", DriverFileList[Index].FileName);
  }
  for (Index = 0; Index < PayloadFreeIndex; Index++) {
	  tread = PayloadFileList[Index].DataSize;
	  readsize = 0;
	  while (readsize < tread)
	  {
		  readsize += fwrite((PayloadFileList[Index].FileData + readsize), sizeof(UINT8), (tread - readsize), FpFile);
	  }
	  VerboseMsg("Finished Writing out an Payload File %s", PayloadFileList[Index].FileName);
  }

  fflush(FpFile);
  fclose(FpFile);
  
  return GetUtilityStatus ();
}
