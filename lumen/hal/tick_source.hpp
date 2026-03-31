#pragma once

/// Tick source concepts for Lumen.
///
/// A tick source must provide:
///   TickMs now();        — current time in milliseconds
///   void  delay(TickMs); — blocking delay (used sparingly)
///
/// Implementations provided:
///   - SysTickSource     (bare metal, Cortex-M SysTick)
///   - StdChronoTick     (desktop simulator)
///   - FreeRtosTick      (FreeRTOS wrapper)

#include "lumen/core/types.hpp"

#if defined(LUMEN_PLATFORM_DESKTOP)
#include <chrono>
#include <thread>
#endif

namespace lumen::hal {

/// Desktop tick source using std::chrono.
#if defined(LUMEN_PLATFORM_DESKTOP) || defined(__linux__) || defined(_WIN32) || defined(__APPLE__)

class StdChronoTick
{
  public:
	void init() { start_ = std::chrono::steady_clock::now(); }

	TickMs now() const
	{
		auto elapsed = std::chrono::steady_clock::now() - start_;
		return static_cast<TickMs>(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
	}

	void delay(TickMs ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

  private:
	std::chrono::steady_clock::time_point start_;
};

#endif

/// SysTick source for bare metal Cortex-M.
/// Users must call SysTickSource::increment() from SysTick_Handler().
class SysTickSource
{
  public:
	void init() { tick_count_ = 0; }

	TickMs now() const { return tick_count_; }

	void delay(TickMs ms)
	{
		TickMs start = tick_count_;
		while (tick_count_ - start < ms)
		{
			// Busy wait — prefer scheduler-based delays
		}
	}

	/// Call from SysTick_Handler() ISR.
	void increment() { ++tick_count_; }

  private:
	volatile TickMs tick_count_ = 0;
};

} // namespace lumen::hal
