# SPDX-License-Identifier: MPL-2.0
# Common C++ compiler options for the Inox compiler.

function(inox_apply_compiler_options target)
    target_compile_features(${target} PRIVATE cxx_std_20)
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
endfunction()
