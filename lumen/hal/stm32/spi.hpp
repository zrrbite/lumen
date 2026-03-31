#pragma once

/// Minimal STM32 SPI abstraction — direct register access.

#include <cstdint>

namespace lumen::hal::stm32 {

/// SPI peripheral base addresses (STM32F4)
enum class SpiInstance : uint32_t
{
	SPI1 = 0x40013000,
	SPI2 = 0x40003800,
	SPI3 = 0x40003C00,
	SPI4 = 0x40013400,
	SPI5 = 0x40015000,
	SPI6 = 0x40015400,
};

struct SpiRegs
{
	volatile uint32_t CR1;
	volatile uint32_t CR2;
	volatile uint32_t SR;
	volatile uint32_t DR;
	volatile uint32_t CRCPR;
	volatile uint32_t RXCRCR;
	volatile uint32_t TXCRCR;
	volatile uint32_t I2SCFGR;
	volatile uint32_t I2SPR;
};

/// Zero-cost SPI driver. Blocking transmit only (DMA optional later).
template <SpiInstance Inst> struct Spi
{
	static SpiRegs* regs() { return reinterpret_cast<SpiRegs*>(static_cast<uint32_t>(Inst)); }

	/// Enable SPI clock in RCC.
	static void enable_clock()
	{
		if constexpr (Inst == SpiInstance::SPI5)
		{
			// SPI5 is on APB2 (RCC_APB2ENR bit 20)
			volatile uint32_t* rcc_apb2enr = reinterpret_cast<volatile uint32_t*>(0x40023844);
			*rcc_apb2enr |= (1U << 20);
		}
		// Add other SPI instances as needed
	}

	/// Configure SPI as master, 8-bit, CPOL=0, CPHA=0.
	static void init_master(uint8_t prescaler_bits = 3) // default /16
	{
		enable_clock();
		// Disable SPI first
		regs()->CR1 = 0;
		// Master mode, software SS management, SSI high
		// BR[2:0] = prescaler_bits (0=/2, 1=/4, 2=/8, 3=/16, ...)
		regs()->CR1 = (1U << 2) |									 // MSTR: master mode
					  (static_cast<uint32_t>(prescaler_bits) << 3) | // BR: baud rate
					  (1U << 9) |									 // SSM: software slave management
					  (1U << 8);									 // SSI: internal slave select
		// Enable SPI
		regs()->CR1 |= (1U << 6); // SPE
	}

	/// Transmit a single byte (blocking).
	static void transmit_byte(uint8_t data)
	{
		// Wait for TXE (transmit buffer empty)
		while ((regs()->SR & (1U << 1)) == 0)
		{}
		*reinterpret_cast<volatile uint8_t*>(&regs()->DR) = data;
		// Wait for BSY clear
		while ((regs()->SR & (1U << 7)) != 0)
		{}
	}

	/// Transmit a buffer (blocking).
	static void transmit(const uint8_t* data, uint32_t len)
	{
		for (uint32_t idx = 0; idx < len; ++idx)
		{
			transmit_byte(data[idx]);
		}
	}

	/// Transmit 16-bit values (for pixel data).
	static void transmit16(const uint16_t* data, uint32_t count)
	{
		for (uint32_t idx = 0; idx < count; ++idx)
		{
			transmit_byte(static_cast<uint8_t>(data[idx] >> 8));
			transmit_byte(static_cast<uint8_t>(data[idx] & 0xFF));
		}
	}

	static constexpr bool has_dma = false; // TODO: add DMA support
};

} // namespace lumen::hal::stm32
