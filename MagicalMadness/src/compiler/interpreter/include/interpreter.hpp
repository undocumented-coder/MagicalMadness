#pragma once
#include "../include/basetypes.hpp"
#include "../../parser/parser.hpp"

namespace interpreter
{
	std::shared_ptr<runtime_value_t> run_tree(std::unique_ptr<stmt_t> current, std::shared_ptr<environment_t> environment);
}