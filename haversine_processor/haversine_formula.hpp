#ifndef WS_HAVERSINEFORMULA_HPP
#define WS_HAVERSINEFORMULA_HPP

struct globe_point
{
    double x{};
    double y{};
};

inline constexpr double default_earth_radius = 6372.8;

double haversine_distance(double x0, double y0, double x1, double y1, double earth_radius = default_earth_radius);
double haversine_distance(globe_point p0, globe_point p1, double earth_radius = default_earth_radius);

#endif
