// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Process.h"
#include "Platform.h"

#include <string>
#include <vector>

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <string>
#else
#  include <cerrno>
#  include <spawn.h>
#  include <sys/wait.h>
#  include <unistd.h>
#  include <fcntl.h>
extern char** environ;
#endif

namespace inox::compiler::support {

#if defined(_WIN32)

namespace {

// Quote a single argument following the rules that CommandLineToArgvW (and the
// MSVC runtime argv parser) use. This is NOT shell quoting — it is the Windows
// command-line tokenization contract. Backslashes are only special when they
// precede a double quote.
std::wstring quoteWindowsArg(const std::wstring& arg)
{
    if (!arg.empty() &&
        arg.find_first_of(L" \t\n\v\"") == std::wstring::npos) {
        return arg; // no quoting needed
    }

    std::wstring result;
    result.push_back(L'"');
    for (auto it = arg.begin();; ++it) {
        std::size_t backslashes = 0;
        while (it != arg.end() && *it == L'\\') {
            ++it;
            ++backslashes;
        }

        if (it == arg.end()) {
            // Escape all trailing backslashes so they don't escape the closing
            // quote we are about to add.
            result.append(backslashes * 2, L'\\');
            break;
        }

        if (*it == L'"') {
            // Escape the backslashes AND the quote.
            result.append(backslashes * 2 + 1, L'\\');
            result.push_back(L'"');
        } else {
            // Backslashes are not special here; emit them as-is.
            result.append(backslashes, L'\\');
            result.push_back(*it);
        }
    }
    result.push_back(L'"');
    return result;
}

std::wstring widen(const std::string& narrow)
{
    if (narrow.empty()) {
        return std::wstring();
    }
    const int needed = MultiByteToWideChar(
        CP_UTF8, 0, narrow.data(), static_cast<int>(narrow.size()), nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(needed), L'\0');
    MultiByteToWideChar(
        CP_UTF8, 0, narrow.data(), static_cast<int>(narrow.size()),
        wide.data(), needed);
    return wide;
}

} // namespace

int runProcess(const std::vector<std::string>& args, bool captureToNull)
{
    if (args.empty()) {
        return -1;
    }

    // Build a properly quoted command line. CreateProcessW takes a single
    // command-line string; we assemble it with Windows argv quoting so that
    // arguments containing spaces (e.g. C:\Program Files\...) survive intact.
    std::wstring commandLine;
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (i != 0) {
            commandLine.push_back(L' ');
        }
        commandLine += quoteWindowsArg(widen(args[i]));
    }

    STARTUPINFOW startupInfo{};
    startupInfo.cb = sizeof(startupInfo);

    HANDLE nullHandle = INVALID_HANDLE_VALUE;
    if (captureToNull) {
        SECURITY_ATTRIBUTES sa{};
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;
        nullHandle = CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_WRITE,
                                 &sa, OPEN_EXISTING, 0, nullptr);
        if (nullHandle != INVALID_HANDLE_VALUE) {
            startupInfo.dwFlags |= STARTF_USESTDHANDLES;
            startupInfo.hStdOutput = nullHandle;
            startupInfo.hStdError = nullHandle;
            startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        }
    }

    // CreateProcessW may modify the command-line buffer in place, so give it a
    // writable copy.
    std::wstring mutableCommandLine = commandLine;
    PROCESS_INFORMATION processInfo{};

    const BOOL created = CreateProcessW(
        nullptr,
        mutableCommandLine.data(),
        nullptr,
        nullptr,
        captureToNull ? TRUE : FALSE,
        0,
        nullptr,
        nullptr,
        &startupInfo,
        &processInfo);

    if (nullHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(nullHandle);
    }

    if (!created) {
        return -1;
    }

    WaitForSingleObject(processInfo.hProcess, INFINITE);

    DWORD exitCode = 1;
    GetExitCodeProcess(processInfo.hProcess, &exitCode);

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    return static_cast<int>(exitCode);
}

#else // POSIX

int runProcess(const std::vector<std::string>& args, bool captureToNull)
{
    if (args.empty()) {
        return -1;
    }

    // Build a NULL-terminated argv array of mutable C strings. execvp/posix_spawn
    // take each entry literally; no shell parses the command, so spaces and
    // metacharacters in any argument are safe.
    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (const std::string& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_t* actionsPtr = nullptr;
    if (captureToNull) {
        if (posix_spawn_file_actions_init(&actions) == 0) {
            posix_spawn_file_actions_addopen(
                &actions, STDOUT_FILENO, nullDevicePath().data(),
                O_WRONLY, 0);
            posix_spawn_file_actions_addopen(
                &actions, STDERR_FILENO, nullDevicePath().data(),
                O_WRONLY, 0);
            actionsPtr = &actions;
        }
    }

    pid_t pid = 0;
    // posix_spawnp resolves args[0] through PATH, matching the previous behavior.
    const int spawnResult = posix_spawnp(
        &pid, argv[0], actionsPtr, nullptr, argv.data(), environ);

    if (actionsPtr != nullptr) {
        posix_spawn_file_actions_destroy(actionsPtr);
    }

    if (spawnResult != 0) {
        return -1;
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        return -1;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    // Killed by a signal (e.g. the program trapped): report a non-zero code so
    // callers see it as failure/abort.
    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return -1;
}

#endif

bool commandExists(std::string_view command)
{
    if (command.empty()) {
        return false;
    }
    // Probe via `command --version`, discarding all output. No shell involved.
    return runProcess({std::string(command), "--version"}, /*captureToNull=*/true) == 0;
}

} // namespace inox::compiler::support
