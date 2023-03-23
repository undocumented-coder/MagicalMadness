#include "../include/basetypes.hpp"
#include "../include/interpreter.hpp"

using namespace interpreter;

std::shared_ptr<runtime_value_t> eval_primary(primary_types_t type, std::unique_ptr<stmt_t> current, std::shared_ptr<environment_t> environment)
{
	switch (type)
	{
		case PRIMARY_NUMBER:
		{
			std::unique_ptr<number_expr_t> number_expr = cast_stmt(number_expr_t, current);
			return std::make_shared<runtime_number_t>(number_expr->value);
		}
		case PRIMARY_STRING:
		{
			std::unique_ptr<string_expr_t> string_expr = cast_stmt(string_expr_t, current);
			return std::make_shared<runtime_string_t>(string_expr->value);
		}
		case PRIMARY_IDENTIFIER: // todo: do I grab value from env here? is identifier a primary?
		{
			std::unique_ptr<identifier_expr_t> identifier_expr = cast_stmt(identifier_expr_t, current);
			return environment->retrieve(identifier_expr->variable_name);
		}
		default:
		{
			std::printf("Primary type %d not implemented yet!\n", current->type);
			break;
		}
	}

	return std::make_shared<runtime_value_t>(RUNTIME_VOID);
}

std::shared_ptr<runtime_value_t> eval_unary(std::unique_ptr<unary_expr_t> current, std::shared_ptr<environment_t> environment)
{
	std::shared_ptr<runtime_value_t> child = run_tree(std::move(current->child), environment);

	if (child->type != RUNTIME_NUMBER)
		return std::make_shared<runtime_value_t>(RUNTIME_VOID);
	else
	{
		std::shared_ptr<runtime_number_t> runtime_number = cast_stmt(runtime_number_t, child);
		return std::make_shared<runtime_number_t>(!(runtime_number->value));
	}
}

std::shared_ptr<runtime_value_t> eval_negate(std::unique_ptr<negate_expr_t> current, std::shared_ptr<environment_t> environment)
{
	std::shared_ptr<runtime_value_t> child = run_tree(std::move(current->child), environment);

	if (child->type != RUNTIME_NUMBER)
		return std::make_shared<runtime_value_t>(RUNTIME_VOID);
	else
	{
		std::shared_ptr<runtime_number_t> runtime_number = cast_stmt(runtime_number_t, child);
		return std::make_shared<runtime_number_t>(-(runtime_number->value));
	}
}

std::shared_ptr<runtime_value_t> eval_binop(std::unique_ptr<binary_expr_t> current, std::shared_ptr<environment_t> environment)
{
	std::shared_ptr<runtime_value_t> left = run_tree(std::move(current->left), environment);
	std::shared_ptr<runtime_value_t> right = run_tree(std::move(current->right), environment);

	if (left->type != RUNTIME_NUMBER || right->type != RUNTIME_NUMBER)
	{
		std::string err = "attempt to ";
		err += current->operand + " " + type_strings[left->type] + " and " + type_strings[right->type];

		throw std::exception(err.c_str());
	}

	std::shared_ptr<runtime_number_t> n_left = cast_stmt(runtime_number_t, left);
	std::shared_ptr<runtime_number_t> n_right = cast_stmt(runtime_number_t, right);

	switch (current->operand)
	{
		case '+':
			return std::make_shared<runtime_number_t>(n_left->value + n_right->value);
		case '-':
			return std::make_shared<runtime_number_t>(n_left->value - n_right->value);
		case '*':
			return std::make_shared<runtime_number_t>(n_left->value * n_right->value);
		case '/':
			return std::make_shared<runtime_number_t>(n_left->value / n_right->value);
		case '%':
			return std::make_shared<runtime_number_t>(static_cast<float>(static_cast<int>(n_left->value) % static_cast<int>(n_right->value)));
		case '^':
			return std::make_shared<runtime_number_t>(powf(n_left->value, n_right->value));
		default:
		{
			std::printf("Unknown binary operation: \"%c\"\n", current->operand || ' ');
		}
	}

	return std::make_shared<runtime_value_t>(RUNTIME_VOID);
}

std::shared_ptr<runtime_value_t> eval_assignment(std::unique_ptr<assignment_stmt_t> current, std::shared_ptr<environment_t> environment)
{
	if (current->variable->type != EXPR_PRIMARY)
	{
		throw std::exception("Attempt to assign a value to something that isn't a primary!");
	}

	std::unique_ptr<primary_expr_t> primary_value = cast_stmt(primary_expr_t, current->variable);

	std::shared_ptr<runtime_value_t> new_value = run_tree(std::move(current->assignment), environment);

	if (primary_value->type != PRIMARY_IDENTIFIER)
		throw std::exception("Attempt to assign a value to something that isn't a variable!");
	
	std::unique_ptr<identifier_expr_t> identifier_expr = cast_stmt(identifier_expr_t, current->variable);

	environment->assign(identifier_expr->variable_name, new_value);

	return new_value;
}

std::shared_ptr<runtime_value_t> eval_call(std::unique_ptr<call_stmt_t> call_info, std::shared_ptr<environment_t> environment)
{
	std::shared_ptr<runtime_value_t> value = run_tree(std::move(call_info->function), environment);

	if (value->type != RUNTIME_FUNCTION)
		throw std::exception("Attempt to call a value that isn't a function!");

	std::shared_ptr<runtime_function_t> function = cast_stmt(runtime_function_t, value);

	if (function->is_native)
	{
		function->function();
	}
	else
		throw std::exception("Non-native functions not implemented!");

	return std::make_shared<runtime_value_t>(RUNTIME_VOID);
}

void eval_program(std::unique_ptr<program_t> program, std::shared_ptr<environment_t> environment)
{
	std::int32_t current_statement = 0;

	for (std::unique_ptr<stmt_t>& stmt : program->statements)
	{
		std::shared_ptr<runtime_value_t> result = run_tree(std::move(stmt), environment);
		// std::printf("Result [%d]:\n", ++current_statement);
		result->dump();
	}
}

std::shared_ptr<runtime_value_t> interpreter::run_tree(std::unique_ptr<stmt_t> current, std::shared_ptr<environment_t> environment)
{
	switch (current->type)
	{
		case STMT_PROGRAM:
		{
			std::unique_ptr<program_t> program = cast_stmt(program_t, current);
			eval_program(std::move(program), environment);
			break;
		}
		case STMT_ASSIGNMENT:
		{
			std::unique_ptr<assignment_stmt_t> assignment = cast_stmt(assignment_stmt_t, current);
			return eval_assignment(std::move(assignment), environment);
		}
		case STMT_CALL:
		{
			std::unique_ptr<call_stmt_t> call = cast_stmt(call_stmt_t, current);
			eval_call(std::move(call), environment);
			break;
		}
		case EXPR_BINARY:
		{
			std::unique_ptr<binary_expr_t> binary_expr = cast_stmt(binary_expr_t, current);
			return eval_binop(std::move(binary_expr), environment);
		}
		case EXPR_UNARY:
		{
			std::unique_ptr<unary_expr_t> unary_expr = cast_stmt(unary_expr_t, current);
			return eval_unary(std::move(unary_expr), environment);
		}
		case EXPR_NEGATE:
		{
			std::unique_ptr<negate_expr_t> negate_expr = cast_stmt(negate_expr_t, current);
			return eval_negate(std::move(negate_expr), environment);
		}
		case EXPR_PRIMARY:
		{
			std::unique_ptr<primary_expr_t> primary_expr = cast_stmt(primary_expr_t, current); // data is lost upon this conversion, just cast for the type.
			return eval_primary(primary_expr->type, std::move(current), environment);
		}
		default:
		{
			std::printf("[Runtime] Unexpected statement: %d\n", current->type);
			break;
		}
	}

	return std::make_shared<runtime_value_t>(RUNTIME_VOID);
}