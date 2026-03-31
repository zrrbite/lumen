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

	// --- DMA Support ---
	// SPI5 TX = DMA2 Stream 6 Channel 7

	/// DMA2 Stream 6 registers (for SPI5 TX)
	struct DmaStreamRegs
	{
		volatile uint32_t CR;
		volatile uint32_t NDTR;
		volatile uint32_t PAR;
		volatile uint32_t M0AR;
		volatile uint32_t M1AR;
		volatile uint32_t FCR;
	};

	static constexpr uint32_t DMA2_BASE			= 0x40026000;
	static constexpr uint32_t DMA2_STREAM6_BASE = DMA2_BASE + 0x10 + 6 * 0x18;
	static constexpr uint32_t DMA2_HIFCR		= DMA2_BASE + 0x0C; // High interrupt flag clear

	static DmaStreamRegs* dma_stream() { return reinterpret_cast<DmaStreamRegs*>(DMA2_STREAM6_BASE); }

	/// Enable DMA2 clock.
	static void enable_dma_clock()
	{
		volatile uint32_t* rcc_ahb1enr = reinterpret_cast<volatile uint32_t*>(0x40023830);
		*rcc_ahb1enr |= (1U << 22); // DMA2EN
	}

	/// Start a non-blocking DMA transfer of 16-bit pixel data.
	/// Call dma_wait() or poll dma_busy() before starting another.
	static void transmit16_dma(const uint16_t* data, uint32_t count)
	{
		// Switch SPI to 16-bit mode
		regs()->CR1 &= ~(1U << 6); // Disable SPE
		regs()->CR1 |= (1U << 11); // DFF: 16-bit
		regs()->CR1 |= (1U << 6);  // Enable SPE

		// Clear DMA2 Stream 6 interrupt flags (bits 16-21 in HIFCR)
		volatile uint32_t* hifcr = reinterpret_cast<volatile uint32_t*>(DMA2_HIFCR);
		*hifcr					 = (0x3FU << 16); // Clear all flags for stream 6

		auto* dma = dma_stream();

		// Disable stream first
		dma->CR = 0;
		while (dma->CR & 1U)
		{} // Wait EN clear

		// Configure DMA2 Stream 6:
		// - Channel 7 (bits 27:25 = 7)
		// - Memory-to-peripheral
		// - Memory increment, no peripheral increment
		// - 16-bit memory and peripheral size
		// - Priority high
		dma->PAR  = reinterpret_cast<uint32_t>(&regs()->DR);
		dma->M0AR = reinterpret_cast<uint32_t>(data);
		dma->NDTR = count;
		dma->CR	  = (7U << 25) | // Channel 7
				  (1U << 6) |	 // DIR: memory-to-peripheral
				  (1U << 10) |	 // MINC: memory increment
				  (1U << 11) |	 // MSIZE: 16-bit
				  (1U << 13) |	 // PSIZE: 16-bit
				  (2U << 16);	 // PL: high priority

		// Enable SPI TX DMA request
		regs()->CR2 |= (1U << 1); // TXDMAEN

		// Start DMA transfer
		dma->CR |= 1U; // EN
	}

	/// Check if DMA transfer is still in progress.
	static bool dma_busy()
	{
		// Check DMA2 Stream 6 transfer complete flag (bit 21 in HISR)
		volatile uint32_t* hisr = reinterpret_cast<volatile uint32_t*>(DMA2_BASE + 0x08);
		return (*hisr & (1U << 21)) == 0 && (dma_stream()->CR & 1U);
	}

	/// Wait for DMA transfer to complete, then restore 8-bit SPI mode.
	static void dma_wait()
	{
		// Wait for DMA transfer complete
		volatile uint32_t* hisr = reinterpret_cast<volatile uint32_t*>(DMA2_BASE + 0x08);
		while ((*hisr & (1U << 21)) == 0)
		{} // Wait TCIF6

		// Clear transfer complete flag
		volatile uint32_t* hifcr = reinterpret_cast<volatile uint32_t*>(DMA2_HIFCR);
		*hifcr					 = (1U << 21);

		// Wait for SPI to finish
		wait_idle();

		// Disable DMA request
		regs()->CR2 &= ~(1U << 1); // TXDMAEN off

		// Back to 8-bit mode
		regs()->CR1 &= ~(1U << 6);	// Disable SPE
		regs()->CR1 &= ~(1U << 11); // DFF: 8-bit
		regs()->CR1 |= (1U << 6);	// Enable SPE
	}

	static constexpr bool has_dma = true;
};

} // namespace lumen::hal::stm32
