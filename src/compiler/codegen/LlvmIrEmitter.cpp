#include "LlvmIrEmitter.h"

#include <cctype>
#include <string_view>
#include <utility>

namespace inox::compiler::codegen {

namespace {

bool equalsIgnoreCase(std::string_view left, std::string_view right)
{
    if (left.size() != right.size()) {
        return false;
    }

    for (std::size_t index = 0; index < left.size(); ++index) {
        if (std::tolower(static_cast<unsigned char>(left[index])) !=
            std::tolower(static_cast<unsigned char>(right[index]))) {
            return false;
        }
    }

    return true;
}

} // namespace

CodegenError::CodegenError(std::string message)
    : std::runtime_error(std::move(message))
{
}

std::string LlvmIrEmitter::emit(const ast::ModuleNode& module) const
{
    const ast::FunctionDeclaration* mainFunction = nullptr;

    for (const auto& item : module.items()) {
        if (item->kind() != ast::AstNodeKind::FunctionDeclaration) {
            continue;
        }

        const auto& function = static_cast<const ast::FunctionDeclaration&>(*item);
        if (!equalsIgnoreCase(function.name(), "Main")) {
            throw CodegenError(
                "LLVM emission currently supports only an empty Main()");
        }
        mainFunction = &function;
    }

    if (mainFunction == nullptr) {
        throw CodegenError("LLVM emission requires Main()");
    }
    if (!mainFunction->body().empty()) {
        throw CodegenError(
            "LLVM emission currently supports only an empty Main()");
    }

    return
        "define i32 @main() {\n"
        "entry:\n"
        "  ret i32 0\n"
        "}\n";
}

} // namespace inox::compiler::codegen
