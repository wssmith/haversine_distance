#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>

#include "haversine_formula.hpp"
#include "json/json.hpp"

namespace
{
    struct haversine_arguments
    {
        const char* input_path = nullptr;
        const char* reference_path = nullptr;
    };

    struct globe_point
    {
        double x{};
        double y{};
    };

    struct globe_point_pair
    {
        globe_point point1{};
        globe_point point2{};
    };

    double read_reference_distance(const std::string& path, long long expected_points)
    {
        std::vector<double> data;
        data.reserve(expected_points);

        std::ifstream input_file{ path, std::ios::binary };

        if (!input_file)
            throw std::exception{ "Cannot open reference binary file." };

        for (auto i = 0LL; input_file.peek() && !input_file.eof(); ++i)
        {
            double distance = 0.0;
            input_file.read(reinterpret_cast<char*>(&distance), sizeof(decltype(distance)));
            data.push_back(distance);
        }

        const double average_distance = data.back();
        data.pop_back();

        if (data.size() != expected_points)
            throw std::exception{ "" };

        return average_distance;
    }
}

int main(int argc, char* argv[])
{
    // read command line arguments
    const std::string exe_filename = std::filesystem::path(argv[0]).filename().string();
    const std::string usage_message = "Usage: " + exe_filename + " [haversine_input.json]\n"
                                      "       " + exe_filename + " [haversine_input.json] [answers.f64]";

    haversine_arguments app_args;
    if (argc == 2)
    {
        app_args = { .input_path = argv[1] };
    }
    else if (argc == 3)
    {
        app_args = haversine_arguments
        {
            .input_path = argv[1],
            .reference_path = argv[2]
        };
    }
    else
    {
        std::cout << usage_message << "\n";
        return EXIT_FAILURE;
    }

    try
    {
        auto input_file_info = std::filesystem::path(app_args.input_path);
        const std::string input_filename = input_file_info.filename().string();
        const uintmax_t input_file_size = file_size(input_file_info);

        std::cout << "--- Haversine Distance Processor ---\n\n";
        std::cout << "Input file: " << input_filename << "\n";
        if (app_args.reference_path)
        {
            const std::string reference_filename = std::filesystem::path(app_args.reference_path).filename().string();
            std::cout << "Reference file: " << input_filename << "\n";
        }
        std::cout << '\n';

        // deserialize input json
        using namespace json;
        const json_document document = deserialize_json(app_args.input_path);
        //std::cout << std::setprecision(13) << document << '\n';

        const json_object* root = document.as<json_object>();
        if (!root)
            throw std::exception{ "" };

        const json_array* point_pairs = root->get_as<json_array>("pairs");
        if (!point_pairs)
            throw std::exception{ "" };

        // calculate average haversine distance
        const size_t point_pair_count = point_pairs->size();
        std::vector<double> distances;
        distances.reserve(point_pair_count);

        constexpr long long max_pair_count = 1ULL << 30;
        if (point_pair_count > max_pair_count)
            throw std::exception{ "" };

        const double sum_coeff = 1.0 / distances.size();

        for (const json_element& pair_element : *point_pairs)
        {
            const json_object* point_pair = pair_element.as<json_object>();
            if (!point_pair)
                throw std::exception{ "" };

            const auto* p_x0 = point_pair->get_as<float_literal>("x0");
            const auto* p_y0 = point_pair->get_as<float_literal>("y0");
            const auto* p_x1 = point_pair->get_as<float_literal>("x1");
            const auto* p_y1 = point_pair->get_as<float_literal>("y1");

            if (!p_x0 || !p_y0 || !p_x1 || !p_y1)
                throw std::exception{ "" };

            globe_point p1{ .x = *p_x0, .y = *p_y0 };
            globe_point p2{ .x = *p_x1, .y = *p_y1 };

            double distance = haversine_distance(p1.x, p1.y, p2.x, p2.y);
            distances.push_back(distance);
        }

        // calculate the average distance
        const double average_distance = std::accumulate(distances.begin(), distances.end(), 0.0) / (1.0 * distances.size());

        // read reference binary file
        double reference_distance = 0.0;
        double distance_difference = 0.0;
        if (app_args.reference_path)
        {
            reference_distance = read_reference_distance(app_args.reference_path, point_pair_count);
            distance_difference = std::abs(average_distance - reference_distance);
        }

        // print results
        std::cout << std::vformat(std::locale("en_US"), "Input size: {:Ld} bytes\n", std::make_format_args(input_file_size));
        std::cout << std::vformat(std::locale("en_US"), "Pair count: {:Ld}\n", std::make_format_args(distances.size()));
        std::cout << std::vformat("Haversine sum: {:.16f}\n\n", std::make_format_args(average_distance));

        if (app_args.reference_path)
        {
            std::cout << "Validation:\n";
            std::cout << std::vformat("Reference sum: {:.16f}\n", std::make_format_args(reference_distance));
            std::cout << std::vformat("Difference: {:.16f}\n", std::make_format_args(distance_difference));
        }
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
