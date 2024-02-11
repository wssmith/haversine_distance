#ifndef WS_MATCH_HPP
#define WS_MATCH_HPP

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
        case 0: std::forward<decltype(f)>(f)(*std::get_if<0>(&v)); break;
        case 1: std::forward<decltype(f)>(f)(*std::get_if<1>(&v)); break;
        case 2: std::forward<decltype(f)>(f)(*std::get_if<2>(&v)); break;
        case 3: std::forward<decltype(f)>(f)(*std::get_if<3>(&v)); break;
        case 4: std::forward<decltype(f)>(f)(*std::get_if<4>(&v)); break;
        case 5: std::forward<decltype(f)>(f)(*std::get_if<5>(&v)); break;
        case 6: std::forward<decltype(f)>(f)(*std::get_if<6>(&v)); break;
        case 7: std::forward<decltype(f)>(f)(*std::get_if<7>(&v)); break;
    }
}

#endif
