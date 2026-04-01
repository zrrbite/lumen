#pragma once

/// Raspberry Pi 4 Board Configuration for Lumen.
/// Display: DRM/KMS via /dev/dri/card*
/// Touch:   evdev via /dev/input/event*

#ifdef __linux__

#include "drivers/display/linux_drm.hpp"
#include "drivers/touch/linux_evdev.hpp"
#include "lumen/hal/input_driver.hpp"
#include "lumen/hal/os/linux.hpp"
#include "lumen/hal/tick_source.hpp"

namespace lumen::platform {

struct RaspberryPiConfig
{
	using PixFmt  = Argb8888;
	using Display = drivers::LinuxDrm;
	using Touch	  = drivers::LinuxEvdev;
	using Input	  = hal::NullInput;
	using Tick	  = hal::StdChronoTick;
	using OS	  = hal::LinuxOS;

	static constexpr size_t framebuffer_count	= 2; // Double buffered
	static constexpr size_t scratch_buffer_size = 0;
	static constexpr bool use_external_ram		= false;
	static constexpr bool has_dma2d				= false;

	/// Software buffer fill (no DMA2D on Linux).
	static void hw_fill(uint32_t* buf, uint16_t width, uint16_t height, uint32_t color)
	{
		uint32_t count = static_cast<uint32_t>(width) * height;
		for (uint32_t i = 0; i < count; ++i)
			buf[i] = color;
	}

	Display display;
	Touch touch;
	Input input;
	Tick tick;

	void init_hardware()
	{
		tick.init();

		if (!display.init())
		{
			// Could not open DRM — fatal
			return;
		}

		touch.set_display_size(display.width(), display.height());
		touch.init();
	}

	bool poll_events() { return true; }
};

} // namespace lumen::platform

#endif // __linux__
