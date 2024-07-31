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
    uint64_t exclusive_duration{};
    uint64_t inclusive_duration{};
    uint64_t hit_count{};
};

class profiler final
{
    friend class profile_block;

private:
    inline static uint64_t overall_start_time{};
    inline static uint64_t overall_end_time{};
    inline static bool profiling = false;

    inline constexpr static size_t max_anchors = 1024;
    inline constexpr static size_t max_depth = 1024;

    inline static profiler_array<profile_anchor, max_anchors> anchors{};
    inline static profiler_stack<profile_anchor*, max_depth> anchor_stack{};

public:
    static std::span<profile_anchor> get_anchors()
    {
        return { anchors.begin(), anchors.end() };
    }

    static uint64_t get_overall_duration()
    {
        return overall_end_time - overall_start_time;
    }
};

// Records the duration of a block of code in CPU time. Not thread-safe.
class profile_block final
{
private:
    uint64_t m_start_time{};
    uint64_t m_prev_inclusive_duration{};

    using p = profiler;

public:
    explicit profile_block(const char* operation_name)
    {
        if (p::anchors.size() >= decltype(p::anchors)::max_size())
            throw std::exception{ "Too many profiler anchors" };

        if (p::anchor_stack.size() >= decltype(p::anchor_stack)::max_size())
            throw std::exception{ "Too many nested profiler blocks" };

        profile_anchor* anchor = nullptr;
        for (profile_anchor& a : p::anchors)
        {
            if (std::strcmp(a.name, operation_name) == 0)
            {
                anchor = &a;
                m_prev_inclusive_duration = anchor->inclusive_duration;
                break;
            }
        }

        if (anchor == nullptr)
        {
            p::anchors.push_back(profile_anchor{ .name = operation_name });
            anchor = &p::anchors.back();
        }

        p::anchor_stack.push(anchor);

        if (!p::profiling)
        {
            p::profiling = true;
            m_start_time = read_cpu_timer();
            p::overall_start_time = m_start_time;
        }
        else
        {
            m_start_time = read_cpu_timer();
        }
    }

    ~profile_block()
    {
        const uint64_t end_time = read_cpu_timer();
        p::overall_end_time = end_time;
        const uint64_t elapsed_time = end_time - m_start_time;

        profile_anchor* current_anchor = p::anchor_stack.top();
        p::anchor_stack.pop();

        current_anchor->exclusive_duration += elapsed_time;
        current_anchor->inclusive_duration = m_prev_inclusive_duration + elapsed_time;
        current_anchor->hit_count += 1;

        if (!p::anchor_stack.empty())
        {
            profile_anchor* parent = p::anchor_stack.top();
            parent->exclusive_duration -= elapsed_time;
        }
    }

    profile_block(const profile_block&) = delete;
    profile_block& operator=(const profile_block&) = delete;
    profile_block(profile_block&&) noexcept = delete;
    profile_block& operator=(profile_block&&) noexcept = delete;
};

#endif
