#ifndef WS_JSON_UTILITIES_HPP
#define WS_JSON_UTILITIES_HPP

#include <string>
#include <vector>

namespace json
{
    std::string format_error(const std::string& message, int line);
    std::string join(const std::vector<std::string>& parts, const std::string& delimiter);
}

#endif
