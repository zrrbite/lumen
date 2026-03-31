#pragma once

#include "lumen/core/types.hpp"

namespace lumen::gfx {

/// Canvas — draws primitives into a pixel buffer.
/// Template on pixel format for zero-overhead color conversion.
template <typename PixFmt> class Canvas
{
  public:
	using pixel_t = typename PixFmt::pixel_t;

	Canvas(pixel_t* buffer, uint16_t buf_width, uint16_t buf_height)
		: buf_(buffer), width_(buf_width), height_(buf_height)
	{}

	/// Set the origin offset for band rendering.
	/// Widgets draw in screen coordinates; the canvas subtracts
	/// the origin to index into the scratch buffer.
	void set_origin(int16_t x, int16_t y)
	{
		origin_x_ = x;
		origin_y_ = y;
	}

	/// Set clipping rectangle. All drawing is clipped to this area.
	void set_clip(Rect clip) { clip_ = clip; }

	/// Clear the clip area with a color.
	void clear(Color c)
	{
		pixel_t px = to_pixel(c);
		fill_rect(clip_, c);
	}

	/// Fill a rectangle with a solid color.
	void fill_rect(Rect r, Color c)
	{
		Rect clipped = r.intersection(clip_);
		if (clipped.empty())
			return;

		pixel_t px = to_pixel(c);
		for (int16_t y = clipped.y; y < clipped.bottom(); ++y)
		{
			for (int16_t x = clipped.x; x < clipped.right(); ++x)
			{
				put_pixel(x, y, px);
			}
		}
	}

	/// Draw a 1px border rectangle.
	void draw_rect(Rect r, Color c)
	{
		pixel_t px = to_pixel(c);
		// Top
		for (int16_t x = r.x; x < r.right(); ++x)
			put_pixel_clipped(x, r.y, px);
		// Bottom
		for (int16_t x = r.x; x < r.right(); ++x)
			put_pixel_clipped(x, r.bottom() - 1, px);
		// Left
		for (int16_t y = r.y; y < r.bottom(); ++y)
			put_pixel_clipped(r.x, y, px);
		// Right
		for (int16_t y = r.y; y < r.bottom(); ++y)
			put_pixel_clipped(r.right() - 1, y, px);
	}

	/// Draw a horizontal line.
	void h_line(int16_t x, int16_t y, uint16_t len, Color c)
	{
		pixel_t px = to_pixel(c);
		for (uint16_t i = 0; i < len; ++i)
		{
			put_pixel_clipped(x + i, y, px);
		}
	}

	/// Draw a vertical line.
	void v_line(int16_t x, int16_t y, uint16_t len, Color c)
	{
		pixel_t px = to_pixel(c);
		for (uint16_t i = 0; i < len; ++i)
		{
			put_pixel_clipped(x, y + i, px);
		}
	}

	/// Direct pixel write (no clipping). Coordinates are in screen space;
	/// origin offset is subtracted to map into the buffer.
	void put_pixel(int16_t x, int16_t y, pixel_t px)
	{
		int16_t bx = static_cast<int16_t>(x - origin_x_);
		int16_t by = static_cast<int16_t>(y - origin_y_);
		if (bx >= 0 && bx < width_ && by >= 0 && by < height_)
		{
			buf_[by * width_ + bx] = px;
		}
	}

	uint16_t width() const { return width_; }
	uint16_t height() const { return height_; }

  private:
	void put_pixel_clipped(int16_t x, int16_t y, pixel_t px)
	{
		if (clip_.contains({x, y}))
		{
			int16_t bx = static_cast<int16_t>(x - origin_x_);
			int16_t by = static_cast<int16_t>(y - origin_y_);
			if (bx >= 0 && bx < width_ && by >= 0 && by < height_)
			{
				buf_[by * width_ + bx] = px;
			}
		}
	}

  public:
	static pixel_t to_pixel_static(Color c) { return to_pixel(c); }

  private:
	static pixel_t to_pixel(Color c)
	{
		if constexpr (sizeof(pixel_t) == 2)
			return static_cast<pixel_t>(c.to_rgb565());
		else
			return static_cast<pixel_t>(c.to_argb8888());
	}

	pixel_t* buf_;
	uint16_t width_;
	uint16_t height_;
	int16_t origin_x_ = 0;
	int16_t origin_y_ = 0;
	Rect clip_{0, 0, 0xFFFF, 0xFFFF}; // Default: no clipping
};

} // namespace lumen::gfx
