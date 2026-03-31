#pragma once

#include "lumen/core/pixel_format.hpp"
#include "lumen/ui/widget.hpp"

namespace lumen::ui {

/// A non-interactive progress bar.
class ProgressBar : public Widget
{
  public:
	ProgressBar() = default;

	void set_value(uint8_t percent)
	{
		if (percent > 100)
			percent = 100;
		if (percent != value_)
		{
			value_ = percent;
			invalidate();
		}
	}

	uint8_t value() const { return value_; }

	void set_fill_color(Color color)
	{
		fill_color_ = color;
		invalidate();
	}

	void draw(gfx::Canvas<ActivePixFmt>& canvas) override
	{
		// Background
		canvas.fill_rect(bounds(), Color::rgb(50, 50, 60));
		canvas.draw_rect(bounds(), Color::rgb(70, 70, 80));

		// Fill
		uint16_t fill_w = static_cast<uint16_t>((static_cast<uint32_t>(bounds().w - 2) * value_) / 100);
		if (fill_w > 0)
		{
			Rect fill{static_cast<int16_t>(bounds().x + 1),
					  static_cast<int16_t>(bounds().y + 1),
					  fill_w,
					  static_cast<uint16_t>(bounds().h - 2)};
			canvas.fill_rect(fill, fill_color_);
		}
	}

  private:
	uint8_t value_	  = 0;
	Color fill_color_ = Color::rgb(50, 180, 80);
};

} // namespace lumen::ui
