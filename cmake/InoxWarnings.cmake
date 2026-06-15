# SPDX-License-Identifier: MPL-2.0
# Central warning policy for the Inox compiler.

function(inox_apply_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()
