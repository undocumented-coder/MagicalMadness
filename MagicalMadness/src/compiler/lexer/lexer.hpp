#pragma once
#include <vector>
#include <string>
#include <variant>


enum token_def_t : std::int8_t
{
	TOK_NONE = -1,	// panic token; not real.
	TOK_ENDLINE,	// represented by the ;
	TOK_IDENTIFIER, // variable names, function names, etc
	TOK_TYPE,		// types: int, float, string, double, bool, void, etc
	TOK_EOF,		// end of file
	TOK_LPAREN,		// '('
	TOK_RPAREN,		// ')'
	TOK_CTXBEGIN,	// '{'
	TOK_CTXEND,		// '}'
	TOK_COMMA,		// ','
	TOK_BINOP,		// infix operations
	TOK_PREFIX,		// prefix operations
	TOK_POSTFIX,	// postfix operations
	TOK_INCREMENT,	// increment (++)
	TOK_DECREMENT,  // decrement (--)
	TOK_NUMBER,
	TOK_STRING
};

class token_t
{
public:
	std::string str{};
	token_def_t type = TOK_NONE;

	token_t(token_def_t type) : type{ type } {}
	token_t(token_def_t type, const std::string& str) : type{ type }, str{ str } {}
	token_t(token_def_t type, char character) : type{ type }
	{
		str.push_back(character);
	}

	void dump()
	{
		std::printf("%s | %d\n", str.c_str(), type);
	}
};


class lexer_t
{
private:
	std::string script{};
public:
	lexer_t(const std::string& script) : script{ script } {};
	lexer_t(const lexer_t&) = delete; // don't want copies.

	std::vector<token_t> tokens{};
	std::vector<token_t>& tokenize();

	std::size_t current_token_index = 0;
	token_t consume();
	token_t current();
	bool is_done();
};