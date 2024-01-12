/** @file
  STM tool generate STM binary

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  # MU_CHANGE [WHOLE FILE] - Add the GenStm application

**/

#include "GenStm.h"

#pragma pack (push, 1)

typedef struct {
  UINT16  LimitLow;
  UINT16  BaseLow;
  UINT8   BaseMid;
  UINT8   Attribute;
  UINT8   LimitHi;
  UINT8   BaseHi;
} GDT_ENTRY;

#pragma pack (pop)

GDT_ENTRY mGdtEntries[] = {
// LimitLow BaseLow BaseMid Attribute LimitHi BaseHi
  {0,       0,      0,      0,        0,      0,    }, /* 0x0:  reserve */
  {0,       0,      0,      0,        0,      0,    }, /* 0x0:  reserve */
  {0xFFFF,  0,      0,      0x9B,     0xCF,   0,    }, /* 0x10: code 32 seg */
  {0xFFFF,  0,      0,      0x93,     0xCF,   0,    }, /* 0x18: data seg */
  {0,       0,      0,      0,        0,      0,    }, /* 0x20: reserve */
  {104,     0,      0,      0x89,     0x80,   0,    }, /* 0x28: TSS */
  {0,       0,      0,      0,        0,      0,    }, /* 0x30: reserve */
  {0xFFFF,  0,      0,      0x9B,     0xAF,   0,    }, /* 0x38: code 64 seg */
  {0xFFFF,  0,      0,      0x93,     0xCF,   0,    }, /* 0x40: data seg */
};

UINT64  DebugLevel = 0;
BOOLEAN DebugMode = FALSE;
BOOLEAN VerboseMode = FALSE;
BOOLEAN QuietMode = FALSE;

/**

  Displays the standard utility information to SDTOUT.

**/
VOID
Version (
  VOID
  )
{
  fprintf (stdout, "%s Version %d.%d\n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION);
}

/**

  Displays the utility usage syntax to STDOUT.

**/
VOID
Usage (
  VOID
  )
{
  //
  // Summary usage
  //
  fprintf (stdout, "Usage: %s -e [options] <input_file>\n\n", UTILITY_NAME);

  //
  // Copyright declaration
  //
  fprintf (stdout, "Copyright (c) 2015, Intel Corporation. All rights reserved.\n\n");

  //
  // Details Option
  //
  fprintf (stdout, "Options:\n");
  fprintf (stdout, "  -o FileName, --output FileName\n\
            File will be created to store the ouput content.\n");
  fprintf (stdout, "  -v, --verbose\n\
           Turn on verbose output with informational messages.\n");
  fprintf (stdout, "  -q, --quiet\n\
           Disable all messages except key message and fatal error\n");
  fprintf (stdout, "  --debug [0-9]\n\
           Enable debug messages, at input debug level.\n");
  fprintf (stdout, "  --version\n\
           Show program's version number and exit.\n");
  fprintf (stdout, "  -h, --help\n\
           Show this help message and exit.\n");
}

/**

  This function dump raw data.

  @param  Data  raw data
  @param  Size  raw data size

**/
VOID
DumpData (
  IN UINT8  *Data,
  IN UINT32 Size
  )
{
  UINT32 Index;
  for (Index = 0; Index < Size; Index++) {
    printf ("%02x", Data[Index]);
  }
}

/**

  This function dump raw data with colume format.

  @param  Data  raw data
  @param  Size  raw data size

**/
VOID
DumpHex (
  IN UINT8  *Data,
  IN UINT32 Size
  )
{
  UINT32  Index;
  UINT32  Count;
  UINT32  Left;

#define COLUME_SIZE  (16 * 2)

  Count = Size / COLUME_SIZE;
  Left  = Size % COLUME_SIZE;
  for (Index = 0; Index < Count; Index++) {
    printf ("%04x: ", Index * COLUME_SIZE);
    DumpData (Data + Index * COLUME_SIZE, COLUME_SIZE);
    printf ("\n");
  }

  if (Left != 0) {
    printf ("%04x: ", Index * COLUME_SIZE);
    DumpData (Data + Index * COLUME_SIZE, Left);
    printf ("\n");
  }
}

/**

  This function dump STM image.

  @param  StmHeader   STM image header

**/
VOID
DumpStmImage (
  IN STM_HEADER *StmHeader
  )
{
  UINT32  Index;

  printf (
    "*****************************************************************************\n"
    "*         STM Header                                                        *\n"
    "*****************************************************************************\n"
    );

  printf ("STM Header (%08x)\n", (UINT32)(UINTN)StmHeader);
  printf ("Hardware field:\n");
  printf ("  StmHeaderRevision           - %08x\n", StmHeader->HwStmHdr.MsegHeaderRevision);
  printf ("  MonitorFeatures             - %08x\n", StmHeader->HwStmHdr.MonitorFeatures);
  printf ("  GdtrLimit                   - %08x\n", StmHeader->HwStmHdr.GdtrLimit);
  printf ("  GdtrBaseOffset              - %08x\n", StmHeader->HwStmHdr.GdtrBaseOffset);
  printf ("  CsSelector                  - %08x\n", StmHeader->HwStmHdr.CsSelector);
  printf ("  EipOffset                   - %08x\n", StmHeader->HwStmHdr.EipOffset);
  printf ("  EspOffset                   - %08x\n", StmHeader->HwStmHdr.EspOffset);
  printf ("  Cr3Offset                   - %08x\n", StmHeader->HwStmHdr.Cr3Offset);
  printf ("Software field:\n");
  printf ("  StmSpecVerMajor             - %02x\n", StmHeader->SwStmHdr.StmSpecVerMajor);
  printf ("  StmSpecVerMinor             - %02x\n", StmHeader->SwStmHdr.StmSpecVerMinor);
  printf ("  StaticImageSize             - %08x\n", StmHeader->SwStmHdr.StaticImageSize);
  printf ("  PerProcDynamicMemorySize    - %08x\n", StmHeader->SwStmHdr.PerProcDynamicMemorySize);
  printf ("  AdditionalDynamicMemorySize - %08x\n", StmHeader->SwStmHdr.AdditionalDynamicMemorySize);
  printf ("  Intel64ModeSupported        - %08x\n", StmHeader->SwStmHdr.StmFeatures.Intel64ModeSupported);
  printf ("  EptSupported                - %08x\n", StmHeader->SwStmHdr.StmFeatures.EptSupported);
  printf ("  NumberOfRevIDs              - %08x\n", StmHeader->SwStmHdr.NumberOfRevIDs);
  for (Index = 0; Index < StmHeader->SwStmHdr.NumberOfRevIDs; Index++) {
    printf ("  StmSmmRevID(%02d)             - %08x\n", Index, StmHeader->SwStmHdr.StmSmmRevID[Index]);
  }

  printf (
    "*****************************************************************************\n\n"
    );

  return ;
}

/**

  Read input file.

  @param FileName      - The input file name
  @param FileData      - The input file data, the memory is alligned.
  @param FileSize      - The input file size

  @retval STATUS_SUCCESS - The file found and data read
  @retval STATUS_ERROR   - The file data is not read
  @retval STATUS_WARNING - The file is not found

**/
STATUS
ReadInputFile (
  IN CHAR8    *FileName,
  OUT UINT8   **FileData,
  OUT UINT32  *FileSize
  )
{
  FILE                        *FpIn;
  UINT32                      TempResult;

  //
  // Open the Input file
  //
  if ((FpIn = fopen (FileName, "rb")) == NULL) {
    //
    // Return WARNING, let caller make decision
    //
    Error (NULL, 0, 0, "Unable to open file", FileName);
    return STATUS_WARNING;
  }
  //
  // Get the Input file size
  //
  fseek (FpIn, 0, SEEK_END);
  *FileSize = ftell (FpIn);
  //
  // Read the contents of input file to memory buffer
  //
  *FileData = (UINT8 *) malloc (*FileSize);
  fseek (FpIn, 0, SEEK_SET);
  TempResult = fread (*FileData, 1, *FileSize, FpIn);
  if (TempResult != *FileSize) {
    Error (NULL, 0, 0, "Read input file error!", NULL);
    free ((VOID *)*FileData);
    fclose (FpIn);
    return STATUS_ERROR;
  }

  //
  // Close the input file
  //
  fclose (FpIn);

  return STATUS_SUCCESS;
}

/**

  Write output file.

  @param FileName      - The input file name
  @param FileData      - The input file data
  @param FileSize      - The input file size

  @retval STATUS_SUCCESS - Write file data successfully
  @retval STATUS_ERROR   - The file data is not written

**/
STATUS
WriteOutputFile (
  IN CHAR8   *FileName,
  IN UINT8   *FileData,
  IN UINT32  FileSize
  )
{
  FILE                        *FpOut;

  //
  // Open the output file
  //
  if ((FpOut = fopen (FileName, "w+b")) == NULL) {
    Error (NULL, 0, 0, "Unable to open file", FileName);
    return STATUS_ERROR;
  }
  //
  // Write the output file
  //
  if ((fwrite (FileData, 1, FileSize, FpOut)) != FileSize) {
    Error (NULL, 0, 0, "Write output file error!", NULL);
    return STATUS_ERROR;
  }

  //
  // Close the output file
  //
  fclose (FpOut);

  return STATUS_SUCCESS;
}

/**

  Construct STM image.

                        +--------------------+ --
                        | SMM VMCS           |  |
                        +--------------------+  |-> Per-Processor VMCS
                        | SMI VMCS           |  |
                        +--------------------+ --
                        | SMM VMCS           |  |
                        +--------------------+  |-> Per-Processor VMCS
                        | SMI VMCS           |  |
                        +--------------------+ --
                        | Stack              |  |-> Per-Processor Dynamic
                        +--------------------+ --
                        | Stack              |  |-> Per-Processor Dynamic
                  RSP-> +--------------------+ --
                        | Heap               |  |
                        +--------------------+  |-> Additional Dynamic
                        | Page Table (24K)   |  |
                  CR3-> +--------------------+ --
                  RIP-> | STM Code           |  |
                        +--------------------+  |
                        | GDT (4K)           |  |-> Static Image
                  GDT-> +--------------------+  |
                        | STM Header (4K)    |  |
                 MSEG-> +--------------------+ --

  @param InStmImage       STM input PE/COFF image
  @param InStmImageSize   STM input PE/COFF image size
  @param OutStmImage      STM output image (See static image above)
  @param OutStmImageSize  STM output image size

  @retval STATUS_SUCCESS - STM image is constructed correctly
  @retval STATUS_ERROR   - Input file format incorrect

**/
STATUS
ConstructStmImage (
  IN  UINT8   *InStmImage,
  IN  UINT32  InStmImageSize,
  OUT UINT8   **OutStmImage,
  OUT UINT32  *OutStmImageSize
  )
{
  EFI_IMAGE_DOS_HEADER                 *DosHdr;
  UINT32                               PeCoffHeaderOffset;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION  Hdr;
  UINT16                               Magic;
  STM_HEADER                           *StmHeader;

  UINT32                               AddressOfEntryPoint;
  UINT32                               SizeOfImage;
  UINT32                               SizeOfStack;
  UINT32                               SizeOfHeap;
  UINT32                               SizeOfImageRaw;
  EFI_IMAGE_SECTION_HEADER             *SectionHeader;
  UINTN                                Index;

  UINTN                                OutPadSectionSize;
  EFI_COMMON_SECTION_HEADER2           *OutPadSection;
  BOOLEAN                              NeedAddSectionHeader;

  //
  // Step 0, collect info
  //
  DosHdr = (EFI_IMAGE_DOS_HEADER *)InStmImage;
  PeCoffHeaderOffset = 0;
  OutPadSectionSize = 0;
  OutPadSection = NULL;
  NeedAddSectionHeader = FALSE;

  if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    PeCoffHeaderOffset = DosHdr->e_lfanew;
  } else {
    //
    // EDK2 build environment may add section header, so we need skip it
    //
    OutPadSection = (EFI_COMMON_SECTION_HEADER2 *)InStmImage;
    if (OutPadSection->Type != EFI_SECTION_RAW) {
      Error (NULL, 0, 1001, "Input file incorrect", "Input file should be DLL with RAW section header.");
      return STATUS_ERROR;
    }
    NeedAddSectionHeader = TRUE;
    //
    // Skip Section Header
    //
    if ((*(UINT32 *)OutPadSection->Size & 0xFFFFFF) != 0xFFFFFF) {
      OutPadSectionSize = sizeof (EFI_COMMON_SECTION_HEADER);
    } else {
      OutPadSectionSize = sizeof (EFI_COMMON_SECTION_HEADER2);
    }
    InStmImage += OutPadSectionSize;
    InStmImageSize -= OutPadSectionSize;
    //
    // Check again
    //
    DosHdr = (EFI_IMAGE_DOS_HEADER *)InStmImage;
    if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
      PeCoffHeaderOffset = DosHdr->e_lfanew;
    } else {
      Error (NULL, 0, 1001, "Input file incorrect", "Input file should be DLL or DLL with RAW section header.");
      return STATUS_ERROR;
    }
  }
  Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)(UINTN)(InStmImage + PeCoffHeaderOffset);
  Magic = Hdr.Pe32->OptionalHeader.Magic;

  if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    AddressOfEntryPoint = (UINT32)Hdr.Pe32->OptionalHeader.AddressOfEntryPoint;
    SizeOfImage         = (UINT32)Hdr.Pe32->OptionalHeader.SizeOfImage;
    SizeOfHeap          = (UINT32)Hdr.Pe32->OptionalHeader.SizeOfHeapCommit;
    SizeOfStack         = (UINT32)Hdr.Pe32->OptionalHeader.SizeOfStackCommit;
  } else {
    AddressOfEntryPoint = (UINT32)Hdr.Pe32Plus->OptionalHeader.AddressOfEntryPoint;
    SizeOfImage         = (UINT32)Hdr.Pe32Plus->OptionalHeader.SizeOfImage;
    SizeOfHeap          = (UINT32)Hdr.Pe32Plus->OptionalHeader.SizeOfHeapCommit;
    SizeOfStack         = (UINT32)Hdr.Pe32Plus->OptionalHeader.SizeOfStackCommit;
  }
  SizeOfImageRaw = SizeOfImage;
  SizeOfImage = STM_PAGES_TO_SIZE (STM_SIZE_TO_PAGES (SizeOfImage));

  if (NeedAddSectionHeader) {
    //
    // Adjust size when necessary
    //
    if (OutPadSectionSize == sizeof (EFI_COMMON_SECTION_HEADER)) {
      if (SizeOfImageRaw + STM_CODE_OFFSET + sizeof (EFI_COMMON_SECTION_HEADER2) > 0xFFFFFF) {
        //
        // If it exceeds 0xFFFFFF, we need use EFI_COMMON_SECTION_HEADER2
        //
        OutPadSectionSize = sizeof (EFI_COMMON_SECTION_HEADER2);
      }
    }
  }

  *OutStmImageSize = SizeOfImageRaw + STM_CODE_OFFSET;
  if (NeedAddSectionHeader) {
    *OutStmImageSize += OutPadSectionSize;
  }
  *OutStmImage = (UINT8 *) malloc (*OutStmImageSize);
  ZeroMem (*OutStmImage, *OutStmImageSize);

  //
  // Fill Section header
  //
  if (NeedAddSectionHeader) {
    if (OutPadSectionSize != 0) {
      OutPadSection = (EFI_COMMON_SECTION_HEADER2 *)*OutStmImage;
      *OutStmImage += OutPadSectionSize;
      if (OutPadSectionSize == sizeof (EFI_COMMON_SECTION_HEADER)) {
        *(UINT32 *)OutPadSection->Size = *OutStmImageSize;
      } else {
        *(UINT32 *)OutPadSection->Size = 0xFFFFFF;
        OutPadSection->ExtendedSize = *OutStmImageSize;
      }
      OutPadSection->Type = EFI_SECTION_RAW;
    } else {
      OutPadSection = NULL;
    }
  }

  //
  // Step 1: STM header
  //

  //
  // 1.1 hardware field
  //

  StmHeader = (STM_HEADER *)(*OutStmImage);
  StmHeader->HwStmHdr.MsegHeaderRevision = 0;
  if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    StmHeader->HwStmHdr.MonitorFeatures = 0;
    StmHeader->HwStmHdr.CsSelector      = 0x10;
  } else {
    StmHeader->HwStmHdr.MonitorFeatures = STM_FEATURES_IA32E;
    StmHeader->HwStmHdr.CsSelector      = 0x38;
  }
  StmHeader->HwStmHdr.GdtrLimit         = sizeof(mGdtEntries) - 1;
  StmHeader->HwStmHdr.GdtrBaseOffset    = STM_GDT_OFFSET;
  StmHeader->HwStmHdr.EipOffset         = STM_CODE_OFFSET + AddressOfEntryPoint;
  StmHeader->HwStmHdr.Cr3Offset         = STM_CODE_OFFSET + SizeOfImage;
  StmHeader->HwStmHdr.EspOffset         = STM_CODE_OFFSET + SizeOfImage + SizeOfHeap  + STM_PAGES_TO_SIZE (6); // let it point to first stack bottom

  //
  // 1.2 software field
  //
  StmHeader->SwStmHdr.StmSpecVerMajor             = STM_SPEC_VERSION_MAJOR;
  StmHeader->SwStmHdr.StmSpecVerMinor             = STM_SPEC_VERSION_MINOR;
  StmHeader->SwStmHdr.StaticImageSize             = STM_CODE_OFFSET + SizeOfImageRaw;
  StmHeader->SwStmHdr.AdditionalDynamicMemorySize = SizeOfHeap  + STM_PAGES_TO_SIZE (6);
  StmHeader->SwStmHdr.PerProcDynamicMemorySize    = SizeOfStack;
  if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    StmHeader->SwStmHdr.StmFeatures.Intel64ModeSupported = 0;
  } else {
    StmHeader->SwStmHdr.StmFeatures.Intel64ModeSupported = 1;
  }
  StmHeader->SwStmHdr.StmFeatures.EptSupported    = 1;
  StmHeader->SwStmHdr.NumberOfRevIDs              = 1;
  StmHeader->SwStmHdr.StmSmmRevID[0]              = STM_SMM_REV_ID;

  //
  // Step 2: GDT
  //
  CopyMem ((VOID *)((UINTN)StmHeader + StmHeader->HwStmHdr.GdtrBaseOffset), (VOID *)mGdtEntries, sizeof(mGdtEntries));

  //
  // Step 3: code
  //

  //
  // 3.1 PE header
  //
  CopyMem ((VOID *)((UINTN)StmHeader + STM_CODE_OFFSET), InStmImage, Hdr.Pe32->OptionalHeader.SizeOfHeaders);

  //
  // 3.2 section
  //
  SectionHeader = (EFI_IMAGE_SECTION_HEADER *)(UINTN)(InStmImage +
                     PeCoffHeaderOffset +
                     sizeof (UINT32) +
                     sizeof (EFI_IMAGE_FILE_HEADER) +
                     Hdr.Pe32->FileHeader.SizeOfOptionalHeader
                     );
  for (Index = 0; Index < Hdr.Pe32->FileHeader.NumberOfSections; Index++) {
    if (SectionHeader[Index].SizeOfRawData != 0) {
      CopyMem (
        (VOID *)((UINTN)StmHeader + STM_CODE_OFFSET + SectionHeader[Index].VirtualAddress),
        InStmImage + SectionHeader[Index].PointerToRawData,
        SectionHeader[Index].Misc.VirtualSize
        );
    }
  }

  if (NeedAddSectionHeader) {
    if ((OutPadSectionSize != 0) && (OutPadSection != NULL)) {
      *OutStmImage = (UINT8 *)OutPadSection;
    }
  }

  return STATUS_SUCCESS;
}

/**

  Main function.

  @param  argc  Number of command line parameters.
  @param  argv  Array of pointers to parameter strings.

  @retval STATUS_SUCCESS            Utility exits successfully.
  @retval STATUS_ERROR              Some error occurred during execution.

**/
STATUS
main (
  IN INT32  argc,
  IN CHAR8  **argv
  )
{
  STATUS  Status;
  CHAR8   *InFileName;
  CHAR8   *OutFileName;
  UINT8   *InStmImage;
  UINT32  InStmImageSize;
  UINT8   *OutStmImage;
  UINT32  OutStmImageSize;

  SetUtilityName (UTILITY_NAME);

  //
  // Verify the correct number of arguments
  //
  if (argc == 1) {
    Error (NULL, 0, 1001, "Missing options", "No input options specified.");
    Usage();
    return 0;
  }

  if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0)) {
    Usage();
    return 0;
  }

  if ((strcmp(argv[1], "--version") == 0)) {
    Version();
    return 0;
  }

  argc--;
  argv++;
  if (strcmp(argv[0],"-e") == 0) {
    argc--;
    argv++;
  } else {
    //
    // Error command line
    //
    Error (NULL, 0, 1003, "Invalid option value", "the options specified are not recognized.");
    Usage();
    return 1;
  }

  InFileName = NULL;
  OutFileName = NULL;
  while (argc > 0) {
    if ((strcmp(argv[0], "-v") == 0) || (stricmp(argv[0], "--verbose") == 0)) {
      VerboseMode = TRUE;
      argc--;
      argv++;
      continue;
    }

    if (stricmp (argv[0], "--debug") == 0) {
      argc-=2;
      argv++;
      Status = AsciiStringToUint64(argv[0], FALSE, &DebugLevel);
      if (DebugLevel > 9) {
        Error (NULL, 0 ,2000, "Invalid parameter", "Unrecognized argument %s", argv[0]);
        goto ERROR;
      }
      if (DebugLevel>=5 && DebugLevel <=9){
        DebugMode = TRUE;
      } else {
        DebugMode = FALSE;
      }
      argv++;
      continue;
    }

    if ((strcmp(argv[0], "-q") == 0) || (stricmp (argv[0], "--quiet") == 0)) {
      QuietMode = TRUE;
      argc--;
      argv++;
      continue;
    }

    if ((strcmp(argv[0], "-o") == 0) || (stricmp (argv[0], "--output") == 0)) {
      if (argv[1] == NULL || argv[1][0] == '-') {
        Error (NULL, 0, 1003, "Invalid option value", "Output File name is missing for -o option");
        goto ERROR;
      }
      OutFileName = argv[1];
      argc -=2;
      argv +=2;
      continue;
    }

    if (argv[0][0] != '-') {
      InFileName = argv[0];
      argc--;
      argv++;
      continue;
    }

    Error (NULL, 0, 1000, "Unknown option", argv[0]);
    goto ERROR;
  }

  if (InFileName == NULL) {
    Error (NULL, 0, 1001, "Missing options", "No input files specified.");
    goto ERROR;
  }

  if (OutFileName == NULL) {
    Error (NULL, 0, 1001, "Missing options", "No output files specified.");
    goto ERROR;
  }

  //
  // All Parameters has been parsed, now set the message print level
  //
  if (QuietMode) {
    SetPrintLevel(40);
  } else if (VerboseMode) {
    SetPrintLevel(15);
  } else if (DebugMode) {
    SetPrintLevel(DebugLevel);
  }

  if (VerboseMode) {
    VerboseMsg("%s tool start.\n", UTILITY_NAME);
  }
  //
  // Read STM efi image
  //
  Status = ReadInputFile (InFileName, &InStmImage, &InStmImageSize);
  if (Status != STATUS_SUCCESS) {
    goto ERROR;
  }

  Status = ConstructStmImage (InStmImage, InStmImageSize, &OutStmImage, &OutStmImageSize);
  if (Status != STATUS_SUCCESS) {
    goto ERROR;
  }

  //
  // Dump DLME descriptor
  //
  if (DebugMode) {
    DumpStmImage ((STM_HEADER *)OutStmImage);
  }

  //
  // Write STM binary image
  //
  Status = WriteOutputFile (OutFileName, OutStmImage, OutStmImageSize);
  if (Status != STATUS_SUCCESS) {
    goto ERROR;
  }
  if (VerboseMode) {
    VerboseMsg("%s successfully\n", UTILITY_NAME);
  }

  return 0;
ERROR:
  return GetUtilityStatus ();
}

