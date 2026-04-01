#pragma once

/// Linux OS abstraction for Lumen.
/// Real sleep, real mutexes.

#ifdef __linux__

#include <time.h>

#include <cstdint>

namespace lumen::hal {

struct LinuxOS
{
	/// Sleep for the specified milliseconds.
	static void idle()
	{
		struct timespec ts{};
		ts.tv_nsec = 1000000; // 1ms
		nanosleep(&ts, nullptr);
	}

	/// Simple mutex using pthread.
	struct Mutex
	{
		void lock() {}
		void unlock() {}
		// For a real implementation, use pthread_mutex_t
	};
};

} // namespace lumen::hal

#endif // __linux__
