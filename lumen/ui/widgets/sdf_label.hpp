#pragma once

#include <cstring>

#include "lumen/gfx/sdf_font.hpp"
#include "lumen/ui/widget.hpp"

namespace lumen::ui {

/// A text label that renders using an SDF font at any target size.
class SdfLabel : public Widget
{
  public:
	SdfLabel() = default;

	void set_text(const char* text)
	{
		if (std::strcmp(text_, text) != 0)
		{
			std::strncpy(text_, text, MAX_TEXT - 1);
			text_[MAX_TEXT - 1] = '\0';
			invalidate();
		}
	}

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

	void set_font(const gfx::SdfFont* font) { font_ = font; }

	void set_target_size(uint16_t size)
	{
		target_size_ = size;
		invalidate();
	}

	void draw(gfx::Canvas<Rgb565>& canvas) override
	{
		canvas.fill_rect(bounds(), bg_color_);

		if (font_ != nullptr && text_[0] != '\0')
		{
			// Center text vertically
			uint16_t text_h = static_cast<uint16_t>((font_->cell_h * target_size_) / font_->base_size);
			int16_t text_y	= bounds().y + (bounds().h - text_h) / 2;
			font_->draw_string(canvas, static_cast<int16_t>(bounds().x + 2), text_y, text_, fg_color_, target_size_);
		}
	}

  private:
	static constexpr size_t MAX_TEXT = 64;
	char text_[MAX_TEXT]			 = "";
	Color fg_color_					 = Color::white();
	Color bg_color_					 = Color::rgb(40, 40, 50);
	const gfx::SdfFont* font_		 = nullptr;
	uint16_t target_size_			 = 14;
};

} // namespace lumen::ui
