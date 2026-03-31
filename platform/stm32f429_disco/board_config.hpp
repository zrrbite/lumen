#pragma once

/// STM32F429-DISCO Board Configuration for Lumen.
///
/// Display: ILI9341 240x320 via SPI5
/// Touch:   STMPE811 resistive via I2C3
/// SPI5:    PF7 (SCK), PF8 (MISO), PF9 (MOSI)
/// I2C3:    PA8 (SCL), PC9 (SDA)
/// LCD_CS:  PC2
/// LCD_DCX: PD13

#include <cstddef>

#include "drivers/display/ili9341_spi.hpp"
#include "drivers/touch/stmpe811.hpp"
#include "lumen/hal/input_driver.hpp"
#include "lumen/hal/os/bare_metal.hpp"
#include "lumen/hal/stm32/dma2d.hpp"
#include "lumen/hal/stm32/gpio.hpp"
#include "lumen/hal/stm32/i2c.hpp"
#include "lumen/hal/stm32/spi.hpp"
#include "lumen/hal/tick_source.hpp"

namespace lumen::platform {

using namespace hal::stm32;

// SPI5 pins (AF5)
using SPI5_SCK	= GpioPin<Port::F, 7>;
using SPI5_MISO = GpioPin<Port::F, 8>;
using SPI5_MOSI = GpioPin<Port::F, 9>;

// LCD control pins
using LCD_CS  = GpioPin<Port::C, 2>;
using LCD_DCX = GpioPin<Port::D, 13>;
using LCD_RST = GpioPin<Port::D, 12>;

// I2C3 pins (AF4) for touch
using I2C3_SCL = GpioPin<Port::A, 8>;
using I2C3_SDA = GpioPin<Port::C, 9>;

// Peripherals
using SPI5Drv = Spi<SpiInstance::SPI5>;
using I2C3Drv = I2c<I2cInstance::I2C3>;

// Drivers
using Ili9341	  = drivers::Ili9341Spi<SPI5Drv, LCD_DCX, LCD_CS, LCD_RST>;
using TouchDriver = drivers::Stmpe811<I2C3Drv>;

extern hal::SysTickSource sys_tick;
inline void delay_ms(uint32_t ms)
{
	sys_tick.delay(ms);
}

struct Stm32f429DiscoConfig
{
	using PixFmt  = Rgb565;
	using Display = Ili9341;
	using Touch	  = TouchDriver;
	using Input	  = hal::NullInput;
	using Tick	  = hal::SysTickSource;
	using OS	  = hal::BareMetalOS;

	static constexpr size_t framebuffer_count	= 0;
	static constexpr size_t scratch_buffer_size = 4800; // 10 lines × 240px × 2B
	static constexpr bool use_external_ram		= false;

	Display display{delay_ms};
	Touch touch{delay_ms};
	Input input;
	Tick& tick = sys_tick;

	void init_hardware()
	{
		// Enable GPIO clocks
		enable_gpio_clock(Port::A);
		enable_gpio_clock(Port::C);
		enable_gpio_clock(Port::D);
		enable_gpio_clock(Port::F);

		// SPI5 pins (AF5)
		SPI5_SCK::init_af(5);
		SPI5_MISO::init_af(5);
		SPI5_MOSI::init_af(5);

		// LCD control pins
		LCD_CS::init_output();
		LCD_DCX::init_output();
		LCD_RST::init_output();

		// I2C3 pins (AF4, open-drain for I2C)
		I2C3_SCL::init_af_od(4);
		I2C3_SDA::init_af_od(4);

		// Init peripherals
		SPI5Drv::init_master(2); // ~11MHz (APB2=90MHz / 8)
		SPI5Drv::enable_dma_clock();
		I2C3Drv::init();
		Dma2d::enable_clock();

		// Init drivers
		display.init();
		touch.init();
	}

	bool poll_events() { return true; }
};

} // namespace lumen::platform
