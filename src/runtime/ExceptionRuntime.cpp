// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// Minimal bootstrap exception runtime for Inox.
// The compiler-facing ABI deliberately hides the C++ exception ABI so that
// the transport can later be replaced without changing Inox source semantics.

#include <cstdint>
#include <exception>
#include <memory>
#include <new>
#include <type_traits>

#if !defined(_WIN32)
#include <cxxabi.h>
#endif

namespace {

struct InoxExceptionCarrier final {
    std::uint64_t typeId;
};

static_assert(std::is_trivially_copyable_v<InoxExceptionCarrier>);
static_assert(std::is_nothrow_copy_constructible_v<InoxExceptionCarrier>);

struct InoxExceptionState final {
    std::exception_ptr exception;
    std::uint64_t typeId = 0;
};

std::uint64_t classify(const std::exception_ptr& exception) noexcept
{
    if (!exception) {
        return 0;
    }

    try {
        std::rethrow_exception(exception);
    } catch (const InoxExceptionCarrier& carrier) {
        return carrier.typeId;
    } catch (...) {
        // Zero is reserved for a foreign/non-Inox exception. It can be caught
        // by a catch-all handler and otherwise propagates unchanged.
        return 0;
    }
}

} // namespace

extern "C" [[noreturn]] void __inox_raise(std::uint64_t typeId)
{
    throw InoxExceptionCarrier{typeId};
}

extern "C" InoxExceptionState* __inox_exception_capture(void* rawException) noexcept
{
#if !defined(_WIN32)
    // A landingpad gives us the platform exception object. Establish a C++
    // catch only long enough to obtain a stable exception_ptr, then end it.
    __cxxabiv1::__cxa_begin_catch(rawException);
    std::exception_ptr exception = std::current_exception();
    __cxxabiv1::__cxa_end_catch();
#else
    // The Windows lowering is intentionally kept behind this ABI. When the
    // frontend emits a catchpad, std::current_exception() is the carrier.
    (void)rawException;
    std::exception_ptr exception = std::current_exception();
#endif

    auto* state = new (std::nothrow) InoxExceptionState{exception, classify(exception)};
    if (state == nullptr) {
        std::terminate();
    }
    return state;
}

extern "C" std::uint64_t __inox_exception_type(const InoxExceptionState* state) noexcept
{
    return state != nullptr ? state->typeId : 0;
}

extern "C" void __inox_exception_release(InoxExceptionState* state) noexcept
{
    delete state;
}

extern "C" [[noreturn]] void __inox_exception_rethrow(InoxExceptionState* state)
{
    if (state == nullptr || !state->exception) {
        std::terminate();
    }

    std::exception_ptr exception = state->exception;
    delete state;
    std::rethrow_exception(exception);
}
