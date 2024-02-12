#include "token.hpp"

#include <ostream>
#include <string>
#include <variant>

namespace json
{
    namespace
    {
        const char* to_string(token_type type)
        {
            switch (type)
            {
                case token_type::left_object_brace:
                    return "left_object_brace";
                case token_type::right_object_brace:
                    return "right_object_brace";
                case token_type::left_array_brace:
                    return "left_array_brace";
                case token_type::right_array_brace:
                    return "right_array_brace";
                case token_type::colon:
                    return "colon";
                case token_type::comma:
                    return "comma";
                case token_type::string:
                    return "string";
                case token_type::number_integer:
                    return "number_integer";
                case token_type::number_float:
                    return "number_float";
                case token_type::boolean_false:
                    return "boolean_false";
                case token_type::boolean_true:
                    return "boolean_true";
                case token_type::null:
                    return "null";
                case token_type::eof:
                    return "eof";
                case token_type::unknown:
                default:
                    return "unknown";
            }
        }
    }

    std::ostream& operator<<(std::ostream& os, token_type type)
    {
        os << to_string(type);
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const token& t)
    {
        os << to_string(t.type) << " " << t.lexeme;

        switch (t.type)
        {
            case token_type::string:
            {
                os << " " + std::get<std::string>(t.literal);
                break;
            }

            case token_type::number_integer:
            {
                const auto literal_value = std::get<integer_literal>(t.literal);
                os << " " + std::to_string(literal_value);
                break;
            }

            case token_type::number_float:
            {
                const auto literal_value = std::get<float_literal>(t.literal);
                os << " " + std::to_string(literal_value);
                break;
            }

            default:
                break;
        }

        return os;
    }
}
