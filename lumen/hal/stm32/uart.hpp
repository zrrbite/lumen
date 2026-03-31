#pragma once

/// Minimal STM32 USART driver — direct register access.
/// Supports polling TX/RX. No DMA, no interrupts (yet).

#include <cstdint>

namespace lumen::hal::stm32 {

enum class UsartInstance : uint32_t
{
	USART1 = 0x40011000, // APB2
	USART2 = 0x40004400, // APB1
	USART3 = 0x40004800, // APB1
	USART6 = 0x40011400, // APB2
};

struct UsartRegs
{
	volatile uint32_t SR;
	volatile uint32_t DR;
	volatile uint32_t BRR;
	volatile uint32_t CR1;
	volatile uint32_t CR2;
	volatile uint32_t CR3;
	volatile uint32_t GTPR;
};

template <UsartInstance Inst> struct Usart
{
	static UsartRegs* regs() { return reinterpret_cast<UsartRegs*>(static_cast<uint32_t>(Inst)); }

	/// Enable USART clock in RCC.
	static void enable_clock()
	{
		if constexpr (Inst == UsartInstance::USART1)
		{
			// USART1 on APB2 (RCC_APB2ENR bit 4)
			volatile uint32_t* rcc_apb2enr = reinterpret_cast<volatile uint32_t*>(0x40023844);
			*rcc_apb2enr |= (1U << 4);
		}
	}

	/// Init USART: 8N1, TX+RX enabled.
	/// @param apb_clock  APB bus clock in Hz (APB2=90MHz for USART1 on F429)
	/// @param baud       Desired baud rate
	static void init(uint32_t apb_clock, uint32_t baud)
	{
		enable_clock();
		regs()->CR1 = 0; // Disable

		// Calculate BRR: USARTDIV = apb_clock / (16 * baud)
		// BRR = mantissa << 4 | fraction
		uint32_t div = (apb_clock + baud / 2) / baud; // Rounded
		regs()->BRR	 = div;

		// 8 data bits, no parity (default CR1), 1 stop bit (default CR2)
		regs()->CR1 = (1U << 13) | // UE: USART enable
					  (1U << 3) |  // TE: transmitter enable
					  (1U << 2);   // RE: receiver enable
	}

	/// Send a single byte (blocking).
	static void send(uint8_t byte)
	{
		while ((regs()->SR & (1U << 7)) == 0)
		{} // Wait TXE
		regs()->DR = byte;
	}

	/// Send a null-terminated string.
	static void send_string(const char* str)
	{
		while (*str)
		{
			send(static_cast<uint8_t>(*str));
			++str;
		}
	}

	/// Check if a byte is available to read.
	static bool rx_available() { return (regs()->SR & (1U << 5)) != 0; } // RXNE

	/// Read a received byte (non-blocking, check rx_available first).
	static uint8_t receive() { return static_cast<uint8_t>(regs()->DR); }
};

} // namespace lumen::hal::stm32
