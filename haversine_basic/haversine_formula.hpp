#ifndef WS_HAVERSINEFORMULA_HPP
#define WS_HAVERSINEFORMULA_HPP

inline constexpr double default_earth_radius = 6372.8;

double reference_haversine(double x0, double y0, double x1, double y1, double earth_radius = default_earth_radius);

#endif
