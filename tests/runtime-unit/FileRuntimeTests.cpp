// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// Unit tests for runtime/FileRuntime.cpp (OpenSpec change file-io), run with
// the String runtime tests by inoxrt-tests. The Inox exception a failure
// raises is checked by the runtime tests (tests/runtime/files-*.inox); here a
// failure only has to throw.

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

extern "C" {
void __inox_str_release(void* value) noexcept;
void* __inox_str_getln(int (*readByte)(), std::int32_t* status) noexcept;
void* __inox_file_open(const void* path, std::int32_t mode);
void __inox_file_retain(void* file) noexcept;
void __inox_file_release(void* file) noexcept;
void __inox_file_close(void* file);
void* __inox_file_readln(void* file);
bool __inox_file_eof(void* file);
void __inox_file_write_str(void* file, const void* text);
void __inox_file_write_i64(void* file, std::int64_t number);
void __inox_file_write_f64(void* file, double number);
void __inox_file_write_bool(void* file, bool flag);
void __inox_file_write_newline(void* file);
bool __inox_file_exists(const void* path) noexcept;
void __inox_file_delete(const void* path);
void __inox_file_rename(const void* from, const void* to);
}

namespace {

struct Block {
    std::int64_t count;
    std::int64_t length;
    char bytes[1];
};

int failures = 0;

void check(bool condition, const char* what)
{
    if (!condition) {
        std::printf("[FAIL] inoxrt file %s\n", what);
        ++failures;
    }
}

// A String block owned by the test (immortal, so the runtime never frees it).
struct Text {
    explicit Text(const std::string& value) : storage(sizeof(std::int64_t) * 2 + value.size() + 1, '\0')
    {
        auto* block = reinterpret_cast<Block*>(storage.data());
        block->count = -1;
        block->length = static_cast<std::int64_t>(value.size());
        std::memcpy(block->bytes, value.data(), value.size());
    }
    const void* get() const { return storage.data(); }
    std::string storage;
};

std::string contents(void* value)
{
    const auto* block = static_cast<const Block*>(value);
    return std::string(block->bytes, static_cast<std::size_t>(block->length));
}

template <typename Action>
bool throws(Action action)
{
    try {
        action();
    } catch (...) {
        return true;
    }
    return false;
}

const std::string kPath = "inoxrt-file-test.txt";
const std::string kRenamed = "inoxrt-file-test-renamed.txt";

void cleanUp()
{
    std::remove(kPath.c_str());
    std::remove(kRenamed.c_str());
}

void testRoundTrip()
{
    const Text path(kPath);
    void* file = __inox_file_open(path.get(), 1);
    __inox_file_write_str(file, Text("a\xC3\xA7\xC3\xA3o ").get());
    __inox_file_write_i64(file, -42);
    __inox_file_write_newline(file);
    __inox_file_write_bool(file, true);
    __inox_file_write_str(file, Text(" ").get());
    __inox_file_write_f64(file, 1.5);
    __inox_file_write_newline(file);
    __inox_file_close(file);
    __inox_file_close(file);
    check(throws([&] { __inox_file_write_newline(file); }), "writing a closed file throws");
    __inox_file_release(file);

    check(__inox_file_exists(path.get()), "Exists after Create");
    void* reader = __inox_file_open(path.get(), 0);
    check(!__inox_file_eof(reader), "Eof is false before the first line");
    void* first = __inox_file_readln(reader);
    check(contents(first) == "a\xC3\xA7\xC3\xA3o -42", "ReadLn returns the first line");
    void* second = __inox_file_readln(reader);
    check(contents(second) == "true 1.500000", "Bool and Float are written like PutLn");
    check(__inox_file_eof(reader), "Eof after the last line");
    void* past = __inox_file_readln(reader);
    check(contents(past).empty(), "ReadLn past the end returns \"\"");
    check(throws([&] { __inox_file_write_newline(reader); }), "writing a file opened for reading throws");
    __inox_str_release(first);
    __inox_str_release(second);
    __inox_str_release(past);
    __inox_file_retain(reader);
    __inox_file_release(reader);
    __inox_file_release(reader);
}

void testAppendAndCrLf()
{
    const Text path(kPath);
    void* file = __inox_file_open(path.get(), 1);
    __inox_file_write_str(file, Text("one\r\ntwo").get());
    __inox_file_release(file);
    void* appender = __inox_file_open(path.get(), 2);
    __inox_file_write_str(appender, Text("\nthree").get());
    __inox_file_release(appender);
    void* reader = __inox_file_open(path.get(), 0);
    void* one = __inox_file_readln(reader);
    void* two = __inox_file_readln(reader);
    void* three = __inox_file_readln(reader);
    check(contents(one) == "one", "ReadLn drops CR LF");
    check(contents(two) == "two", "OpenAppend keeps the old content");
    check(contents(three) == "three", "a last line without newline is read");
    check(__inox_file_eof(reader), "Eof after a last line without newline");
    __inox_str_release(one);
    __inox_str_release(two);
    __inox_str_release(three);
    __inox_file_release(reader);
}

void testErrors()
{
    const Text missing("inoxrt-no-such-file.txt");
    check(throws([&] { __inox_file_open(missing.get(), 0); }), "opening a missing file throws");
    check(!__inox_file_exists(missing.get()), "Exists of a missing file is false");
    check(throws([&] { __inox_file_delete(missing.get()); }), "deleting a missing file throws");
    check(throws([&] { __inox_file_open(Text(std::string("a\0b", 3)).get(), 1); }), "a path with NUL throws");

    const Text path(kPath);
    void* file = __inox_file_open(path.get(), 1);
    __inox_file_write_str(file, Text("ok\n\xC3\x28\n").get());
    __inox_file_release(file);
    void* reader = __inox_file_open(path.get(), 0);
    void* ok = __inox_file_readln(reader);
    check(contents(ok) == "ok", "a valid line before an invalid one is read");
    check(throws([&] { __inox_file_readln(reader); }), "invalid UTF-8 throws");
    __inox_str_release(ok);
    __inox_file_release(reader);
}

void testRenameAndDelete()
{
    const Text path(kPath);
    const Text renamed(kRenamed);
    __inox_file_release(__inox_file_open(path.get(), 1));
    __inox_file_rename(path.get(), renamed.get());
    check(!__inox_file_exists(path.get()) && __inox_file_exists(renamed.get()), "RenameFile moves the file");
    __inox_file_release(__inox_file_open(path.get(), 1));
    check(throws([&] { __inox_file_rename(path.get(), renamed.get()); }), "RenameFile onto an existing file throws");
    __inox_file_delete(renamed.get());
    __inox_file_delete(path.get());
    check(!__inox_file_exists(renamed.get()) && !__inox_file_exists(path.get()), "DeleteFile removes the files");
    check(!__inox_file_exists(Text(".").get()), "Exists is false for a directory");
}

} // namespace

int runFileRuntimeTests()
{
    cleanUp();
    testRoundTrip();
    testAppendAndCrLf();
    testErrors();
    testRenameAndDelete();
    cleanUp();
    if (failures == 0) {
        std::printf("[PASS] inoxrt file runtime unit tests\n");
    }
    return failures;
}
