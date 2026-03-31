#pragma once

/// ILI9341 SPI display driver for Lumen.
/// Template parameters: SPI peripheral, DC pin, CS pin, RST pin.

#include "lumen/core/types.hpp"

namespace lumen::drivers {

/// Delay callback type — provided by the board config.
using DelayMs = void (*)(uint32_t ms);

template <typename SpiDrv, typename PinDC, typename PinCS, typename PinRST> class Ili9341Spi
{
  public:
	using PixelFormat = Rgb565;
	using pixel_t	  = uint16_t;

	static constexpr uint16_t width() { return 240; }
	static constexpr uint16_t height() { return 320; }

	explicit Ili9341Spi(DelayMs delay_fn) : delay_(delay_fn) {}

	void init()
	{
		PinCS::init_output();
		PinDC::init_output();
		PinRST::init_output();
		PinCS::high();

		// Hardware reset
		PinRST::high();
		delay_(5);
		PinRST::low();
		delay_(20);
		PinRST::high();
		delay_(150);

		// Software reset
		send_cmd(0x01);
		delay_(50);

		// Sleep out
		send_cmd(0x11);
		delay_(120);

		// Pixel format: 16-bit RGB565
		send_cmd(0x3A);
		send_data(0x55);

		// Memory access control: row/col exchange for landscape (optional)
		send_cmd(0x36);
		send_data(0x48); // MX=0, MY=1, MV=0, RGB order

		// Display on
		send_cmd(0x29);
		delay_(50);
	}

	/// Set the pixel write window.
	void set_window(Rect area)
	{
		uint16_t x0 = area.x;
		uint16_t y0 = area.y;
		uint16_t x1 = area.x + area.w - 1;
		uint16_t y1 = area.y + area.h - 1;

		send_cmd(0x2A); // Column address set
		send_data16(x0);
		send_data16(x1);

		send_cmd(0x2B); // Page address set
		send_data16(y0);
		send_data16(y1);

		send_cmd(0x2C); // Memory write
	}

	/// Write pixel data (RGB565, big-endian on wire).
	void write_pixels(const pixel_t* data, uint32_t count)
	{
		PinDC::high();
		PinCS::low();
		SpiDrv::transmit16(data, count);
		PinCS::high();
	}

	/// No-op for SPI displays — data is streamed directly.
	void flush() {}

	bool supports_dma() const { return SpiDrv::has_dma; }

	/// Fill a region with a single color (optimized — avoids framebuffer).
	void fill(Rect area, pixel_t color)
	{
		set_window(area);
		uint32_t pixels = static_cast<uint32_t>(area.w) * area.h;
		PinDC::high();
		PinCS::low();
		for (uint32_t idx = 0; idx < pixels; ++idx)
		{
			SpiDrv::transmit_byte(static_cast<uint8_t>(color >> 8));
			SpiDrv::transmit_byte(static_cast<uint8_t>(color & 0xFF));
		}
		PinCS::high();
	}

  private:
	DelayMs delay_;

	void send_cmd(uint8_t cmd)
	{
		PinDC::low();
		PinCS::low();
		SpiDrv::transmit_byte(cmd);
		PinCS::high();
	}

	void send_data(uint8_t data)
	{
		PinDC::high();
		PinCS::low();
		SpiDrv::transmit_byte(data);
		PinCS::high();
	}

	void send_data16(uint16_t data)
	{
		PinDC::high();
		PinCS::low();
		SpiDrv::transmit_byte(static_cast<uint8_t>(data >> 8));
		SpiDrv::transmit_byte(static_cast<uint8_t>(data & 0xFF));
		PinCS::high();
	}
};

} // namespace lumen::drivers
