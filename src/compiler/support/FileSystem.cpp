// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "FileSystem.h"

#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <cstdint>
#elif defined(__FreeBSD__)
#include <sys/sysctl.h>
#include <sys/types.h>
#endif

namespace inox::compiler::support {
namespace {

namespace fs = std::filesystem;

// The path of the running executable as the operating system knows it;
// empty when it cannot tell. argv[0] is only what the user typed: `inox`
// found through PATH would otherwise be taken for ./inox.
fs::path runningExecutable()
{
#if defined(_WIN32)
    std::vector<wchar_t> buffer(MAX_PATH);
    for (;;) {
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            return {};
        }
        if (length < buffer.size()) {
            return fs::path(std::wstring(buffer.data(), length));
        }
        buffer.resize(buffer.size() * 2);
    }
#elif defined(__APPLE__)
    std::uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size + 1, '\0');
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
        return {};
    }
    std::error_code error;
    const fs::path resolved = fs::weakly_canonical(fs::path(buffer.data()), error);
    return error ? fs::path(buffer.data()) : resolved;
#elif defined(__FreeBSD__)
    int request[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
    std::vector<char> buffer(4096, '\0');
    std::size_t size = buffer.size();
    if (sysctl(request, 4, buffer.data(), &size, nullptr, 0) != 0) {
        return {};
    }
    return fs::path(buffer.data());
#else
    std::error_code error;
    const fs::path link = fs::read_symlink("/proc/self/exe", error);
    return error ? fs::path{} : link;
#endif
}

// `inox` typed without a directory: the first PATH entry that holds it.
fs::path searchPath(const fs::path& name)
{
    const char* path = std::getenv("PATH");
    if (path == nullptr) {
        return {};
    }
#if defined(_WIN32)
    constexpr char separator = ';';
    const std::vector<std::string> suffixes{"", ".exe"};
#else
    constexpr char separator = ':';
    const std::vector<std::string> suffixes{""};
#endif
    const std::string_view entries(path);
    std::size_t start = 0;
    while (start <= entries.size()) {
        std::size_t end = entries.find(separator, start);
        if (end == std::string_view::npos) {
            end = entries.size();
        }
        const std::string_view entry = entries.substr(start, end - start);
        if (!entry.empty()) {
            for (const std::string& suffix : suffixes) {
                const fs::path candidate = fs::path(std::string(entry)) / (name.string() + suffix);
                std::error_code error;
                if (fs::is_regular_file(candidate, error)) {
                    return fs::absolute(candidate, error);
                }
            }
        }
        start = end + 1;
    }
    return {};
}

} // namespace

std::filesystem::path executableDirectory(const char* argv0)
{
    if (const fs::path running = runningExecutable(); !running.empty()) {
        return running.parent_path();
    }
    if (argv0 == nullptr || std::string_view(argv0).empty()) {
        return {};
    }

    fs::path executablePath(argv0);
    std::error_code error;
    if (!executablePath.has_parent_path()) {
        if (const fs::path found = searchPath(executablePath); !found.empty()) {
            return found.parent_path();
        }
    }
    if (executablePath.is_relative()) {
        executablePath = fs::absolute(executablePath, error);
        if (error) {
            return {};
        }
    }

    return executablePath.parent_path();
}

} // namespace inox::compiler::support
