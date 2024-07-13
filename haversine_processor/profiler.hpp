#ifndef WS_PROFILER_HPP
#define WS_PROFILER_HPP

#include <cstdint>
#include <exception>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

#include "platform_metrics.hpp"

// todo: add functions returning wall clock time

// Records the duration of a block of code in CPU time. Not thread-safe.
class profiler final
{
public:
    explicit profiler(std::string operation_name)
        : m_operation_name{ std::move(operation_name) }
    {
        // prevent same-operation nesting
        if (m_active[m_operation_name])
            throw std::exception{ "Profiler does not allow same-operation nesting." };

        m_active[m_operation_name] = true;

        // start the timer
        m_start_time = read_cpu_timer();
    }

    ~profiler()
    {
        const uint64_t end_time = read_cpu_timer();
        const uint64_t elapsed_time = end_time - m_start_time;

        try
        {
            m_active[m_operation_name] = false;
            m_profiles[m_operation_name].push_back(elapsed_time);
        }
        catch (std::exception& ex)
        {
            // discard the measurement when this fails
        }
    }

    profiler(const profiler&) = delete;
    profiler& operator=(const profiler&) = delete;
    profiler(profiler&&) noexcept = delete;
    profiler& operator=(profiler&&) noexcept = delete;

    static uint64_t get_total_duration(const std::string& operation_name)
    {
        // todo: check if the profile exists

        const std::vector<uint64_t>& profile = m_profiles[operation_name];
        const uint64_t total_duration = std::accumulate(profile.begin(), profile.end(), 0ULL);
        return total_duration;
    }

    static uint64_t get_execution_count(const std::string& operation_name)
    {
        // todo: check if the profile exists
        // todo: combine get_total_duration and get_execution_count into a single function

        return m_profiles[operation_name].size();
    }

    static double get_mean_duration(const std::string& operation_name)
    {
        const uint64_t total_duration = get_total_duration(operation_name);
        const uint64_t execution_count = get_execution_count(operation_name);

        return (1.0 * total_duration) / execution_count;
    }

private:
    std::string m_operation_name;
    uint64_t m_start_time;

    inline static std::unordered_map<std::string, std::vector<uint64_t>> m_profiles;
    inline static std::unordered_map<std::string, bool> m_active;
};

#endif
