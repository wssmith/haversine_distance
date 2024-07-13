#include "json.hpp"

#include <exception>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "scanner.hpp"
#include "token.hpp"
#include "parser.hpp"

#include "../profiler.hpp"

namespace json
{
    json_document deserialize_json(const std::string& filepath)
    {
        profiler deserialize_activity{ "deserialize" };

        if (!std::filesystem::exists(filepath))
            throw std::exception{ "JSON file does not exist." };

        std::ifstream json_file{ filepath };
        if (!json_file)
            throw std::exception{ "Cannot open JSON file." };

        const std::vector<token> tokens = scanner::scan(json_file);
        json_file.close();

        return parser::parse(tokens);
    }
}
