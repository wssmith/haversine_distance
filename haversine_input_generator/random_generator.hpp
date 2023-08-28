#ifndef WS_RANDOMGENERATOR_HPP
#define WS_RANDOMGENERATOR_HPP

#include <memory>
#include <random>
#include <utility>
#include <type_traits>

template<typename Distribution, typename Engine = std::mt19937>
class random_generator
{
public:
    using result_type = typename Distribution::result_type;
    using seed_type = unsigned int; // same as std::random_device::result_type

    template<typename... Params, typename = std::enable_if_t<(... && std::is_arithmetic_v<std::decay_t<Params>>)>>
    explicit random_generator(Params... args)
    {
        std::random_device seed_generator;
        _impl = std::make_unique<random_generator_impl>(Engine{ seed_generator() }, args...);
    }

    template<typename UnaryOperation, typename = std::enable_if_t<std::is_base_of_v<std::discrete_distribution<>, std::decay_t<Distribution>>>>
    random_generator(size_t nw, double x_min, double x_max, UnaryOperation fw)
    {
        std::random_device seed_generator;
        _impl = std::make_unique<random_generator_impl>(Engine{ seed_generator() }, nw, x_min, x_max, fw);
    }

    ~random_generator() = default;
    random_generator(random_generator&& rhs) noexcept = default;
    random_generator& operator=(random_generator&& rhs) noexcept = default;
    random_generator(const random_generator& rhs) = delete;
    random_generator& operator=(const random_generator& rhs) = delete;

    result_type operator()()
    {
        return _impl->draw_sample();
    }

    void seed(seed_type s)
    {
        _impl->seed(s);
    }

private:
    struct random_generator_impl
    {
        template<typename... Params>
        explicit random_generator_impl(Engine&& engine, Params&&... args)
            : _dist{ std::forward<Params>(args)... }, _engine{ std::forward<Engine>(engine) }
        {
        }

        void seed(const seed_type s)
        {
            _engine.seed(s);
        }

        result_type draw_sample()
        {
            return _dist(_engine);
        }

    private:
        Distribution _dist;
        Engine _engine;
    };

    std::unique_ptr<random_generator_impl> _impl;
};

template<typename TResult = double>
using uniform_real_generator = random_generator<std::uniform_real_distribution<TResult>>;

#endif
