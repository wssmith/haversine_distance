#ifndef WS_JSON_PARSER_HPP
#define WS_JSON_PARSER_HPP

#include <vector>

namespace json
{
	struct json_element;
	struct token;

	namespace parser
	{
		json_element parse(const std::vector<token>& tokens);
	}
}

#endif
