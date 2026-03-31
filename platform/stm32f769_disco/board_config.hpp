#pragma once

/// STM32F769-DISCO Board Configuration for Lumen.
///
/// Display: OTM8009A 800x480 via MIPI-DSI + LTDC
/// Touch:   FT6206 capacitive via I2C4
/// SDRAM:   16MB (IS42S32800G) via FMC — used for framebuffers
///
/// This board uses a framebuffer in SDRAM. The LTDC peripheral
/// continuously refreshes the display from the framebuffer via DMA.

#include <cstddef>

#include "drivers/display/ltdc_dsi.hpp"
#include "drivers/touch/ft6206.hpp"
#include "lumen/hal/input_driver.hpp"
#include "lumen/hal/os/bare_metal.hpp"
#include "lumen/hal/stm32/i2c.hpp"
#include "lumen/hal/tick_source.hpp"

namespace lumen::platform {

using namespace hal::stm32;

// I2C4 for touch (PA14 = SCL, PB9 = SDA on F769-DISCO)
// Note: I2C4 base = 0x40006000, clock via RCC_APB1ENR bit 24
using I2C4Drv = I2c<I2cInstance::I2C3>; // TODO: add I2C4 instance

// Drivers
using Display769	 = drivers::LtdcDsi<Rgb565>;
using TouchDriver769 = drivers::Ft6206<I2C4Drv>;

extern hal::SysTickSource sys_tick;
inline void delay_ms(uint32_t ms)
{
	sys_tick.delay(ms);
}

struct Stm32f769DiscoConfig
{
	using PixFmt  = Rgb565;
	using Display = Display769;
	using Touch	  = TouchDriver769;
	using Input	  = hal::NullInput;
	using Tick	  = hal::SysTickSource;
	using OS	  = hal::BareMetalOS;

	// 800x480 RGB565 = 768,000 bytes per framebuffer
	static constexpr size_t framebuffer_count	= 1;
	static constexpr size_t scratch_buffer_size = 0; // Not needed with framebuffer
	static constexpr bool use_external_ram		= true;

	Display display;
	Touch touch;
	Input input;
	Tick& tick = sys_tick;

	// Framebuffer in SDRAM (address set during init)
	// SDRAM starts at 0xC0000000 on STM32F7
	static constexpr uint32_t SDRAM_BASE = 0xC0000000;
	Display::pixel_t* fb				 = reinterpret_cast<Display::pixel_t*>(SDRAM_BASE);

	void init_hardware()
	{
		// TODO: Full hardware init sequence:
		// 1. System clock to 216MHz (HSE + PLL)
		// 2. Enable GPIO clocks
		// 3. Configure FMC for SDRAM
		// 4. Configure I2C4 for touch
		// 5. Configure DSI + LTDC
		// 6. Init display with framebuffer
		// 7. Init touch controller

		display.set_framebuffer(fb);
		display.init();
		touch.init();
	}

	bool poll_events() { return true; }
};

} // namespace lumen::platform
