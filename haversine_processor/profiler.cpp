#include "profiler.hpp"

#include <format>
#include <iostream>
#include <locale>

#if PROFILER
#include <iomanip>
#define PRINT_ANCHORS(...) print_anchors(__VA_ARGS__)
#else
#define PRINT_ANCHORS(...)
#endif

#if PROFILER
namespace
{
    void print_anchor(const profile_anchor& anchor, uint64_t cpu_freq, uint64_t overall_duration)
    {
        const double exclusive_duration_ms = 1000.0 * anchor.exclusive_duration / cpu_freq;
        const double exclusive_percent = 100.0 * anchor.exclusive_duration / overall_duration;

        constexpr int column_width = 35;
        std::cout << std::left << std::setw(column_width) << std::fixed << std::setfill(' ');
        std::cout << std::format(std::locale("en_US"), "  {}[{:Ld}]: ", anchor.name, anchor.hit_count);

        if (anchor.inclusive_duration == anchor.exclusive_duration)
        {
            std::cout << std::format("{:.4f} ms ({:.2f}%)\n", exclusive_duration_ms, exclusive_percent);
        }
        else
        {
            const double inclusive_duration_ms = 1000.0 * anchor.inclusive_duration / cpu_freq;
            const double inclusive_percent = 100.0 * anchor.inclusive_duration / overall_duration;

            std::cout << std::format("{:.4f} ms ({:.2f}%, {:.2f}% w/ children)\n", exclusive_duration_ms, exclusive_percent, inclusive_percent);
        }
    }

    void print_anchors(uint64_t cpu_freq, uint64_t overall_duration)
    {
        const auto& anchors = profiler::get_anchors();

        std::cout << "\nProfiles:\n";

        for (const profile_anchor& anchor : anchors)
        {
            print_anchor(anchor, cpu_freq, overall_duration);
        }
    }
}
#endif

void profiler::print_results()
{
    const uint64_t cpu_freq = estimate_cpu_timer_freq();

    if (!cpu_freq)
    {
        std::cout << "Failed to estimate CPU frequency.\n";
        return;
    }

    const uint64_t overall_duration = get_overall_duration();

    const double overall_duration_ms = 1000.0 * overall_duration / cpu_freq;
    std::cout << std::format(std::locale("en_US"), "Total time: {:.4f} ms (CPU freq {:Ld})\n", overall_duration_ms, cpu_freq);

    PRINT_ANCHORS(cpu_freq, overall_duration);
}
