#pragma once

#include <cstring>

#include "lumen/core/pixel_format.hpp"
#include "lumen/gfx/font_face.hpp"
#include "lumen/ui/widget.hpp"

namespace lumen::ui {

using ButtonCallback = void (*)();

/// Callback type for script execution: receives the script text.
using ScriptCallback = void (*)(const char* script);

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

	void set_font(const gfx::BitmapFont* font) { font_ = gfx::FontFace(font); }
	void set_font(const gfx::SdfFont* font, uint16_t size = 14) { font_ = gfx::FontFace(font, size); }
	void set_on_click(ButtonCallback callback) { on_click_ = callback; }

	/// Bind a script string to this button. When clicked, the script
	/// is passed to the ScriptCallback for execution.
	/// Both pointers must remain valid for the button's lifetime.
	void set_on_click_script(const char* script, ScriptCallback runner)
	{
		script_		   = script;
		script_runner_ = runner;
	}

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
				if (script_ != nullptr && script_runner_ != nullptr)
					script_runner_(script_);
				if (on_click_ != nullptr)
					on_click_();
			}
			awaiting_release_ = false;
			return true;
		}
		return false;
	}

	bool is_pressed() const { return pressed_ || press_visual_timer_ > 0; }

	void tick_visual()
	{
		if (press_visual_timer_ > 0)
		{
			--press_visual_timer_;
			if (press_visual_timer_ == 0 && !pressed_)
				invalidate();
		}
	}

	void draw(gfx::Canvas<ActivePixFmt>& canvas) override
	{
		Color bg = is_pressed() ? pressed_color_ : normal_color_;
		canvas.fill_rect(bounds(), bg);
		canvas.draw_rect(bounds(), Color::white());

		if (font_.is_valid() && label_[0] != '\0')
		{
			uint16_t text_w = font_.string_width(label_);
			int16_t text_x	= bounds().x + (bounds().w - text_w) / 2;
			int16_t text_y	= bounds().y + (bounds().h - font_.height()) / 2;
			font_.draw_string(canvas, text_x, text_y, label_, Color::white());
		}
	}

  private:
	static constexpr size_t MAX_LABEL		   = 32;
	char label_[MAX_LABEL]					   = "";
	Color normal_color_						   = Color::rgb(60, 120, 200);
	Color pressed_color_					   = Color::rgb(40, 80, 150);
	static constexpr uint8_t PRESS_VISUAL_HOLD = 3;
	ButtonCallback on_click_				   = nullptr;
	const char* script_						   = nullptr;
	ScriptCallback script_runner_			   = nullptr;
	bool pressed_							   = false;
	bool awaiting_release_					   = false;
	uint8_t press_visual_timer_				   = 0;
	gfx::FontFace font_{&gfx::font_6x8};
};

} // namespace lumen::ui
