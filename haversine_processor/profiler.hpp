#ifndef WS_PROFILER_HPP
#define WS_PROFILER_HPP

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include "platform_metrics.hpp"

#ifndef PROFILER
#define PROFILER 0
#endif

#if PROFILER

#define CONCAT_CORE(a, b) a##b
#define CONCAT(a, b) CONCAT_CORE(a, b)
#define VAR_NAME(x) CONCAT(x, __LINE__)

#define PROFILE_DATA_BLOCK(name, data_processed) profile_block VAR_NAME(activity){ (name), anchor_id<structural_string{ (name) }>, (data_processed) };
#define PROFILE_DATA_FUNCTION(data_processed) PROFILE_DATA_BLOCK(__func__, (data_processed))

#define PROFILE_BLOCK(name) PROFILE_DATA_BLOCK((name), 0)
#define PROFILE_FUNCTION PROFILE_DATA_FUNCTION(0)

// this generates unique identifiers from anchor names in a way that works across translation units
// it counts template instantiations based on string literal _content_ using c++20 structural nttp

inline uint32_t anchor_id_counter = 1; // 0 is reserved for "no anchor"

template<auto AnchorName>
inline const uint32_t anchor_id = anchor_id_counter++;

template<size_t N>
struct structural_string
{
    char chars[N];
};

template<size_t N>
structural_string(const char(&)[N]) -> structural_string<N>;

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
    uint64_t data_processed{};
};

class profiler
{
    friend class profile_block;

private:
    inline static uint64_t overall_start_time{};
    inline static uint64_t overall_end_time{};

    inline constexpr static size_t max_anchors = 1024;
    inline static std::array<profile_anchor, max_anchors> anchors{};

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
        overall_start_time = READ_BLOCK_TIMER();
    }

    static void stop_profiling()
    {
        overall_end_time = READ_BLOCK_TIMER();
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
    uint64_t m_data_processed{};
    uint32_t m_parent_index{};
    uint32_t m_anchor_index{};

    inline static uint32_t m_global_parent_index = 0;

    using p = profiler;

public:
    profile_block(const char* operation_name, uint32_t anchor_index, uint64_t data_processed)
    {
        assert(anchor_index < p::max_anchors, "Too many profile anchors");

        m_parent_index = m_global_parent_index;
        m_anchor_index = anchor_index;
        m_operation_name = operation_name;
        m_data_processed = data_processed;

        const profile_anchor& anchor = p::anchors[m_anchor_index];
        m_prev_inclusive_duration = anchor.inclusive_duration;

        m_global_parent_index = m_anchor_index;
        m_start_time = READ_BLOCK_TIMER();
    }

    ~profile_block()
    {
        const uint64_t end_time = READ_BLOCK_TIMER();
        const uint64_t elapsed_time = end_time - m_start_time;

        m_global_parent_index = m_parent_index;

        profile_anchor& parent = p::anchors[m_parent_index];
        parent.exclusive_duration -= elapsed_time;

        profile_anchor& anchor = p::anchors[m_anchor_index];
        anchor.exclusive_duration += elapsed_time;
        anchor.inclusive_duration = m_prev_inclusive_duration + elapsed_time;
        ++anchor.hit_count;
        anchor.data_processed += m_data_processed;
        anchor.name = m_operation_name;
    }

    profile_block(const profile_block&) = delete;
    profile_block& operator=(const profile_block&) = delete;
    profile_block(profile_block&&) noexcept = delete;
    profile_block& operator=(profile_block&&) noexcept = delete;
};

#endif
