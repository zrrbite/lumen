#pragma once

/// STM32 DMA2D (Chrom-ART Accelerator) driver.
/// Hardware-accelerated rectangle fills, copies, and alpha blending.
/// Available on STM32F4x9, F7, H7 series.

#include <cstdint>

#include "lumen/core/types.hpp"

namespace lumen::hal::stm32 {

struct Dma2dRegs
{
	volatile uint32_t CR;	   // 0x00 Control
	volatile uint32_t ISR;	   // 0x04 Interrupt Status
	volatile uint32_t IFCR;	   // 0x08 Interrupt Flag Clear
	volatile uint32_t FGMAR;   // 0x0C Foreground Memory Address
	volatile uint32_t FGOR;	   // 0x10 Foreground Offset
	volatile uint32_t BGMAR;   // 0x14 Background Memory Address
	volatile uint32_t BGOR;	   // 0x18 Background Offset
	volatile uint32_t FGPFCCR; // 0x1C Foreground PFC Control
	volatile uint32_t FGCOLR;  // 0x20 Foreground Color
	volatile uint32_t BGPFCCR; // 0x24 Background PFC Control
	volatile uint32_t BGCOLR;  // 0x28 Background Color
	volatile uint32_t FGCMAR;  // 0x2C Foreground CLUT Memory Address
	volatile uint32_t BGCMAR;  // 0x30 Background CLUT Memory Address
	volatile uint32_t OPFCCR;  // 0x34 Output PFC Control
	volatile uint32_t OCOLR;   // 0x38 Output Color
	volatile uint32_t OMAR;	   // 0x3C Output Memory Address
	volatile uint32_t OOR;	   // 0x40 Output Offset
	volatile uint32_t NLR;	   // 0x44 Number of Lines
	volatile uint32_t LWR;	   // 0x48 Line Watermark
	volatile uint32_t AMTCR;   // 0x4C AHB Master Timer
};

/// DMA2D pixel format codes for FGPFCCR/BGPFCCR/OPFCCR
enum class Dma2dPixFmt : uint32_t
{
	ARGB8888 = 0,
	RGB888	 = 1,
	RGB565	 = 2,
	ARGB1555 = 3,
	ARGB4444 = 4,
};

/// DMA2D transfer modes (CR[17:16])
enum class Dma2dMode : uint32_t
{
	M2M		  = 0, // Memory-to-memory (copy)
	M2M_PFC	  = 1, // Memory-to-memory with pixel format conversion
	M2M_BLEND = 2, // Memory-to-memory with blending
	R2M		  = 3, // Register-to-memory (solid fill)
};

struct Dma2d
{
	static constexpr uint32_t BASE = 0x4002B000;

	static Dma2dRegs* regs() { return reinterpret_cast<Dma2dRegs*>(BASE); }

	/// Enable DMA2D clock (RCC AHB1ENR bit 23).
	static void enable_clock()
	{
		volatile uint32_t* rcc_ahb1enr = reinterpret_cast<volatile uint32_t*>(0x40023830);
		*rcc_ahb1enr |= (1U << 23);
	}

	/// Wait for current transfer to complete.
	static void wait()
	{
		while (regs()->CR & 1U)
		{} // Poll START bit
	}

	/// Fill a rectangle in a buffer with a solid color.
	/// @param dst     Destination buffer pointer
	/// @param dst_w   Total width of destination buffer (pixels)
	/// @param area    Rectangle to fill (x, y, w, h within dst)
	/// @param color   Pixel color value (in destination format)
	/// @param fmt     Pixel format of destination
	static void fill_rect(void* dst, uint16_t dst_w, Rect area, uint32_t color, Dma2dPixFmt fmt)
	{
		wait();

		uint32_t bpp	  = bytes_per_pixel(fmt);
		uint8_t* dst_addr = static_cast<uint8_t*>(dst) + (area.y * dst_w + area.x) * bpp;

		regs()->CR	   = (static_cast<uint32_t>(Dma2dMode::R2M) << 16);
		regs()->OPFCCR = static_cast<uint32_t>(fmt);
		regs()->OCOLR  = color;
		regs()->OMAR   = reinterpret_cast<uint32_t>(dst_addr);
		regs()->OOR	   = dst_w - area.w;
		regs()->NLR	   = (static_cast<uint32_t>(area.w) << 16) | area.h;
		regs()->CR |= 1U; // START
	}

	/// Copy a rectangle from src to dst (same pixel format).
	/// @param src      Source buffer
	/// @param src_w    Source buffer width
	/// @param src_rect Source rectangle
	/// @param dst      Destination buffer
	/// @param dst_w    Destination buffer width
	/// @param dst_x    Destination X position
	/// @param dst_y    Destination Y position
	/// @param fmt      Pixel format
	static void copy_rect(const void* src,
						  uint16_t src_w,
						  Rect src_rect,
						  void* dst,
						  uint16_t dst_w,
						  int16_t dst_x,
						  int16_t dst_y,
						  Dma2dPixFmt fmt)
	{
		wait();

		uint32_t bpp			= bytes_per_pixel(fmt);
		const uint8_t* src_addr = static_cast<const uint8_t*>(src) + (src_rect.y * src_w + src_rect.x) * bpp;
		uint8_t* dst_addr		= static_cast<uint8_t*>(dst) + (dst_y * dst_w + dst_x) * bpp;

		regs()->CR		= (static_cast<uint32_t>(Dma2dMode::M2M) << 16);
		regs()->FGPFCCR = static_cast<uint32_t>(fmt);
		regs()->FGMAR	= reinterpret_cast<uint32_t>(src_addr);
		regs()->FGOR	= src_w - src_rect.w;
		regs()->OPFCCR	= static_cast<uint32_t>(fmt);
		regs()->OMAR	= reinterpret_cast<uint32_t>(dst_addr);
		regs()->OOR		= dst_w - src_rect.w;
		regs()->NLR		= (static_cast<uint32_t>(src_rect.w) << 16) | src_rect.h;
		regs()->CR |= 1U; // START
	}

	/// Blend foreground onto background with alpha.
	/// Both buffers must be ARGB8888 for per-pixel alpha,
	/// or use constant alpha with any format.
	static void blend(const void* fg,
					  uint16_t fg_w,
					  const void* bg,
					  uint16_t bg_w,
					  void* dst,
					  uint16_t dst_w,
					  uint16_t width,
					  uint16_t height,
					  Dma2dPixFmt fg_fmt,
					  Dma2dPixFmt bg_fmt,
					  Dma2dPixFmt dst_fmt,
					  uint8_t fg_alpha = 255)
	{
		wait();

		regs()->CR		= (static_cast<uint32_t>(Dma2dMode::M2M_BLEND) << 16);
		regs()->FGPFCCR = static_cast<uint32_t>(fg_fmt) | (static_cast<uint32_t>(fg_alpha) << 24);
		regs()->FGMAR	= reinterpret_cast<uint32_t>(fg);
		regs()->FGOR	= fg_w - width;
		regs()->BGPFCCR = static_cast<uint32_t>(bg_fmt);
		regs()->BGMAR	= reinterpret_cast<uint32_t>(bg);
		regs()->BGOR	= bg_w - width;
		regs()->OPFCCR	= static_cast<uint32_t>(dst_fmt);
		regs()->OMAR	= reinterpret_cast<uint32_t>(dst);
		regs()->OOR		= dst_w - width;
		regs()->NLR		= (static_cast<uint32_t>(width) << 16) | height;
		regs()->CR |= 1U; // START
	}

	/// Check if a transfer is in progress.
	static bool is_busy() { return (regs()->CR & 1U) != 0; }

  private:
	static uint32_t bytes_per_pixel(Dma2dPixFmt fmt)
	{
		switch (fmt)
		{
		case Dma2dPixFmt::ARGB8888:
			return 4;
		case Dma2dPixFmt::RGB888:
			return 3;
		case Dma2dPixFmt::RGB565:
		case Dma2dPixFmt::ARGB1555:
		case Dma2dPixFmt::ARGB4444:
			return 2;
		default:
			return 2;
		}
	}
};

} // namespace lumen::hal::stm32
