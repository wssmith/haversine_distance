#include "parser.hpp"

#include <exception>
#include <span>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

#include "model.hpp"
#include "token.hpp"
#include "utilities.hpp"

namespace json::parser
{
    namespace
    {
        using token_span = std::span<const token>;
        using token_iterator = token_span::iterator;

        json_member parse_member(token_iterator& iter, token_span token_view, std::vector<std::string>& errors);
        json_object parse_object(token_iterator& iter, token_span token_view, std::vector<std::string>& errors);
        json_array parse_array(token_iterator& iter, token_span token_view, std::vector<std::string>& errors);
        json_element parse_element(token_iterator& iter, token_span token_view, std::vector<std::string>& errors);

        void peek(const token_iterator& iter, token_span token_view, token& t)
        {
            if (iter >= token_view.end())
                throw std::exception{ "Cannot peek out-of-range token." };

            t = *iter;
        }

        void read_and_advance(token_iterator& iter, token_span token_view, token& t)
        {
            if (iter >= token_view.end())
                throw std::exception{ "Cannot read out-of-range token." };

            t = *iter++;
        }

        void advance(token_iterator& iter, token_span token_view)
        {
            if (iter >= token_view.end())
                throw std::exception{ "Cannot advance past the end of the token list." };

            ++iter;
        }

        void back_up(token_iterator& iter, token_span token_view)
        {
            if (iter <= token_view.begin())
                throw std::exception{ "Cannot back up past the beginning of the token list." };

            --iter;
        }

        json_member parse_member(token_iterator& iter, token_span token_view, std::vector<std::string>& errors)
        {
            token t;
            read_and_advance(iter, token_view, t);
            const auto key = std::get<std::string>(t.literal);

            read_and_advance(iter, token_view, t);
            if (t.type != token_type::colon)
            {
                errors.push_back(format_error("Unexpected character after member name. Expected ':'. Found '" + t.lexeme + "'.", t.line));
            }

            const json_element element = parse_element(iter, token_view, errors);

            return { .key = key, .value = element };
        }

        json_object parse_object(token_iterator& iter, token_span token_view, std::vector<std::string>& errors)
        {
            json_object obj;
            std::unordered_set<std::string> unique_keys;

            token t;
            constexpr int no_previous_line = -1;
            int previous_line = no_previous_line;
            bool expecting_member = false;
            bool done = false;
            while (!done)
            {
                read_and_advance(iter, token_view, t);

                switch (t.type)
                {
                    case token_type::right_object_brace:
                    {
                        if (expecting_member)
                            errors.push_back(format_error("Unexpected end of object. A comma is not allowed after the final member.", previous_line));

                        done = true;
                        break;
                    }

                    case token_type::string:
                    {
                        if (!expecting_member && previous_line != no_previous_line)
                            errors.push_back(format_error("Expected a comma after the previous member.", t.line));

                        back_up(iter, token_view);
                        auto member = parse_member(iter, token_view, errors);
                        obj.members.push_back(member);

                        if (unique_keys.contains(member.key))
                            errors.push_back(format_error("Object has a duplicate key '" + member.key + "'.", t.line));
                        else
                            unique_keys.insert(member.key);

                        token next;
                        peek(iter, token_view, next);
                        if (next.type == token_type::comma)
                        {
                            expecting_member = true;
                            advance(iter, token_view);
                            t = next;
                        }
                        else
                        {
                            expecting_member = false;
                        }
                        break;
                    }

                    default:
                        errors.push_back(format_error("Unexpected token '" + t.lexeme + "' found inside object.", t.line));
                        break;
                }

                previous_line = t.line;
            }

            return obj;
        }

        json_array parse_array(token_iterator& iter, token_span token_view, std::vector<std::string>& errors)
        {
            json_array list;

            token t;
            constexpr int no_previous_line = -1;
            int previous_line = no_previous_line;
            bool expecting_element = false;
            bool done = false;
            while (!done)
            {
                peek(iter, token_view, t);

                switch (t.type)
                {
                    case token_type::right_array_brace:
                    {
                        advance(iter, token_view);

                        if (expecting_element)
                            errors.push_back(format_error("Unexpected end of array. A comma is not allowed after the final element.", previous_line));

                        done = true;
                        break;
                    }

                    default:
                    {
                        if (!expecting_element && previous_line != no_previous_line)
                            errors.push_back(format_error("Expected a comma after the previous element.", t.line));

                        json_element element = parse_element(iter, token_view, errors);
                        list.elements.push_back(element);

                        token next;
                        peek(iter, token_view, next);
                        if (next.type == token_type::comma)
                        {
                            expecting_element = true;
                            advance(iter, token_view);
                            t = next;
                        }
                        else
                        {
                            expecting_element = false;
                        }
                    }

                    previous_line = t.line;
                }
            }

            return list;
        }

        json_element parse_element(token_iterator& iter, token_span token_view, std::vector<std::string>& errors)
        {
            token t;
            read_and_advance(iter, token_view, t);

            switch (t.type)
            {
                case token_type::left_object_brace:
                    return { parse_object(iter, token_view, errors) };

                case token_type::left_array_brace:
                    return { parse_array(iter, token_view, errors) };

                case token_type::string:
                    return { std::get<std::string>(t.literal) };

                case token_type::number_integer:
                    return { std::get<integer_literal>(t.literal) };

                case token_type::number_float:
                    return { std::get<float_literal>(t.literal) };

                case token_type::boolean_false:
                    return { false };

                case token_type::boolean_true:
                    return { true };

                case token_type::null:
                    return { nullptr };

                case token_type::eof:
                default:
                    errors.push_back(format_error("Unexpected token '" + t.lexeme + "' while parsing element.", t.line));
                    break;
            }

            return { nullptr };
        }
    }

    json_element parse(const std::vector<token>& tokens)
    {
        const std::span token_view{ tokens.cbegin(), tokens.cend() };
        token_iterator iter = token_view.begin();

        std::vector<std::string> errors;
        json_element document = parse_element(iter, token_view, errors);

        if (!errors.empty())
        {
            const std::string message = "Errors occurred while parsing JSON.\n" + join("\n", errors);
            throw std::exception{ message.c_str() };
        }

        return document;
    }
}
