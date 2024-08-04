#ifndef WS_PLATFORMMETRICS_HPP
#define WS_PLATFORMMETRICS_HPP

#include <cstdint>

#ifndef READ_BLOCK_TIMER
#define READ_BLOCK_TIMER read_cpu_timer
#endif

#if _WIN32

#include <intrin.h>
#include <windows.h>

inline uint64_t get_os_timer_freq()
{
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	return freq.QuadPart;
}

inline uint64_t read_os_timer()
{
	LARGE_INTEGER value;
	QueryPerformanceCounter(&value);
	return value.QuadPart;
}

#else

#include <x86intrin.h>
#include <sys/time.h>

inline uint64_t get_os_timer_freq()
{
	return 1000000;
}

inline uint64_t read_os_timer()
{
	timeval value;
	gettimeofday(&value, 0);

	const uint64_t result = get_os_timer_freq() * static_cast<uint64_t>(value.tv_sec) + static_cast<uint64_t>(value.tv_usec);
	return result;
}

#endif

inline uint64_t read_cpu_timer()
{
	return __rdtsc();
}

inline uint64_t estimate_cpu_timer_freq()
{
	constexpr uint64_t milliseconds_to_wait = 100;
	const uint64_t os_freq = get_os_timer_freq();

	const uint64_t cpu_start = READ_BLOCK_TIMER();
	const uint64_t os_start = read_os_timer();
	uint64_t os_elapsed = 0;
	const uint64_t os_wait_time = os_freq * milliseconds_to_wait / 1000;

	while (os_elapsed < os_wait_time)
	{
		const uint64_t os_end = read_os_timer();
		os_elapsed = os_end - os_start;
	}

	const uint64_t cpu_end = READ_BLOCK_TIMER();
	const uint64_t cpu_elapsed = cpu_end - cpu_start;

	uint64_t cpu_freq = 0;
	if (os_elapsed)
	{
		cpu_freq = os_freq * (cpu_elapsed / os_elapsed);
	}

	return cpu_freq;
}

#endif
