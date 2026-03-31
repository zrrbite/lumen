#pragma once

#include "lumen/ui/widget.hpp"

namespace lumen::ui {

using ToggleCallback = void (*)(bool on);

/// A toggle switch (on/off).
class Toggle : public Widget
{
  public:
	Toggle() = default;

	void set_on(bool on)
	{
		if (on != on_)
		{
			on_ = on;
			invalidate();
		}
	}

	bool is_on() const { return on_; }

	void set_on_change(ToggleCallback callback) { on_change_ = callback; }

	bool on_touch(const TouchEvent& event) override
	{
		if (event.type == TouchEvent::Type::Press)
		{
			on_ = !on_;
			invalidate();
			if (on_change_)
			{
				on_change_(on_);
			}
			return true;
		}
		return false;
	}

	void draw(gfx::Canvas<Rgb565>& canvas) override
	{
		// Track
		Color track_color = on_ ? Color::rgb(50, 160, 80) : Color::rgb(80, 80, 90);
		canvas.fill_rect(bounds(), track_color);

		// Knob (left when off, right when on)
		uint16_t knob_w = bounds().w / 2;
		int16_t knob_x	= on_ ? bounds().x + static_cast<int16_t>(bounds().w - knob_w) : bounds().x;
		Rect knob{knob_x, bounds().y, knob_w, bounds().h};
		Color knob_color = on_ ? Color::white() : Color::rgb(180, 180, 180);
		canvas.fill_rect(knob, knob_color);
		canvas.draw_rect(bounds(), Color::rgb(60, 60, 70));
	}

  private:
	bool on_				  = false;
	ToggleCallback on_change_ = nullptr;
};

} // namespace lumen::ui
