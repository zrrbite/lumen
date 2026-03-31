#pragma once

/// STM32F429-DISCO Board Configuration for Lumen.
///
/// Display: ILI9341 240x320 via SPI5
/// SPI5 pins: PF7 (SCK), PF8 (MISO), PF9 (MOSI)
/// LCD_CS:  PC2
/// LCD_DCX: PD13

#include "drivers/display/ili9341_spi.hpp"
#include "lumen/hal/input_driver.hpp"
#include "lumen/hal/os/bare_metal.hpp"
#include "lumen/hal/stm32/gpio.hpp"
#include "lumen/hal/stm32/spi.hpp"
#include "lumen/hal/tick_source.hpp"
#include "lumen/hal/touch_driver.hpp"

namespace lumen::platform {

// F429-DISCO pin definitions
using namespace hal::stm32;

// SPI5 pins (AF5)
using SPI5_SCK	= GpioPin<Port::F, 7>;
using SPI5_MISO = GpioPin<Port::F, 8>;
using SPI5_MOSI = GpioPin<Port::F, 9>;

// LCD control pins
using LCD_CS  = GpioPin<Port::C, 2>;
using LCD_DCX = GpioPin<Port::D, 13>;
using LCD_RST = GpioPin<Port::D, 12>; // Active-low reset

// SPI5 peripheral
using SPI5 = Spi<SpiInstance::SPI5>;

// ILI9341 display driver
using Ili9341 = drivers::Ili9341Spi<SPI5, LCD_DCX, LCD_CS, LCD_RST>;

/// Blocking millisecond delay using SysTick.
/// Must be set up before use.
extern hal::SysTickSource sys_tick;
inline void delay_ms(uint32_t ms)
{
	sys_tick.delay(ms);
}

struct Stm32f429DiscoConfig
{
	using PixFmt  = Rgb565;
	using Display = Ili9341;
	using Touch	  = hal::NullTouch; // No touch on basic ILI9341
	using Input	  = hal::NullInput;
	using Tick	  = hal::SysTickSource;
	using OS	  = hal::BareMetalOS;

	static constexpr size_t framebuffer_count	= 0;   // Direct-to-display (no FB)
	static constexpr size_t scratch_buffer_size = 960; // 240px * 2 bytes = 1 line
	static constexpr bool use_external_ram		= false;

	Display display{delay_ms};
	Touch touch;
	Input input;
	Tick& tick = sys_tick;

	void init_hardware()
	{
		// Enable GPIO clocks
		enable_gpio_clock(Port::C);
		enable_gpio_clock(Port::D);
		enable_gpio_clock(Port::F);

		// Configure SPI5 pins as AF5
		SPI5_SCK::init_af(5);
		SPI5_MISO::init_af(5);
		SPI5_MOSI::init_af(5);

		// Configure control pins as outputs
		LCD_CS::init_output();
		LCD_DCX::init_output();
		LCD_RST::init_output();

		// Init SPI5 (master, ~11MHz at 180MHz/16)
		SPI5::init_master(3); // prescaler /16

		// Init display
		display.init();
	}

	/// Always returns true (no windowing system to close).
	bool poll_events() { return true; }
};

} // namespace lumen::platform
