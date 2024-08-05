#ifndef WS_JSON_SCANNER_HPP
#define WS_JSON_SCANNER_HPP

#include <cstdint>
#include <istream>
#include <vector>

namespace json
{
    struct token;

    namespace scanner
    {
        std::vector<token> scan(std::istream& input_file, uintmax_t file_size);
    }
}

#endif
