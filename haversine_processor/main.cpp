#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

#include "haversine_formula.hpp"
#include "json/json.hpp"
#include "platform_metrics.hpp"
#include "profiler.hpp"

namespace
{
    struct haversine_arguments
    {
        const char* input_path = nullptr;
        const char* reference_path = nullptr;
    };

    struct globe_point_pair
    {
        globe_point point1{};
        globe_point point2{};
    };

    void print_json_document(const json::json_document& document)
    {
        PROFILE_FUNCTION;
        std::cout << std::setprecision(13) << document << "\n\n";
    }

    struct haversine_result
    {
        double mean_distance{};
        int pair_count{};
    };

    haversine_result calculate_haversine(const json::json_document& document)
    {
        PROFILE_FUNCTION;

        using namespace json;
        const json_object* root = document.as<json_object>();
        if (!root)
            throw std::exception{ "The JSON root element is not an object." };

        const json_array* point_pairs = root->get_as<json_array>("pairs");
        if (!point_pairs)
            throw std::exception{ "Could not find array member 'pairs'." };

        // calculate average haversine distance
        const size_t point_pair_count = point_pairs->size();
        constexpr long long max_pair_count = 1ULL << 30;
        if (point_pair_count > max_pair_count)
            throw std::exception{ "The input JSON has too many point pairs." };

        const double sum_coeff = 1.0 / point_pair_count;
        double mean_distance = 0.0;
        int pair_count = 0;

        for (const json_element& pair_element : *point_pairs)
        {
            const auto* point_pair = pair_element.as<json_object>();
            if (!point_pair)
                throw std::exception{ "Unexpected non-object found in pair array." };

            if (point_pair->size() != 4)
                throw std::exception{ "Point pair objects must have exactly 4 members: x0, y0, x1, y1" };

            std::optional<float_literal> p_x0;
            std::optional<float_literal> p_y0;
            std::optional<float_literal> p_x1;
            std::optional<float_literal> p_y1;

            for (const auto& [name, value] : *point_pair)
            {
                if (name.size() != 2)
                    throw std::exception{ "Unexpected point pair member found." };

                switch (name[0])
                {
                    case 'x':
                        switch (name[1])
                        {
                            case '0': p_x0 = value.as_number(); break;
                            case '1': p_x1 = value.as_number(); break;
                        }
                        break;

                    case 'y':
                        switch (name[1])
                        {
                            case '0': p_y0 = value.as_number(); break;
                            case '1': p_y1 = value.as_number(); break;
                        }
                        break;
                }
            }

            if (!p_x0 || !p_y0 || !p_x1 || !p_y1)
                throw std::exception{ "Could not find all 4 point pair members: x0, y0, x1, y1" };

            const globe_point p1{ .x = *p_x0, .y = *p_y0 };
            const globe_point p2{ .x = *p_x1, .y = *p_y1 };

            const double distance = haversine_distance(p1, p2);
            mean_distance += sum_coeff * distance;

            ++pair_count;
        }

        return { mean_distance, pair_count };
    }

    double read_reference_distance(const std::string& path, size_t expected_points)
    {
        PROFILE_FUNCTION;

        std::ifstream input_file{ path, std::ios::binary };

        if (!input_file)
            throw std::exception{ "Cannot open reference binary file." };

        size_t distance_count = 0;
        double distance = 0.0;
        while (input_file && !input_file.eof())
        {
            input_file.read(reinterpret_cast<char*>(&distance), sizeof(decltype(distance)));
            ++distance_count;
            input_file.peek();
        }
        distance_count -= (distance_count > 0);

        if (distance_count != expected_points)
            throw std::exception{ "The binary answers file and input JSON do not have the same number of point pairs." };

        return distance;
    }

    void print_haversine_results(uintmax_t input_file_size, double mean_distance, int pair_count)
    {
        std::cout << std::format(std::locale("en_US"), "Input size: {:Ld} bytes\n", input_file_size);
        std::cout << std::format(std::locale("en_US"), "Pair count: {:Ld}\n", pair_count);
        std::cout << std::format("Haversine mean: {:.16f}\n\n", mean_distance);
    }

    void print_validation_results(double reference_mean_distance, double distance_difference)
    {
        std::cout << "Validation:\n";
        std::cout << std::format("  Reference mean: {:.16f}\n", reference_mean_distance);
        std::cout << std::format("  Difference: {:.16f}\n\n", distance_difference);
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
        const uintmax_t input_file_size = std::filesystem::file_size(input_file_info);

        std::cout << "--- Haversine Distance Processor ---\n\n";
        std::cout << "Input file: " << input_filename << "\n";

        if (app_args.reference_path)
        {
            const std::string reference_filename = std::filesystem::path(app_args.reference_path).filename().string();
            std::cout << "Reference file: " << reference_filename << "\n";
        }

        std::cout << '\n';

        double reference_mean_distance = 0.0;
        double distance_difference = 0.0;

        profiler::start_profiling();

        using namespace json;
        json_document document = deserialize_json(app_args.input_path);
        //print_json_document(document);

        auto [mean_distance, pair_count] = calculate_haversine(document);

        if (app_args.reference_path)
        {
            reference_mean_distance = read_reference_distance(app_args.reference_path, pair_count);
            distance_difference = std::abs(mean_distance - reference_mean_distance);
        }

        profiler::stop_profiling();

        print_haversine_results(input_file_size, mean_distance, pair_count);

        if (app_args.reference_path)
            print_validation_results(reference_mean_distance, distance_difference);

        profiler::print_results();
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
