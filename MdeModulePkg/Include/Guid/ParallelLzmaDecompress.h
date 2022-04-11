/** @file
  Parallel Lzma Custom decompress algorithm Guid definition.

  This marker GUID is used for custom sections that are compressed with LZMA,
  but are intended to be unpacked in parallel with other execution and so should
  not be handled by the main LZMA GUIDed section extractor.

**/

#ifndef __LZMA_PARALLEL_DECOMPRESS_GUID_H__
#define __LZMA_PARALLEL_DECOMPRESS_GUID_H__

///
/// The Global ID used to identify a section of an FFS file of type
/// EFI_SECTION_GUID_DEFINED, whose contents have been compressed using LZMA.
///
#define PARALLEL_LZMA_CUSTOM_DECOMPRESS_GUID  \
  { 0xbd9921ea, 0xed91, 0x404a, { 0x8b, 0x2f, 0xb4, 0xd7, 0x24, 0x74, 0x7c, 0x8c } }

extern GUID  gParallelLzmaCustomDecompressGuid;

typedef struct {
  VOID     *SourceBuffer;
  VOID     *DecompressedBuffer;
  UINTN    DecompressedSize;
} PARALLEL_DECOMPRESSED_BUFFER;

#endif
