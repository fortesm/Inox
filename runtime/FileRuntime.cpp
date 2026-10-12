// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// File runtime for Inox (OpenSpec change file-io; modules Std.Files and
// Std.IOUtils).
//
// A File value is a pointer to a handle owned by this library:
//     { int64 count, FILE* stream, int32 mode }
// The language forbids copying a File, so the count only follows the moves
// the compiler makes (a call result held for one statement, then stored in
// its variable); the same retain/release code paths as String apply, and the
// file is closed when the last reference goes away: at every exit of the
// routine that holds it, also through an exception.
//
// Every FILE* is opened, used and closed here: on Windows the program and
// this library may use different C runtimes, so a FILE* never crosses that
// boundary. Files are opened in binary mode: lines are written with "\n" on
// every system; reading accepts "\n" and "\r\n".
//
// Failures raise the Inox exceptions of CANON-15 (FileNotFound,
// PermissionDenied, AlreadyExists, DiskFull, otherwise IOError), mapped from
// errno, which the C runtime sets on Linux and on Windows.

#include "StringBlock.h"

#include "../src/compiler/exceptions/ExceptionTypes.h"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>

#if defined(_WIN32)
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <wchar.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

extern "C" [[noreturn]] void __inox_raise(std::uint64_t typeId);

namespace {

using inox::runtime::InoxString;

enum class FileMode : std::int32_t {
    Read = 0,
    Create = 1,
    Append = 2,
};

struct InoxFile final {
    std::int64_t count;
    std::FILE* stream;  // null after Close
    FileMode mode;
};

[[noreturn]] void raiseInox(std::string_view exceptionName)
{
    const auto* info = inox::compiler::exceptions::findExceptionType(exceptionName);
    __inox_raise(info != nullptr ? info->typeId : 0x105 /* IOError */);
}

[[noreturn]] void raiseForErrno(int error)
{
    switch (error) {
    case ENOENT:
    case ENOTDIR:
        raiseInox("FileNotFound");
    case EACCES:
    case EPERM:
#if defined(EROFS)
    case EROFS:
#endif
        raiseInox("PermissionDenied");
    case EEXIST:
        raiseInox("AlreadyExists");
    case ENOSPC:
#if defined(EDQUOT)
    case EDQUOT:
#endif
        raiseInox("DiskFull");
    default:
        raiseInox("IOError");
    }
}

// CANON-19 fault kind 12. Raised from the library, which cannot reach the
// program's fault function; the message and exit status are the same.
[[noreturn]] void outOfMemory()
{
    std::fflush(nullptr);
    std::fputs("Inox runtime error: out of memory\n", stderr);
    std::_Exit(70);
}

const InoxString* asString(const void* value)
{
    return static_cast<const InoxString*>(value);
}

// A path is a UTF-8 String without NUL bytes.
bool isValidPath(const InoxString* path)
{
    return std::memchr(path->bytes, '\0', static_cast<std::size_t>(path->length)) == nullptr;
}

#if defined(_WIN32)
// UTF-8 to UTF-16 for the wide Windows file functions; empty on failure.
std::wstring widen(const InoxString* text)
{
    if (text->length == 0) {
        return std::wstring();
    }
    const int length = static_cast<int>(text->length);
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text->bytes, length, nullptr, 0);
    if (size <= 0) {
        return std::wstring();
    }
    std::wstring wide(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text->bytes, length, wide.data(), size);
    return wide;
}
#endif

std::FILE* openStream(const InoxString* path, FileMode mode)
{
#if defined(_WIN32)
    const wchar_t* wideMode = mode == FileMode::Read ? L"rb" : mode == FileMode::Create ? L"wb" : L"ab";
    const std::wstring widePath = widen(path);
    if (widePath.empty()) {
        errno = ENOENT;
        return nullptr;
    }
    return _wfopen(widePath.c_str(), wideMode);
#else
    const char* narrowMode = mode == FileMode::Read ? "rb" : mode == FileMode::Create ? "wb" : "ab";
    return std::fopen(path->bytes, narrowMode);
#endif
}

bool isRegularFile(const InoxString* path)
{
    if (!isValidPath(path)) {
        return false;
    }
#if defined(_WIN32)
    const std::wstring widePath = widen(path);
    struct _stat64 info;
    return !widePath.empty() && _wstat64(widePath.c_str(), &info) == 0 && (info.st_mode & _S_IFREG) != 0;
#else
    struct stat info;
    return ::stat(path->bytes, &info) == 0 && S_ISREG(info.st_mode);
#endif
}

bool pathExists(const InoxString* path)
{
#if defined(_WIN32)
    const std::wstring widePath = widen(path);
    struct _stat64 info;
    return !widePath.empty() && _wstat64(widePath.c_str(), &info) == 0;
#else
    struct stat info;
    return ::stat(path->bytes, &info) == 0;
#endif
}

InoxFile* asFile(void* value)
{
    return static_cast<InoxFile*>(value);
}

// The stream of an open file, or IOError (closed, or opened for the other
// direction).
std::FILE* readable(void* value)
{
    InoxFile* file = asFile(value);
    if (file == nullptr || file->stream == nullptr || file->mode != FileMode::Read) {
        raiseInox("IOError");
    }
    return file->stream;
}

std::FILE* writable(void* value)
{
    InoxFile* file = asFile(value);
    if (file == nullptr || file->stream == nullptr || file->mode == FileMode::Read) {
        raiseInox("IOError");
    }
    return file->stream;
}

void writeBytes(void* value, const char* bytes, std::size_t length)
{
    std::FILE* stream = writable(value);
    if (length != 0 && std::fwrite(bytes, 1, length, stream) != length) {
        raiseForErrno(errno);
    }
}

int readFromStream(void* context)
{
    const int ch = std::getc(static_cast<std::FILE*>(context));
    return ch == EOF ? -1 : ch;
}

} // namespace

extern "C" {

// Std.IOUtils OpenRead (mode 0), Create (1, new or truncated) and OpenAppend
// (2). Returns an owned (+1) handle.
void* __inox_file_open(const void* path, std::int32_t mode)
{
    const InoxString* text = asString(path);
    if (!isValidPath(text)) {
        raiseInox("IOError");
    }
    const auto fileMode = static_cast<FileMode>(mode);
    errno = 0;
    std::FILE* stream = openStream(text, fileMode);
    if (stream == nullptr) {
        raiseForErrno(errno);
    }
    auto* file = static_cast<InoxFile*>(std::malloc(sizeof(InoxFile)));
    if (file == nullptr) {
        std::fclose(stream);
        outOfMemory();
    }
    file->count = 1;
    file->stream = stream;
    file->mode = fileMode;
    return file;
}

void __inox_file_retain(void* value) noexcept
{
    if (InoxFile* file = asFile(value)) {
        ++file->count;
    }
}

// The last reference closes the file (errors are not reported here: this
// runs on routine exit and during unwinding; Close reports them).
void __inox_file_release(void* value) noexcept
{
    InoxFile* file = asFile(value);
    if (file == nullptr || --file->count != 0) {
        return;
    }
    if (file->stream != nullptr) {
        std::fclose(file->stream);
    }
    std::free(file);
}

// Std.IOUtils Close: flushes and closes; closing twice does nothing.
void __inox_file_close(void* value)
{
    InoxFile* file = asFile(value);
    if (file == nullptr || file->stream == nullptr) {
        return;
    }
    std::FILE* stream = file->stream;
    file->stream = nullptr;
    errno = 0;
    const int flushed = std::fflush(stream);
    const int flushError = errno;
    const int closed = std::fclose(stream);
    if (flushed != 0) {
        raiseForErrno(flushError);
    }
    if (closed != 0) {
        raiseForErrno(errno);
    }
}

// Std.IOUtils ReadLn(F): the next line without its line end, "" at the end of
// the file. Invalid UTF-8 or a read error raises IOError.
void* __inox_file_readln(void* value)
{
    std::FILE* stream = readable(value);
    inox::runtime::LineStatus status = inox::runtime::LineStatus::Ok;
    InoxString* line = inox::runtime::readLine(readFromStream, stream, status);
    if (status == inox::runtime::LineStatus::OutOfMemory) {
        outOfMemory();
    }
    if (status == inox::runtime::LineStatus::InvalidUtf8 || std::ferror(stream) != 0) {
        raiseInox("IOError");
    }
    return line;
}

// Std.IOUtils Eof(F): true when no byte is left to read.
bool __inox_file_eof(void* value)
{
    std::FILE* stream = readable(value);
    const int ch = std::getc(stream);
    if (ch == EOF) {
        if (std::ferror(stream) != 0) {
            raiseInox("IOError");
        }
        return true;
    }
    std::ungetc(ch, stream);
    return false;
}

// Std.IOUtils Write/WriteLn, one call per argument; the text matches PutLn.
void __inox_file_write_str(void* value, const void* text)
{
    const InoxString* string = asString(text);
    writeBytes(value, string->bytes, static_cast<std::size_t>(string->length));
}

void __inox_file_write_i64(void* value, std::int64_t number)
{
    char text[32];
    const int length = std::snprintf(text, sizeof text, "%lld", static_cast<long long>(number));
    writeBytes(value, text, static_cast<std::size_t>(length));
}

void __inox_file_write_f64(void* value, double number)
{
    char text[512];
    const int length = std::snprintf(text, sizeof text, "%f", number);
    writeBytes(value, text, static_cast<std::size_t>(length));
}

void __inox_file_write_bool(void* value, bool flag)
{
    if (flag) {
        writeBytes(value, "true", 4);
    } else {
        writeBytes(value, "false", 5);
    }
}

void __inox_file_write_newline(void* value)
{
    writeBytes(value, "\n", 1);
}

// Std.IOUtils Exists(Path): a regular file exists at Path. Never raises.
bool __inox_file_exists(const void* path) noexcept
{
    return isRegularFile(asString(path));
}

// Std.IOUtils DeleteFile(Path): deletes a file (never a directory).
void __inox_file_delete(const void* path)
{
    const InoxString* text = asString(path);
    if (!isValidPath(text)) {
        raiseInox("IOError");
    }
    errno = 0;
#if defined(_WIN32)
    const std::wstring widePath = widen(text);
    const int result = widePath.empty() ? (errno = ENOENT, -1) : _wremove(widePath.c_str());
#else
    const int result = ::unlink(text->bytes);
#endif
    if (result != 0) {
        raiseForErrno(errno);
    }
}

// Std.IOUtils RenameFile(From, To): AlreadyExists when To exists, on every
// system (POSIX rename would replace it; Windows would fail).
void __inox_file_rename(const void* from, const void* to)
{
    const InoxString* source = asString(from);
    const InoxString* target = asString(to);
    if (!isValidPath(source) || !isValidPath(target)) {
        raiseInox("IOError");
    }
    if (pathExists(target)) {
        raiseInox("AlreadyExists");
    }
    errno = 0;
#if defined(_WIN32)
    const std::wstring wideSource = widen(source);
    const std::wstring wideTarget = widen(target);
    const int result = wideSource.empty() || wideTarget.empty()
                           ? (errno = ENOENT, -1)
                           : _wrename(wideSource.c_str(), wideTarget.c_str());
#else
    const int result = std::rename(source->bytes, target->bytes);
#endif
    if (result != 0) {
        raiseForErrno(errno);
    }
}

} // extern "C"
