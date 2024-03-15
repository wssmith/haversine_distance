#include <algorithm>
#include <array>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <format>
#include <iostream>
#include <locale>
#include <numeric>
#include <string>
#include <vector>

#include "haversine_formula.hpp"
#include "random_generator.hpp"

namespace
{
    struct haversine_arguments
    {
        double x_center_r1{};
        double y_center_r1{};
        double width_r1{};
        double height_r1{};

        double x_center_r2{};
        double y_center_r2{};
        double width_r2{};
        double height_r2{};

        long long pair_count{};
        bool cluster_mode{};
    };

    struct cluster_dimensions
    {
        double x_min_r1{};
        double x_max_r1{};
        double y_min_r1{};
        double y_max_r1{};
        double x_min_r2{};
        double x_max_r2{};
        double y_min_r2{};
        double y_max_r2{};
    };

    struct globe_point_pair
    {
        globe_point point1{};
        globe_point point2{};
    };

    bool validate_arguments(const haversine_arguments& app_args, const std::string& usage_message)
    {
        constexpr long long max_pair_count = 1LL << 34;
        if (app_args.pair_count >= max_pair_count)
        {
            std::cout << usage_message << "\n\n";
            std::cout << "Number of pairs must be less than " << max_pair_count << ". (value = " << app_args.pair_count << ")\n";
            return false;
        }

        if (app_args.pair_count < 0)
        {
            std::cout << usage_message << "\n\n";
            std::cout << "Number of pairs must be positive. (value = " << app_args.pair_count << ")\n";
            return false;
        }

        const std::array point_arguments{ app_args.x_center_r1, app_args.y_center_r1, app_args.width_r1, app_args.height_r1,
                                          app_args.x_center_r2, app_args.y_center_r2, app_args.width_r2, app_args.height_r2 };

        for (size_t i = 0; i < point_arguments.size(); ++i)
        {
            const double& val = point_arguments[i];
            int min_val = 0;
            int max_val = 0;

            switch (i)
            {
                case 0:  case 4: // region center x
                    min_val = -180;
                    max_val = 180;
                    break;

                case 1: case 5: // region center y
                    min_val = -90;
                    max_val = 90;
                    break;

                case 2: case 6: // region width
                    min_val = 0;
                    max_val = 360;
                    break;

                case 3: case 7: // region height
                    min_val = 0;
                    max_val = 180;
                    break;

                default:
                    break; // unreachable
            }

            if (val < min_val || val > max_val)
            {
                std::cout << usage_message << "\n\n";
                std::cout << "The argument at position " << (i + 2) << " must be in [" << min_val << ", " << max_val << "]. (value = " << val << ")\n";
                return false;
            }
        }

        return true;
    }

    cluster_dimensions get_cluster_dimensions(const haversine_arguments& app_args)
    {
        constexpr double y_max = 90.0;
        constexpr double y_min = -90.0;
        constexpr double x_max = 180.0;
        constexpr double x_min = -180.0;

        if (app_args.cluster_mode)
        {
            const double x_radius_r1 = app_args.width_r1 / 2.0;
            const double y_radius_r1 = app_args.height_r1 / 2.0;
            const double x_radius_r2 = app_args.width_r2 / 2.0;
            const double y_radius_r2 = app_args.height_r2 / 2.0;

            return cluster_dimensions
            {
                .x_min_r1 = std::clamp(app_args.x_center_r1 - x_radius_r1, x_min, x_max),
                .x_max_r1 = std::clamp(app_args.x_center_r1 + x_radius_r1, x_min, x_max),
                .y_min_r1 = std::clamp(app_args.y_center_r1 - y_radius_r1, y_min, y_max),
                .y_max_r1 = std::clamp(app_args.y_center_r1 + y_radius_r1, y_min, y_max),
                .x_min_r2 = std::clamp(app_args.x_center_r2 - x_radius_r2, x_min, x_max),
                .x_max_r2 = std::clamp(app_args.x_center_r2 + x_radius_r2, x_min, x_max),
                .y_min_r2 = std::clamp(app_args.y_center_r2 - y_radius_r2, y_min, y_max),
                .y_max_r2 = std::clamp(app_args.y_center_r2 + y_radius_r2, y_min, y_max)
            };
        }
        else
        {
            return cluster_dimensions
            {
                .x_min_r1 = x_min,
                .x_max_r1 = x_max,
                .y_min_r1 = y_min,
                .y_max_r1 = y_max,
                .x_min_r2 = x_min,
                .x_max_r2 = x_max,
                .y_min_r2 = y_min,
                .y_max_r2 = y_max,
            };
        }
    }

    void write_point_pair(std::ofstream& output_stream, const globe_point_pair& point_pair)
    {
        const auto& [p1, p2] = point_pair;
        output_stream << std::format(R"({{ "x0": {}, "y0": {}, "x1": {}, "y1": {} }})", p1.x, p1.y, p2.x, p2.y);
    }

    void save_haversine_json(const char* path, const std::vector<globe_point_pair>& data)
    {
        std::ofstream output_stream{ path };

        if (!output_stream)
            throw std::exception{ "Could not write input data JSON file." };

        output_stream << "{\n  \"pairs\": [\n";

        if (!data.empty())
        {
            output_stream << "    ";
            write_point_pair(output_stream, data[0]);

            for (size_t i = 1; i < data.size(); ++i)
            {
                output_stream << ",\n    ";
                write_point_pair(output_stream, data[i]);
            }
        }

        output_stream << "\n  ]\n}\n";

        output_stream.close();
    }

    void save_haversine_distances(const char* path, const std::vector<double>& haversine_distances, double average_distance)
    {
        std::ofstream output_stream{ path, std::ios::binary };

        if (!output_stream)
            throw std::exception{ "Could not write reference distance binary file." };

        if (!haversine_distances.empty())
        {
            for (const double distance : haversine_distances)
            {
                output_stream.write(reinterpret_cast<const char*>(&distance), sizeof(decltype(distance)));
            }

            output_stream.write(reinterpret_cast<const char*>(&average_distance), sizeof(decltype(average_distance)));
        }

        output_stream.close();
    }

    std::vector<double> read_binary_file(const std::string& path, long long expected_points)
    {
        std::vector<double> data;
        data.reserve(expected_points);

        std::ifstream input_file{ path, std::ios::binary };

        if (!input_file)
            throw std::exception{ "Cannot open binary file." };

        for (auto i = 0LL; i < expected_points && input_file.peek() && !input_file.eof(); ++i)
        {
            double distance = 0.0;
            input_file.read(reinterpret_cast<char*>(&distance), sizeof(decltype(distance)));
            data.push_back(distance);
        }

        return data;
    }
}

int main(int argc, char* argv[])
{
    try
    {
        // read command line arguments
        const std::string exe_filename = std::filesystem::path(argv[0]).filename().string();
        const std::string usage_message = "Usage: " + exe_filename + " pair_count " +
                                          "[x_center_r1] [y_center_r1] [width_r1] [height_r1] " +
                                          "[x_center_r2] [y_center_r2] [width_r2] [height_r2]";

        haversine_arguments app_args;
        if (argc == 2)
        {
            // points are uniformly distributed on the globe
            app_args = haversine_arguments
            {
                .pair_count = std::stoi(argv[1]),
                .cluster_mode = false
            };
        }
        else if (argc == 10)
        {
            // points are clustered in two regions
            app_args = haversine_arguments
            {
                .x_center_r1 = std::stod(argv[2]),
                .y_center_r1 = std::stod(argv[3]),
                .width_r1 = std::stod(argv[4]),
                .height_r1 = std::stod(argv[5]),
                .x_center_r2 = std::stod(argv[6]),
                .y_center_r2 = std::stod(argv[7]),
                .width_r2 = std::stod(argv[8]),
                .height_r2 = std::stod(argv[9]),
                .pair_count = std::stoll(argv[1]),
                .cluster_mode = true
            };
        }
        else
        {
            std::cout << usage_message << '\n';
            return EXIT_FAILURE;
        }

        if (!validate_arguments(app_args, usage_message))
            return EXIT_FAILURE;

        std::cout << "--- Haversine Distance Input Generator ---\n\n";

        // generate pairs of coordinates
        std::cout << "Generating coordinate pairs...";

        const cluster_dimensions dimensions = get_cluster_dimensions(app_args);

        uniform_real_generator x_rand_r1{ dimensions.x_min_r1, dimensions.x_max_r1 };
        uniform_real_generator y_rand_r1{ dimensions.y_min_r1, dimensions.y_max_r1 };
        uniform_real_generator x_rand_r2{ dimensions.x_min_r2, dimensions.x_max_r2 };
        uniform_real_generator y_rand_r2{ dimensions.y_min_r2, dimensions.y_max_r2 };

        std::vector<globe_point_pair> points;
        points.reserve(app_args.pair_count);

        std::vector<double> distances;
        distances.reserve(app_args.pair_count);

        for (int i = 0; i < app_args.pair_count; ++i)
        {
            auto point_pair = globe_point_pair
            {
                .point1 = { .x = x_rand_r1(), .y = y_rand_r1() },
                .point2 = { .x = x_rand_r2(), .y = y_rand_r2() }
            };

            points.push_back(point_pair);

            const auto& [p1, p2] = point_pair;
            double distance = haversine_distance(p1, p2);
            distances.push_back(distance);
        }

        std::cout << " done.\n\n";

        // calculate the average distance
        const double average_distance = std::accumulate(distances.begin(), distances.end(), 0.0) / (1.0 * distances.size());

        // summarize the results
        std::cout << "Method: " << (app_args.cluster_mode ? "cluster" : "uniform") << '\n';
        std::cout << std::format(std::locale("en_US"), "Pair count: {:Ld}\n", app_args.pair_count);
        std::cout << "Average distance: " << average_distance << '\n';

        if (app_args.cluster_mode)
        {
            // this will be fairly accurate for small clusters, though because we're using rectangular clusters it will never be exact
            const double expected_distance = haversine_distance(app_args.x_center_r1, app_args.y_center_r1, app_args.x_center_r2, app_args.y_center_r2);
            std::cout << "Expected distance: " << expected_distance << '\n';
        }

        // save coordinates to a json file
        constexpr auto data_filename = "haversine_points.json";
        std::cout << "\nSaving coordinate pairs to '" << data_filename << "'...";

        save_haversine_json(data_filename, points);

        std::cout << " done.\n\n";

        // save distances to a binary file
        constexpr auto distances_filename = "haversine_answers.f64";
        std::cout << "Saving reference haversine distances to '" << distances_filename << "'...";

        save_haversine_distances(distances_filename, distances, average_distance);

        std::cout << " done.\n\n";
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
