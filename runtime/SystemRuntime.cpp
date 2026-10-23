// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Std.SystemUtils and the clock of Std.DateTime (change
// datetime-systemutils, canon r3.92, decisions D1, D2 and D4 of Marcelo
// Fortes, 2026-10-21). Text crosses as UTF-8 on every system: on Windows the
// command line, the environment and the current directory are read with the
// wide functions and converted.

#include "Console.h"
#include "StringBlock.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")
#else
#include <unistd.h>
#endif

using inox::runtime::InoxString;

namespace {

std::vector<std::string> gArguments;

// An owned String of the bytes; "" when there are none. Out of memory stops
// the program (fault kind 12).
void* makeString(const std::string& text) noexcept
{
    if (text.empty()) {
        return inox::runtime::emptyString();
    }
    InoxString* result = inox::runtime::allocateString(static_cast<std::int64_t>(text.size()));
    if (result == nullptr) {
        inox::runtime::fault("out of memory");
    }
    std::memcpy(result->bytes, text.data(), text.size());
    return result;
}

std::string textOf(const void* value)
{
    const auto* string = static_cast<const InoxString*>(value);
    return std::string(string->bytes, static_cast<std::size_t>(string->length));
}

#if defined(_WIN32)
std::string narrow(const wchar_t* text, int length = -1)
{
    const int size = WideCharToMultiByte(CP_UTF8, 0, text, length, nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return {};
    }
    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, length, result.data(), size, nullptr, nullptr);
    if (length < 0 && !result.empty() && result.back() == '\0') {
        result.pop_back();
    }
    return result;
}

std::wstring widen(const std::string& text)
{
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring result(static_cast<std::size_t>(size > 0 ? size : 0), L'\0');
    if (size > 0) {
        MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), size);
    }
    return result;
}

// Microseconds between 1601-01-01 (FILETIME) and 0001-01-01.
constexpr std::int64_t kFileTimeEpochMicros = 50491123200LL * 1000000;
#endif

// Microseconds between 0001-01-01 and the Unix epoch, 1970-01-01.
constexpr std::int64_t kUnixEpochMicros = 62135596800LL * 1000000;

} // namespace

namespace inox::runtime {

void setCommandLine(int argc, char** argv) noexcept
{
    try {
        gArguments.clear();
#if defined(_WIN32)
        (void)argc;
        (void)argv;
        int count = 0;
        LPWSTR* wide = CommandLineToArgvW(GetCommandLineW(), &count);
        if (wide != nullptr) {
            for (int index = 0; index < count; ++index) {
                gArguments.push_back(narrow(wide[index]));
            }
            LocalFree(wide);
        }
#else
        for (int index = 0; index < argc; ++index) {
            gArguments.emplace_back(argv[index] != nullptr ? argv[index] : "");
        }
#endif
    } catch (...) {
        fault("out of memory");
    }
}

} // namespace inox::runtime

extern "C" {

// D4: ParamCount is the number of arguments after the program's own name;
// ParamStr(0) is the program, ParamStr(I) the I-th argument (outside
// 0..ParamCount: "value out of range").
std::int64_t __inox_sys_param_count() noexcept
{
    return gArguments.empty() ? 0 : static_cast<std::int64_t>(gArguments.size()) - 1;
}

void* __inox_sys_param_str(std::int64_t index) noexcept
{
    if (index < 0 || index >= static_cast<std::int64_t>(gArguments.size())) {
        inox::runtime::fault("value out of range");
    }
    return makeString(gArguments[static_cast<std::size_t>(index)]);
}

// GetEnv(Name): the variable's value, "" when it is not set; EnvExists(Name)
// tells the two apart.
void* __inox_sys_get_env(const void* name) noexcept
{
    try {
#if defined(_WIN32)
        const std::wstring key = widen(textOf(name));
        const DWORD size = GetEnvironmentVariableW(key.c_str(), nullptr, 0);
        if (size == 0) {
            return makeString({});
        }
        std::wstring value(size, L'\0');
        const DWORD written = GetEnvironmentVariableW(key.c_str(), value.data(), size);
        return makeString(narrow(value.c_str(), static_cast<int>(written)));
#else
        const char* value = std::getenv(textOf(name).c_str());
        return makeString(value != nullptr ? value : "");
#endif
    } catch (...) {
        inox::runtime::fault("out of memory");
    }
}

bool __inox_sys_env_exists(const void* name) noexcept
{
    try {
#if defined(_WIN32)
        const std::wstring key = widen(textOf(name));
        SetLastError(0);
        return GetEnvironmentVariableW(key.c_str(), nullptr, 0) != 0 || GetLastError() != ERROR_ENVVAR_NOT_FOUND;
#else
        return std::getenv(textOf(name).c_str()) != nullptr;
#endif
    } catch (...) {
        inox::runtime::fault("out of memory");
    }
}

// Halt(Code): ends the program now with exit status Code (0..255, else
// "value out of range"); the terminal is given back and output flushed.
[[noreturn]] void __inox_sys_halt(std::int64_t code) noexcept
{
    if (code < 0 || code > 255) {
        inox::runtime::fault("value out of range");
    }
    inox::runtime::shutdownConsole();
    std::exit(static_cast<int>(code));
}

// Sleep(Ms): waits Ms milliseconds (negative: "value out of range"); what
// was written is shown first.
void __inox_sys_sleep(std::int64_t milliseconds) noexcept
{
    if (milliseconds < 0) {
        inox::runtime::fault("value out of range");
    }
    std::fflush(stdout);
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void* __inox_sys_current_dir() noexcept
{
    try {
#if defined(_WIN32)
        const DWORD size = GetCurrentDirectoryW(0, nullptr);
        std::wstring path(size, L'\0');
        const DWORD written = GetCurrentDirectoryW(size, path.data());
        return makeString(narrow(path.c_str(), static_cast<int>(written)));
#else
        std::vector<char> buffer(256);
        while (::getcwd(buffer.data(), buffer.size()) == nullptr) {
            if (buffer.size() > (1u << 20)) {
                return makeString({});
            }
            buffer.resize(buffer.size() * 2);
        }
        return makeString(buffer.data());
#endif
    } catch (...) {
        inox::runtime::fault("out of memory");
    }
}

void* __inox_sys_platform_name() noexcept
{
#if defined(_WIN32)
    return makeString("Windows");
#elif defined(__APPLE__)
    return makeString("macOS");
#elif defined(__linux__)
    return makeString("Linux");
#elif defined(__FreeBSD__)
    return makeString("FreeBSD");
#elif defined(__OpenBSD__)
    return makeString("OpenBSD");
#elif defined(__NetBSD__)
    return makeString("NetBSD");
#else
    return makeString("Unix");
#endif
}

// D1: microseconds since 0001-01-01T00:00:00 UTC.
std::int64_t __inox_clock_now_utc() noexcept
{
#if defined(_WIN32)
    FILETIME time;
    GetSystemTimePreciseAsFileTime(&time);
    const std::int64_t ticks =
        static_cast<std::int64_t>((static_cast<std::uint64_t>(time.dwHighDateTime) << 32) | time.dwLowDateTime);
    return ticks / 10 + kFileTimeEpochMicros;
#else
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(now).count() + kUnixEpochMicros;
#endif
}

// D2: the offset of the system's local zone from UTC at the UTC instant
// `utc`, in microseconds (daylight saving included).
std::int64_t __inox_clock_utc_offset(std::int64_t utc) noexcept
{
#if defined(_WIN32)
    std::int64_t micros = utc - kFileTimeEpochMicros;
    if (micros < 0) {
        micros = 0;
    }
    const auto ticks = static_cast<std::uint64_t>(micros) * 10;
    FILETIME universal;
    universal.dwLowDateTime = static_cast<DWORD>(ticks);
    universal.dwHighDateTime = static_cast<DWORD>(ticks >> 32);
    SYSTEMTIME universalTime;
    SYSTEMTIME localTime;
    FILETIME local;
    if (!FileTimeToSystemTime(&universal, &universalTime) ||
        !SystemTimeToTzSpecificLocalTime(nullptr, &universalTime, &localTime) ||
        !SystemTimeToFileTime(&localTime, &local)) {
        return 0;
    }
    const std::int64_t localTicks =
        static_cast<std::int64_t>((static_cast<std::uint64_t>(local.dwHighDateTime) << 32) | local.dwLowDateTime);
    return (localTicks - static_cast<std::int64_t>(ticks)) / 10;
#else
    const std::int64_t seconds = (utc - kUnixEpochMicros) / 1000000;
    const auto instant = static_cast<std::time_t>(seconds);
    std::tm local{};
    if (::localtime_r(&instant, &local) == nullptr) {
        return 0;
    }
    return static_cast<std::int64_t>(local.tm_gmtoff) * 1000000;
#endif
}

} // extern "C"
