#include "haversine_formula.hpp"

#include <cmath>

namespace
{
    double square(double x)
    {
        return x * x;
    }

    double radians_from_degrees(double degrees)
    {
        return 0.01745329251994329577 * degrees;
    }
}

double reference_haversine(double x0, double y0, double x1, double y1, double earth_radius)
{
    const double d_lat = radians_from_degrees(y1 - y0);
    const double d_lon = radians_from_degrees(x1 - x0);
    const double lat1 = radians_from_degrees(y0);
    const double lat2 = radians_from_degrees(y1);

    const double a = square(std::sin(d_lat / 2.0)) + std::cos(lat1) * std::cos(lat2) * square(std::sin(d_lon / 2.0));
    const double c = 2.0 * std::asin(std::sqrt(a));

    return earth_radius * c;
}
