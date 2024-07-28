#include "haversine_formula.hpp"

#include <cmath>
#include <numbers>

#include "profiler.hpp"

namespace
{
    double square(double x)
    {
        return x * x;
    }

    double radians_from_degrees(double degrees)
    {
        constexpr double factor = std::numbers::pi / 180.0;
        return factor * degrees;
    }
}

double haversine_distance(double x0, double y0, double x1, double y1, double earth_radius)
{
    PROFILE_FUNCTION;

    const double d_lat = radians_from_degrees(y1 - y0);
    const double d_lon = radians_from_degrees(x1 - x0);
    const double lat1 = radians_from_degrees(y0);
    const double lat2 = radians_from_degrees(y1);

    const double a = square(std::sin(d_lat / 2.0)) + std::cos(lat1) * std::cos(lat2) * square(std::sin(d_lon / 2.0));
    const double c = 2.0 * std::asin(std::sqrt(a));

    return earth_radius * c;
}

double haversine_distance(globe_point p0, globe_point p1, double earth_radius)
{
    return haversine_distance(p0.x, p0.y, p1.x, p1.y, earth_radius);
}
