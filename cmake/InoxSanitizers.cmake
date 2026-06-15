# SPDX-License-Identifier: MPL-2.0
# Optional sanitizer toggles. Initially intended for Clang/GCC debug builds on POSIX.

option(INOX_ENABLE_ASAN "Enable AddressSanitizer for supported compilers" OFF)
option(INOX_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer for supported compilers" OFF)

function(inox_apply_sanitizers target)
    set(_inox_sanitizers "")
    if(INOX_ENABLE_ASAN)
        list(APPEND _inox_sanitizers "address")
    endif()
    if(INOX_ENABLE_UBSAN)
        list(APPEND _inox_sanitizers "undefined")
    endif()
    if(_inox_sanitizers AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        list(JOIN _inox_sanitizers "," _inox_sanitizer_arg)
        target_compile_options(${target} PRIVATE -fsanitize=${_inox_sanitizer_arg})
        target_link_options(${target} PRIVATE -fsanitize=${_inox_sanitizer_arg})
    endif()
endfunction()
