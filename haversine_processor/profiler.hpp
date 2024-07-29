#ifndef WS_PROFILER_HPP
#define WS_PROFILER_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <span>

#include "platform_metrics.hpp"
#include "profiler_containers.hpp"

#define CONCAT_CORE(a, b) a##b
#define CONCAT(a, b) CONCAT_CORE(a, b)

#define PROFILE_BLOCK(name) profile_block CONCAT(activity, __LINE__){ (name) }
#define PROFILE_FUNCTION PROFILE_BLOCK(__func__)

// stores information about a single profiling unit
struct profile_anchor
{
    const char* name = nullptr;
    uint64_t duration_exclusive{};
    uint64_t hit_count{};
};

// Records the duration of a block of code in CPU time. Not thread-safe.
class profile_block final
{
private:
    uint64_t m_start_time{};

    inline static uint64_t m_overall_start_time{};
    inline static uint64_t m_overall_end_time{};
    inline static bool m_profiling = false;

    inline constexpr static size_t max_anchors = 1024;
    inline constexpr static size_t max_depth = 1024;

    inline static profiler_array<profile_anchor, max_anchors> m_blocks{};
    inline static profiler_stack<profile_anchor*, max_depth> m_anchor_stack{};

public:
    explicit profile_block(const char* operation_name)
    {
        if (m_blocks.size() >= decltype(m_blocks)::max_size())
            throw std::exception{ "Too many profiler blocks" };

        if (m_anchor_stack.size() >= decltype(m_anchor_stack)::max_size())
            throw std::exception{ "Too many nested profiler blocks" };

        profile_anchor* new_block = nullptr;
        for (profile_anchor& m_block : m_blocks)
        {
            if (std::strcmp(m_block.name, operation_name) == 0)
            {
                new_block = &m_block;
                break;
            }
        }

        if (new_block == nullptr)
        {
            m_blocks.push_back({ .name = operation_name });
            new_block = &m_blocks.back();
        }

        m_anchor_stack.push(new_block);

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

    ~profile_block()
    {
        const uint64_t end_time = read_cpu_timer();
        m_overall_end_time = end_time;
        const uint64_t elapsed_time = end_time - m_start_time;

        profile_anchor* current_block = m_anchor_stack.top();
        m_anchor_stack.pop();

        current_block->duration_exclusive += elapsed_time;
        current_block->hit_count += 1;

        if (!m_anchor_stack.empty())
        {
            profile_anchor* parent = m_anchor_stack.top();
            parent->duration_exclusive -= elapsed_time;
        }
    }

    profile_block(const profile_block&) = delete;
    profile_block& operator=(const profile_block&) = delete;
    profile_block(profile_block&&) noexcept = delete;
    profile_block& operator=(profile_block&&) noexcept = delete;

    static std::span<profile_anchor> get_profile_blocks()
    {
        return { m_blocks.begin(), m_blocks.end() };
    }

    static uint64_t get_overall_duration()
    {
        return m_overall_end_time - m_overall_start_time;
    }
};

#endif
