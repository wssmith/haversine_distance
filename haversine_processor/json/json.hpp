#ifndef WS_JSON_HPP
#define WS_JSON_HPP

#include <chrono>
#include <string>

#include "model.hpp"

namespace json
{
    json_document deserialize_json(const std::string& filepath);
}

#endif
