#pragma once

/// SDF (Signed Distance Field) font renderer.
/// A single SDF font atlas can render text at any size with smooth edges.
///
/// Distance field encoding: 0 = far outside, 128 = on edge, 255 = far inside.
/// Rendering: sample the SDF, threshold at 128 for crisp edges, or use a
/// gradient for anti-aliased rendering.

#include <cstdint>

#include "lumen/core/types.hpp"
#include "lumen/gfx/canvas.hpp"

namespace lumen::gfx {

struct SdfFont
{
	const uint8_t* data; ///< 8-bit distance field data, one cell per glyph
	uint16_t cell_w;	 ///< Width of each glyph cell in pixels
	uint16_t cell_h;	 ///< Height of each glyph cell in pixels
	uint16_t base_size;	 ///< Base rasterization size (SDF was generated at this size)
	float spread;		 ///< SDF spread in base-size pixels
	uint8_t first_char;
	uint8_t last_char;

	/// Draw a string at the given position and target size.
	template <typename PixFmt>
	void draw_string(
		Canvas<PixFmt>& canvas, int16_t x_pos, int16_t y_pos, const char* text, Color color, uint16_t target_size) const
	{
		uint32_t scale_num = target_size;
		uint32_t scale_den = base_size;
		uint16_t out_w	   = static_cast<uint16_t>((cell_w * scale_num) / scale_den);
		int16_t advance	   = static_cast<int16_t>((out_w * 3) / 4);

		int16_t cursor_x = x_pos;
		while (*text)
		{
			uint8_t ch = static_cast<uint8_t>(*text);
			if (ch >= first_char && ch <= last_char)
			{
				draw_char_sdf(canvas, cursor_x, y_pos, ch, color, target_size);
			}
			cursor_x += advance;
			++text;
		}
	}

	/// Measure string width at target size.
	uint16_t string_width(const char* text, uint16_t target_size) const
	{
		uint16_t len = 0;
		while (*text)
		{
			++len;
			++text;
		}
		if (len == 0)
			return 0;
		uint16_t out_w	 = static_cast<uint16_t>((cell_w * target_size) / base_size);
		uint16_t advance = static_cast<uint16_t>((out_w * 3) / 4);
		return static_cast<uint16_t>((len - 1) * advance + out_w);
	}

	/// Draw a single SDF character, scaled to target_size.
	/// Uses 3-level rendering for anti-aliasing on RGB565:
	///   - Above edge_hi: full color
	///   - Between edge_lo and edge_hi: half-intensity (simple AA)
	///   - Below edge_lo: skip (background)
	template <typename PixFmt>
	void draw_char_sdf(
		Canvas<PixFmt>& canvas, int16_t x_pos, int16_t y_pos, uint8_t ch, Color color, uint16_t target_size) const
	{
		if (ch < first_char || ch > last_char)
			return;

		uint32_t glyph_idx	  = ch - first_char;
		uint32_t glyph_offset = glyph_idx * cell_w * cell_h;
		const uint8_t* sdf	  = data + glyph_offset;

		uint32_t scale_num = target_size;
		uint32_t scale_den = base_size;
		uint16_t out_w	   = static_cast<uint16_t>((cell_w * scale_num) / scale_den);
		uint16_t out_h	   = static_cast<uint16_t>((cell_h * scale_num) / scale_den);

		auto px_full = Canvas<PixFmt>::to_pixel_static(color);

		// Half-intensity color for edge anti-aliasing
		auto px_edge = Canvas<PixFmt>::to_pixel_static(Color::rgb(color.r / 2, color.g / 2, color.b / 2));

		// Thresholds
		uint8_t edge_lo = 105; // Below: outside
		uint8_t edge_hi = 150; // Above: fully inside

		for (uint16_t oy = 0; oy < out_h; ++oy)
		{
			for (uint16_t ox = 0; ox < out_w; ++ox)
			{
				uint32_t sx = (ox * scale_den) / scale_num;
				uint32_t sy = (oy * scale_den) / scale_num;

				if (sx >= cell_w)
					sx = cell_w - 1;
				if (sy >= cell_h)
					sy = cell_h - 1;

				uint8_t dist = sdf[sy * cell_w + sx];

				int16_t out_x = static_cast<int16_t>(x_pos + ox);
				int16_t out_y = static_cast<int16_t>(y_pos + oy);

				if (dist >= edge_hi)
				{
					canvas.put_pixel(out_x, out_y, px_full);
				}
				else if (dist >= edge_lo)
				{
					canvas.put_pixel(out_x, out_y, px_edge);
				}
			}
		}
	}
};

} // namespace lumen::gfx
