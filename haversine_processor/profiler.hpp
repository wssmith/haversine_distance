﻿#ifndef WS_PROFILER_HPP
#define WS_PROFILER_HPP

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include "platform_metrics.hpp"
#include "profiler_containers.hpp"

#ifndef PROFILER
#define PROFILER 0
#endif

#if PROFILER
#define CONCAT_CORE(a, b) a##b
#define CONCAT(a, b) CONCAT_CORE(a, b)
#define PROFILE_BLOCK(name) static constexpr char CONCAT(anchor, __LINE__)[] = "name"; profile_block CONCAT(activity, __LINE__){ (name), anchor_id<CONCAT(anchor, __LINE__)> };
#define PROFILE_FUNCTION PROFILE_BLOCK(__func__)
#else
#define PROFILE_BLOCK(...)
#define PROFILE_FUNCTION
#endif

inline uint64_t anchor_id_counter = 0;

template<const char* AnchorName>
inline const uint64_t anchor_id = anchor_id_counter++;

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
    profile_anchor* m_anchor = nullptr;
    uint64_t m_anchor_index = 0;
    uint64_t m_start_time{};
    uint64_t m_prev_inclusive_duration{};

    inline constexpr static size_t max_depth = 1024;
    inline static profiler_stack<profile_anchor*, max_depth> anchor_stack{};

    using p = profiler;

public:
    explicit profile_block(const char* operation_name, uint64_t anchor_index)
    {
        assert(anchor_index < p::max_anchors, "Too many profile anchors");
        assert(anchor_stack.size() < decltype(anchor_stack)::max_size(), "Too many nested profiler blocks");

        m_anchor = &p::anchors[anchor_index];
        m_anchor->name = operation_name;
        m_prev_inclusive_duration = m_anchor->inclusive_duration;

        anchor_stack.push(m_anchor);

        m_start_time = read_cpu_timer();
    }

    ~profile_block()
    {
        const uint64_t end_time = read_cpu_timer();
        const uint64_t elapsed_time = end_time - m_start_time;

        m_anchor->exclusive_duration += elapsed_time;
        m_anchor->inclusive_duration = m_prev_inclusive_duration + elapsed_time;
        ++m_anchor->hit_count;

        anchor_stack.pop();

        if (!anchor_stack.empty())
        {
            profile_anchor* parent = anchor_stack.top();
            parent->exclusive_duration -= elapsed_time;
        }
    }

    profile_block(const profile_block&) = delete;
    profile_block& operator=(const profile_block&) = delete;
    profile_block(profile_block&&) noexcept = delete;
    profile_block& operator=(profile_block&&) noexcept = delete;
};

void print_profiler_results();

#endif
