#ifndef WS_JSON_TOKEN_HPP
#define WS_JSON_TOKEN_HPP

#include <cstdint>
#include <ostream>
#include <string>
#include <variant>

#include "literals.hpp"

namespace json
{
    enum class token_type : uint8_t
    {
        unknown = 0,

        // single-character tokens
        left_object_brace,
        right_object_brace,
        left_array_brace,
        right_array_brace,
        colon,
        comma,

        // literals
        string,
        number_integer,
        number_float,
        boolean_false,
        boolean_true,
        null,

        eof
    };

    using token_literal = std::variant
    <
        std::monostate,
        std::string,
        float_literal,
        integer_literal
    >;

    struct token
    {
        token_type type = token_type::unknown;
        std::string lexeme;
        token_literal literal;
        int line = 0;
    };

    std::ostream& operator<<(std::ostream& os, token_type type);
    std::ostream& operator<<(std::ostream& os, const token& t);
}

#endif
