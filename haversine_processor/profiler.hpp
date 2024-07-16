#ifndef WS_PROFILER_HPP
#define WS_PROFILER_HPP

#include <array>
#include <cstdint>
#include <exception>
#include <numeric>
#include <string>
#include <vector>

#include "platform_metrics.hpp"

struct profile_block
{
    const char* name = nullptr;
    profile_block* parent = nullptr;
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
            if (m_block_count >= max_blocks)
                throw std::exception{ "Too many profiler blocks" };

            m_blocks[m_block_count] = profile_block
            {
                .name = operation_name,
                .parent = m_current_block
            };
            new_block = &m_blocks[m_block_count];

            ++m_block_count;
        }

        m_current_block = new_block;
        m_start_time = read_cpu_timer();
    }

    ~profiler()
    {
        const uint64_t end_time = read_cpu_timer();
        const uint64_t elapsed_time = end_time - m_start_time;

        m_current_block->duration += elapsed_time;
        m_current_block->hit_count += 1;

        m_current_block = m_current_block->parent;
    }

    profiler(const profiler&) = delete;
    profiler& operator=(const profiler&) = delete;
    profiler(profiler&&) noexcept = delete;
    profiler& operator=(profiler&&) noexcept = delete;

    static std::vector<profile_block> get_profile_blocks()
    {
        return std::vector<profile_block>{ m_blocks.begin(), m_blocks.begin() + m_block_count };
    }

private:
    uint64_t m_start_time{};
    
    inline constexpr static size_t max_blocks = 1024;
    inline static std::array<profile_block, max_blocks> m_blocks{};
    inline static size_t m_block_count = 0;

    inline static profile_block* m_current_block = nullptr;
};

#endif
