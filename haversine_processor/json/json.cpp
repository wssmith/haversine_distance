#include "json.hpp"

#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "scanner.hpp"
#include "token.hpp"
#include "parser.hpp"

namespace json
{
    json_document deserialize_json(const std::string& filepath, std::chrono::milliseconds& scan_time, std::chrono::milliseconds& parse_time)
    {
        if (!std::filesystem::exists(filepath))
            throw std::exception{ "JSON file does not exist." };

        std::ifstream json_file{ filepath };
        if (!json_file)
            throw std::exception{ "Cannot open JSON file." };

        const auto start_scan{ std::chrono::steady_clock::now() };
        const std::vector<token> tokens = scanner::scan(json_file);
        const auto end_scan{ std::chrono::steady_clock::now() };
        json_file.close();

        const auto start_parse{ std::chrono::steady_clock::now() };
        auto document = parser::parse(tokens);
        const auto end_parse{ std::chrono::steady_clock::now() };

        scan_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_scan - start_scan);
        parse_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_parse - start_parse);

        return document;
    }
}
