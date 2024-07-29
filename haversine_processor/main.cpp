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

    double read_reference_distance(const std::string& path, size_t expected_points)
    {
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

        // deserialize input json
        const uint64_t cpu_freq = estimate_cpu_timer_freq();
        const auto start_overall_cpu = read_cpu_timer();
        uint64_t end_overall_cpu = start_overall_cpu;

        double reference_distance = 0.0;
        double distance_difference = 0.0;
        double average_distance = 0.0;
        int pair_count = 0;
        {
            PROFILE_BLOCK("overall");

            using namespace json;
            const json_document document = deserialize_json(app_args.input_path);

            {
                PROFILE_BLOCK("print");
                std::cout << std::setprecision(13) << document << "\n\n";
            }

            {
                PROFILE_BLOCK("calculate");

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
                            continue;

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

                    globe_point p1{ .x = *p_x0, .y = *p_y0 };
                    globe_point p2{ .x = *p_x1, .y = *p_y1 };

                    double distance = haversine_distance(p1, p2);
                    average_distance += sum_coeff * distance;

                    ++pair_count;
                }
            }

            // read reference binary file
            if (app_args.reference_path)
            {
                reference_distance = read_reference_distance(app_args.reference_path, pair_count);
                distance_difference = std::abs(average_distance - reference_distance);
            }

            end_overall_cpu = read_cpu_timer();
        }

        // print results
        std::cout << std::format(std::locale("en_US"), "Input size: {:Ld} bytes\n", input_file_size);
        std::cout << std::format(std::locale("en_US"), "Pair count: {:Ld}\n", pair_count);
        std::cout << std::format("Haversine sum: {:.16f}\n\n", average_distance);

        if (app_args.reference_path)
        {
            std::cout << "Validation:\n";
            std::cout << std::format("  Reference sum: {:.16f}\n", reference_distance);
            std::cout << std::format("  Difference: {:.16f}\n\n", distance_difference);
        }

        const uint64_t overall_cpu = end_overall_cpu - start_overall_cpu;
        const double overall_time_cpu_ms = 1000.0 * overall_cpu / cpu_freq;

        std::cout << "Performance:\n";
        //std::cout << "  Scanning completed in " << scan_time << '\n';
        //std::cout << "  Parsing completed in " << parse_time << '\n';
        //std::cout << "  Overall deserialized JSON in " << deserialize_time << '\n';
        //std::cout << "  Pretty-printed JSON in " << print_time << '\n';
        //std::cout << "  Calculated average distance in " << calculate_time << '\n';

        //if (app_args.reference_path)
        //    std::cout << "  Read binary reference file in " << comparison_time << '\n';

        std::cout << std::format("  (Legacy) Overall finished in {:.4f} ms\n\n", overall_time_cpu_ms);

        const auto& profile_blocks = profile_block::get_profile_blocks();
        const uint64_t overall_duration = profile_block::get_overall_duration();

        double total_percent = 0.0;
        double total_duration_ms = 0.0;
        for (const profile_anchor& block : profile_blocks)
        {
            const double duration_ms = 1000.0 * block.duration_exclusive / cpu_freq;
            const double percentage = 100.0 * block.duration_exclusive / overall_duration;

            total_duration_ms += duration_ms;
            total_percent += percentage;

            std::cout << std::format("  {} finished in {:.4f} ms ({:.2f}%)\n", block.name, duration_ms, percentage);
        }

        const double overall_duration_ms = 1000.0 * overall_duration / cpu_freq;
        std::cout << std::format("\n  Total: {:.4f} {:.4f} ms ({:.2f}%)\n", overall_duration_ms, total_duration_ms, total_percent);
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
