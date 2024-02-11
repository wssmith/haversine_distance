#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

#include "json/json.hpp"

namespace
{
    struct haversine_arguments
    {
        const char* input_path = nullptr;
    };
}

int main(int argc, char* argv[])
{
    // read command line arguments
    constexpr int expected_args = 1;
    if (argc != expected_args + 1)
    {
        constexpr auto usage_message = "Usage: haversine_basic input_file";
        std::cout << usage_message << '\n';
        return EXIT_FAILURE;
    }

    const haversine_arguments app_args
    {
        .input_path = argv[argc - 1]
    };

    try
    {
        const std::string input_filename = std::filesystem::path(app_args.input_path).filename().string();
        std::cout << "--- Parsing " << input_filename << " ---\n\n";

        using namespace json;
        json_document document = deserialize_json(app_args.input_path);
        std::cout << document << '\n';

        /*if (auto* root = document.as<json_object>())
        {
            if (auto* pn = root->get_as<int>("my_number"))
            {
            	std::cout << "my number = " << *pn << '\n';
            }

            if (auto* pa = root->get_as<json_array>("nice_array"))
            {
                for (const json_element& e : *pa)
                {
                    std::cout << e << '\n';
                }
            }
        }*/
    }
    catch (std::exception& ex)
    {
        std::cout << "ERROR!! " << ex.what() << '\n';
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cout << "UNKNOWN ERROR!!\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
