#pragma once

#include <cstdint>

#include "lumen/core/types.hpp"
#include "lumen/gfx/canvas.hpp"

namespace lumen::gfx {

/// Bitmap font glyph — each glyph is a fixed-size bitmap.
struct BitmapFont
{
	const uint8_t* data;   // Packed 1bpp bitmap data
	uint8_t char_width;	   // Width per character in pixels
	uint8_t char_height;   // Height per character in pixels
	uint8_t first_char;	   // First ASCII code (usually 32 = space)
	uint8_t last_char;	   // Last ASCII code (usually 126 = ~)
	uint8_t bytes_per_row; // Bytes per row per character

	/// Draw a string at (x, y) into the canvas.
	template <typename PixFmt>
	void draw_string(Canvas<PixFmt>& canvas, int16_t x_pos, int16_t y_pos, const char* text, Color color) const
	{
		int16_t cursor_x = x_pos;
		while (*text)
		{
			uint8_t ch = static_cast<uint8_t>(*text);
			if (ch >= first_char && ch <= last_char)
			{
				draw_char(canvas, cursor_x, y_pos, ch, color);
			}
			cursor_x += char_width + 1; // 1px spacing
			++text;
		}
	}

	/// Measure string width in pixels.
	uint16_t string_width(const char* text) const
	{
		uint16_t len = 0;
		while (*text)
		{
			++len;
			++text;
		}
		if (len == 0)
			return 0;
		return len * (char_width + 1) - 1;
	}

	template <typename PixFmt>
	void draw_char(Canvas<PixFmt>& canvas, int16_t x_pos, int16_t y_pos, uint8_t ch, Color color) const
	{
		if (ch < first_char || ch > last_char)
			return;

		uint16_t glyph_offset = (ch - first_char) * char_height * bytes_per_row;

		for (uint8_t row = 0; row < char_height; ++row)
		{
			for (uint8_t col = 0; col < char_width; ++col)
			{
				uint8_t byte_idx = col / 8;
				uint8_t bit_idx	 = 7 - (col % 8);
				uint8_t byte_val = data[glyph_offset + row * bytes_per_row + byte_idx];

				if (byte_val & (1 << bit_idx))
				{
					canvas.put_pixel(x_pos + col, y_pos + row, Canvas<PixFmt>::to_pixel_static(color));
				}
			}
		}
	}
};

/// Built-in 6x8 monospace font (ASCII 32-126).
/// Classic embedded font — readable at small sizes.
extern const BitmapFont font_6x8;

} // namespace lumen::gfx
