#pragma once
#include <string>
#include <unordered_map>
#include <memory>

#include "../../parser/parser.hpp"


#define cast_stmt(_ty, val) std::make_unique<_ty>(std::move(dynamic_cast<_ty&>(*val)))


enum runtime_type
{
	RUNTIME_VOID,
	RUNTIME_NUMBER,
	RUNTIME_STRING,
	RUNTIME_IDENTIFIER,
	RUNTIME_FUNCTION
};

const std::string type_strings[] = {
	"void",
	"number",
	"string",
	"identifier",
	"function"
};

using native_function_t = std::int32_t(*)();

class runtime_value_t
{
public:
	runtime_value_t(runtime_type type) : type{ type } {};
	virtual ~runtime_value_t() = default;

	runtime_type type = RUNTIME_VOID;

	virtual void dump()
	{
		std::printf("%s | NULL\n", type_strings[type].c_str());
	}
};

class runtime_number_t : public runtime_value_t
{
public:
	runtime_number_t(float value) : value{ value }, runtime_value_t{ RUNTIME_NUMBER } {};

	float value = 0;

	void dump() override
	{
		std::printf("%s | %.02f\n", type_strings[type].c_str(), value);
	}
};

class runtime_string_t : public runtime_value_t
{
public:
	runtime_string_t(const std::string& value) : value{ value }, runtime_value_t{RUNTIME_STRING} {};

	std::string value;

	void dump() override
	{
		std::printf("%s | \"%s\"\n", type_strings[type].c_str(), value.c_str());
	}
};

class runtime_identifier_t : public runtime_value_t
{
public:
	runtime_identifier_t(const std::string& name) : name{ name }, runtime_value_t{RUNTIME_IDENTIFIER} {};

	std::string name;

	void dump() override
	{
		std::printf("%s | \"%s\"", type_strings[type].c_str(), name.c_str());
	}
};

class runtime_function_t : public runtime_value_t
{
public:
	runtime_function_t(const std::string& debug_name, native_function_t function ) : is_native{ true }, function{ function }, debug_name { debug_name }, runtime_value_t{ RUNTIME_FUNCTION } {};
	runtime_function_t(native_function_t function) : is_native{ true }, function{ function }, runtime_value_t{ RUNTIME_FUNCTION } {};
	runtime_function_t(const std::string& debug_name, std::unique_ptr<stmt_t> body) : is_native{ false }, body{ std::move(body) }, debug_name{ debug_name }, runtime_value_t{ RUNTIME_FUNCTION } {};
	runtime_function_t(std::unique_ptr<stmt_t> body) : is_native{ false }, body{ std::move(body) }, runtime_value_t{ RUNTIME_FUNCTION } {};

	std::string debug_name{ "anonymous function" };
	bool is_native = false;

	std::unique_ptr<stmt_t> body{};							// Native function
	native_function_t function = nullptr;					// C++ function

	void dump() override
	{
		std::printf("%s | %s | native?=%s\n", type_strings[type].c_str(), debug_name.c_str(), is_native ? "true" : "false");
	}
};


// One scope, it falls back on previous scopes
class environment_t
{
private:
	// Helps to follow assign rules. "If it's in the current scope then override, else check parents, if nothing in parents then override this container."
	bool assign_internal(const std::string& var_name, std::shared_ptr<runtime_value_t> value)
	{
		auto found = variables.find(var_name);
		if (found != variables.end())
		{
			found->second = value;
			return true;
		}
		else if (parent)
			return parent->assign_internal(var_name, value);

		return false;
	}
public:
	environment_t() = default;
	environment_t(std::unique_ptr<environment_t> parent) : parent{ std::move(parent)} {};

	void assign(const std::string& var_name, std::shared_ptr<runtime_value_t> value)
	{
		auto found = variables.find(var_name);
		if (found != variables.end())
			found->second = value;
		else if (!assign_internal(var_name, value))
			variables[var_name] = value;
	}
	std::shared_ptr<runtime_value_t> retrieve(const std::string& var_name)
	{
		auto found = variables.find(var_name);
		if (found != variables.end())
			return found->second;
		else if (parent)
			return parent->retrieve(var_name);

		return std::make_unique<runtime_value_t>(RUNTIME_VOID);
	}

	std::unordered_map<std::string, std::shared_ptr<runtime_value_t>> variables{};
	std::unique_ptr<environment_t> parent;
};


// This holds the context for all execution, current call stack, etc. Anything execution this will be used in
class execution_state_t
{
public:
	std::vector<std::shared_ptr<runtime_value_t>> stack{};
	std::shared_ptr<runtime_function_t> current_function;
	std::shared_ptr<environment_t> global_env;

	void push_stack(std::shared_ptr<runtime_value_t> value) // do I separate these or no?
	{
		stack.push_back(value);
	}

	std::shared_ptr<runtime_value_t> pop_stack()
	{
		stack.pop_back();
	}

	std::size_t stack_size()
	{
		return stack.size();
	}
};