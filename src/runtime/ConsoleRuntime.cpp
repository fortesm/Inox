// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// The console of an Inox program (OpenSpec change console-abi-terminal,
// canon r3.88, decisions E1 and F1-F2 of Marcelo Fortes, 2026-10-21).
//
// Every program links libinoxrt and reaches the console only through the
// functions below: Put/PutLn, Get/GetLn and Std.Terminal. One C runtime (this
// library's) owns standard output and standard input, so on Windows a FILE*
// never crosses between two C runtimes, and output stays in order whatever
// prints it. On Windows the console is switched to UTF-8 and to VT
// processing at start, and given back as it was at exit.
//
// Input is read into one buffer shared by Get, GetLn and ReadKey. Before the
// program waits for input, standard output is flushed, so a prompt written
// with Put is seen before the program stops to read.

#include "Console.h"
#include "Decimal.h"
#include "NumberText.h"
#include "StringBlock.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <csignal>
#include <cerrno>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

using inox::runtime::InoxString;

namespace {

// ---------------------------------------------------------------- state ----

bool gInitialized = false;
bool gRaw = false;           // Std.Terminal RawMode(True) is in force
bool gCursorHidden = false;  // ShowCursor(False) was called
bool gColored = false;       // SetColor was called after the last reset
std::terminate_handler gPreviousTerminate = nullptr;

#if defined(_WIN32)
HANDLE gInputHandle = INVALID_HANDLE_VALUE;
HANDLE gOutputHandle = INVALID_HANDLE_VALUE;
bool gInputIsConsole = false;
bool gOutputIsConsole = false;
DWORD gOriginalInputMode = 0;
DWORD gOriginalOutputMode = 0;
UINT gOriginalInputCodePage = 0;
UINT gOriginalOutputCodePage = 0;
wchar_t gPendingHighSurrogate = 0;
#else
bool gHaveOriginalTermios = false;
struct termios gOriginalTermios;
#endif

// ---------------------------------------------------------------- output ---

void writeOut(const char* bytes, std::size_t length) noexcept
{
    if (length != 0) {
        std::fwrite(bytes, 1, length, stdout);
    }
}

void writeText(const char* text) noexcept
{
    writeOut(text, std::strlen(text));
}

// ----------------------------------------------------------------- input ---

constexpr std::size_t kInputCapacity = 4096;
unsigned char gInput[kInputCapacity];
std::size_t gInputStart = 0;
std::size_t gInputEnd = 0;
bool gInputEnded = false;

#if defined(_WIN32)
// UTF-16 read from the console, appended to the input buffer as UTF-8. A
// high surrogate at the end of one read waits for its pair.
void appendWide(const wchar_t* text, DWORD count) noexcept
{
    wchar_t joined[1026];
    DWORD length = 0;
    if (gPendingHighSurrogate != 0) {
        joined[length++] = gPendingHighSurrogate;
        gPendingHighSurrogate = 0;
    }
    for (DWORD index = 0; index < count && length < 1024; ++index) {
        joined[length++] = text[index];
    }
    if (length != 0 && joined[length - 1] >= 0xD800 && joined[length - 1] <= 0xDBFF) {
        gPendingHighSurrogate = joined[--length];
    }
    if (length == 0) {
        return;
    }
    const int room = static_cast<int>(kInputCapacity - gInputEnd);
    const int written = WideCharToMultiByte(CP_UTF8, 0, joined, static_cast<int>(length),
                                            reinterpret_cast<char*>(gInput + gInputEnd), room, nullptr, nullptr);
    if (written > 0) {
        gInputEnd += static_cast<std::size_t>(written);
    }
}

// Whether a key press is waiting in the console within `milliseconds`;
// events that are not key presses (focus, mouse, resize) are dropped.
bool consoleKeyWithin(DWORD milliseconds) noexcept
{
    for (;;) {
        if (WaitForSingleObject(gInputHandle, milliseconds) != WAIT_OBJECT_0) {
            return false;
        }
        INPUT_RECORD record;
        DWORD count = 0;
        if (!PeekConsoleInputW(gInputHandle, &record, 1, &count) || count == 0) {
            return false;
        }
        if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown &&
            record.Event.KeyEvent.uChar.UnicodeChar != 0) {
            return true;
        }
        ReadConsoleInputW(gInputHandle, &record, 1, &count);
    }
}
#endif

// Reads more input into the empty buffer. `waitMilliseconds` < 0 waits for
// it; otherwise false when nothing arrives in time. False at end of input.
bool fillInput(int waitMilliseconds) noexcept
{
    if (gInputEnded) {
        return false;
    }
    std::fflush(stdout);
    gInputStart = 0;
    gInputEnd = 0;
#if defined(_WIN32)
    if (gInputIsConsole) {
        if (waitMilliseconds >= 0 && !consoleKeyWithin(static_cast<DWORD>(waitMilliseconds))) {
            return false;
        }
        wchar_t wide[1024];
        DWORD count = 0;
        // In raw mode one key at a time; a cooked read returns one line.
        const DWORD wanted = gRaw ? 1 : 1024;
        if (!ReadConsoleW(gInputHandle, wide, wanted, &count, nullptr) || count == 0) {
            gInputEnded = true;
            return false;
        }
        // Ctrl+Z at the start of a cooked line is the end of input.
        if (!gRaw && wide[0] == 0x1A) {
            gInputEnded = true;
            return false;
        }
        appendWide(wide, count);
        return gInputEnd != 0 || fillInput(waitMilliseconds);
    }
    if (waitMilliseconds >= 0 && WaitForSingleObject(gInputHandle, static_cast<DWORD>(waitMilliseconds)) !=
                                     WAIT_OBJECT_0) {
        return false;
    }
    DWORD count = 0;
    if (!ReadFile(gInputHandle, gInput, static_cast<DWORD>(kInputCapacity), &count, nullptr) || count == 0) {
        gInputEnded = true;
        return false;
    }
    gInputEnd = count;
    return true;
#else
    if (waitMilliseconds >= 0) {
        struct pollfd descriptor{0, POLLIN, 0};
        int ready = 0;
        do {
            ready = ::poll(&descriptor, 1, waitMilliseconds);
        } while (ready < 0 && errno == EINTR);
        if (ready <= 0) {
            return false;
        }
    }
    for (;;) {
        const ssize_t count = ::read(0, gInput, kInputCapacity);
        if (count > 0) {
            gInputEnd = static_cast<std::size_t>(count);
            return true;
        }
        if (count < 0 && errno == EINTR) {
            continue;
        }
        gInputEnded = true;
        return false;
    }
#endif
}

// The next byte of input, or -1 at its end.
int peekByte() noexcept
{
    if (gInputStart == gInputEnd && !fillInput(-1)) {
        return -1;
    }
    return gInput[gInputStart];
}

int readRawByte() noexcept
{
    const int byte = peekByte();
    if (byte >= 0) {
        ++gInputStart;
    }
    return byte;
}

// The next byte of input as a program reads it: on Windows, outside raw
// mode, "\r\n" is one "\n", as a text-mode C stream reads it.
int readByte() noexcept
{
    const int byte = readRawByte();
#if defined(_WIN32)
    if (byte == '\r' && !gRaw && peekByte() == '\n') {
        return readRawByte();
    }
#endif
    return byte;
}

// The next byte if one arrives within `milliseconds` (always waiting when
// standard input is not a terminal), else -1.
int readByteWithin(int milliseconds) noexcept
{
    if (gInputStart == gInputEnd) {
        bool terminal = false;
#if defined(_WIN32)
        terminal = gInputIsConsole;
#else
        terminal = ::isatty(0) != 0;
#endif
        if (!fillInput(terminal ? milliseconds : -1)) {
            return -1;
        }
    }
    return gInput[gInputStart++];
}

int readForLine(void*) noexcept
{
    return readByte();
}

// ------------------------------------------------------------- terminal ---

#if !defined(_WIN32)
// Async-signal-safe: what a signal handler may do before the program dies.
void restoreTerminalFromSignal() noexcept
{
    if (gColored) {
        const char reset[] = "\x1b[0m";
        [[maybe_unused]] const ssize_t written = ::write(1, reset, sizeof reset - 1);
    }
    if (gCursorHidden) {
        const char show[] = "\x1b[?25h";
        [[maybe_unused]] const ssize_t written = ::write(1, show, sizeof show - 1);
    }
    if (gRaw && gHaveOriginalTermios) {
        ::tcsetattr(0, TCSAFLUSH, &gOriginalTermios);
    }
}

extern "C" void inoxRestoreOnSignal(int signalNumber)
{
    restoreTerminalFromSignal();
    std::signal(signalNumber, SIG_DFL);
    std::raise(signalNumber);
}
#endif

void restoreTerminal() noexcept
{
    if (gColored) {
        writeText("\x1b[0m");
        gColored = false;
    }
    if (gCursorHidden) {
        writeText("\x1b[?25h");
        gCursorHidden = false;
    }
    std::fflush(stdout);
#if defined(_WIN32)
    if (gInputIsConsole) {
        SetConsoleMode(gInputHandle, gOriginalInputMode);
    }
    if (gOutputIsConsole) {
        SetConsoleMode(gOutputHandle, gOriginalOutputMode);
    }
    if (gOriginalOutputCodePage != 0) {
        SetConsoleOutputCP(gOriginalOutputCodePage);
    }
    if (gOriginalInputCodePage != 0) {
        SetConsoleCP(gOriginalInputCodePage);
    }
#else
    if (gRaw && gHaveOriginalTermios) {
        ::tcsetattr(0, TCSAFLUSH, &gOriginalTermios);
    }
#endif
    gRaw = false;
}

void restoreAtExit()
{
    inox::runtime::shutdownConsole();
}

[[noreturn]] void restoreOnTerminate()
{
    inox::runtime::shutdownConsole();
    if (gPreviousTerminate != nullptr) {
        gPreviousTerminate();
    }
    std::abort();
}

void setRaw(bool raw) noexcept
{
    if (raw == gRaw) {
        return;
    }
    std::fflush(stdout);
#if defined(_WIN32)
    if (!gInputIsConsole) {
        return;
    }
    if (raw) {
        // Keys arrive one at a time, not echoed, with the arrows and the
        // function keys as VT sequences (the same ones a Unix terminal sends).
        SetConsoleMode(gInputHandle, ENABLE_VIRTUAL_TERMINAL_INPUT);
    } else {
        SetConsoleMode(gInputHandle, gOriginalInputMode);
    }
    gRaw = raw;
#else
    if (!gHaveOriginalTermios) {
        if (::tcgetattr(0, &gOriginalTermios) != 0) {
            return;  // not a terminal: raw mode means nothing
        }
        gHaveOriginalTermios = true;
        std::signal(SIGINT, inoxRestoreOnSignal);
        std::signal(SIGTERM, inoxRestoreOnSignal);
        std::signal(SIGHUP, inoxRestoreOnSignal);
        std::signal(SIGQUIT, inoxRestoreOnSignal);
    }
    if (raw) {
        struct termios settings = gOriginalTermios;
        // Bytes as typed: no echo, no line editing, no signals from Ctrl+C or
        // Ctrl+Z, no flow control, no CR->NL. Output keeps NL -> CRLF, so
        // PutLn works the same in raw mode.
        settings.c_iflag &= static_cast<tcflag_t>(~(BRKINT | ICRNL | INPCK | ISTRIP | IXON));
        settings.c_cflag |= CS8;
        settings.c_lflag &= static_cast<tcflag_t>(~(ECHO | ICANON | IEXTEN | ISIG));
        settings.c_cc[VMIN] = 1;
        settings.c_cc[VTIME] = 0;
        if (::tcsetattr(0, TCSAFLUSH, &settings) != 0) {
            return;
        }
    } else {
        ::tcsetattr(0, TCSAFLUSH, &gOriginalTermios);
    }
    gRaw = raw;
#endif
}

// ReadKey: what generated code receives, packed in an Int64 (codegen builds
// the TKey from it): the TKeyKind ordinal in bits 0..7, Ctrl in bit 8, Alt in
// bit 9, the Char's code point from bit 16; -1 at the end of input.
enum KeyKind : std::int64_t {
    KeyChar = 0,
    KeyUp,
    KeyDown,
    KeyLeft,
    KeyRight,
    KeyHome,
    KeyEnd,
    KeyPageUp,
    KeyPageDown,
    KeyEnter,
    KeyBackspace,
    KeyDelete,
    KeyTab,
    KeyEscape,
    KeyF1,
};

constexpr std::int64_t kCtrl = 1 << 8;
constexpr std::int64_t kAlt = 1 << 9;
constexpr std::int64_t kNoKey = -2;  // a sequence this reader does not know

std::int64_t packKey(std::int64_t kind, std::int64_t code = 0, std::int64_t modifiers = 0) noexcept
{
    return kind | modifiers | (code << 16);
}

// The modifiers of an xterm parameter (1 + Shift 1 + Alt 2 + Ctrl 4).
std::int64_t xtermModifiers(int parameter) noexcept
{
    std::int64_t modifiers = 0;
    if (parameter > 1) {
        const int bits = parameter - 1;
        if ((bits & 2) != 0) {
            modifiers |= kAlt;
        }
        if ((bits & 4) != 0) {
            modifiers |= kCtrl;
        }
    }
    return modifiers;
}

// The rest of a UTF-8 character whose first byte is `lead`.
std::int64_t readCharacter(int lead) noexcept
{
    int length = 0;
    std::int64_t code = 0;
    if (lead < 0x80) {
        return lead;
    }
    if ((lead & 0xE0) == 0xC0) {
        length = 2;
        code = lead & 0x1F;
    } else if ((lead & 0xF0) == 0xE0) {
        length = 3;
        code = lead & 0x0F;
    } else if ((lead & 0xF8) == 0xF0) {
        length = 4;
        code = lead & 0x07;
    } else {
        return 0xFFFD;
    }
    for (int index = 1; index < length; ++index) {
        const int next = peekByte();
        if (next < 0 || (next & 0xC0) != 0x80) {
            return 0xFFFD;
        }
        readRawByte();
        code = (code << 6) | (next & 0x3F);
    }
    const std::int64_t smallest = length == 2 ? 0x80 : length == 3 ? 0x800 : 0x10000;
    if (code < smallest || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF)) {
        return 0xFFFD;
    }
    return code;
}

// A key that is one byte (or one UTF-8 character).
std::int64_t plainKey(int byte, std::int64_t modifiers) noexcept
{
    switch (byte) {
    case '\r':
    case '\n':
        return packKey(KeyEnter, 0, modifiers);
    case '\t':
        return packKey(KeyTab, 0, modifiers);
    case 0x7F:
    case 0x08:
        return packKey(KeyBackspace, 0, modifiers);
    case 0x1B:
        return packKey(KeyEscape, 0, modifiers);
    default:
        break;
    }
    if (byte == 0) {
        return packKey(KeyChar, ' ', modifiers | kCtrl);  // Ctrl+Space
    }
    if (byte < 0x1B) {
        return packKey(KeyChar, 'a' + byte - 1, modifiers | kCtrl);
    }
    if (byte < 0x20) {
        return packKey(KeyChar, byte + 0x40, modifiers | kCtrl);  // Ctrl+\ ] ^ _
    }
    return packKey(KeyChar, readCharacter(byte), modifiers);
}

// A CSI sequence after "\x1b[": parameters, then the final byte.
std::int64_t csiKey() noexcept
{
    int parameters[4] = {0, 0, 0, 0};
    int count = 0;
    int byte = readByteWithin(50);
    while (byte >= 0 && ((byte >= '0' && byte <= '9') || byte == ';')) {
        if (byte == ';') {
            if (count < 3) {
                ++count;
            }
        } else {
            parameters[count] = parameters[count] * 10 + (byte - '0');
        }
        byte = readByteWithin(50);
    }
    if (byte < 0) {
        return packKey(KeyEscape);
    }
    const std::int64_t modifiers = xtermModifiers(parameters[1]);
    switch (byte) {
    case 'A':
        return packKey(KeyUp, 0, modifiers);
    case 'B':
        return packKey(KeyDown, 0, modifiers);
    case 'C':
        return packKey(KeyRight, 0, modifiers);
    case 'D':
        return packKey(KeyLeft, 0, modifiers);
    case 'H':
        return packKey(KeyHome, 0, modifiers);
    case 'F':
        return packKey(KeyEnd, 0, modifiers);
    case 'P':
        return packKey(KeyF1, 0, modifiers);
    case 'Q':
        return packKey(KeyF1 + 1, 0, modifiers);
    case 'R':
        return packKey(KeyF1 + 2, 0, modifiers);
    case 'S':
        return packKey(KeyF1 + 3, 0, modifiers);
    case '~':
        break;
    default:
        return kNoKey;
    }
    switch (parameters[0]) {
    case 1:
    case 7:
        return packKey(KeyHome, 0, modifiers);
    case 4:
    case 8:
        return packKey(KeyEnd, 0, modifiers);
    case 3:
        return packKey(KeyDelete, 0, modifiers);
    case 5:
        return packKey(KeyPageUp, 0, modifiers);
    case 6:
        return packKey(KeyPageDown, 0, modifiers);
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        return packKey(KeyF1 + (parameters[0] - 11), 0, modifiers);
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
        return packKey(KeyF1 + 5 + (parameters[0] - 17), 0, modifiers);
    case 23:
    case 24:
        return packKey(KeyF1 + 10 + (parameters[0] - 23), 0, modifiers);
    default:
        return kNoKey;
    }
}

std::int64_t readKey() noexcept
{
    for (;;) {
        const int byte = readRawByte();
        if (byte < 0) {
            return -1;
        }
        if (byte == '\r' && gInputStart < gInputEnd && gInput[gInputStart] == '\n') {
            ++gInputStart;  // "\r\n" from a file or a pipe is one Enter
        }
        if (byte != 0x1B) {
            return plainKey(byte, 0);
        }
        const int next = readByteWithin(50);
        if (next < 0) {
            return packKey(KeyEscape);
        }
        std::int64_t key = kNoKey;
        if (next == '[') {
            key = csiKey();
        } else if (next == 'O') {
            const int final = readByteWithin(50);
            switch (final) {
            case 'A':
                key = packKey(KeyUp);
                break;
            case 'B':
                key = packKey(KeyDown);
                break;
            case 'C':
                key = packKey(KeyRight);
                break;
            case 'D':
                key = packKey(KeyLeft);
                break;
            case 'H':
                key = packKey(KeyHome);
                break;
            case 'F':
                key = packKey(KeyEnd);
                break;
            case 'P':
            case 'Q':
            case 'R':
            case 'S':
                key = packKey(KeyF1 + (final - 'P'));
                break;
            case -1:
                key = packKey(KeyChar, 'O', kAlt);
                break;
            default:
                key = kNoKey;
                break;
            }
        } else {
            // Escape then a key: that key with Alt.
            key = plainKey(next, kAlt);
        }
        if (key != kNoKey) {
            return key;
        }
    }
}

} // namespace

namespace inox::runtime {

void shutdownConsole() noexcept
{
    restoreTerminal();
    std::fflush(stdout);
    std::fflush(stderr);
}

// A fault of the library itself (a ParamStr index, a Halt code): the same
// message and status as the program's fault function (CANON-19).
[[noreturn]] void fault(const char* category) noexcept
{
    shutdownConsole();
    std::fputs("Inox runtime error: ", stderr);
    std::fputs(category, stderr);
    std::fputs("\n", stderr);
    std::fflush(stderr);
    std::_Exit(70);
}

} // namespace inox::runtime

extern "C" {

// Called first in Main.
void __inox_runtime_init(std::int32_t argc, char** argv) noexcept
{
    if (gInitialized) {
        return;
    }
    gInitialized = true;
    inox::runtime::setCommandLine(argc, argv);
#if defined(_WIN32)
    gInputHandle = GetStdHandle(STD_INPUT_HANDLE);
    gOutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    gInputIsConsole = GetConsoleMode(gInputHandle, &gOriginalInputMode) != 0;
    gOutputIsConsole = GetConsoleMode(gOutputHandle, &gOriginalOutputMode) != 0;
    if (gInputIsConsole || gOutputIsConsole) {
        gOriginalOutputCodePage = GetConsoleOutputCP();
        gOriginalInputCodePage = GetConsoleCP();
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
    }
    if (gOutputIsConsole) {
        SetConsoleMode(gOutputHandle,
                       gOriginalOutputMode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif
    gPreviousTerminate = std::set_terminate(restoreOnTerminate);
    std::atexit(restoreAtExit);
}

// Called by the program's fault function before it stops (CANON-19).
void __inox_runtime_shutdown() noexcept
{
    inox::runtime::shutdownConsole();
}

void __inox_con_write(const char* bytes, std::int64_t length) noexcept
{
    writeOut(bytes, static_cast<std::size_t>(length));
}

void __inox_con_put_str(const void* value) noexcept
{
    const auto* string = static_cast<const InoxString*>(value);
    writeOut(string->bytes, static_cast<std::size_t>(string->length));
}

void __inox_con_put_i64(std::int64_t number) noexcept
{
    std::printf("%lld", static_cast<long long>(number));
}

void __inox_con_put_u64(std::uint64_t number) noexcept
{
    std::printf("%llu", static_cast<unsigned long long>(number));
}

// B3 (r3.90): the shortest text that reads back as the same value.
void __inox_con_put_f64(double number) noexcept
{
    char text[inox::runtime::kRealTextCapacity];
    writeOut(text, inox::runtime::formatReal(number, false, text));
}

void __inox_con_put_f32(float number) noexcept
{
    char text[inox::runtime::kRealTextCapacity];
    writeOut(text, inox::runtime::formatReal(static_cast<double>(number), true, text));
}

void __inox_con_put_bool(bool flag) noexcept
{
    writeText(flag ? "true" : "false");
}

void __inox_con_put_newline() noexcept
{
    writeOut("\n", 1);
}

// One byte of input (0..255), or -1 at its end: Get reads through it.
std::int32_t __inox_con_getc() noexcept
{
    return readByte();
}

// GetLn: the rest of the line without its end, owned (+1) unless "".
void* __inox_con_getln(std::int32_t* status) noexcept
{
    inox::runtime::LineStatus lineStatus = inox::runtime::LineStatus::Ok;
    InoxString* line = inox::runtime::readLine(readForLine, nullptr, lineStatus);
    *status = static_cast<std::int32_t>(lineStatus);
    return line;
}

// ---- Get of a Char, a Float and EndOfInput (change input-chars-exponents) ----

// Get(C): the next character exactly (a space, a line end), its code point;
// -1 at the end of input, -2 for bytes that are not UTF-8.
std::int64_t __inox_con_get_char() noexcept
{
    const int lead = readByte();
    if (lead < 0) {
        return -1;
    }
    int length = 0;
    std::int64_t code = 0;
    if (lead < 0x80) {
        return lead;
    }
    if ((lead & 0xE0) == 0xC0) {
        length = 2;
        code = lead & 0x1F;
    } else if ((lead & 0xF0) == 0xE0) {
        length = 3;
        code = lead & 0x0F;
    } else if ((lead & 0xF8) == 0xF0) {
        length = 4;
        code = lead & 0x07;
    } else {
        return -2;
    }
    for (int index = 1; index < length; ++index) {
        const int next = peekByte();
        if (next < 0 || (next & 0xC0) != 0x80) {
            return -2;
        }
        readRawByte();
        code = (code << 6) | (next & 0x3F);
    }
    const std::int64_t smallest = length == 2 ? 0x80 : length == 3 ? 0x800 : 0x10000;
    if (code < smallest || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF)) {
        return -2;
    }
    return code;
}

// EndOfInput: nothing is left to read (waits for input on a terminal).
bool __inox_con_at_end() noexcept
{
    return peekByte() < 0;
}

} // extern "C"

namespace {

bool isInputSpace(int byte) noexcept
{
    return byte == ' ' || byte == '\t' || byte == '\n' || byte == '\r';
}

// One real number of the input, strictly: optional '-', digits, an optional
// fraction and an optional exponent, then white space or the end. Status 0
// ok, 1 invalid (also at the end of input), 2 beyond the type's range.
std::int32_t readRealToken(char* text, std::size_t capacity) noexcept
{
    int byte = readByte();
    while (byte >= 0 && isInputSpace(byte)) {
        byte = readByte();
    }
    std::size_t length = 0;
    bool valid = true;
    const auto take = [&](int ch) {
        if (length + 1 < capacity) {
            text[length++] = static_cast<char>(ch);
        } else {
            valid = false;
        }
    };
    const auto digits = [&]() {
        if (byte < '0' || byte > '9') {
            valid = false;
            return;
        }
        while (byte >= '0' && byte <= '9') {
            take(byte);
            byte = readByte();
        }
    };
    if (byte == '-') {
        take(byte);
        byte = readByte();
    }
    digits();
    if (valid && byte == '.') {
        take(byte);
        byte = readByte();
        digits();
    }
    if (valid && (byte == 'e' || byte == 'E')) {
        take(byte);
        byte = readByte();
        if (byte == '+' || byte == '-') {
            take(byte);
            byte = readByte();
        }
        digits();
    }
    if (valid && byte >= 0 && !isInputSpace(byte)) {
        valid = false;
    }
    text[length] = '\0';
    return valid ? 0 : 1;
}

} // namespace

extern "C" {

std::int32_t __inox_con_get_f64(double* out) noexcept
{
    char text[400];
    if (readRealToken(text, sizeof text) != 0) {
        return 1;
    }
    const double value = std::strtod(text, nullptr);
    if (value == HUGE_VAL || value == -HUGE_VAL) {
        return 2;
    }
    *out = value;
    return 0;
}

// A Float32 is read from the text straight into a float, rounded once.
std::int32_t __inox_con_get_f32(float* out) noexcept
{
    char text[400];
    if (readRealToken(text, sizeof text) != 0) {
        return 1;
    }
    const float value = std::strtof(text, nullptr);
    if (value == HUGE_VALF || value == -HUGE_VALF) {
        return 2;
    }
    *out = value;
    return 0;
}

// ---- Currency and Crypto (change decimals, r3.91) ----

// A5: at least two decimals, no trailing zeros after them.
void __inox_con_put_dec(std::uint64_t low, std::int64_t high, std::int32_t places) noexcept
{
    char text[inox::runtime::kDecimalTextCapacity];
    writeOut(text, inox::runtime::formatDecimal(inox::runtime::decimalOf(low, high), places, text));
}

// C2: Get of a Currency or a Crypto, strictly: an optional '-', digits and
// at most `places` decimals, then white space or the end. Status 0 ok, 1
// invalid (also at the end of input), 2 beyond the type.
std::int32_t __inox_con_get_dec(std::int32_t places, void* out) noexcept
{
    int byte = readByte();
    while (byte >= 0 && isInputSpace(byte)) {
        byte = readByte();
    }
    char text[128];
    std::size_t length = 0;
    while (byte >= 0 && !isInputSpace(byte)) {
        if (length + 1 >= sizeof text) {
            return 1;
        }
        text[length++] = static_cast<char>(byte);
        byte = readByte();
    }
    inox::runtime::Decimal value;
    const int status = inox::runtime::parseDecimal(text, length, places, value);
    if (status == 0) {
        std::memcpy(out, &value.low, 8);
        std::memcpy(static_cast<char*>(out) + 8, &value.high, 8);
    }
    return status;
}

// ---- Std.Terminal (F1) ----

void __inox_term_raw_mode(bool raw) noexcept
{
    setRaw(raw);
}

std::int64_t __inox_term_read_key() noexcept
{
    return readKey();
}

// Rows in bits 32..63, columns in bits 0..31; 24 x 80 when standard output
// is not a terminal.
std::int64_t __inox_term_screen_size() noexcept
{
    std::int64_t rows = 24;
    std::int64_t columns = 80;
#if defined(_WIN32)
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (gOutputIsConsole && GetConsoleScreenBufferInfo(gOutputHandle, &info)) {
        rows = info.srWindow.Bottom - info.srWindow.Top + 1;
        columns = info.srWindow.Right - info.srWindow.Left + 1;
    }
#else
    struct winsize size{};
    if (::ioctl(1, TIOCGWINSZ, &size) == 0 && size.ws_row != 0 && size.ws_col != 0) {
        rows = size.ws_row;
        columns = size.ws_col;
    }
#endif
    return (rows << 32) | columns;
}

void __inox_term_move_to(std::int64_t row, std::int64_t column) noexcept
{
    std::printf("\x1b[%lld;%lldH", static_cast<long long>(row), static_cast<long long>(column));
}

void __inox_term_clear_screen() noexcept
{
    writeText("\x1b[2J\x1b[H");
}

void __inox_term_clear_line() noexcept
{
    writeText("\x1b[2K");
}

// Colors are the 16 of TColor: 0..7 normal, 8..15 bright.
void __inox_term_set_color(std::int64_t foreground, std::int64_t background) noexcept
{
    const long long fg = foreground < 8 ? 30 + foreground : 90 + (foreground - 8);
    const long long bg = background < 8 ? 40 + background : 100 + (background - 8);
    std::printf("\x1b[%lld;%lldm", fg, bg);
    gColored = true;
}

void __inox_term_reset_color() noexcept
{
    writeText("\x1b[0m");
    gColored = false;
}

void __inox_term_show_cursor(bool visible) noexcept
{
    writeText(visible ? "\x1b[?25h" : "\x1b[?25l");
    gCursorHidden = !visible;
}

void __inox_term_flush() noexcept
{
    std::fflush(stdout);
}

} // extern "C"
