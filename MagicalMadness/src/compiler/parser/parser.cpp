#include "parser.hpp"

std::unique_ptr<stmt_t> parser_t::parse_primary()
{
	token_t current = lexer->consume();

	switch (current.type)
	{
		case TOK_NUMBER:
		{
			float num = strtof(current.str.c_str(), nullptr);
			std::unique_ptr<number_expr_t> number = std::make_unique<number_expr_t>(num);
			return number;
		}
		case TOK_STRING:
		{
			std::unique_ptr<string_expr_t> string = std::make_unique<string_expr_t>(current.str);
			return string;
		}
		case TOK_IDENTIFIER:
		{
			std::unique_ptr<identifier_expr_t> identifier = std::make_unique<identifier_expr_t>(current.str);
			return identifier;
		}
		default:
		{
			throw std::exception(("Expected identifier when parsing expression, got: \"" + current.str + "\"").c_str());
			break;
		}
	}

	return {};
}

std::unique_ptr<stmt_t> parser_t::parse_parenthesis()
{
	std::unique_ptr<stmt_t> root;

	if (lexer->current().type == TOK_LPAREN)
	{
		lexer->consume();

		root = parse_expr();

		if (lexer->current().type != TOK_RPAREN)
		{
			throw std::exception("Missing closing parenthesis!");
		}

		lexer->consume();
	}
	else
		root = parse_primary();

	return root;
}

std::unique_ptr<stmt_t> parser_t::parse_call()
{
	std::unique_ptr<stmt_t> root = parse_parenthesis();

	token_def_t current = lexer->current().type;
	if (current == TOK_LPAREN)
	{
		lexer->consume();
		std::unique_ptr<call_stmt_t> call_statement = std::make_unique<call_stmt_t>(std::move(root));

		if (lexer->current().type != TOK_RPAREN) // if not zero args
		{
			do // parse current expression, eat presumed comma, else if its a paren it's checked later
			{
				call_statement->add_argument(parse_expr());
				current = lexer->consume().type;
			} while (current == TOK_COMMA);
		}
		else
			current = lexer->consume().type; // eat closing parenthesis

		if (current != TOK_RPAREN) // check if parenthesis is the end of call
		{
			throw std::exception("[CALL] Missing closing parenthesis!");
		}

		return call_statement;
	}

	return root;
}

std::unique_ptr<stmt_t> parser_t::parse_unary_expr()
{
	std::unique_ptr<stmt_t> root;

	char current = lexer->current().str[0];
	if (current == '!')
	{
		lexer->consume();
		root = std::make_unique<unary_expr_t>(parse_call());
	}
	else if (current == '-') // THIS IS WRONG FIX THIS TO WORK WITH ACTUAL TOKENS. (such as --)
	{
		lexer->consume();
		root = std::make_unique<negate_expr_t>(parse_call());
	}
	else
	{
		root = parse_call();
	}


	return root;
}

std::unique_ptr<stmt_t> parser_t::parse_multiplicative_expr()
{
	std::unique_ptr<stmt_t> root = parse_unary_expr();

	char current = lexer->current().str[0];
	while (current == '*' || current == '/' || current == '%' || current == '^')
	{
		lexer->consume();
		root = std::make_unique<binary_expr_t>(current, std::move(root), parse_unary_expr());
		current = lexer->current().str[0]; // increment lexer
	}

	return root;
}

std::unique_ptr<stmt_t> parser_t::parse_additive_expr()
{
	std::unique_ptr<stmt_t> root = parse_multiplicative_expr();

	char current = lexer->current().str[0];
	while (current == '+' || current == '-')
	{
		lexer->consume();
		root = std::make_unique<binary_expr_t>(current, std::move(root), parse_multiplicative_expr()); // reparent
		current = lexer->current().str[0]; // increment lexer
	}

	return root;
}

std::unique_ptr<stmt_t> parser_t::parse_expr()
{
	return parse_additive_expr();
}

std::unique_ptr<stmt_t> parser_t::parse_assignment()
{
	std::unique_ptr<stmt_t> root = parse_expr();

	char current = lexer->current().str[0];
	if (current == '=') // No lexer incrementing (looping) because this output result should be a STATEMENT.
	{
		lexer->consume();
		root = std::make_unique<assignment_stmt_t>(std::move(root), parse_expr());
	}

	return root;
}

std::unique_ptr<stmt_t> parser_t::parse_statement()
{
	return parse_assignment();
}

std::unique_ptr<program_t> parser_t::parse_program()
{
	std::unique_ptr<program_t> program = std::make_unique<program_t>();

	while (!lexer->is_done())
	{
		program->add_statement(parse_statement());
		if (lexer->consume().type != TOK_ENDLINE)
		{
			throw std::exception("Missing semi-colon!");
		}
	}

	return program;
}

std::unique_ptr<program_t> parser_t::parse()
{
	return parse_program();
}



// My parser is a tail recursive parser.
// This is based on the LL parser design, recursive descent parser
// My parser eliminates the problem of recursive descent by re-parenting.
// Typical recursive descent parsers make parsing left recursive grammars impossible (because of an infinite loop problem). 
// Tail recursive parsers use a node reparenting technique that makes this allowable.