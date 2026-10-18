// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// Unit tests for runtime/StringRuntime.cpp (OpenSpec change string-runtime,
// task 1.3); FileRuntimeTests.cpp adds those of runtime/FileRuntime.cpp.
// Built as inoxrt-tests next to the compiler and run by scripts/run-tests.sh
// and scripts/run-tests.ps1 on Linux and Windows.

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

extern "C" {
void* __inox_str_empty() noexcept;
void __inox_str_retain(void* value) noexcept;
void __inox_str_release(void* value) noexcept;
bool __inox_str_equal(const void* left, const void* right) noexcept;
std::int32_t __inox_str_compare(const void* left, const void* right) noexcept;
void* __inox_str_getln(int (*readByte)(), std::int32_t* status) noexcept;
}

namespace {

// The block layout the compiler relies on: count at 0, length at 8, bytes at 16.
struct Block {
    std::int64_t count;
    std::int64_t length;
    char bytes[1];
};

int failures = 0;

void check(bool condition, const char* what)
{
    if (!condition) {
        std::printf("[FAIL] inoxrt %s\n", what);
        ++failures;
    }
}

// __inox_str_getln reads through a plain function pointer, as the program
// passes its own getchar; the tests feed it from this buffer.
std::string input;
std::size_t position = 0;

int readFromInput()
{
    if (position >= input.size()) {
        return EOF;
    }
    return static_cast<unsigned char>(input[position++]);
}

void* readLine(const std::string& text, std::int32_t& status)
{
    input = text;
    position = 0;
    status = -1;
    return __inox_str_getln(readFromInput, &status);
}

std::string contents(void* value)
{
    const auto* block = static_cast<const Block*>(value);
    return std::string(block->bytes, static_cast<std::size_t>(block->length));
}

void testEmpty()
{
    void* empty = __inox_str_empty();
    const auto* block = static_cast<const Block*>(empty);
    check(block->count == -1, "empty string is immortal");
    check(block->length == 0 && block->bytes[0] == '\0', "empty string has no bytes and a NUL");
    __inox_str_retain(empty);
    __inox_str_release(empty);
    __inox_str_release(empty);
    check(block->count == -1, "retain and release leave an immortal block alone");
    __inox_str_retain(nullptr);
    __inox_str_release(nullptr);
}

void testGetLn()
{
    std::int32_t status = 0;
    void* line = readLine("abc\nnext", status);
    check(status == 0 && contents(line) == "abc", "GetLn reads up to the newline");
    check(static_cast<Block*>(line)->count == 1, "GetLn returns an owned (+1) block");
    check(static_cast<Block*>(line)->bytes[3] == '\0', "GetLn result is NUL terminated");
    check(position == 4, "GetLn consumes the newline and nothing after it");

    void* again = readLine("abc\r\n", status);
    check(status == 0 && contents(again) == "abc", "GetLn drops CR before the newline");
    check(__inox_str_equal(line, again), "equal bytes in different blocks compare equal");
    __inox_str_retain(again);
    check(static_cast<Block*>(again)->count == 2, "retain increments the count");
    __inox_str_release(again);
    __inox_str_release(again);
    __inox_str_release(line);

    void* last = readLine("no newline", status);
    check(status == 0 && contents(last) == "no newline", "GetLn reads a last line without newline");
    __inox_str_release(last);

    check(readLine("", status) == __inox_str_empty() && status == 0, "GetLn at end of input is \"\"");
    check(readLine("\n", status) == __inox_str_empty() && status == 0, "GetLn of an empty line is \"\"");

    const std::string longLine(1000, 'x');
    void* big = readLine(longLine + "\n", status);
    check(status == 0 && contents(big) == longLine, "GetLn grows its buffer for long lines");
    __inox_str_release(big);
}

void testUtf8()
{
    struct Case {
        const char* text;
        bool valid;
        const char* what;
    };
    const Case cases[] = {
        {"a\xC3\xA7\xC3\xA3o", true, "two-byte sequences"},
        {"\xE2\x82\xAC", true, "three-byte sequence (euro sign)"},
        {"\xF0\x9F\x98\x80", true, "four-byte sequence"},
        {"\xF4\x8F\xBF\xBF", true, "U+10FFFF"},
        {"\xC0\x80", false, "overlong two-byte form"},
        {"\xE0\x80\xAF", false, "overlong three-byte form"},
        {"\xED\xA0\x80", false, "UTF-16 surrogate"},
        {"\xF4\x90\x80\x80", false, "above U+10FFFF"},
        {"\xF5\x80\x80\x80", false, "invalid lead byte"},
        {"\x80", false, "lone continuation byte"},
        {"\xE2\x82", false, "truncated sequence"},
        {"\xC3\x28", false, "bad continuation byte"},
    };
    for (const Case& item : cases) {
        std::int32_t status = 0;
        void* line = readLine(std::string(item.text) + "\n", status);
        const std::string what = std::string("UTF-8 ") + (item.valid ? "accepts " : "rejects ") + item.what;
        if (item.valid) {
            check(status == 0 && contents(line) == item.text, what.c_str());
        } else {
            check(status == 1 && line == __inox_str_empty(), what.c_str());
        }
        __inox_str_release(line);
    }
}

void testEqual()
{
    std::int32_t status = 0;
    void* a = readLine("ab\n", status);
    void* b = readLine("abc\n", status);
    void* c = readLine("aB\n", status);
    check(!__inox_str_equal(a, b), "different lengths compare different");
    check(!__inox_str_equal(a, c), "comparison is byte by byte (case matters)");
    check(__inox_str_equal(a, a), "a block equals itself");
    check(!__inox_str_equal(a, __inox_str_empty()), "a non-empty string differs from \"\"");
    __inox_str_release(a);
    __inox_str_release(b);
    __inox_str_release(c);
}

void testCompare()
{
    struct Case {
        const char* left;
        const char* right;
        std::int32_t order;
        const char* what;
    };
    const Case cases[] = {
        {"abc", "abc", 0, "equal strings"},
        {"abc", "abd", -1, "first different byte decides"},
        {"abd", "abc", 1, "first different byte decides (reversed)"},
        {"ab", "abc", -1, "a prefix comes first"},
        {"abc", "ab", 1, "a prefix comes first (reversed)"},
        {"Z", "a", -1, "upper case before lower case (code points)"},
        {"z", "\xC3\xA1", -1, "bytes compare unsigned: z before a-acute"},
        {"\xC3\xA1", "\xC3\xA9", -1, "a-acute before e-acute"},
    };
    std::int32_t status = 0;
    for (const Case& item : cases) {
        void* left = readLine(std::string(item.left) + "\n", status);
        void* right = readLine(std::string(item.right) + "\n", status);
        const std::string what = std::string("compare: ") + item.what;
        check(__inox_str_compare(left, right) == item.order, what.c_str());
        __inox_str_release(left);
        __inox_str_release(right);
    }
    void* word = readLine("a\n", status);
    check(__inox_str_compare(__inox_str_empty(), word) == -1, "compare: \"\" comes first");
    check(__inox_str_compare(word, word) == 0, "compare: a block equals itself");
    check(__inox_str_compare(__inox_str_empty(), __inox_str_empty()) == 0, "compare: \"\" equals \"\"");
    __inox_str_release(word);
}

} // namespace

int runFileRuntimeTests();
int runOwnRuntimeTests();
int runStringFunctionsTests();

int main()
{
    testEmpty();
    testGetLn();
    testUtf8();
    testEqual();
    testCompare();
    if (failures == 0) {
        std::printf("[PASS] inoxrt string runtime unit tests\n");
    }
    const int fileFailures = runFileRuntimeTests();
    const int ownFailures = runOwnRuntimeTests();
    const int functionFailures = runStringFunctionsTests();
    return failures == 0 && fileFailures == 0 && ownFailures == 0 && functionFailures == 0 ? 0 : 1;
}
