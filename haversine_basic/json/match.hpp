#ifndef WS_MATCH_HPP
#define WS_MATCH_HPP

#include <exception>
#include <type_traits>
#include <variant>

template<typename... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};

template<typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

auto match(auto&& f, const auto& v)
{
    constexpr int max_size = 8;
    constexpr auto size = std::variant_size_v<std::remove_cvref_t<decltype(v)>>;
    static_assert(size <= max_size, "Variant has more alternatives than match() can handle.");

    switch (v.index())
    {
        case 0: return std::forward<decltype(f)>(f)(*std::get_if<0>(&v));
        case 1: return std::forward<decltype(f)>(f)(*std::get_if<1>(&v));
        case 2: return std::forward<decltype(f)>(f)(*std::get_if<2>(&v));
        case 3: return std::forward<decltype(f)>(f)(*std::get_if<3>(&v));
        case 4: return std::forward<decltype(f)>(f)(*std::get_if<4>(&v));
        case 5: return std::forward<decltype(f)>(f)(*std::get_if<5>(&v));
        case 6: return std::forward<decltype(f)>(f)(*std::get_if<6>(&v));
        case 7: return std::forward<decltype(f)>(f)(*std::get_if<7>(&v));
        default: throw std::exception{ "Could not match variant alternative." }; // unreachable
    }
}

#endif
