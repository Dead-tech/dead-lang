#include "Statement.hpp"

#include <fmt/format.h>
#include <utility>

std::string EmptyStatement::evaluate() const noexcept { return ""; }

BlockStatement::BlockStatement(std::vector<std::shared_ptr<Statement>> block) noexcept
  : m_block{ std::move(block) } {}

std::string BlockStatement::evaluate() const noexcept {
    return std::accumulate(m_block.begin(), m_block.end(), std::string{}, [](const auto& acc, const auto& statement) {
        // check if statement is EmptyStatement
        if (const auto empty_statement = std::dynamic_pointer_cast<EmptyStatement>(statement); empty_statement) {
            return acc + statement->evaluate();
        } else {
            return acc + statement->evaluate() + "\n";
        }
    });
}

auto BlockStatement::empty() const noexcept { return m_block.empty(); }

ModuleStatement::ModuleStatement(
  std::string              name,
  std::vector<std::string> c_includes,
  BlockStatement           structs,
  BlockStatement           functions
) noexcept
  : m_name{ std::move(name) },
    m_c_includes{ std::move(c_includes) },
    m_structs{ std::move(structs) },
    m_functions{ std::move(functions) } {}

std::string ModuleStatement::evaluate() const noexcept {
    std::string c_module_code;

    for (const auto& c_include : m_c_includes) {
        c_module_code += fmt::format("#include <{}>\n", c_include.substr(1, c_include.size() - 2));
    }
    c_module_code += "\n";

    c_module_code += m_structs.evaluate();
    c_module_code += "\n";

    c_module_code += m_functions.evaluate();

    return c_module_code;
}

FunctionStatement::FunctionStatement(
  std::string    name,
  std::string    args,
  std::string    return_type,
  BlockStatement body
) noexcept
  : m_name{ std::move(name) },
    m_args{ std::move(args) },
    m_return_type{ std::move(return_type) },
    m_body{ std::move(body) } {}

std::string FunctionStatement::evaluate() const noexcept {
    std::string c_function_code;

    const std::string c_return_type = Typechecker::builtin_type_to_c_type(m_return_type);

    std::string c_args;
    const auto  arguments = dts::split_str(m_args, ',');
    for (const auto& argument : arguments) {
        const auto pieces = dts::split_str(argument, ' ');

        const std::size_t is_mutable = pieces.front() == "mut" ? 1 : 0;

        const std::string& variable_type   = pieces[0 + is_mutable];
        const std::string  type_extensions = std::accumulate(
          pieces.begin() + 1 + static_cast<long>(is_mutable),
          pieces.end() - 1,
          std::string{},
          [](const auto& acc, const auto& piece) { return acc + piece; }
        );
        const std::string& variable_name = pieces.back();

        c_args += static_cast<bool>(is_mutable) ? "" : "const ";
        c_args +=
          fmt::format("{}{} {}", Typechecker::builtin_type_to_c_type(variable_type), type_extensions, variable_name);

        if (&argument != &arguments.back()) { c_args += ", "; }
    }

    c_function_code += c_return_type + " " + m_name + "(" + c_args + ") {\n";
    c_function_code += m_body.evaluate();
    c_function_code += "}\n";

    return c_function_code;
}

IfStatement::IfStatement(std::string condition, BlockStatement then_block, BlockStatement else_block) noexcept
  : m_condition{ std::move(condition) },
    m_then_block{ std::move(then_block) },
    m_else_block{ std::move(else_block) } {}

std::string IfStatement::evaluate() const noexcept {
    std::string c_if_code;

    c_if_code += "if (" + m_condition + ") {\n";
    c_if_code += m_then_block.evaluate();

    if (!m_else_block.empty()) {
        c_if_code += "} else {\n";
        c_if_code += m_else_block.evaluate();
    }

    c_if_code += "}\n";

    return c_if_code;
}

ReturnStatement::ReturnStatement(std::string expression) noexcept
  : m_expression{ std::move(expression) } {}

std::string ReturnStatement::evaluate() const noexcept { return "return " + m_expression + ";"; }

VariableStatement::VariableStatement(
  const bool               is_mutable,
  Typechecker::BuiltinType type,
  std::string              type_extensions,
  std::string              name,
  std::string              expression
) noexcept
  : m_is_mutable{ is_mutable },
    m_type{ type },
    m_type_extensions{ std::move(type_extensions) },
    m_name{ std::move(name) },
    m_expression{ std::move(expression) } {}

std::string VariableStatement::evaluate() const noexcept {
    const std::string mutability = m_is_mutable ? "" : "const ";
    return mutability + Typechecker::builtin_type_to_c_type(m_type) + m_type_extensions + " " + m_name + " = "
           + m_expression + ";";
}

PlusEqualStatement::PlusEqualStatement(std::string name, std::string expression) noexcept
  : m_name{ std::move(name) },
    m_expression{ std::move(expression) } {}

std::string PlusEqualStatement::evaluate() const noexcept { return m_name + " += " + m_expression + ";"; }

WhileStatement::WhileStatement(std::string condition, BlockStatement body) noexcept
  : m_condition{ std::move(condition) },
    m_body{ std::move(body) } {}

std::string WhileStatement::evaluate() const noexcept {
    std::string c_while_code;

    c_while_code += "while (" + m_condition + ") {\n";
    c_while_code += m_body.evaluate();
    c_while_code += "}\n";

    return c_while_code;
}

ForStatement::ForStatement(
  std::shared_ptr<Statement> init_statement,
  std::string                condition,
  std::string                increment_statement,
  BlockStatement             body
) noexcept
  : m_init_statement{ std::move(init_statement) },
    m_condition{ std::move(condition) },
    m_increment_statement{ std::move(increment_statement) },
    m_body{ std::move(body) } {}

std::string ForStatement::evaluate() const noexcept {
    std::string c_for_code;

    c_for_code += "for (" + m_init_statement->evaluate() + " " + m_condition + m_increment_statement + ") {\n";
    c_for_code += m_body.evaluate();
    c_for_code += "}\n";

    return c_for_code;
}

ExpressionStatement::ExpressionStatement(std::string expression) noexcept
  : m_expression{ std::move(expression) } {}

std::string ExpressionStatement::evaluate() const noexcept { return m_expression + ";"; }


ArrayStatement::ArrayStatement(
  bool                     is_mutable,
  Typechecker::BuiltinType type,
  std::string              type_extensions,
  std::string              name,
  std::string              elements
) noexcept
  : m_is_mutable{ is_mutable },
    m_type{ type },
    m_type_extensions{ std::move(type_extensions) },
    m_name{ std::move(name) },
    m_elements{ std::move(elements) } {}

std::string ArrayStatement::evaluate() const noexcept {
    const std::string mutability = m_is_mutable ? "" : "const ";
    return fmt::format(
      "{} {} {}{} = {{ {} }};",
      mutability,
      Typechecker::builtin_type_to_c_type(m_type),
      m_name,
      m_type_extensions,
      m_elements
    );
}

IndexOperatorStatement::IndexOperatorStatement(
  std::string variable_name,
  std::string index,
  std::string expression
) noexcept
  : m_variable_name{ std::move(variable_name) },
    m_index{ std::move(index) },
    m_expression{ std::move(expression) } {}

std::string IndexOperatorStatement::evaluate() const noexcept {
    return fmt::format("{}[{}] = {};", m_variable_name, m_index, m_expression);
}

FunctionCallStatement::FunctionCallStatement(std::string name, std::string args) noexcept
  : m_name{ std::move(name) },
    m_args{ std::move(args) } {}

std::string FunctionCallStatement::evaluate() const noexcept { return fmt::format("{}({});", m_name, m_args); }


StructStatement::StructStatement(std::string name, std::vector<std::string> member_variables) noexcept
  : m_name{ std::move(name) },
    m_member_variables{ std::move(member_variables) } {}

std::string StructStatement::evaluate() const noexcept {
    std::string c_struct_code;

    c_struct_code += "typedef struct " + m_name + " {\n";
    for (const auto& member_variable : m_member_variables) {
        c_struct_code += fmt::format("    {};\n", member_variable);
    }
    c_struct_code += fmt::format("}} {};\n", m_name);

    return c_struct_code;
}
