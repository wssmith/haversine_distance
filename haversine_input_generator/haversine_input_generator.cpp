#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "haversine_formula.hpp"
#include "random_generator.hpp"

namespace
{
    using namespace std::string_literals;

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

        int pair_count{};

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

    void write_point_pair(std::ofstream& output_stream, const globe_point_pair& point_pair)
    {
        output_stream << "{ \"x0\": " << point_pair.point1.x << ", \"y0\": " << point_pair.point1.y << ", \"x1\": " << point_pair.point2.x << ", \"y1\": " << point_pair.point2.y << " }";
    }

    void save_haversine_json(const char* path, const std::vector<globe_point_pair>& data)
    {
        std::ofstream output_stream{ path };

        output_stream << "{\n  \"coordinates\": [\n";

        if (!data.empty())
        {
            const auto& [p1, p2] = data[0];
            double distance = reference_haversine(p1.x, p1.y, p2.x, p2.y);

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
}

cluster_dimensions get_cluster_dimensions(const haversine_arguments& app_args)
{
    if (app_args.cluster_mode)
    {
        const double x_radius_r1 = app_args.width_r1 / 2.0;
        const double y_radius_r1 = app_args.height_r1 / 2.0;
        const double x_radius_r2 = app_args.width_r2 / 2.0;
        const double y_radius_r2 = app_args.height_r2 / 2.0;

        return cluster_dimensions
        {
            .x_min_r1 = app_args.x_center_r1 - x_radius_r1,
            .x_max_r1 = app_args.x_center_r1 + x_radius_r1,
            .y_min_r1 = app_args.y_center_r1 - y_radius_r1,
            .y_max_r1 = app_args.y_center_r1 + y_radius_r1,
            .x_min_r2 = app_args.x_center_r2 - x_radius_r2,
            .x_max_r2 = app_args.x_center_r2 + x_radius_r2,
            .y_min_r2 = app_args.y_center_r2 - y_radius_r2,
            .y_max_r2 = app_args.y_center_r2 + y_radius_r2,
        };
    }
    else
    {
        constexpr double y_max_uniform = 90.0;
        constexpr double y_min_uniform = 0.0;
        constexpr double x_max_uniform = 180.0;
        constexpr double x_min_uniform = 0.0;

        return cluster_dimensions
        {
            .x_min_r1 = x_min_uniform,
            .x_max_r1 = x_max_uniform,
            .y_min_r1 = y_min_uniform,
            .y_max_r1 = y_max_uniform,
            .x_min_r2 = x_min_uniform,
            .x_max_r2 = x_max_uniform,
            .y_min_r2 = y_min_uniform,
            .y_max_r2 = y_max_uniform,
        };
    }
}

int main(int argc, char* argv[])
{
    try
    {
        // read command line arguments
        const std::string exe_filename = std::filesystem::path(argv[0]).filename().string();
        const std::string usage_message = "Usage:\n  " + exe_filename + " pair_count " + 
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
                .pair_count = std::stoi(argv[1]),
                .cluster_mode = true
            };
        }
        else
        {
            std::cout << usage_message << '\n';
            return EXIT_FAILURE;
        }

        // generate pairs of coordinates
        const int pair_count = app_args.pair_count;
        const cluster_dimensions dimensions = get_cluster_dimensions(app_args);

        uniform_real_generator x_rand_r1{ dimensions.x_min_r1, dimensions.x_max_r1 };
        uniform_real_generator y_rand_r1{ dimensions.y_min_r1, dimensions.y_max_r1 };
        uniform_real_generator x_rand_r2{ dimensions.x_min_r2, dimensions.x_max_r2 };
        uniform_real_generator y_rand_r2{ dimensions.y_min_r2, dimensions.y_max_r2 };

        std::vector<globe_point_pair> points;
        points.reserve(pair_count);

        for (int i = 0; i < pair_count; ++i)
        {
            auto point_pair = globe_point_pair
            {
                .point1 = { .x = x_rand_r1(), .y = y_rand_r1() },
                .point2 = { .x = x_rand_r2(), .y = y_rand_r2() }
            };

            points.push_back(point_pair);
        }

        // save coordinates to a json file
        constexpr auto data_filename = "coordinates.json";
        save_haversine_json(data_filename, points);

        std::cout << "Saved memory to '" << data_filename << "'.\n";
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
