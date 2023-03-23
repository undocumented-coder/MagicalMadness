#include "lexer.hpp"

std::vector<token_t>& lexer_t::tokenize()
{
	std::size_t line_numbers = 0;

	for (std::size_t index = 0; index < script.size(); index++)
	{
		char current = script[index];

		if (isspace(current))
			continue;

		if (current == '"')
		{
			std::string temp{};
			while (true)
			{
				if (++index < script.size())
				{
					current = script[index];

					if (current == '"')
						break;

					temp.push_back(current);
				}
				else
					throw std::exception("Failed to terminate string literal.");
			}

			tokens.emplace_back(TOK_STRING, temp);
			continue;
		}

		if (isdigit(current))
		{
			std::string temp{};
			if (current == '0' && tolower(script[index + 1]) == 'x') // hexadecimal notation?
			{
				temp.push_back(current);
				temp.push_back(script[++index]);

				++index;

				// It's a hex string so it must be tokenized differently than decimal]
				while (isdigit(current = script[index++]) || tolower(current) == 'a' || tolower(current) == 'b' || tolower(current) == 'c' || tolower(current) == 'd' || tolower(current) == 'e' || tolower(current) == 'f')
					temp.push_back(current);

				index -= 2;
			}
			else 
			{ // Just a decimal string
				while (isdigit(current = script[index++]) || current == '.')
					temp.push_back(current);

				index -= 2;
			}

			tokens.emplace_back(TOK_NUMBER, temp);
			continue;
		}

		if (isalnum(current))
		{
			std::string temp{};
			while (isalnum(current = script[index++]))
				temp.push_back(current);

			index -= 2;

			if (temp == "void" || temp == "bool" || temp == "string" || temp == "int" || temp == "float" || temp == "double")
			{
				tokens.emplace_back(TOK_TYPE, temp);
				continue;
			}

			tokens.emplace_back(TOK_IDENTIFIER, temp);
			continue;
		}

		if (current == '+' && script[index + 1] == '+')
		{
			++index;
			tokens.emplace_back(TOK_INCREMENT, "++");
			continue;
		}
		else if (current == '-' && script[index + 1] == '-')
		{
			++index;
			tokens.emplace_back(TOK_DECREMENT, "--");
			continue;
		}

		switch (current)
		{
			case '+':
			case '-':
			case '*':
			case '/':
			case '^':
			case '%':
			case '=':
				tokens.emplace_back(TOK_BINOP, current);
				break;
			case '!':
				tokens.emplace_back(TOK_PREFIX, current);
				break;
			case '(':
				tokens.emplace_back(TOK_LPAREN, current);
				break;
			case ')':
				tokens.emplace_back(TOK_RPAREN, current);
				break;
			case '{':
				tokens.emplace_back(TOK_CTXBEGIN, current);
				break;
			case '}':
				tokens.emplace_back(TOK_CTXEND, current);
				break;
			case ',':
				tokens.emplace_back(TOK_COMMA, current);
				break;
			case ';':
			{
				std::string temp = std::to_string(++line_numbers);
				tokens.emplace_back(TOK_ENDLINE, temp);
				break;
			}
			default:
				// tokens.emplace_back(TOK_NONE, current);
				break;
		}

	}

	script.clear();	// No point in holding script anymore

	tokens.emplace_back(TOK_EOF);

	return tokens;
}

// functions used for reading output

// Consumes the current token and returns it
token_t lexer_t::consume()
{
	token_t previous = current();
	current_token_index = current_token_index + 1 >= tokens.size() ? current_token_index : current_token_index + 1;

	return previous;
}

// Retrieves the current token
token_t lexer_t::current()
{
	return tokens[current_token_index];
}

// Checks if lexer is finished
bool lexer_t::is_done()
{
	return current().type == TOK_EOF;
}
