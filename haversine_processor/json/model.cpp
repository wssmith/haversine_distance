﻿#include "model.hpp"

#include <cstddef>
#include <ostream>
#include <ranges>
#include <string>

#include "match.hpp"
#include "scoped_indent.hpp"

namespace json
{
    namespace
    {
        std::ostream& operator<<(std::ostream& os, const json_member& m)
        {
            os << "\"" << m.key << "\": " << m.value;
            return os;
        }

        void write_escaped_string(std::ostream& os, const std::string& s)
        {
            os << "\"";

            for (const char ch : s)
            {
                switch (ch)
                {
                    case '"':  os << R"(\")"; break;
                    case '\\': os << R"(\\)"; break;
                    case '\b': os << R"(\b)"; break;
                    case '\f': os << R"(\f)"; break;
                    case '\n': os << R"(\n)"; break;
                    case '\r': os << R"(\r)"; break;
                    case '\t': os << R"(\t)"; break;
                    default:   os << ch;      break;
                }
            }

            os << '\"';
        }
    }

    std::ostream& operator<<(std::ostream& os, const json_object& o)
    {
        os << "{";

        if (!o.members.empty())
        {
            scoped_indent indent{ os };

            const json_member& m_0 = o.members[0];
            os << '\n' << m_0;

            for (size_t i = 1; i < o.members.size(); ++i)
            {
                const json_member& m = o.members[i];
                os << ",\n" << m;
            }

            os << '\n';
        }

        os << "}";

        return os;
    }

    std::ostream& operator<<(std::ostream& os, const json_array& a)
    {
        os << "[";

        if (!a.elements.empty())
        {
            scoped_indent indent{ os };

            const json_element& e_0 = a.elements[0];
            os << '\n' << e_0;

            for (size_t i = 1; i < a.elements.size(); ++i)
            {
                const json_element& e = a.elements[i];
                os << ",\n" << e;
            }

            os << '\n';
        }

        os << "]";

        return os;
    }

    std::ostream& operator<<(std::ostream& os, const json_element& e)
    {
        match(overloaded
        {
            [&os](const json_object& o)
            {
                os << o;
            },
            [&os](const json_array& a)
            {
                os << a;
            },
            [&os](const std::string& s)
            {
                write_escaped_string(os, s);
            },
            [&os](integer_literal i)
            {
                os << i;
            },
            [&os](float_literal f)
            {
                os << f;
            },
            [&os](boolean_literal b)
            {
                os << (b ? "true" : "false");
            },
            [&os](null_literal)
            {
                os << "null";
            },
            [](std::monostate) {}
        }, e.value);

        return os;
    }

    std::optional<float_literal> json_element::as_number() const
    {
        if (const float_literal* f = std::get_if<float_literal>(&value))
            return *f;
        else if (const integer_literal* i = std::get_if<integer_literal>(&value))
            return *i;
        else
            return {};
    }

    const json_element* json_object::get(const std::string& key) const
    {
        const auto val = std::ranges::find_if(members, [&key](const json_member& m) { return m.key == key; });

        if (val == members.end())
            return nullptr;

        return &val->value;
    }

    json_element* json_object::get(const std::string& key)
    {
        return const_cast<json_element*>(static_cast<const json_object*>(this)->get(key));
    }

    std::optional<float_literal> json_object::get_as_number(const std::string& key) const
    {
        if (const json_element* val = get(key))
            return val->as_number();

        return {};
    }

    const json_element* json_object::find(member_filter predicate) const
    {
        const auto val = std::ranges::find_if(members, predicate);

        if (val == members.end())
            return nullptr;

        return &val->value;
    }

    json_element* json_object::find(member_filter predicate)
    {
        return const_cast<json_element*>(static_cast<const json_object*>(this)->find(predicate));
    }

    const json_element* json_array::find(element_filter predicate) const
    {
        const auto val = std::ranges::find_if(elements, predicate);

        if (val == elements.end())
            return nullptr;

        return &*val;
    }

    json_element* json_array::find(element_filter predicate)
    {
        return const_cast<json_element*>(static_cast<const json_array*>(this)->find(predicate));
    }
}
