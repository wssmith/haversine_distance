#ifndef WS_JSON_SCANNER_HPP
#define WS_JSON_SCANNER_HPP

#include <fstream>
#include <vector>

namespace json
{
    struct token;

    namespace scanner
    {
        std::vector<token> scan(std::ifstream& input_file);
    }
}

#endif
