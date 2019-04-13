/** @file -- UnitTestPersistenceFilesystemLib.c

This is an instance of the Unit Test Persistence Lib that will utilize
the filesystem that a test application is running from to save a serialized
version of the internal test state in case the test needs to quit and restore.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.


Copyright (C) 2016 Microsoft Corporation. All Rights Reserved.

**/
#include <PiDxe.h>
#include <UnitTestTypes.h>
#include <Library/UnitTestPersistenceLib.h>
//#include <Library/UnitTestLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/ShellLib.h>

#include <Protocol/LoadedImage.h>

/**
  TODO: STUFF!!

  @retval     !NULL   A pointer to the EFI_FILE protocol instance for the filesystem.
  @retval     NULL    Filesystem could not be found or an error occurred.

**/
STATIC
EFI_DEVICE_PATH_PROTOCOL*
GetCacheFileDevicePath (
  IN  UNIT_TEST_FRAMEWORK_HANDLE  FrameworkHandle
  )
{
  EFI_STATUS                      Status;
  UNIT_TEST_FRAMEWORK             *Framework = (UNIT_TEST_FRAMEWORK*)FrameworkHandle;
  EFI_LOADED_IMAGE_PROTOCOL       *LoadedImage;
  CHAR16                          *AppPath = NULL, *CacheFilePath = NULL;
  CHAR16                          *FileSuffix = L"_Cache.dat";
  UINTN                           DirectorySlashOffset, CacheFilePathLength;
  EFI_DEVICE_PATH_PROTOCOL        *CacheFileDevicePath = NULL;

  //
  // First, we need to get some information from the loaded image.
  // Namely, where the hell are you?
  //
  Status = gBS->HandleProtocol( gImageHandle,
                                &gEfiLoadedImageProtocolGuid,
                                (VOID**)&LoadedImage );
  if (EFI_ERROR( Status ))
  {
    DEBUG(( DEBUG_WARN, "%a - Failed to locate DevicePath for loaded image. %r\n", __FUNCTION__, Status ));
    return NULL;
  }

  //
  // Now we should have the device path of the root device and a file path for the rest.
  // In order to target the directory for the test application, we must process
  // the file path a little.
  //
  // NOTE: This may not be necessary... Path processing functions exist...
  // PathCleanUpDirectories (FileNameCopy);
  //     if (PathRemoveLastItem (FileNameCopy)) {
  AppPath = ConvertDevicePathToText( LoadedImage->FilePath, TRUE, TRUE );    // NOTE: This must be freed.
  DirectorySlashOffset = StrLen( AppPath );
  // Make sure we didn't get any weird data.
  if (DirectorySlashOffset == 0)
  {
    DEBUG(( DEBUG_ERROR, "%a - Weird 0-length string when processing app path.\n", __FUNCTION__ ));
    goto Exit;
  }
  // Now that we know we have a decent string, let's take a deeper look.
  do
  {
    if (AppPath[DirectorySlashOffset] == L'\\')
    {
      break;
    }
    DirectorySlashOffset--;
  } while (DirectorySlashOffset > 0);

  //
  // After that little maneuver, DirectorySlashOffset should be pointing at the last '\' in AppString.
  // That would be the path to the parent directory that the test app is executing from.
  // Let's check and make sure that's right.
  //
  if (AppPath[DirectorySlashOffset] != L'\\')
  {
    DEBUG(( DEBUG_ERROR, "%a - Could not find a single directory separator in app path.\n", __FUNCTION__ ));
    goto Exit;
  }

  //
  // Now we know some things, we're ready to produce our output string, I think.
  //
  CacheFilePathLength = DirectorySlashOffset + 1;
  CacheFilePathLength += StrLen( Framework->ShortTitle );
  CacheFilePathLength += StrLen( FileSuffix );
  CacheFilePathLength += 1;   // Don't forget the NULL terminator.
  CacheFilePath       = AllocateZeroPool( CacheFilePathLength * sizeof( CHAR16 ) );

  //
  // Let's produce our final path string, shall we?
  //
  StrnCpyS( CacheFilePath, CacheFilePathLength, AppPath, DirectorySlashOffset + 1 ); // Copy the path for the parent directory.
  StrCatS( CacheFilePath, CacheFilePathLength, Framework->ShortTitle );              // Copy the base name for the test cache.
  StrCatS( CacheFilePath, CacheFilePathLength, FileSuffix );                         // Copy the file suffix.

  //
  // Finally, try to create the device path for the thing thing.
  //
  CacheFileDevicePath = FileDevicePath( LoadedImage->DeviceHandle, CacheFilePath );

Exit:
  // Always put away your toys.
  if (AppPath != NULL)
  {
    FreePool( AppPath );
  }
  if (CacheFilePath != NULL)
  {
    FreePool( CacheFilePath);
  }

  return CacheFileDevicePath;
} // GetCacheFileDevicePath()


/**
  Determines whether a persistence cache already exists for
  the given framework.

  @param[in]  FrameworkHandle   A pointer to the framework that is being persisted.

  @retval     TRUE
  @retval     FALSE   Cache doesn't exist or an error occurred.

**/
BOOLEAN
EFIAPI
DoesCacheExist (
  IN  UNIT_TEST_FRAMEWORK_HANDLE  FrameworkHandle
  )
{
  EFI_DEVICE_PATH_PROTOCOL      *FileDevicePath;
  EFI_STATUS                    Status;
  SHELL_FILE_HANDLE             FileHandle;

  // NOTE: This devpath is allocated and must be freed.
  FileDevicePath = GetCacheFileDevicePath( FrameworkHandle );

  // Check to see whether the file exists.
  // If the file can be opened for reading, it exists.
  // Otherwise, probably not.
  Status = ShellOpenFileByDevicePath( &FileDevicePath,
                                      &FileHandle,
                                      EFI_FILE_MODE_READ,
                                      0 );
  if (!EFI_ERROR( Status ))
  {
    ShellCloseFile( &FileHandle );
  }

  if (FileDevicePath != NULL)
  {
    FreePool( FileDevicePath );
  }

  DEBUG(( DEBUG_VERBOSE, "%a - Returning %d\n", __FUNCTION__, !EFI_ERROR( Status ) ));

  return !EFI_ERROR( Status );
} // DoesCacheExist()


/**
  Will save the data associated with an internal Unit Test Framework
  state in a manner that can persist a Unit Test Application quit or
  even a system reboot.

  @param[in]  FrameworkHandle   A pointer to the framework that is being persisted.
  @param[in]  SaveData          A pointer to the buffer containing the serialized
                                framework internal state.

  @retval     EFI_SUCCESS   Data is persisted and the test can be safely quit.
  @retval     Others        Data is not persisted and test cannot be resumed upon exit.

**/
EFI_STATUS
EFIAPI
SaveUnitTestCache (
  IN  UNIT_TEST_FRAMEWORK_HANDLE  FrameworkHandle,
  IN  UNIT_TEST_SAVE_HEADER       *SaveData
  )
{
  EFI_DEVICE_PATH_PROTOCOL      *FileDevicePath;
  EFI_STATUS                    Status;
  SHELL_FILE_HANDLE             FileHandle;
  UINTN                         WriteCount;

  //
  // Check the inputs for sanity.
  if (FrameworkHandle == NULL || SaveData == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Determine the path for the cache file.
  // NOTE: This devpath is allocated and must be freed.
  FileDevicePath = GetCacheFileDevicePath( FrameworkHandle );

  //
  //First lets open the file if it exists so we can delete it...This is the work around for truncation
  //
  Status = ShellOpenFileByDevicePath(&FileDevicePath,
                                     &FileHandle,
                                     (EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE),
                                     0);

  if (!EFI_ERROR(Status))
  {
    //if file handle above was opened it will be closed by the delete.
    Status = ShellDeleteFile(&FileHandle);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a failed to delete file %r\n", __FUNCTION__, Status));
    }
  }

  //
  // Now that we know the path to the file... let's open it for writing.
  //
  Status = ShellOpenFileByDevicePath( &FileDevicePath,
                                      &FileHandle,
                                      (EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE),
                                      0 );
  if (EFI_ERROR( Status ))
  {
    DEBUG(( DEBUG_ERROR, "%a - Opening file for writing failed! %r\n", __FUNCTION__, Status ));
    goto Exit;
  }

  //
  // Write the data to the file.
  //
  WriteCount = SaveData->BlobSize;
  DEBUG(( DEBUG_INFO, "%a - Writing %d bytes to file...\n", __FUNCTION__, WriteCount ));
  Status = ShellWriteFile( FileHandle,
                           &WriteCount,
                           SaveData );

  if (EFI_ERROR( Status ) || WriteCount != SaveData->BlobSize)
  {
    DEBUG(( DEBUG_ERROR, "%a - Writing to file failed! %r\n", __FUNCTION__, Status ));
  }
  else
  {
    DEBUG(( DEBUG_INFO, "%a - SUCCESS!\n", __FUNCTION__ ));
  }

  //
  // No matter what, we should probably close the file.
  //
  ShellCloseFile( &FileHandle );

Exit:
  if (FileDevicePath != NULL)
  {
    FreePool( FileDevicePath );
  }

  return Status;
} // SaveUnitTestCache()


/**
  Will retrieve any cached state associated with the given framework.
  Will allocate a buffer to hold the loaded data.

  @param[in]  FrameworkHandle   A pointer to the framework that is being persisted.
  @param[in]  SaveData          A pointer pointer that will be updated with the address
                                of the loaded data buffer.

  @retval     EFI_SUCCESS       Data has been loaded successfully and SaveData is updated
                                with a pointer to the buffer.
  @retval     Others            An error has occurred and no data has been loaded. SaveData
                                is set to NULL.

**/
EFI_STATUS
EFIAPI
LoadUnitTestCache (
  IN  UNIT_TEST_FRAMEWORK_HANDLE  FrameworkHandle,
  OUT UNIT_TEST_SAVE_HEADER       **SaveData
  )
{
  EFI_STATUS                    Status;
  EFI_DEVICE_PATH_PROTOCOL      *FileDevicePath;
  SHELL_FILE_HANDLE             FileHandle;
  BOOLEAN                       IsFileOpened = FALSE;
  UINT64                        LargeFileSize;
  UINTN                         FileSize;
  UNIT_TEST_SAVE_HEADER         *Buffer = NULL;

  //
  // Check the inputs for sanity.
  if (FrameworkHandle == NULL || SaveData == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Determine the path for the cache file.
  // NOTE: This devpath is allocated and must be freed.
  FileDevicePath = GetCacheFileDevicePath( FrameworkHandle );

  //
  // Now that we know the path to the file... let's open it for writing.
  //
  Status = ShellOpenFileByDevicePath( &FileDevicePath,
                                      &FileHandle,
                                      EFI_FILE_MODE_READ,
                                      0 );
  if (EFI_ERROR( Status ))
  {
    DEBUG(( DEBUG_ERROR, "%a - Opening file for writing failed! %r\n", __FUNCTION__, Status ));
    goto Exit;
  }
  else
  {
    IsFileOpened = TRUE;
  }

  //
  // Now that the file is opened, we need to determine how large a buffer we need.
  Status = ShellGetFileSize( FileHandle, &LargeFileSize );
  if (EFI_ERROR( Status ))
  {
    DEBUG(( DEBUG_ERROR, "%a - Failed to determine file size! %r\n", __FUNCTION__, Status ));
    goto Exit;
  }

  //
  // Now that we know the size, let's allocated a buffer to hold the contents.
  FileSize = (UINTN)LargeFileSize;    // You know what... if it's too large, this lib don't care.
  Buffer = AllocatePool( FileSize );
  if (Buffer == NULL)
  {
    DEBUG(( DEBUG_ERROR, "%a - Failed to allocate a pool to hold the file contents! %r\n", __FUNCTION__, Status ));
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  //
  // Finally, let's read the bloody data.
  Status = ShellReadFile( FileHandle, &FileSize, Buffer );
  if (EFI_ERROR( Status ))
  {
    DEBUG(( DEBUG_ERROR, "%a - Failed to read the file contents! %r\n", __FUNCTION__, Status ));
  }

Exit:
  //
  // Always put away your toys.
  if (FileDevicePath != NULL)
  {
    FreePool( FileDevicePath );
  }
  if (IsFileOpened)
  {
    ShellCloseFile( &FileHandle );
  }

  //
  // If we're returning an error, make sure
  // the state is sane.
  if (EFI_ERROR( Status ) && Buffer != NULL)
  {
    FreePool( Buffer );
    Buffer = NULL;
  }

  *SaveData = Buffer;
  return Status;
} // LoadUnitTestCache()
