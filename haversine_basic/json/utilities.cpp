#include "utilities.hpp"

#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

namespace json
{
    std::string format_error(const std::string& message, int line)
    {
        return "[line " + std::to_string(line) + "] Error: " + message;
    }

    std::string join(const std::vector<std::string>& parts, const std::string& delimiter)
    {
        if (parts.empty())
            return std::string{};

        std::ostringstream builder;
        builder << parts[0];

        for (size_t i = 1; i < parts.size(); ++i)
        {
            builder << delimiter << parts[i];
        }

        return builder.str();
    }
}
