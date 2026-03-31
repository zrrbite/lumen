#pragma once

#include "lumen/core/types.hpp"

namespace lumen::hal {

/// Bare metal OS abstraction — no RTOS, just WFI.
struct BareMetalOS
{
	/// Called when scheduler has no work. Default: busy wait.
	/// On Cortex-M, override to call __WFI() for power saving.
	static void idle()
	{
		// On real hardware: __WFI();
		// On desktop: no-op (scheduler handles timing)
	}

	/// No mutex needed on bare metal (single-threaded).
	struct Mutex
	{
		void lock() {}
		void unlock() {}
	};
};

} // namespace lumen::hal
