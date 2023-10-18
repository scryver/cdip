typedef enum OsFileError
{
    OsFile_Uninitialized,
    OsFile_NoError,
    OsFile_OutOfMemory,
    OsFile_NotAFile,
    OsFile_FileNotFound,
    OsFile_FileCantBeCreated,
    OsFile_FileCantBeSynced,
    OsFile_FileSeekFailed,
    OsFile_PartialRead,
    OsFile_PartialWrite,
    OsFile_ReadFailed,
    OsFile_WriteFailed,
    OsFile_InvalidOsHandle,
    OsFile_InvalidFileCursor,
    OsFile_InvalidOpenFlag,
    OsFile_InvalidFileSize,
} OsFileError;

typedef struct OsFile
{
    uptr platform;
    OsFileError error;
    u32 flags;
    u64 size;
    s8 filename;
} OsFile;

func OsFile *open_file(s8 filename, u32 flags, Arena *perm, Arena scratch);
func void    close_file(OsFile *file);
func sze     read_from_file(OsFile *file, buf dest);
func sze     write_to_file(OsFile *file, buf source);

func u64     get_file_offset(OsFile *file);
func u64     set_file_offset(OsFile *file, u64 offset);

typedef struct FileResult
{
    OsFileError error;
    buf fileBuf;
} FileResult;
func FileResult  read_entire_file(s8 filename, Arena *perm, Arena scratch);
func OsFileError write_entire_file(s8 filename, buf source, Arena scratch);


#if PLATFORM_WIN32

#elif PLATFORM_LINUX || PLATFORM_APPLE

#endif
