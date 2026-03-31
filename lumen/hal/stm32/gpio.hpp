#pragma once

/// Minimal STM32 GPIO abstraction — zero-cost template wrappers.
/// No HAL dependency, just CMSIS register definitions.

#include <cstdint>

namespace lumen::hal::stm32 {

/// GPIO port base addresses (STM32F4)
enum class Port : uint32_t
{
	A = 0x40020000,
	B = 0x40020400,
	C = 0x40020800,
	D = 0x40020C00,
	E = 0x40021000,
	F = 0x40021400,
	G = 0x40021800,
};

/// GPIO register offsets
struct GpioRegs
{
	volatile uint32_t MODER;
	volatile uint32_t OTYPER;
	volatile uint32_t OSPEEDR;
	volatile uint32_t PUPDR;
	volatile uint32_t IDR;
	volatile uint32_t ODR;
	volatile uint32_t BSRR;
	volatile uint32_t LCKR;
	volatile uint32_t AFRL;
	volatile uint32_t AFRH;
};

/// Zero-cost GPIO pin. All operations inline to single register writes.
template <Port P, uint8_t Pin> struct GpioPin
{
	static_assert(Pin < 16, "GPIO pin must be 0-15");

	static GpioRegs* regs() { return reinterpret_cast<GpioRegs*>(static_cast<uint32_t>(P)); }

	/// Set pin HIGH.
	static void high() { regs()->BSRR = (1U << Pin); }

	/// Set pin LOW.
	static void low() { regs()->BSRR = (1U << (Pin + 16)); }

	/// Read pin state.
	static bool read() { return (regs()->IDR & (1U << Pin)) != 0; }

	/// Configure as output (push-pull, high speed).
	static void init_output()
	{
		// MODER: 01 = output
		regs()->MODER &= ~(3U << (Pin * 2));
		regs()->MODER |= (1U << (Pin * 2));
		// OSPEEDR: 11 = very high speed
		regs()->OSPEEDR |= (3U << (Pin * 2));
		// OTYPER: 0 = push-pull
		regs()->OTYPER &= ~(1U << Pin);
	}

	/// Configure as alternate function.
	static void init_af(uint8_t af_num)
	{
		// MODER: 10 = alternate function
		regs()->MODER &= ~(3U << (Pin * 2));
		regs()->MODER |= (2U << (Pin * 2));
		// OSPEEDR: 11 = very high speed
		regs()->OSPEEDR |= (3U << (Pin * 2));
		// Set AF number
		if constexpr (Pin < 8)
		{
			regs()->AFRL &= ~(0xFU << (Pin * 4));
			regs()->AFRL |= (static_cast<uint32_t>(af_num) << (Pin * 4));
		}
		else
		{
			regs()->AFRH &= ~(0xFU << ((Pin - 8) * 4));
			regs()->AFRH |= (static_cast<uint32_t>(af_num) << ((Pin - 8) * 4));
		}
	}
};

/// Enable GPIO port clock (RCC AHB1ENR).
inline void enable_gpio_clock(Port port)
{
	volatile uint32_t* rcc_ahb1enr = reinterpret_cast<volatile uint32_t*>(0x40023830);
	uint8_t port_idx			   = (static_cast<uint32_t>(port) - 0x40020000) / 0x400;
	*rcc_ahb1enr |= (1U << port_idx);
	// Small delay for clock to stabilize
	volatile uint32_t dummy = *rcc_ahb1enr;
	(void)dummy;
}

} // namespace lumen::hal::stm32
