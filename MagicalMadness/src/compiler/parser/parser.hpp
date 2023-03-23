#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>

#include "../lexer/lexer.hpp"

enum stmt_types_t
{
	STMT_PROGRAM,
	STMT_ASSIGNMENT,
	STMT_CALL,

	EXPR_BINARY,
	EXPR_UNARY,
	EXPR_NEGATE,
	EXPR_PRIMARY
};

enum primary_types_t
{
	PRIMARY_NUMBER,
	PRIMARY_STRING,
	PRIMARY_IDENTIFIER
};

// Not evaluated at compile time.
class stmt_t
{
public:
	stmt_t(stmt_types_t type) : type{ type } {};
	virtual ~stmt_t() = default;

	stmt_types_t type;
};

// Can be evaluated at compile time.
class expr_ast_t : public stmt_t
{
public:
	expr_ast_t(stmt_types_t type) : stmt_t{ type } {};
	virtual ~expr_ast_t() = default;
};

class primary_expr_t : public expr_ast_t
{
public:
	primary_expr_t(primary_types_t type) : type{ type }, expr_ast_t{ EXPR_PRIMARY } {}
	virtual ~primary_expr_t() = default;

	primary_types_t type;
};

class number_expr_t : public primary_expr_t
{
public:
	number_expr_t(float value) : value(value), primary_expr_t{ PRIMARY_NUMBER } {}

	float value;
};

class string_expr_t : public primary_expr_t
{
public:
	string_expr_t(std::string value) : value{ value }, primary_expr_t{ PRIMARY_STRING } {}

	std::string value;
};

class identifier_expr_t : public primary_expr_t
{
public:
	identifier_expr_t(std::string variable_name) : variable_name{ std::move(variable_name) }, primary_expr_t{PRIMARY_IDENTIFIER} {}

	std::string variable_name;
};

class binary_expr_t : public expr_ast_t
{
public:
	binary_expr_t() = default;
	binary_expr_t(std::unique_ptr<stmt_t> left, std::unique_ptr<stmt_t> right) : left{ std::move(left) }, right{ std::move(right) }, expr_ast_t{ EXPR_BINARY } {}
	binary_expr_t(char operand, std::unique_ptr<stmt_t> left, std::unique_ptr<stmt_t> right) : operand{ operand }, left{ std::move(left) }, right{ std::move(right) }, expr_ast_t{ EXPR_BINARY } {}
	
	char operand = ' ';
	std::unique_ptr<stmt_t> left;
	std::unique_ptr<stmt_t> right;
};

class unary_expr_t : public expr_ast_t
{
public:
	unary_expr_t() = default;
	unary_expr_t(std::unique_ptr<stmt_t> child) : child{ std::move(child) }, expr_ast_t{ EXPR_UNARY } {}

	std::unique_ptr<stmt_t> child;
};

class negate_expr_t : public expr_ast_t
{
public:
	negate_expr_t() = default;
	negate_expr_t(std::unique_ptr<stmt_t> child) : child{ std::move(child) }, expr_ast_t{ EXPR_NEGATE } {}

	std::unique_ptr<stmt_t> child;
};

class call_stmt_t : public stmt_t
{
public:
	call_stmt_t() = default;
	call_stmt_t(std::unique_ptr<stmt_t> function) : function{ std::move(function) }, stmt_t{ STMT_CALL } {};

	std::unique_ptr<stmt_t> function; // We don't check if this is a valid value to assign in the parser (if it's an lvalue or value). That is job for semantic analysis.
	std::vector<std::unique_ptr<stmt_t>> arguments; // We don't check if this is valid either.

	void add_argument(std::unique_ptr<stmt_t> argument)
	{
		arguments.push_back(std::move(argument));
	}
};

class assignment_stmt_t : public stmt_t
{
public:
	assignment_stmt_t() = default;
	assignment_stmt_t(std::unique_ptr<stmt_t> variable, std::unique_ptr<stmt_t> assignment) : variable{ std::move(variable) }, assignment{ std::move(assignment) }, stmt_t{ STMT_ASSIGNMENT } {};

	std::unique_ptr<stmt_t> variable; // We don't check if this is a valid value to assign in the parser (if it's an lvalue or value). That is job for semantic analysis.
	std::unique_ptr<stmt_t> assignment;
};

class program_t : public stmt_t
{
public:
	program_t() : stmt_t{ STMT_PROGRAM } {};

	std::vector<std::unique_ptr<stmt_t>> statements{};

	void add_statement(std::unique_ptr<stmt_t> statement)
	{
		if (statement)
		{
			statements.push_back(std::move(statement));
		}
		else
		{
			throw std::exception("Attempt to add an empty statement to main block.");
		}
	}
};

class parser_t
{
private:
	std::unique_ptr<lexer_t> lexer;

	// Expressions
	std::unique_ptr<stmt_t> parse_primary();
	std::unique_ptr<stmt_t> parse_parenthesis();
	std::unique_ptr<stmt_t> parse_unary_expr();
	std::unique_ptr<stmt_t> parse_multiplicative_expr();
	std::unique_ptr<stmt_t> parse_additive_expr();
	std::unique_ptr<stmt_t> parse_expr();

	// Statements
	std::unique_ptr<stmt_t> parse_call(); // Technically an expression since it returns, todo: fix
	std::unique_ptr<stmt_t> parse_assignment(); // Assignments CAN be expressions, todo: fix
	std::unique_ptr<stmt_t> parse_statement();
	std::unique_ptr<program_t> parse_program();
public:
	parser_t(const std::string& script)
	{
		lexer = std::make_unique<lexer_t>(script);
		lexer->tokenize();
	};
	parser_t(parser_t&) = delete;

	std::unique_ptr<program_t> parse();
};