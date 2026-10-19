// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// Unit tests for runtime/StringFunctions.cpp (OpenSpec change std-strings,
// task 1.3): concatenation, in-place append, positions and the Unicode
// functions. Every String made here is released, so a leak shows under the
// sanitizer build.

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

extern "C" {
void __inox_str_retain(void* value) noexcept;
void __inox_str_release(void* value) noexcept;
void* __inox_str_concat(std::int64_t count, void* const* parts, std::int32_t* status) noexcept;
void __inox_str_append(void** slot, std::int64_t count, void* const* parts, std::int32_t* status) noexcept;
std::int64_t __inox_str_length(void* string) noexcept;
void* __inox_str_substr(void* value, std::int64_t start, std::int64_t count, std::int32_t* status) noexcept;
void* __inox_str_edge(void* value, std::int64_t count, std::int32_t side, std::int32_t* status) noexcept;
std::int64_t __inox_str_pos(void* sub, void* value) noexcept;
void* __inox_str_case(void* value, std::int32_t mode, std::int32_t* status) noexcept;
void* __inox_str_trim(void* value, std::int32_t sides, std::int32_t* status) noexcept;
void* __inox_str_pad(void* value, std::int64_t width, std::int32_t side, std::int32_t* status) noexcept;
void* __inox_str_replicate(void* value, std::int64_t times, std::int32_t* status) noexcept;
void* __inox_str_replace(void* value, void* oldValue, void* newValue, std::int32_t* status) noexcept;
bool __inox_str_find(void* value, void* sub, std::int32_t where) noexcept;
std::int32_t __inox_str_compare_text(void* left, void* right) noexcept;
void* __inox_str_from_char(std::int64_t code, std::int32_t* status) noexcept;
std::int64_t __inox_str_char_at(void* value, std::int64_t position, std::int32_t* status) noexcept;
}

namespace {

struct Block {
    std::int64_t count;
    std::int64_t length;
    char bytes[64];
};

int failures = 0;

void check(bool condition, const std::string& what)
{
    if (!condition) {
        std::printf("[FAIL] inoxrt strings %s\n", what.c_str());
        ++failures;
    }
}

// An immortal block, as the compiler emits for a literal.
std::deque<Block> literals;

void* literal(const char* text)
{
    Block block{-1, static_cast<std::int64_t>(std::strlen(text)), {}};
    std::memcpy(block.bytes, text, std::strlen(text) + 1);
    literals.push_back(block);
    return &literals.back();
}

std::string text(void* value)
{
    const auto* block = static_cast<const Block*>(value);
    return std::string(block->bytes, static_cast<std::size_t>(block->length));
}

// A String of this library (count 1), as the program gets from a concatenation.
void* owned(const char* value)
{
    void* parts[] = {literal(value)};
    std::int32_t status = -1;
    return __inox_str_concat(1, parts, &status);
}

// The text of a result, released after reading; "<status N>" on a failure.
// `status` is a reference: it is read after the call that sets it, whatever
// the order in which the arguments of take were evaluated.
std::string take(void* result, const std::int32_t& status)
{
    if (status != 0) {
        return "<status " + std::to_string(status) + ">";
    }
    std::string value = text(result);
    __inox_str_release(result);
    return value;
}

void expect(const std::string& actual, const std::string& expected, const std::string& what)
{
    check(actual == expected, what + ": expected [" + expected + "], got [" + actual + "]");
}

void testConcat()
{
    std::int32_t status = -1;
    void* parts[] = {literal("ab"), literal(""), literal("é€"), literal("😀")};
    void* joined = __inox_str_concat(4, parts, &status);
    check(status == 0 && static_cast<Block*>(joined)->count == 1, "concat makes one owned block");
    check(__inox_str_length(joined) == 5, "concat counts characters");
    expect(take(joined, status), "abé€😀", "concat");
    void* none[] = {literal(""), literal("")};
    expect(take(__inox_str_concat(2, none, &status), status), "", "concat of empty parts");
}

void testAppend()
{
    std::int32_t status = -1;
    void* s = owned("ab");
    void* part[] = {literal("cd")};
    __inox_str_append(&s, 1, part, &status);
    expect(text(s), "abcd", "append");
    // The block grew with room to spare: the next append stays in place.
    void* before = s;
    void* small[] = {literal("e")};
    __inox_str_append(&s, 1, small, &status);
    check(s == before, "append in place when the block has room and one owner");
    expect(text(s), "abcde", "append in place");
    // Many appends: few blocks (capacity doubles).
    int blocks = 0;
    for (int index = 0; index < 1000; ++index) {
        void* previous = s;
        __inox_str_append(&s, 1, small, &status);
        blocks += s != previous ? 1 : 0;
    }
    check(blocks <= 12 && __inox_str_length(s) == 1005, "1000 appends need at most 12 new blocks");
    // A shared block is never changed.
    void* other = s;
    __inox_str_retain(other);
    __inox_str_append(&s, 1, part, &status);
    check(s != other && __inox_str_length(other) == 1005 && __inox_str_length(s) == 1007,
          "append to a shared block leaves the other holder unchanged");
    __inox_str_release(other);
    __inox_str_release(s);
    // S := S + S, in place and growing.
    void* twice = owned("xy");
    void* self[] = {twice, twice};
    __inox_str_append(&twice, 2, self, &status);
    expect(text(twice), "xyxyxy", "S := S + S + S when growing");
    __inox_str_append(&twice, 1, small, &status);
    void* again[] = {twice};  // a part is read from the slot just before the call
    __inox_str_append(&twice, 1, again, &status);
    expect(text(twice), "xyxyxyexyxyxye", "S := S + S in place");
    __inox_str_release(twice);
    // An immortal block in the slot is replaced, never written.
    void* constant = literal("lit");
    void* slot = constant;
    __inox_str_append(&slot, 1, part, &status);
    check(text(constant) == "lit" && text(slot) == "litcd", "append to a literal makes a new block");
    __inox_str_release(slot);
}

void testPositions()
{
    std::int32_t status = -1;
    void* hello = literal("héllo");
    check(__inox_str_length(hello) == 5, "Length counts code points");
    expect(take(__inox_str_substr(hello, 2, 3, &status), status), "éll", "Substr in multi-byte text");
    expect(take(__inox_str_substr(hello, 6, 0, &status), status), "", "Substr at Length + 1, none");
    expect(take(__inox_str_substr(hello, 1, 5, &status), status), "héllo", "Substr of everything");
    expect(take(__inox_str_substr(hello, 0, 1, &status), status), "<status 1>", "Substr from 0");
    expect(take(__inox_str_substr(hello, 3, 4, &status), status), "<status 1>", "Substr past the end");
    expect(take(__inox_str_substr(hello, 2, -1, &status), status), "<status 1>", "Substr with a negative count");
    expect(take(__inox_str_substr(literal("abcdef"), 3, 2, &status), status), "cd", "Substr of ASCII");
    expect(take(__inox_str_edge(hello, 2, 1, &status), status), "hé", "Left");
    expect(take(__inox_str_edge(hello, 3, 2, &status), status), "llo", "Right");
    expect(take(__inox_str_edge(hello, 6, 1, &status), status), "<status 1>", "Left beyond the length");
    expect(take(__inox_str_edge(hello, -1, 2, &status), status), "<status 1>", "Right with a negative count");
    check(__inox_str_pos(literal("ll"), hello) == 3, "Pos counts characters");
    check(__inox_str_pos(literal("x"), hello) == 0 && __inox_str_pos(literal(""), hello) == 0,
          "Pos of an absent or empty String is 0");
}

void testCase()
{
    std::int32_t status = -1;
    expect(take(__inox_str_case(literal("straße ñ ı σ"), 0, &status), status), "STRAßE Ñ I Σ",
           "UpperCase: one code point to one, ß unchanged");
    expect(take(__inox_str_case(literal("ÀÉÎ Ω ABC"), 1, &status), status), "àéî ω abc", "LowerCase");
    void* plain = literal("123 -");
    void* same = __inox_str_case(plain, 0, &status);
    check(same == plain, "an unchanged String is returned as it is");
    __inox_str_release(same);
    check(__inox_str_compare_text(literal("ÁRVORE"), literal("árvore")) == 0, "CompareText ignores case");
    check(__inox_str_compare_text(literal("a"), literal("B")) == -1 &&
              __inox_str_compare_text(literal("b"), literal("A")) == 1,
          "CompareText orders after folding");
    check(__inox_str_compare_text(literal("ab"), literal("ABC")) == -1, "CompareText: a prefix first");
}

void testTrimPadReplicate()
{
    std::int32_t status = -1;
    void* spaced = literal("\xC2\xA0 x y\t\n");  // NO-BREAK SPACE, space, ..., tab, line feed
    expect(take(__inox_str_trim(spaced, 3, &status), status), "x y", "Trim with Unicode White_Space");
    expect(take(__inox_str_trim(spaced, 1, &status), status), "x y\t\n", "LTrim");
    expect(take(__inox_str_trim(spaced, 2, &status), status), "\xC2\xA0 x y", "RTrim");
    expect(take(__inox_str_trim(literal(" \t "), 3, &status), status), "", "Trim of only spaces");
    expect(take(__inox_str_pad(literal("ab"), 5, 1, &status), status), "   ab", "PadLeft");
    expect(take(__inox_str_pad(literal("é"), 3, 2, &status), status), "é  ", "PadRight counts characters");
    expect(take(__inox_str_pad(literal("abc"), 2, 1, &status), status), "abc", "Pad of a longer String");
    expect(take(__inox_str_pad(literal("abc"), -1, 1, &status), status), "<status 1>", "Pad to a negative width");
    expect(take(__inox_str_replicate(literal("ab"), 3, &status), status), "ababab", "Replicate");
    expect(take(__inox_str_replicate(literal("ab"), 0, &status), status), "", "Replicate 0 times");
    expect(take(__inox_str_replicate(literal("ab"), -1, &status), status), "<status 1>", "Replicate -1 times");
}

void testReplaceFind()
{
    std::int32_t status = -1;
    expect(take(__inox_str_replace(literal("a-b-c"), literal("-"), literal("+="), &status), status), "a+=b+=c",
           "Replace every occurrence");
    expect(take(__inox_str_replace(literal("aaa"), literal("aa"), literal("b"), &status), status), "ba",
           "Replace left to right, without overlaps");
    expect(take(__inox_str_replace(literal("abc"), literal(""), literal("x"), &status), status), "abc",
           "Replace of an empty String changes nothing");
    expect(take(__inox_str_replace(literal("abab"), literal("ab"), literal(""), &status), status), "",
           "Replace that removes everything");
    void* text = literal("hello world");
    check(__inox_str_find(text, literal("o w"), 0) && !__inox_str_find(text, literal("xyz"), 0), "Contains");
    check(__inox_str_find(text, literal("hell"), 1) && !__inox_str_find(text, literal("world"), 1), "StartsWith");
    check(__inox_str_find(text, literal("world"), 2) && !__inox_str_find(text, literal("hello"), 2), "EndsWith");
    check(__inox_str_find(text, literal(""), 0) && __inox_str_find(text, literal(""), 1) &&
              __inox_str_find(text, literal(""), 2),
          "an empty String is found everywhere");
    check(!__inox_str_find(literal("ab"), literal("abc"), 2), "EndsWith of a longer String");
}

// Change char-values: String(C) and CharAt(S, I).
void testChars()
{
    std::int32_t status = -1;
    void* a = __inox_str_from_char('a', &status);
    check(__inox_str_length(a) == 1, "String(C) is one character");
    expect(take(a, status), "a", "String of an ASCII Char");
    expect(take(__inox_str_from_char(0xE9, &status), status), "é", "String of a two-byte Char");
    expect(take(__inox_str_from_char(0x20AC, &status), status), "€", "String of a three-byte Char");
    expect(take(__inox_str_from_char(0x1F600, &status), status), "😀", "String of a four-byte Char");
    void* word = literal("aé€😀");
    check(__inox_str_char_at(word, 1, &status) == 'a' && status == 0, "CharAt of the first character");
    check(__inox_str_char_at(word, 2, &status) == 0xE9, "CharAt counts characters");
    check(__inox_str_char_at(word, 4, &status) == 0x1F600, "CharAt of the last character");
    __inox_str_char_at(word, 5, &status);
    check(status == 1, "CharAt past the end is out of range");
    __inox_str_char_at(word, 0, &status);
    check(status == 1, "CharAt of position 0 is out of range");
    __inox_str_char_at(literal(""), 1, &status);
    check(status == 1, "CharAt of an empty String is out of range");
}

} // namespace

int runStringFunctionsTests()
{
    testConcat();
    testAppend();
    testPositions();
    testCase();
    testTrimPadReplicate();
    testReplaceFind();
    testChars();
    if (failures == 0) {
        std::printf("[PASS] inoxrt string functions unit tests\n");
    }
    return failures;
}
