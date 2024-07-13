#include "scanner.hpp"

#include <fstream>
#include <exception>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "utilities.hpp"
#include "token.hpp"

#include "../profiler.hpp"

namespace json::scanner
{
    using namespace std::string_literals;

    namespace
    {
        void read_string(std::ifstream& input_file, int line, std::vector<token>& tokens, std::vector<std::string>& errors);
        void read_number(std::ifstream& input_file, int line, std::vector<token>& tokens, std::vector<std::string>& errors);
        void read_literal(std::ifstream& input_file, const std::string& expected, token_type expected_token, int line, std::vector<token>& tokens, std::vector<std::string>& errors);

        using consume_filter = bool(*)(int);

        bool consume_if(std::ifstream& input_file, std::ostringstream& builder, consume_filter predicate)
        {
            if (const auto next = input_file.peek(); !input_file.eof() && predicate(next))
            {
                char ch;
                input_file >> ch;
                builder << ch;

                return true;
            }

            return false;
        }

        bool consume_while(std::ifstream& input_file, std::ostringstream& builder, consume_filter predicate)
        {
            bool consumed = false;
            char ch{};
            for (auto next = input_file.peek(); !input_file.eof() && predicate(next); next = input_file.peek())
            {
                consumed = true;
                input_file >> ch;
                builder << ch;
            }

            return consumed;
        }

        bool consume_while_digits(std::ifstream& input_file, std::ostringstream& builder)
        {
            return consume_while(input_file, builder, [](int c) { return c >= '0' && c <= '9'; });
        }

        void skip_while(std::ifstream& input_file, consume_filter predicate)
        {
            char ch{};
            for (auto next = input_file.peek(); !input_file.eof() && predicate(next); next = input_file.peek())
            {
                input_file >> ch;
            }
        }

        bool reached_end_of_file(std::ifstream& input_file)
        {
            input_file.peek();
            return input_file.eof();
        }

        bool is_hex_digit(int ch)
        {
            return (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f');
        }

        char read_escape_sequence(char ch, int line, std::vector<std::string>& errors)
        {
            switch (ch)
            {
                case '"': case '\\': case '/':
                    return ch;
                case 'b':
                    return '\b';
                case 'f':
                    return '\f';
                case 'n':
                    return '\n';
                case 'r':
                    return '\r';
                case 't':
                    return '\t';

                default:
                    errors.push_back(format_error("Unrecognized escape character '\\"s + ch + "'.", line));
                    return ch;
            }
        }

        void read_string(std::ifstream& input_file, int line, std::vector<token>& tokens, std::vector<std::string>& errors)
        {
            std::ostringstream builder;
            char ch{};
            for (auto next = input_file.peek(); (!input_file.eof() && next != '"' && next != '\n'); next = input_file.peek())
            {
                input_file >> ch;

                constexpr char min_char = 0x20;
                if (ch < min_char)
                {
                    errors.push_back(format_error("Invalid character. Code point: " + std::to_string(ch), line));
                    continue;
                }

                if (ch == '\\')
                {
                    if (reached_end_of_file(input_file))
                        break;

                    input_file >> ch;

                    if (ch == 'u')
                    {
                        std::string hex_digits;

                        bool found_unicode = true;
                        for (int i = 0; i < 4; ++i)
                        {
                            if (reached_end_of_file(input_file) || !is_hex_digit(input_file.peek()))
                            {
                                found_unicode = false;
                                break;
                            }

                            input_file >> ch;
                            hex_digits += ch;
                        }

                        if (!found_unicode)
                        {
                            errors.push_back(format_error("Expected 4 hexadecimal digits after '\\u'.", line));
                            continue;
                        }

                        builder << "\\u" << hex_digits; // leaving the escape sequence as is, since we aren't supporting utf-16
                    }
                    else
                    {
                        ch = read_escape_sequence(ch, line, errors);
                        builder << ch;
                    }
                }
                else
                {
                    builder << ch;
                }
            }

            if (auto next = input_file.peek(); input_file.eof() || next != '"')
            {
                errors.push_back(format_error("Unterminated string \"" + builder.str() + "\".", line));
                return;
            }

            input_file >> ch;

            std::string s = builder.str();
            tokens.push_back({ .type = token_type::string, .lexeme = "\"" + s + "\"", .literal = s, .line = line });
        }


        void read_number(std::ifstream& input_file, int line, std::vector<token>& tokens, std::vector<std::string>& errors)
        {
            std::ostringstream builder;
            bool is_valid = true;
            bool is_float = false;

            // negative sign
            consume_if(input_file, builder, [](int c) { return c == '-'; });

            // if the first integral digit is '0' that's the entire integral part
            if (!consume_if(input_file, builder, [](int c) { return c == '0'; }))
            {
                // multi-digit integral parts cannot begin with '0'
                if (consume_if(input_file, builder, [](int c) { return c >= '1' && c <= '9'; }))
                {
                    // additional integral digits
                    consume_while_digits(input_file, builder); // NOT required
                }
                else
                {
                    is_valid = false;
                    errors.push_back(format_error("Expected number to begin with a digit.", line));
                }
            }

            // decimal point
            if (is_valid && consume_if(input_file, builder, [](int c) { return c == '.'; }))
            {
                is_float = true;

                // fractional digits
                if (!consume_while_digits(input_file, builder))
                {
                    is_valid = false;
                    errors.push_back(format_error("Expected number with a decimal point to have fraction digits.", line));
                }
            }

            // exponent 'e'
            if (is_valid && consume_if(input_file, builder, [](int c) { return c == 'E' || c == 'e'; }))
            {
                is_float = true;

                // exponent sign
                consume_if(input_file, builder, [](int c) { return c == '+' || c == '-'; });

                // exponent digits
                if (!consume_while_digits(input_file, builder))
                {
                    is_valid = false;
                    errors.push_back(format_error("Expected number to contain exponent digits.", line));
                }
            }

            if (is_valid)
            {
                const std::string number_string = builder.str();
                if (is_float)
                {
                    double d = std::stod(number_string);
                    tokens.push_back({ .type = token_type::number_float, .lexeme = number_string, .literal = d, .line = line });
                }
                else
                {
                    int n = std::stoi(number_string);
                    tokens.push_back({ .type = token_type::number_integer, .lexeme = number_string, .literal = n, .line = line });
                }
            }
        }

        void read_literal(std::ifstream& input_file, const std::string& expected, token_type expected_token, int line, std::vector<token>& tokens, std::vector<std::string>& errors)
        {
            bool is_valid = true;
            char ch{};
            for (const char& expected_char : expected)
            {
                if (input_file.peek() == expected_char && !input_file.eof())
                {
                    input_file >> ch;
                }
                else
                {
                    is_valid = false;
                    errors.push_back(format_error("Problem reading literal '" + expected + "'.", line));
                    break;
                }
            }

            if (is_valid)
                tokens.push_back({ .type = expected_token, .lexeme = expected, .line = line });
        }

        void report_unexpected_character(char ch, int line, std::vector<std::string>& errors)
        {
            errors.push_back(format_error("Unexpected character '"s + ch + "'.", line));
        }

        void skip_comment(std::ifstream& input_file, int& line, std::vector<std::string>& errors)
        {
            const int comment_start_line = line;
            const int next = input_file.peek();
            if (input_file.eof())
            {
                errors.push_back("Unexpected end of file after '/'.");
                return;
            }

            switch (next)
            {
                case '/':
                    skip_while(input_file, [](int c) { return c != '\n'; });
                    break;

                case '*':
                    char ch;
                    input_file >> ch;

                    do
                    {
                        skip_while(input_file, [](int c) { return c != '*' && c != '\n'; });

                        if (input_file.peek() == '\n')
                            ++line;

                        input_file >> ch;
                    }
                    while (!(ch == '*' && input_file.peek() == '/') && !input_file.eof());

                    if (input_file.eof())
                    {
                        errors.push_back(format_error("Unterminated block comment.", comment_start_line));
                        return;
                    }

                    input_file >> ch; // consume '/'
                    break;

                default:
                    report_unexpected_character(static_cast<char>(next), comment_start_line, errors);
                    break;
            }
        }
    }

    std::vector<token> scan(std::ifstream& input_file)
    {
        profiler scan_activity{ "scan" };

        std::vector<token> tokens;
        std::vector<std::string> errors;

        input_file >> std::noskipws; // handle whitespace manually to count line-breaks

        int line = 1;
        char ch{};
        while (input_file >> ch)
        {
            switch (ch)
            {
                case '{':
                    tokens.push_back({ .type = token_type::left_object_brace, .lexeme = std::string{ ch }, .line = line });
                    break;

                case '}':
                    tokens.push_back({ .type = token_type::right_object_brace, .lexeme = std::string{ ch }, .line = line });
                    break;

                case '[':
                    tokens.push_back({ .type = token_type::left_array_brace, .lexeme = std::string{ ch }, .line = line });
                    break;

                case ']':
                    tokens.push_back({ .type = token_type::right_array_brace, .lexeme = std::string{ ch }, .line = line });
                    break;

                case ':':
                    tokens.push_back({ .type = token_type::colon, .lexeme = std::string{ ch }, .line = line });
                    break;

                case ',':
                    tokens.push_back({ .type = token_type::comma, .lexeme = std::string{ ch }, .line = line });
                    break;

                case '"':
                    read_string(input_file, line, tokens, errors);
                    break;

                case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': case '-':
                    input_file.unget();
                    read_number(input_file, line, tokens, errors);
                    break;

                case 't':
                    input_file.unget();
                    read_literal(input_file, "true", token_type::boolean_true, line, tokens, errors);
                    break;

                case 'f':
                    input_file.unget();
                    read_literal(input_file, "false", token_type::boolean_false, line, tokens, errors);
                    break;

                case 'n':
                    input_file.unget();
                    read_literal(input_file, "null", token_type::null, line, tokens, errors);
                    break;

                case '/':
                    skip_comment(input_file, line, errors);
                    break;

                case ' ':
                case '\r':
                case '\t':
                    break;

                case '\n':
                    ++line;
                    break;

                default:
                    report_unexpected_character(ch, line, errors);
                    break;
            }
        }

        tokens.push_back({ .type = token_type::eof, .line = line });

        if (!errors.empty())
        {
            const std::string message = "Errors occurred while scanning JSON.\n" + join("\n", errors);
            throw std::exception{ message.c_str() };
        }

        return tokens;
    }
}
