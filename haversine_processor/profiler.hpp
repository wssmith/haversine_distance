#ifndef WS_PROFILER_HPP
#define WS_PROFILER_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <vector>

#include "platform_metrics.hpp"

#define CONCAT_CORE(a, b) a##b
#define CONCAT(a, b) CONCAT_CORE(a, b)

#define PROFILE_BLOCK(name) profiler CONCAT(activity, __LINE__){ (name) }
#define PROFILE_FUNCTION PROFILE_BLOCK(__func__)

template<typename T, size_t N>
class profile_stack
{
public:
    void push(T* block)
    {
        if (m_size >= N)
            throw std::exception{ "Profile stack overflow" };

        m_profiles[m_size] = block;
        ++m_size;
    }

    T* pop()
    {
        if (m_size == 0)
            throw std::exception{ "Profile stack underflow" };

        --m_size;

        return m_profiles[m_size];
    }

    T* top()
    {
        if (m_size == 0)
            throw std::exception{ "Profile stack underflow" };

        return m_profiles[m_size - 1];
    }

    T* operator[](size_t index)
    {
        if (index >= m_size || index < 0)
            throw std::exception{ "Index out of bounds" };

        return m_profiles[index];
    }

    size_t size() const
    {
        return m_size;
    }

    [[nodiscard]]
    bool empty() const
    {
        return m_size == 0;
    }

    void clear()
    {
        m_size = 0;
    }

    T* begin()
    {
        return m_profiles[0];
    }

    T* end()
    {
        return m_profiles[m_size];
    }

private:
    std::array<T*, N> m_profiles;
    size_t m_size = 0;
};

struct profile_block
{
    const char* name = nullptr;
    uint64_t duration{};
    uint64_t hit_count{};
};

// Records the duration of a block of code in CPU time. Not thread-safe.
class profiler final
{
public:
    explicit profiler(const char* operation_name)
    {
        profile_block* new_block = nullptr;
        for (size_t i = 0; i < m_block_count; ++i)
        {
            if (std::strcmp(m_blocks[i].name, operation_name) == 0)
            {
                new_block = &m_blocks[i];
                break;
            }
        }

        if (new_block == nullptr)
        {
            if (m_block_count >= m_blocks.size())
                throw std::exception{ "Too many profiler blocks" };

            m_blocks[m_block_count] = profile_block
            {
                .name = operation_name
            };

            new_block = &m_blocks[m_block_count];
            ++m_block_count;
        }

        if (m_current_block)
            m_profile_stack.push(m_current_block);

        m_current_block = new_block;

        if (!m_profiling)
        {
            m_profiling = true;
            m_start_time = read_cpu_timer();
            m_overall_start_time = m_start_time;
        }
        else
        {
            m_start_time = read_cpu_timer();
        }
    }

    ~profiler()
    {
        const uint64_t end_time = read_cpu_timer();
        m_overall_end_time = end_time;
        const uint64_t elapsed_time = end_time - m_start_time;

        m_current_block->duration += elapsed_time;
        m_current_block->hit_count += 1;

        if (!m_profile_stack.empty())
        {
            profile_block* parent = m_profile_stack.pop();
            parent->duration -= elapsed_time;
            m_current_block = parent;
        }
        else
        {
            m_current_block = nullptr;
        }
    }

    profiler(const profiler&) = delete;
    profiler& operator=(const profiler&) = delete;
    profiler(profiler&&) noexcept = delete;
    profiler& operator=(profiler&&) noexcept = delete;

    static std::vector<profile_block> get_profile_blocks()
    {
        return std::vector<profile_block>{ m_blocks.begin(), m_blocks.begin() + m_block_count };
    }

    static uint64_t get_overall_duration()
    {
        return m_overall_end_time - m_overall_start_time;
    }

private:
    uint64_t m_start_time{};

    inline static uint64_t m_overall_start_time{};
    inline static uint64_t m_overall_end_time{};
    inline static bool m_profiling = false;

    inline constexpr static size_t max_blocks = 1024;
    inline static std::array<profile_block, max_blocks> m_blocks{};
    inline static size_t m_block_count = 0;

    inline static profile_stack<profile_block, max_blocks> m_profile_stack{};

    inline static profile_block* m_current_block = nullptr;
};

#endif
