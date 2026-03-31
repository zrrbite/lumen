/// Minimal STM32F769-DISCO bring-up test.
/// Blinks the user LEDs to verify clock, SDRAM, and SysTick work.
/// LD1 (red) = PJ13, LD2 (green) = PJ5, LD3 (green) = PA12

#include <cstdint>

#include "lumen/hal/stm32/gpio.hpp"
#include "lumen/hal/tick_source.hpp"

namespace lumen::platform {
extern hal::SysTickSource sys_tick;
}

using LD1_RED = lumen::hal::stm32::GpioPin<lumen::hal::stm32::Port::J, 13>;
using LD2_GRN = lumen::hal::stm32::GpioPin<lumen::hal::stm32::Port::J, 5>;

// Quick SDRAM test: write and read back a single word
static bool test_sdram_at(uint32_t addr)
{
	volatile uint32_t* p	= reinterpret_cast<volatile uint32_t*>(addr);
	*p						= 0xCAFEBABE;
	volatile uint32_t dummy = *p; // read back
	(void)dummy;
	return (*p == 0xCAFEBABE);
}

// 0 = both fail, 1 = bank1 (0xC0), 2 = bank2 (0xD0), 3 = both work
static uint8_t sdram_result = 0;

int main()
{
	// Enable GPIO J clock
	lumen::hal::stm32::enable_gpio_clock(lumen::hal::stm32::Port::J);

	LD1_RED::init_output();
	LD2_GRN::init_output();

	// Test both SDRAM banks
	if (test_sdram_at(0xC0000000))
		sdram_result |= 1;
	if (test_sdram_at(0xD0000000))
		sdram_result |= 2;

	// Blink pattern indicates result:
	// Green steady + red blink = at least one bank works
	// Red steady + green blink = both banks fail
	// Both steady = both banks work
	while (true)
	{
		// Red: blink N times for result code, then pause
		for (uint8_t i = 0; i < 4; ++i)
		{
			if (i < sdram_result)
			{
				LD1_RED::high();
				lumen::platform::sys_tick.delay(200);
				LD1_RED::low();
				lumen::platform::sys_tick.delay(200);
			}
			else
			{
				lumen::platform::sys_tick.delay(400);
			}
		}

		// Green blinks = alive
		LD2_GRN::high();
		lumen::platform::sys_tick.delay(100);
		LD2_GRN::low();
		lumen::platform::sys_tick.delay(900);
	}

	return 0;
}
