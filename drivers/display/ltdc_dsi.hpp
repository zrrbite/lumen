#pragma once

/// LTDC + DSI display driver for STM32F769-DISCO.
/// Drives the OTM8009A 800x480 display via MIPI-DSI.
///
/// This driver manages an SDRAM framebuffer and uses the LTDC
/// peripheral to continuously refresh the display. Rendering
/// writes directly to the framebuffer; no explicit flush needed
/// (the LTDC reads it automatically via DMA).

#include <cstdint>
#include <cstring>

#include "lumen/core/types.hpp"

namespace lumen::drivers {

/// LTDC + DSI driver for OTM8009A on STM32F769-DISCO.
/// Template parameter: pixel format tag (Rgb565 or Argb8888).
template <typename PixFmt> class LtdcDsi
{
  public:
	using pixel_t = typename PixFmt::pixel_t;

	static constexpr uint16_t WIDTH	 = 800;
	static constexpr uint16_t HEIGHT = 480;

	static constexpr uint16_t width() { return WIDTH; }
	static constexpr uint16_t height() { return HEIGHT; }

	/// Set the framebuffer address. Must be in SDRAM.
	/// Call before init().
	void set_framebuffer(pixel_t* fb) { fb_ = fb; }

	/// Initialize LTDC, DSI Host, and OTM8009A display.
	/// This configures:
	/// 1. FMC for SDRAM access
	/// 2. DSI Host in video mode
	/// 3. LTDC with one layer pointing to the framebuffer
	/// 4. OTM8009A via DSI command mode for init, then video mode
	void init()
	{
		if (fb_ == nullptr)
			return;

		// Clear framebuffer
		memset(fb_, 0, static_cast<size_t>(WIDTH) * HEIGHT * sizeof(pixel_t));

		// TODO: Full DSI + LTDC initialization sequence
		// This requires ~200 lines of register configuration:
		// 1. Enable clocks (RCC: LTDC, DSI, FMC)
		// 2. Configure FMC for SDRAM (IS42S32800G on F769-DISCO)
		// 3. Configure DSI PLL, lane config, video mode timing
		// 4. Configure LTDC timing (hsync, vsync, porches)
		// 5. Configure LTDC Layer 1 with framebuffer address
		// 6. Send OTM8009A init commands via DSI
		// 7. Enable LTDC and DSI

		initialized_ = true;
	}

	/// Get the framebuffer pointer for direct rendering.
	pixel_t* framebuffer() { return fb_; }

	/// No-op — LTDC reads the framebuffer continuously via DMA.
	void flush() {}

	/// Set the pixel write window (for SPI-style compatibility).
	void set_window(Rect /*area*/) {}

	/// Write pixels (for SPI-style compatibility — copies to framebuffer).
	void write_pixels(const pixel_t* data, uint32_t count)
	{
		if (fb_ != nullptr)
		{
			memcpy(fb_, data, count * sizeof(pixel_t));
		}
	}

	/// Fill a region directly in the framebuffer.
	void fill(Rect area, pixel_t color)
	{
		if (fb_ == nullptr)
			return;

		for (int16_t y = area.y; y < area.bottom(); ++y)
		{
			pixel_t* row = fb_ + y * WIDTH + area.x;
			for (uint16_t x = 0; x < area.w; ++x)
			{
				row[x] = color;
			}
		}
	}

	static constexpr bool has_dma = true;

  private:
	pixel_t* fb_	  = nullptr;
	bool initialized_ = false;
};

} // namespace lumen::drivers
