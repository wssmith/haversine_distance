#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <vector>

#include "random_generator.hpp"

namespace
{
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

    void write_point(std::ofstream& output_stream, globe_point point)
    {
        output_stream << R"({ "lat": )" << point.y << R"(, "lon": )" << point.x << " }";
    }

    void write_point_pair(std::ofstream& output_stream, const globe_point_pair& point_pair)
    {
        output_stream << R"({ "p1": )";
        write_point(output_stream, point_pair.point1);
        output_stream << R"(, "p2": )";
        write_point(output_stream, point_pair.point2);
        output_stream << " }";
    }

    void save_haversine_json(const char* path, const std::vector<globe_point_pair>& data)
    {
        std::ofstream output_stream{ path };

        output_stream << R"({\n  "coordinates": [\n)";

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
}

int main()
{
    try
    {
        // generate pairs of coordinates
        constexpr double x_min = 0.0;
        constexpr double x_max = 180.0;
        constexpr double y_min = 0.0;
        constexpr double y_max = 90.0;
        constexpr int pair_count = 100;

        uniform_real_generator x_rand{ x_min, x_max };
        uniform_real_generator y_rand{ y_min, y_max };

        std::vector<globe_point_pair> points;
        points.reserve(pair_count);

        for (int i = 0; i < pair_count; ++i)
        {
            auto point_pair = globe_point_pair
            {
                .point1 = { .x = x_rand(), .y = y_rand() },
                .point2 = { .x = x_rand(), .y = y_rand() }
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
