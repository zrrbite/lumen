#pragma once

#include <cstring>

#include "lumen/gfx/font.hpp"
#include "lumen/ui/widget.hpp"

namespace lumen::ui {

using ButtonCallback = void (*)();

/// A pressable button with text and visual feedback.
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
			if (!awaiting_release_)
			{
				pressed_			= true;
				awaiting_release_	= true;
				press_visual_timer_ = PRESS_VISUAL_HOLD;
				invalidate();
			}
			return true;
		}
		if (event.type == TouchEvent::Type::Release)
		{
			if (pressed_)
			{
				pressed_ = false;
				invalidate();
				if (on_click_ != nullptr)
				{
					on_click_();
				}
			}
			awaiting_release_ = false;
			return true;
		}
		return false;
	}

	bool is_pressed() const { return pressed_ || press_visual_timer_ > 0; }

	/// Call from update_model() to tick down the visual hold timer.
	void tick_visual()
	{
		if (press_visual_timer_ > 0)
		{
			--press_visual_timer_;
			if (press_visual_timer_ == 0 && !pressed_)
				invalidate();
		}
	}

	void draw(gfx::Canvas<Rgb565>& canvas) override
	{
		Color bg = is_pressed() ? pressed_color_ : normal_color_;
		canvas.fill_rect(bounds(), bg);
		canvas.draw_rect(bounds(), Color::white());

		// Render text centered
		if (font_ && label_[0] != '\0')
		{
			uint16_t text_w = font_->string_width(label_);
			int16_t text_x	= bounds().x + (bounds().w - text_w) / 2;
			int16_t text_y	= bounds().y + (bounds().h - font_->char_height) / 2;
			font_->draw_string(canvas, text_x, text_y, label_, Color::white());
		}
	}

  private:
	static constexpr size_t MAX_LABEL		   = 32;
	char label_[MAX_LABEL]					   = "";
	Color normal_color_						   = Color::rgb(60, 120, 200);
	Color pressed_color_					   = Color::rgb(40, 80, 150);
	static constexpr uint8_t PRESS_VISUAL_HOLD = 3; // Frames to hold pressed look
	ButtonCallback on_click_				   = nullptr;
	bool pressed_							   = false;
	bool awaiting_release_					   = false;
	uint8_t press_visual_timer_				   = 0;
	const gfx::BitmapFont* font_			   = &gfx::font_6x8;
};

} // namespace lumen::ui
