// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.

#include <windows.h>

#include <cstdio>
#include <cstring>
#include <stdexcept>

extern "C" void inox_poc_foreign_else();
extern "C" void inox_poc_foreign_rethrow();
extern "C" void inox_poc_seh();
extern "C" void inox_poc_inox_typed();

extern "C" void inox_poc_report_type(unsigned long long type) noexcept
{
    std::printf("inox-type-%llu\n", type);
}

extern "C" [[noreturn]] void inox_poc_throw_foreign()
{
    throw std::runtime_error("foreign-marker");
}

extern "C" __declspec(noinline) void inox_poc_raise_seh()
{
    RaiseException(0xE0424242u, 0, 0, nullptr);
}

extern "C" void inox_poc_foreign_else_marker() noexcept
{
    std::puts("foreign-else-caught");
}

extern "C" void inox_poc_seh_bridge_marker() noexcept
{
    std::puts("seh-bridge-caught");
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fputs("usage: windows-eh-poc <foreign-else|foreign-rethrow|seh>\n", stderr);
        return 64;
    }

    if (std::strcmp(argv[1], "foreign-else") == 0) {
        inox_poc_foreign_else();
        return 0;
    }

    if (std::strcmp(argv[1], "foreign-rethrow") == 0) {
        try {
            inox_poc_foreign_rethrow();
        } catch (const std::runtime_error& error) {
            if (std::strcmp(error.what(), "foreign-marker") == 0) {
                std::puts("foreign-preserved");
                return 0;
            }
            std::fputs("foreign exception payload changed\n", stderr);
            return 65;
        } catch (...) {
            std::fputs("foreign exception type changed\n", stderr);
            return 66;
        }

        std::fputs("foreign exception was swallowed\n", stderr);
        return 67;
    }

    if (std::strcmp(argv[1], "inox-typed") == 0) {
        inox_poc_inox_typed();
        return 0;
    }

    if (std::strcmp(argv[1], "seh") == 0) {
        inox_poc_seh();
        std::puts("seh-returned");
        return 68;
    }

    std::fputs("unknown mode\n", stderr);
    return 69;
}
