#pragma once

#include <cstring>

#include "lumen/gfx/font_face.hpp"
#include "lumen/ui/widget.hpp"

namespace lumen::ui {

/// A text label supporting both bitmap and SDF fonts via FontFace.
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

	/// Set a bitmap font.
	void set_font(const gfx::BitmapFont* font) { font_ = gfx::FontFace(font); }

	/// Set an SDF font with target render size.
	void set_font(const gfx::SdfFont* font, uint16_t target_size = 14) { font_ = gfx::FontFace(font, target_size); }

	void draw(gfx::Canvas<ActivePixFmt>& canvas) override
	{
		canvas.fill_rect(bounds(), bg_color_);

		if (font_.is_valid() && text_[0] != '\0')
		{
			int16_t text_y = bounds().y + (bounds().h - font_.height()) / 2;
			font_.draw_string(canvas, static_cast<int16_t>(bounds().x + 4), text_y, text_, fg_color_);
		}
	}

  private:
	static constexpr size_t MAX_TEXT = 64;
	char text_[MAX_TEXT]			 = "";
	Color fg_color_					 = Color::white();
	Color bg_color_					 = Color::rgb(40, 40, 50);
	gfx::FontFace font_{&gfx::font_6x8};
};

} // namespace lumen::ui
