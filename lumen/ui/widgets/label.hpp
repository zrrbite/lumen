#pragma once

#include <cstring>

#include "lumen/ui/widget.hpp"

namespace lumen::ui {

/// A simple text label. Renders text as colored rectangle for now
/// (bitmap font rendering comes in Phase 4).
class Label : public Widget
{
  public:
	Label() = default;
	explicit Label(const char* text) { set_text(text); }

	void set_text(const char* text)
	{
		if (std::strcmp(text_, text) != 0)
		{
			std::strncpy(text_, text, MAX_TEXT - 1);
			text_[MAX_TEXT - 1] = '\0';
			invalidate();
		}
	}

	const char* text() const { return text_; }

	void set_color(Color color)
	{
		fg_color_ = color;
		invalidate();
	}

	void set_bg_color(Color color)
	{
		bg_color_ = color;
		invalidate();
	}

	void draw(gfx::Canvas<Rgb565>& canvas) override
	{
		// Draw background
		canvas.fill_rect(bounds(), bg_color_);
		// Draw text placeholder — a colored bar proportional to text length
		uint16_t text_len = static_cast<uint16_t>(std::strlen(text_));
		if (text_len > 0)
		{
			uint16_t bar_w = text_len * 7; // ~7px per char placeholder
			if (bar_w > bounds().w - 4)
				bar_w = bounds().w - 4;
			Rect text_rect{static_cast<int16_t>(bounds().x + 2),
						   static_cast<int16_t>(bounds().y + 2),
						   bar_w,
						   static_cast<uint16_t>(bounds().h - 4)};
			canvas.fill_rect(text_rect, fg_color_);
		}
	}

  private:
	static constexpr size_t MAX_TEXT = 64;
	char text_[MAX_TEXT]			 = "";
	Color fg_color_					 = Color::white();
	Color bg_color_					 = Color::rgb(40, 40, 50);
};

} // namespace lumen::ui
