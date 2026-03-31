#pragma once

#define LUMEN_PLATFORM_DESKTOP 1

#include "drivers/display/sdl2_sim.hpp"
#include "drivers/touch/sdl2_mouse.hpp"
#include "lumen/hal/input_driver.hpp"
#include "lumen/hal/os/bare_metal.hpp"
#include "lumen/hal/tick_source.hpp"

namespace lumen::platform {

/// Desktop simulator BoardConfig.
/// Renders to an SDL2 window with mouse input as touch.
template <uint16_t W = 480, uint16_t H = 272> struct DesktopSdl2Config
{
	using PixFmt  = Rgb565;
	using Display = drivers::Sdl2Display<W, H, PixFmt>;
	using Touch	  = drivers::Sdl2Mouse<Display>;
	using Input	  = hal::NullInput;
	using Tick	  = hal::StdChronoTick;
	using OS	  = hal::BareMetalOS;

	static constexpr size_t framebuffer_count	= 1;
	static constexpr size_t scratch_buffer_size = 4096;
	static constexpr bool use_external_ram		= false;
	static constexpr bool has_dma2d				= false;

	/// Software buffer clear (no DMA2D on desktop).
	static void hw_fill(uint16_t* buf, uint16_t width, uint16_t height, uint16_t color)
	{
		uint32_t count = static_cast<uint32_t>(width) * height;
		for (uint32_t i = 0; i < count; ++i)
			buf[i] = color;
	}

	Display display;
	Touch touch{display};
	Input input;
	Tick tick;

	void init_hardware()
	{
		tick.init();
		display.init();
		touch.init();
		input.init();
	}

	/// Process SDL events. Returns false if window was closed.
	bool poll_events() { return display.poll_events(); }
};

} // namespace lumen::platform
