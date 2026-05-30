#pragma once

#include "../ast/Ast.h"

#include <stdexcept>
#include <string>

namespace inox::compiler::codegen {

class CodegenError final : public std::runtime_error {
public:
    explicit CodegenError(std::string message);
};

class LlvmIrEmitter {
public:
    std::string emit(const ast::ModuleNode& module) const;
};

} // namespace inox::compiler::codegen
