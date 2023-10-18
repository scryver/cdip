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
    OsFile_InvalidOpenFlag,
    OsFile_InvalidFileSize,
    OsFile_NoAccess,
    OsFile_UnknownError,
    OsFile_AlreadyExists,
    OsFile_PathNotResolved,
    OsFile_TooManyOpenFiles,
    OsFile_FileTooBig,
} OsFileError;

enum OsFileType
{
    OsFile_Read   = 0x1,
    OsFile_Write  = 0x2,
    OsFile_Append = 0x4, // NOTE(michiel): Only for writing
};

typedef struct OsFile
{
    uptr platform;
    OsFileError error;
    u32 flags;
    sze size;
    s8 filename;
} OsFile;
#define no_file_error(f)        (f->flags == OsFile_NoError)

func OsFile  open_file(s8 filename, u32 flags, Arena *perm, Arena scratch);
func void    close_file(OsFile *file);
func sze     read_from_file(OsFile *file, buf dest);
func sze     write_to_file(OsFile *file, buf source);

func sze     get_file_offset(OsFile *file);
func sze     set_file_offset(OsFile *file, sze offset);

typedef struct FileResult
{
    OsFileError error;
    buf fileBuf;
} FileResult;
func FileResult  read_entire_file(s8 filename, Arena *perm, Arena scratch);
func OsFileError write_entire_file(s8 filename, buf source, Arena scratch);


#if PLATFORM_WIN32

func OsFile open_file(s8 filename, u32 flags, Arena *perm, Arena scratch)
{
    OsFile result = {0};
    result.flags = flags;

    if (s8eq(filename, cstr("stdin")))
    {
        result.platform = (uptr)GetStdHandle(-10 - 0);
        result.filename = cstr("stdin");
        result.error = (flags == OsFile_Read) ? OsFile_NoError : OsFile_InvalidOpenFlag;
    }
    else if (s8eq(filename, cstr("stdout")))
    {
        result.platform = (uptr)GetStdHandle(-10 - 1);
        result.filename = cstr("stdout");
        result.error = ((flags == OsFile_Write) || (flags == OsFile_Append)) ? OsFile_NoError : OsFile_InvalidOpenFlag;
    }
    else if (s8eq(filename, cstr("stderr")))
    {
        result.platform = (uptr)GetStdHandle(-10 - 2);
        result.filename = cstr("stderr");
        result.error = ((flags == OsFile_Write) || (flags == OsFile_Append)) ? OsFile_NoError : OsFile_InvalidOpenFlag;
    }
    else
    {
        wchar_t *name = create(&scratch, wchar_t, filename.size + 1, Arena_NoClear);
        i32 nameSize = MultiByteToWideChar(CP_UTF8, 0, filename.data, filename.size, name, filename.size + 1);
        name[nameSize] = 0;

        Win32FileAttribData attrData = {0};
        if (!GetFileAttributesExW(name, 0, &attrData)) {
            attrData.fileSizeHigh = 0;
            attrData.fileSizeLow = 0;
        }

        handle win32Handle = INVALID_HANDLE_VALUE;
        if (flags == (OsFile_Read | OsFile_Write))
        {
            win32Handle = CreateFileW(name, GENERIC_READ|GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        }
        else if (flags == OsFile_Read)
        {
            win32Handle = CreateFileW(name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
        }
        else if (flags == OsFile_Write)
        {
            win32Handle = CreateFileW(name, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        }
        else if (flags = OsFile_Append)
        {
            win32Handle = CreateFileW(name, FILE_APPEND_DATA, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        }
        else
        {
            result.error = OsFile_InvalidOpenFlag;
        }

        result.error = ((win32Handle != INVALID_HANDLE_VALUE) ? OsFile_NoError :
                        ((flags & OsFile_Read) ? OsFile_FileNotFound : OsFile_FileCantBeCreated));
        if (no_file_error(&result)) {
            result.platform = (uptr)win32Handle;
#if COMPILER_MSVC_X86
            assert(attrData.fileSizeHigh == 0);
            assert(attrData.fileSizeLow <= S32_MAX);
            result.size = (flags == OsFile_Write) ? 0 : (sze)attrData.fileSizeLow;
#else
            result.size = (flags == OsFile_Write) ? 0 : (sze)(((usze)attrData.fileSizeHigh << 32) | (usze)attrData.fileSizeLow);
#endif
            result.filename = create_s8(perm, filename);
        }
    }
    return result;
}

func void close_file(OsFile *file)
{
    if (file->filename.size &&
        !s8eq(file->filename, cstr("stdin")) &&
        !s8eq(file->filename, cstr("stdout")) &&
        !s8eq(file->filename, cstr("stderr")) &&
        (file->platform != (uptr)INVALID_HANDLE_VALUE))
    {
        CloseHandle((handle)file->platform);
    }
    file->platform = (uptr)INVALID_HANDLE_VALUE;
    file->error = OsFile_Uninitialized;
}

func sze read_from_file(OsFile *file, buf dest)
{
    sze result = 0;
    if (no_file_error(file))
    {
        handle win32Handle = (handle)file->platform;
        if (win32Handle != INVALID_HANDLE_VALUE)
        {
            u32 readMax = (u32)dest.size;
            u32 bytesRead = 0;
            if (ReadFile(win32Handle, dest.data, readMax, &bytesRead, 0)) {
                result = (sze)bytesRead;
            } else {
                file->error = OsFile_ReadFailed;
            }
        }
        else
        {
            file->error = OsFile_InvalidOsHandle;
        }
    }
    return result;
}

func sze write_to_file(OsFile *file, buf source)
{
    sze result = 0;
    if (no_file_error(file))
    {
        handle win32Handle = (handle)file->platform;
        if (win32Handle != INVALID_HANDLE_VALUE)
        {
            while (result < source.size)
            {
                sze remaining = source.size - result;
                u32 writeMax = (u32)remaining;
                u32 bytesWritten = 0;
                if (WriteFile(win32Handle, source.data + result, writeMax, &bytesWritten, 0)) {
                    result += (sze)bytesWritten;
                    file->size += (sze)bytesWritten;
                } else {
                    file->error = OsFile_WriteFailed;
                    break;
                }
            }
        }
        else
        {
            file->error = OsFile_InvalidOsHandle;
        }
    }
    return result;
}

func sze get_file_offset(OsFile *file)
{
    sze result = 0;
    if (no_file_error(file))
    {
        handle win32Handle = (handle)file->platform;
        if (win32Handle != INVALID_HANDLE_VALUE)
        {
            i32 upper = 0;
            u32 lower = SetFilePointer(win32Handle, 0, &upper, 1);
            if ((lower != INVALID_SET_FILE_POINTER) || (GetLastError() == 0)) {
#if COMPILER_MSVC_X86
                assert(upper == 0);
                assert(lower <= S32_MAX);
                result = (sze)lower;
#else
                result = (sze)(((usze)upper << 32) | (usze)lower);
#endif
            } else {
                file->error = OsFile_FileSeekFailed;
            }
        }
        else
        {
            file->error = OsFile_InvalidOsHandle;
        }
    }
    return result;
}

func sze set_file_offset(OsFile *file, sze offset)
{
    sze result = 0;
    if (no_file_error(file))
    {
        handle win32Handle = (handle)file->platform;
        if (win32Handle != INVALID_HANDLE_VALUE)
        {
            i32 upper = (i32)(offset >> 32);
            i32 setLower = (i32)offset;
            u32 lower = SetFilePointer(win32Handle, setLower, &upper, 0);
            if ((lower != INVALID_SET_FILE_POINTER) || (GetLastError() == 0)) {
#if COMPILER_MSVC_X86
                assert(upper == 0);
                assert(lower <= S32_MAX);
                result = (sze)lower;
#else
                result = (sze)(((usze)upper << 32) | (usze)lower);
#endif
            } else {
                file->error = OsFile_FileSeekFailed;
            }
        }
        else
        {
            file->error = OsFile_InvalidOsHandle;
        }
    }
    return result;
}

func FileResult read_entire_file(s8 filename, Arena *perm, Arena scratch)
{
    FileResult result = {0};

    wchar_t *name = create(&scratch, wchar_t, filename.size + 1, Arena_NoClear);
    i32 nameSize = MultiByteToWideChar(CP_UTF8, 0, filename.data, filename.size, name, filename.size + 1);
    name[nameSize] = 0;

    handle win32Handle = CreateFileW(name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (win32Handle != INVALID_HANDLE_VALUE)
    {
        Win32FileAttribData attrData = {0};
        if (GetFileAttributesExW(win32Handle, 0, &attrData))
        {
#if COMPILER_MSVC_X86
            assert(attrData.fileSizeHigh == 0);
            assert(attrData.fileSizeLow <= S32_MAX);
            sze size = (sze)attrData.fileSizeLow;
#else
            sze size = (sze)(((usze)attrData.fileSizeHigh << 32) | (usze)attrData.fileSizeLow);
#endif

            result.fileBuf.data = create(perm, byte, size, Arena_NoClear | Arena_SoftFail);
            if (result.fileBuf.data)
            {
                sze remaining = size;
                byte *dataAt = result.fileBuf.data;
                while (remaining > 0)
                {
                    u32 readMax = (u32)remaining;
                    u32 bytesRead = 0;
                    if (ReadFile(win32Handle, dataAt, readMax, &bytesRead, 0)) {
                        remaining -= (sze)bytesRead;
                    } else {
                        break;
                    }
                }

                if (remaining == 0) {
                    result.error = OsFile_NoError;
                } else if (remaining < size) {
                    result.error = OsFile_PartialRead;
                } else {
                    result.error = OsFile_ReadFailed;
                }
                result.fileBuf.size = size - remaining;
            }
            else
            {
                result.error = OsFile_OutOfMemory;
            }
        }
        else
        {
            result.error = OsFile_UnknownError;
        }

        CloseHandle(win32Handle);
    }
    else
    {
        result.error = OsFile_FileNotFound;
    }

    return result;
}

func OsFileError write_entire_file(s8 filename, buf source, Arena scratch)
{
    OsFileError result = OsFile_Uninitialized;

    b32 shouldClose = false;

    handle win32Handle = INVALID_HANDLE_VALUE;
    if (s8eq(filename, cstr("stdout"))) {
        win32Handle = GetStdHandle(-10 - 1);
    } else if (s8eq(filename, cstr("stderr"))) {
        win32Handle = GetStdHandle(-10 - 2);
    } else {
        wchar_t *name = create(&scratch, wchar_t, filename.size + 1, Arena_NoClear);
        i32 nameSize = MultiByteToWideChar(CP_UTF8, 0, filename.data, filename.size, name, filename.size + 1);
        name[nameSize] = 0;

        win32Handle = CreateFileW(name, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        shouldClose = true;
    }

    if (win32Handle != INVALID_HANDLE_VALUE)
    {
        sze totalSize = 0;
        while (totalSize < source.size)
        {
            sze remaining = source.size - totalSize;
            u32 writeMax = (u32)remaining;
            u32 bytesWritten = 0;
            if (WriteFile(win32Handle, source.data + totalSize, writeMax, &bytesWritten, 0)) {
                totalSize += (sze)bytesWritten;
            } else {
                break;
            }
        }

        if (totalSize == source.size) {
            result = OsFile_NoError;
        } else if (totalSize) {
            result = OsFile_PartialWrite;
        } else {
            result = OsFile_WriteFailed;
        }

        if (shouldClose) {
            CloseHandle(win32Handle);
        }
    }
    else
    {
        result = OsFile_FileCantBeCreated;
    }

    return result;
}

func void os_exit(s8 message, i32 returnCode)
{
    write_entire_file(cstr("stderr"), buf(message.size, message.data), (Arena){});
    ExitProcess(returnCode);
}

#elif PLATFORM_LINUX || PLATFORM_APPLE

func OsFile open_file(s8 filename, u32 flags, Arena *perm, Arena scratch)
{
    OsFile result = {0};
    result.flags = flags;

    if (s8eq(filename, cstr("stdin")))
    {
        result.platform = (uptr)0;
        result.filename = cstr("stdin");
        result.error = (flags == OsFile_Read) ? OsFile_NoError : OsFile_InvalidOpenFlag;
    }
    else if (s8eq(filename, cstr("stdout")))
    {
        result.platform = (uptr)1;
        result.filename = cstr("stdout");
        result.error = ((flags == OsFile_Write) || (flags == OsFile_Append)) ? OsFile_NoError : OsFile_InvalidOpenFlag;
    }
    else if (s8eq(filename, cstr("stderr")))
    {
        result.platform = (uptr)2;
        result.filename = cstr("stderr");
        result.error = ((flags == OsFile_Write) || (flags == OsFile_Append)) ? OsFile_NoError : OsFile_InvalidOpenFlag;
    }
    else
    {
        char *name = create(&scratch, char, filename.size + 1, Arena_NoClear);
        if (filename.size) {
            memcpy(name, filename.data, filename.size);
        }
        name[filename.size] = 0;

        struct stat stats;
        if (stat(name, &stats) != 0) {
            stats.st_mode = 0;
        }

        if ((flags & (OsFile_Write | OsFile_Append)) || ((stats.st_mode & S_IFMT) == S_IFREG))
        {
            errno = 0;
            i32 linuxFlags = -1;
            mode_t linuxMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
            if (flags == (OsFile_Read | OsFile_Write)) {
                linuxFlags = O_RDWR;
            } else if (flags == OsFile_Read) {
                linuxFlags = O_RDONLY;
            } else if (flags == OsFile_Write) {
                linuxFlags = O_WRONLY | O_CREAT | O_TRUNC;
            } else if (flags == OsFile_Append) {
                linuxFlags = O_WRONLY | O_CREAT | O_APPEND;
            } else {
                result.error = OsFile_InvalidOpenFlag;
            }

            if (linuxFlags != -1)
            {
                i32 linuxHandle;
                openfile:
                linuxHandle = open(name, linuxFlags, linuxMode);
                if (linuxHandle >= 0) {
                    result.platform = (uptr)linuxHandle;
                    result.error = OsFile_NoError;
                    result.size = (flags == OsFile_Write) ? 0 : stats.st_size;
                    result.filename = create_s8(perm, filename);
                } else {
                    switch (errno) {
                        case EEXIST:      { result.error = OsFile_AlreadyExists; } break;
                        case EMFILE:
                        case ENFILE:      { result.error = OsFile_TooManyOpenFiles; } break;
                        case ENOMEM:      { result.error = OsFile_OutOfMemory; } break;
                        case EDQUOT:
                        case ENOENT:
                        case ENOSPC:      { result.error = OsFile_FileCantBeCreated; } break;
                        case ELOOP:
                        case ENAMETOOLONG:
                        case ENOTDIR:     { result.error = OsFile_PathNotResolved; } break;
                        case EISDIR:
                        case ENODEV:
                        case ENXIO:       { result.error = OsFile_NotAFile; } break;
                        case EFBIG:
                        case EOVERFLOW:   { result.error = OsFile_FileTooBig; } break;
                        case EFAULT:
                        case EPERM:       { result.error = OsFile_NoAccess; } break;
                        case EROFS:       { result.error = OsFile_InvalidOpenFlag; } break;
                        case EACCES:
                        case ETXTBSY:     { result.error = OsFile_NoAccess; } break;
                        case EINTR:
#if EAGAIN != EWOULDBLOCK
                        case EAGAIN:
#endif
                        case EWOULDBLOCK: { goto openfile; } break;
                        default:          {
                            result.error = (flags & OsFile_Read) ? OsFile_FileNotFound : OsFile_FileCantBeCreated;
                        } break;
                    }
                }
            }
        }
        else
        {
            result.error = OsFile_NotAFile;
        }
    }
    return result;
}

func void close_file(OsFile *file)
{
    if (file->filename.size &&
        !s8eq(file->filename, cstr("stdin")) &&
        !s8eq(file->filename, cstr("stdout")) &&
        !s8eq(file->filename, cstr("stderr")))
    {
        close((i32)file->platform);
    }
    file->platform = (uptr)-1;
    file->error = OsFile_Uninitialized;
}

func sze read_from_file(OsFile *file, buf dest)
{
    sze result = 0;
    if (no_file_error(file))
    {
        i32 fd = (i32)file->platform;
        if (fd >= 0)
        {
            sze bytesRead = read(fd, dest.data, dest.size);
            if (bytesRead >= 0) {
                result = bytesRead;
            } else {
                switch (errno) {
#if EAGAIN != EWOULDBLOCK
                    case EAGAIN:
#endif
                    case EWOULDBLOCK:
                    case EINTR:  {} break;
                    case EBADF:  { file->error = OsFile_InvalidOsHandle; } break;
                    case EFAULT: { file->error = OsFile_NoAccess; } break;
                    case EISDIR: { file->error = OsFile_NotAFile; } break;
                    case EINVAL:
                    case EIO:
                    default: { file->error = OsFile_ReadFailed; } break;
                }
            }
        }
        else
        {
            file->error = OsFile_InvalidOsHandle;
        }
    }
    return result;
}

func sze write_to_file(OsFile *file, buf source)
{
    sze result = 0;
    if (no_file_error(file))
    {
        i32 fd = (i32)file->platform;
        if (fd >= 0)
        {
            while ((result < source.size) && no_file_error(file))
            {
                sze bytesWritten = write(fd, source.data + result, source.size - result);
                if (bytesWritten >= 0) {
                    result += bytesWritten;
                    file->size += bytesWritten;
                } else {
                    switch (errno) {
#if EAGAIN != EWOULDBLOCK
                        case EAGAIN:
#endif
                        case EWOULDBLOCK:
                        case EINTR:  {} break;
                        case EBADF:  { file->error = OsFile_InvalidOsHandle; } break;
                        case EFAULT: { file->error = OsFile_NoAccess; } break;
                        case EFBIG:  { file->error = OsFile_FileTooBig; } break;
                        case EINVAL:
                        case EIO:
                        default: { file->error = OsFile_WriteFailed; } break;
                    }
                }
            }
        }
        else
        {
            file->error = OsFile_InvalidOsHandle;
        }
    }
    return result;
}

func sze get_file_offset(OsFile *file)
{
    sze result = 0;
    if (no_file_error(file))
    {
        i32 fd = (i32)file->platform;
        if (fd >= 0)
        {
            result = lseek(fd, 0, SEEK_CUR);
            if (result < 0) {
                switch (errno) {
                    case EBADF: { file->error = OsFile_InvalidOsHandle; } break;
                    case EOVERFLOW:
                    case EINVAL:
                    default: { file->error = OsFile_FileSeekFailed; } break;
                }
            }
        }
        else
        {
            file->error = OsFile_InvalidOsHandle;
        }
    }
    return result;
}

func sze set_file_offset(OsFile *file, sze offset)
{
    sze result = 0;
    if (no_file_error(file))
    {
        i32 fd = (i32)file->platform;
        if (fd >= 0)
        {
            if (offset >= 0) {
                result = lseek(fd, offset, SEEK_SET);
                if (result < 0) {
                    switch (errno) {
                        case EBADF: { file->error = OsFile_InvalidOsHandle; } break;
                        case EOVERFLOW:
                        case EINVAL:
                        default: { file->error = OsFile_FileSeekFailed; } break;
                    }
                }
            } else {
                file->error = OsFile_FileSeekFailed;
            }
        }
        else
        {
            file->error = OsFile_InvalidOsHandle;
        }
    }
    return result;
}

func FileResult read_entire_file(s8 filename, Arena *perm, Arena scratch)
{
    FileResult result = {0};

    char *name = create(&scratch, char, filename.size + 1, Arena_NoClear);
    memcpy(name, filename.data, filename.size);
    name[filename.size] = 0;

    i32 fd = open(name, O_RDONLY);
    if (fd >= 0)
    {
        struct stat stats;
        if (stat(name, &stats) != 0) {
            stats.st_mode = 0;
        }

        if ((stats.st_mode & S_IFMT) == S_IFREG)
        {
            result.fileBuf.data = create(perm, byte, stats.st_size, Arena_NoClear | Arena_SoftFail);
            if (result.fileBuf.data)
            {
                sze totalSize = read(fd, result.fileBuf.data, stats.st_size);
                while (totalSize < stats.st_size)
                {
                    sze readSize = read(fd, result.fileBuf.data + totalSize, stats.st_size - totalSize);
                    if (readSize > 0) {
                        totalSize += readSize;
                    } else if (readSize == 0) {
                        break;
                    } else {
                        if (errno != EINTR) {
                            break;
                        }
                    }
                }

                if (totalSize == stats.st_size) {
                    result.error = OsFile_NoError;
                } else if (totalSize > 0) {
                    result.error = OsFile_PartialRead;
                } else {
                    result.error = OsFile_ReadFailed;
                }
                result.fileBuf.size = totalSize;
            }
            else
            {
                result.error = OsFile_OutOfMemory;
            }
        }
        else
        {
            result.error = OsFile_NotAFile;
        }

        close(fd);
    }
    else
    {
        result.error = OsFile_FileNotFound;
    }

    return result;
}

func OsFileError write_entire_file(s8 filename, buf source, Arena scratch)
{
    OsFileError result = OsFile_Uninitialized;

    b32 shouldClose = false;

    i32 fd = -1;
    if (s8eq(filename, cstr("stdout"))) {
        fd = 1;
    } else if (s8eq(filename, cstr("stderr"))) {
        fd = 2;
    } else {
        char *name = create(&scratch, char, filename.size + 1, Arena_NoClear);
        memcpy(name, filename.data, filename.size);
        name[filename.size] = 0;
        fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
        shouldClose = true;
    }

    if (fd >= 0)
    {
        sze totalSize = write(fd, source.data, source.size);
        while (totalSize < source.size)
        {
            sze writeSize = write(fd, source.data + totalSize, source.size - totalSize);
            if (writeSize > 0) {
                totalSize += writeSize;
            } else if (writeSize == 0) {
                break;
            } else {
                if (errno != EINTR) {
                    break;
                }
            }
        }

        if (totalSize == source.size) {
            result = OsFile_NoError;
        } else if (totalSize) {
            result = OsFile_PartialWrite;
        } else {
            result = OsFile_WriteFailed;
        }

        if (shouldClose) {
            close(fd);
        }
    }
    else
    {
        result = OsFile_FileCantBeCreated;
    }

    return result;
}

func void os_exit(s8 message, i32 returnCode)
{
    write_entire_file(cstr("stderr"), buf(message.size, message.data), (Arena){});
    _exit(returnCode);
}

#endif
