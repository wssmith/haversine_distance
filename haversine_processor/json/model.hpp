#ifndef WS_JSON_MODEL_HPP
#define WS_JSON_MODEL_HPP

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "..\container_utils.hpp"
#include "literals.hpp"

namespace json
{
    struct json_element;
    struct json_member;

    struct json_object
    {
        // a map would be more general, but it's not required for this project
        std::vector<json_member> members;

        ITERATOR_SUPPORT(members);

        using value_type = decltype(members)::value_type;
        using size_type = decltype(members)::size_type;

        const json_element* get(const std::string& key) const;
        json_element* get(const std::string& key);

        template<typename T> const T* get_as(const std::string& key) const;
        template<typename T> T* get_as(const std::string& key);

        std::optional<float_literal> get_as_number(const std::string& key) const;

        using member_filter = bool(*)(const json_member&);

        const json_element* find(member_filter predicate) const;
        json_element* find(member_filter predicate);

        template<typename T> const T* find_as(member_filter predicate) const;
        template<typename T> T* find_as(member_filter predicate);

        size_type size() const { return members.size(); }
        [[nodiscard]] bool empty() const { return members.empty(); }
    };

    struct json_array
    {
        std::vector<json_element> elements;

        ITERATOR_SUPPORT(elements);
        CONTAINER_TYPE_ALIASES(elements);

        using element_filter = bool(*)(const json_element&);

        const json_element* find(element_filter predicate) const;
        json_element* find(element_filter predicate);

        template<typename T> const T* find_as(element_filter predicate) const;
        template<typename T> T* find_as(element_filter predicate);

        size_type size() const { return elements.size(); }
        [[nodiscard]] bool empty() const { return elements.empty(); }

        reference at(size_type position) { return elements.at(position); }
        const_reference at(size_type position) const { return elements.at(position); }
        reference operator[](size_type position) { return elements[position]; }
        const_reference operator[](size_type position) const { return elements[position]; }
    };

    using json_value = std::variant
    <
        std::monostate,
        json_object,
        json_array,
        std::string,
        integer_literal,
        float_literal,
        boolean_literal,
        null_literal
    >;

    struct json_element
    {
        json_value value;

        template<typename T> const T* as() const;
        template<typename T> T* as();

        std::optional<float_literal> as_number() const;
    };

    using json_document = json_element;

    struct json_member
    {
        std::string key;
        json_element value;
    };

    std::ostream& operator<<(std::ostream& os, const json_object& o);
    std::ostream& operator<<(std::ostream& os, const json_array& a);
    std::ostream& operator<<(std::ostream& os, const json_element& e);

    template<typename T>
    const T* json_object::get_as(const std::string& key) const
    {
        if (const json_element* val = get(key))
            return val->as<T>();

        return nullptr;
    }

    template<typename T>
    T* json_object::get_as(const std::string& key)
    {
        return const_cast<T*>(static_cast<const json_object*>(this)->get_as<T>(key));
    }

    template<typename T>
    const T* json_object::find_as(member_filter predicate) const
    {
        if (const json_element* val = find(predicate))
            return val->as<T>();

        return nullptr;
    }

    template<typename T>
    T* json_object::find_as(member_filter predicate)
    {
        return const_cast<T*>(static_cast<const json_object*>(this)->find_as<T>(predicate));
    }

    template<typename T>
    const T* json_array::find_as(element_filter predicate) const
    {
        if (const json_element* val = find(predicate))
            return val->as<T>();

        return nullptr;
    }

    template<typename T>
    T* json_array::find_as(element_filter predicate)
    {
        return const_cast<T*>(static_cast<const json_array*>(this)->find_as<T>(predicate));
    }

    template<typename T>
    const T* json_element::as() const
    {
        return std::get_if<T>(&value);
    }

    template<typename T>
    T* json_element::as()
    {
        return const_cast<T*>(static_cast<const json_element*>(this)->as<T>());
    }
}

#endif
