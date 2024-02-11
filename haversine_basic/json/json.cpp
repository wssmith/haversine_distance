#include "json.hpp"

#include <exception>
#include <fstream>
#include <string>
#include <vector>

#include "scanner.hpp"
#include "token.hpp"
#include "parser.hpp"

namespace json
{
    json_document deserialize_json(const std::string& filepath)
    {
        std::ifstream json_file{ filepath };
        if (!json_file)
            throw std::exception{ "Cannot open input JSON file." };

        const std::vector<token> tokens = scanner::scan(json_file);
        json_file.close();

        return parser::parse(tokens);
    }
}
