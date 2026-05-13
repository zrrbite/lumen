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

	/// Transmit a single byte (blocking). Waits for BSY after.
	static void transmit_byte(uint8_t data)
	{
		while ((regs()->SR & (1U << 1)) == 0)
		{} // Wait TXE
		*reinterpret_cast<volatile uint8_t*>(&regs()->DR) = data;
		while ((regs()->SR & (1U << 7)) != 0)
		{} // Wait BSY (needed for CS toggle after single-byte commands)
	}

	/// Wait for SPI to finish all pending transfers.
	static void wait_idle()
	{
		while ((regs()->SR & (1U << 1)) == 0)
		{} // Wait TXE
		while ((regs()->SR & (1U << 7)) != 0)
		{} // Wait BSY
	}

	/// Transmit a buffer (blocking).
	static void transmit(const uint8_t* data, uint32_t len)
	{
		for (uint32_t idx = 0; idx < len; ++idx)
		{
			while ((regs()->SR & (1U << 1)) == 0)
			{} // Wait TXE
			*reinterpret_cast<volatile uint8_t*>(&regs()->DR) = data[idx];
		}
		wait_idle();
	}

	/// Transmit 16-bit pixel data using 16-bit SPI mode.
	/// Much faster than byte-by-byte: one register write per pixel,
	/// no BSY wait between pixels, hardware handles byte ordering.
	static void transmit16(const uint16_t* data, uint32_t count)
	{
		// Switch to 16-bit data frame
		regs()->CR1 &= ~(1U << 6); // Disable SPE
		regs()->CR1 |= (1U << 11); // DFF: 16-bit
		regs()->CR1 |= (1U << 6);  // Enable SPE

		for (uint32_t idx = 0; idx < count; ++idx)
		{
			while ((regs()->SR & (1U << 1)) == 0)
			{} // Wait TXE
			regs()->DR = data[idx];
		}

		wait_idle();

		// Back to 8-bit mode
		regs()->CR1 &= ~(1U << 6);	// Disable SPE
		regs()->CR1 &= ~(1U << 11); // DFF: 8-bit
		regs()->CR1 |= (1U << 6);	// Enable SPE
	}

	static constexpr bool has_dma = (Inst == SpiInstance::SPI5);

	/// Enable DMA2 clock (for SPI5 TX via DMA2 Stream 4).
	static void enable_dma_clock()
	{
		static_assert(Inst == SpiInstance::SPI5, "DMA only implemented for SPI5");
		volatile uint32_t* rcc_ahb1enr = reinterpret_cast<volatile uint32_t*>(0x40023830);
		*rcc_ahb1enr |= (1U << 22); // DMA2EN
	}

	/// Start async 16-bit DMA transfer to SPI. Call transmit16_wait() when done.
	/// DMA2 Stream 4, Channel 2 = SPI5_TX.
	static void transmit16_start(const uint16_t* data, uint32_t count)
	{
		static_assert(Inst == SpiInstance::SPI5, "DMA only implemented for SPI5");

		// Switch to 16-bit SPI mode
		regs()->CR1 &= ~(1U << 6); // Disable SPE
		regs()->CR1 |= (1U << 11); // DFF: 16-bit
		regs()->CR1 |= (1U << 6);  // Enable SPE

		// DMA2 Stream 4 registers
		static constexpr uint32_t DMA2_BASE		 = 0x40026400;
		static constexpr uint32_t STREAM4_OFFSET = 0x70;
		auto* s4cr	 = reinterpret_cast<volatile uint32_t*>(DMA2_BASE + STREAM4_OFFSET + 0x00);
		auto* s4ndtr = reinterpret_cast<volatile uint32_t*>(DMA2_BASE + STREAM4_OFFSET + 0x04);
		auto* s4par	 = reinterpret_cast<volatile uint32_t*>(DMA2_BASE + STREAM4_OFFSET + 0x08);
		auto* s4m0ar = reinterpret_cast<volatile uint32_t*>(DMA2_BASE + STREAM4_OFFSET + 0x0C);
		auto* hifcr	 = reinterpret_cast<volatile uint32_t*>(DMA2_BASE + 0x0C);

		// Disable stream and wait
		*s4cr = 0;
		while (*s4cr & 1U)
		{}

		// Clear all interrupt flags for Stream 4 (bits 0,2,3,4,5 of HIFCR)
		*hifcr = 0x3DU;

		// Configure: Channel 2, mem-to-periph, 16-bit, memory increment
		*s4par	= reinterpret_cast<uint32_t>(&regs()->DR);
		*s4m0ar = reinterpret_cast<uint32_t>(data);
		*s4ndtr = count;
		*s4cr	= (2U << 25) // CHSEL = 2 (SPI5_TX)
				| (1U << 13) // MSIZE = 16-bit
				| (1U << 11) // PSIZE = 16-bit
				| (1U << 10) // MINC: memory increment
				| (1U << 6); // DIR: memory-to-peripheral

		// Enable SPI TX DMA request
		regs()->CR2 |= (1U << 1); // TXDMAEN

		// Start transfer
		*s4cr |= 1U; // EN
	}

	/// Wait for async DMA transfer to complete, restore 8-bit SPI mode.
	static void transmit16_wait()
	{
		static_assert(Inst == SpiInstance::SPI5, "DMA only implemented for SPI5");

		static constexpr uint32_t DMA2_BASE		 = 0x40026400;
		static constexpr uint32_t STREAM4_OFFSET = 0x70;
		auto* s4cr								 = reinterpret_cast<volatile uint32_t*>(DMA2_BASE + STREAM4_OFFSET);
		auto* hisr								 = reinterpret_cast<volatile uint32_t*>(DMA2_BASE + 0x04);
		auto* hifcr								 = reinterpret_cast<volatile uint32_t*>(DMA2_BASE + 0x0C);

		// Wait for TCIF4 (bit 5 of HISR)
		while (!(*hisr & (1U << 5)))
		{}

		// Clear flags, disable stream
		*hifcr = 0x3DU;
		*s4cr  = 0;

		// Disable SPI TX DMA request
		regs()->CR2 &= ~(1U << 1);

		// Wait for SPI to finish transmitting
		wait_idle();

		// Back to 8-bit mode
		regs()->CR1 &= ~(1U << 6);	// Disable SPE
		regs()->CR1 &= ~(1U << 11); // DFF: 8-bit
		regs()->CR1 |= (1U << 6);	// Enable SPE
	}
};

} // namespace lumen::hal::stm32
