#ifndef WS_PROFILER_HPP
#define WS_PROFILER_HPP

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include "platform_metrics.hpp"

#ifndef PROFILER
#define PROFILER 0
#endif

#if PROFILER

#if _MSC_VER
#define FUNCTION_NAME __FUNCTION__
#else
#define FUNCTION_NAME __func__
#endif

#define CONCAT_CORE(a, b) a##b
#define CONCAT(a, b) CONCAT_CORE(a, b)

#define PROFILE_BLOCK(name) static constexpr char CONCAT(anchor, __LINE__)[] = FUNCTION_NAME; profile_block CONCAT(activity, __LINE__){ (name), anchor_id<CONCAT(anchor, __LINE__)> };
#define PROFILE_FUNCTION PROFILE_BLOCK(__func__)

inline uint32_t anchor_id_counter = 1;

template<const char* AnchorName>
inline const uint32_t anchor_id = anchor_id_counter++;

#else

#define PROFILE_BLOCK(...)
#define PROFILE_FUNCTION

#endif

// stores information about a single profiling unit
struct profile_anchor
{
    const char* name = nullptr;
    uint64_t exclusive_duration{};
    uint64_t inclusive_duration{};
    uint64_t hit_count{};
};

class profiler
{
    friend class profile_block;

private:
    inline static uint64_t overall_start_time{};
    inline static uint64_t overall_end_time{};

    inline constexpr static size_t max_anchors = 1024;
    inline static std::array<profile_anchor, 1024> anchors{};

public:
    static std::array<profile_anchor, max_anchors>& get_anchors()
    {
        return anchors;
    }

    static uint64_t get_overall_duration()
    {
        return overall_end_time - overall_start_time;
    }

    static void start_profiling()
    {
        overall_start_time = read_cpu_timer();
    }

    static void stop_profiling()
    {
        overall_end_time = read_cpu_timer();
    }

    static void print_results();
};

// Records the duration of a block of code in CPU time. Not thread-safe.
class profile_block final
{
private:
    const char* m_operation_name = nullptr;
    uint64_t m_start_time{};
    uint64_t m_prev_inclusive_duration{};
    uint32_t m_parent_index{};
    uint32_t m_anchor_index{};

    inline static uint32_t m_global_parent_index = 0;

    using p = profiler;

public:
    profile_block(const char* operation_name, uint32_t anchor_index)
    {
        assert(anchor_index < p::max_anchors, "Too many profile anchors");

        m_parent_index = m_global_parent_index;
        m_anchor_index = anchor_index;
        m_operation_name = operation_name;

        const profile_anchor& anchor = p::anchors[m_anchor_index];
        m_prev_inclusive_duration = anchor.inclusive_duration;

        m_global_parent_index = m_anchor_index;
        m_start_time = read_cpu_timer();
    }

    ~profile_block()
    {
        const uint64_t end_time = read_cpu_timer();
        const uint64_t elapsed_time = end_time - m_start_time;

        m_global_parent_index = m_parent_index;

        profile_anchor& parent = p::anchors[m_parent_index];
        profile_anchor& anchor = p::anchors[m_anchor_index];

        parent.exclusive_duration -= elapsed_time;
        anchor.exclusive_duration += elapsed_time;
        anchor.inclusive_duration = m_prev_inclusive_duration + elapsed_time;
        ++anchor.hit_count;

        anchor.name = m_operation_name;
    }

    profile_block(const profile_block&) = delete;
    profile_block& operator=(const profile_block&) = delete;
    profile_block(profile_block&&) noexcept = delete;
    profile_block& operator=(profile_block&&) noexcept = delete;
};

#endif
