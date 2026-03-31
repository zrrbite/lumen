/// STM32F769-DISCO display bring-up — debug version.
/// Blinks red LED N times at each init stage to identify hangs.

#include <cstdint>
#include <cstring>

#include "lumen/hal/stm32/gpio.hpp"
#include "lumen/hal/tick_source.hpp"

namespace lumen::platform {
extern hal::SysTickSource sys_tick;
}

using LD1_RED = lumen::hal::stm32::GpioPin<lumen::hal::stm32::Port::J, 13>;
using LD2_GRN = lumen::hal::stm32::GpioPin<lumen::hal::stm32::Port::J, 5>;

static void delay_ms(uint32_t ms)
{
	lumen::platform::sys_tick.delay(ms);
}

static void blink_red(uint8_t count)
{
	for (uint8_t i = 0; i < count; ++i)
	{
		LD1_RED::high();
		delay_ms(150);
		LD1_RED::low();
		delay_ms(150);
	}
	delay_ms(500);
}

// Register base addresses
static constexpr uint32_t DSI_BASE = 0x40016C00;
static constexpr uint32_t RCC_BASE = 0x40023800;

int main()
{
	lumen::hal::stm32::enable_gpio_clock(lumen::hal::stm32::Port::J);
	LD1_RED::init_output();
	LD2_GRN::init_output();

	volatile uint32_t* RCC_CR		  = reinterpret_cast<volatile uint32_t*>(RCC_BASE);
	volatile uint32_t* RCC_PLLSAICFGR = reinterpret_cast<volatile uint32_t*>(RCC_BASE + 0x88);
	volatile uint32_t* RCC_DCKCFGR1	  = reinterpret_cast<volatile uint32_t*>(RCC_BASE + 0x8C);
	volatile uint32_t* RCC_APB2ENR	  = reinterpret_cast<volatile uint32_t*>(RCC_BASE + 0x44);
	volatile uint32_t* RCC_APB2RSTR	  = reinterpret_cast<volatile uint32_t*>(RCC_BASE + 0x24);
	volatile uint32_t* RCC_AHB1ENR	  = reinterpret_cast<volatile uint32_t*>(RCC_BASE + 0x30);

	// Stage 1: LCD Reset (PJ15)
	blink_red(1);
	*RCC_AHB1ENR |= (1U << 9); // GPIOJEN
	volatile uint32_t* GPIOJ_MODER = reinterpret_cast<volatile uint32_t*>(0x40022400);
	volatile uint32_t* GPIOJ_BSRR  = reinterpret_cast<volatile uint32_t*>(0x40022418);
	*GPIOJ_MODER &= ~(3U << 30);
	*GPIOJ_MODER |= (1U << 30);
	*GPIOJ_BSRR = (1U << (15 + 16)); // Reset low
	delay_ms(20);
	*GPIOJ_BSRR = (1U << 15); // Reset high
	delay_ms(10);

	// Stage 2: Enable LTDC + DSI clocks
	blink_red(2);
	*RCC_APB2ENR |= (1U << 26); // LTDCEN
	*RCC_APB2ENR |= (1U << 27); // DSIEN
	*RCC_APB2RSTR |= (1U << 26);
	*RCC_APB2RSTR &= ~(1U << 26);
	*RCC_APB2RSTR |= (1U << 27);
	*RCC_APB2RSTR &= ~(1U << 27);

	// Stage 3: Configure PLLSAI
	blink_red(3);
	*RCC_CR &= ~(1U << 28); // Disable PLLSAI
	// Wait with timeout
	for (volatile int t = 0; t < 100000; ++t)
	{
		if ((*RCC_CR & (1U << 29)) == 0)
			break;
	}
	*RCC_PLLSAICFGR = (384U << 6) | (7U << 28);
	*RCC_DCKCFGR1 &= ~(3U << 16); // PLLSAIDIVR = /2
	*RCC_CR |= (1U << 28);		  // Enable PLLSAI
	// Wait with timeout
	for (volatile int t = 0; t < 1000000; ++t)
	{
		if (*RCC_CR & (1U << 29))
			break;
	}
	if ((*RCC_CR & (1U << 29)) == 0)
	{
		// PLLSAI failed to lock
		while (true)
		{
			blink_red(3); // Stuck at stage 3
		}
	}

	// Stage 4: DSI regulator + PLL
	blink_red(4);
	volatile uint32_t* DSI_WRPCR = reinterpret_cast<volatile uint32_t*>(DSI_BASE + 0x430);
	volatile uint32_t* DSI_WCR	 = reinterpret_cast<volatile uint32_t*>(DSI_BASE + 0x404);
	volatile uint32_t* DSI_WISR	 = reinterpret_cast<volatile uint32_t*>(DSI_BASE + 0x40C);
	volatile uint32_t* DSI_CR	 = reinterpret_cast<volatile uint32_t*>(DSI_BASE + 0x04);

	// Disable DSI wrapper and host first
	*DSI_WCR = 0;
	*DSI_CR	 = 0;

	// Step 1: Enable DSI regulator
	*DSI_WRPCR |= (1U << 24); // REGEN

	// Wait for regulator ready (RRS = bit 12 of WISR)
	for (volatile int t = 0; t < 1000000; ++t)
	{
		if (*DSI_WISR & (1U << 12))
			break;
	}
	if ((*DSI_WISR & (1U << 12)) == 0)
	{
		while (true)
		{
			blink_red(4); // Regulator failed
		}
	}

	blink_red(5); // Regulator OK

	// Step 2: Configure DSI PLL: NDIV=100, IDF=/5(=2), ODF=/1(=0)
	*DSI_WRPCR &= ~((0x7FU << 2) | (0xFU << 11) | (0x3U << 16)); // Clear NDIV/IDF/ODF
	*DSI_WRPCR |= (100U << 2) | (2U << 11) | (0U << 16);

	// Step 3: Enable PLL
	*DSI_WRPCR |= (1U << 0); // PLLEN
	delay_ms(1);			 // Min 400us before checking lock

	// Wait for PLL lock (WISR bit 8 = PLLLS)
	for (volatile int t = 0; t < 1000000; ++t)
	{
		if (*DSI_WISR & (1U << 8)) // PLLLS
			break;
	}
	if ((*DSI_WISR & (1U << 8)) == 0)
	{
		while (true)
		{
			blink_red(6); // PLL lock failed
		}
	}

	// Stage 7: If we get here, all PLLs locked
	blink_red(7);

	// All PLLs working — show green
	LD2_GRN::high();

	while (true)
	{
		LD2_GRN::high();
		delay_ms(500);
		LD2_GRN::low();
		delay_ms(500);
	}

	return 0;
}
