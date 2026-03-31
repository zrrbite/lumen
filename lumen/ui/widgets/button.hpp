#pragma once

#include <cstring>

#include "lumen/ui/widget.hpp"

namespace lumen::ui {

/// Callback type for button press.
using ButtonCallback = void (*)();

/// A pressable button with visual feedback.
class Button : public Widget
{
  public:
	Button() = default;
	explicit Button(const char* label) { set_label(label); }

	void set_label(const char* label)
	{
		std::strncpy(label_, label, MAX_LABEL - 1);
		label_[MAX_LABEL - 1] = '\0';
		invalidate();
	}

	const char* label() const { return label_; }

	void set_color(Color normal, Color pressed)
	{
		normal_color_  = normal;
		pressed_color_ = pressed;
		invalidate();
	}

	void set_on_click(ButtonCallback callback) { on_click_ = callback; }

	bool on_touch(const TouchEvent& event) override
	{
		if (event.type == TouchEvent::Type::Press)
		{
			pressed_ = true;
			invalidate();
			return true;
		}
		if (event.type == TouchEvent::Type::Release)
		{
			if (pressed_)
			{
				pressed_ = false;
				invalidate();
				if (on_click_)
				{
					on_click_();
				}
			}
			return true;
		}
		return false;
	}

	bool is_pressed() const { return pressed_; }

	void draw(gfx::Canvas<Rgb565>& canvas) override
	{
		Color bg = pressed_ ? pressed_color_ : normal_color_;
		canvas.fill_rect(bounds(), bg);
		canvas.draw_rect(bounds(), Color::white());

		// Text placeholder
		uint16_t text_len = static_cast<uint16_t>(std::strlen(label_));
		if (text_len > 0)
		{
			uint16_t bar_w = text_len * 7;
			if (bar_w > bounds().w - 8)
				bar_w = bounds().w - 8;
			int16_t text_x = bounds().x + (bounds().w - bar_w) / 2;
			int16_t text_y = bounds().y + bounds().h / 4;
			Rect text_rect{text_x, text_y, bar_w, static_cast<uint16_t>(bounds().h / 2)};
			canvas.fill_rect(text_rect, Color::white());
		}
	}

  private:
	static constexpr size_t MAX_LABEL = 32;
	char label_[MAX_LABEL]			  = "";
	Color normal_color_				  = Color::rgb(60, 120, 200);
	Color pressed_color_			  = Color::rgb(40, 80, 150);
	ButtonCallback on_click_		  = nullptr;
	bool pressed_					  = false;
};

} // namespace lumen::ui
