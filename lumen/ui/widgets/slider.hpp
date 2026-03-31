#pragma once

#include "lumen/core/pixel_format.hpp"
#include "lumen/ui/widget.hpp"

namespace lumen::ui {

using SliderCallback = void (*)(int16_t value);

/// A horizontal slider for selecting a value in a range.
class Slider : public Widget
{
  public:
	Slider() = default;
	Slider(int16_t min_val, int16_t max_val) : min_(min_val), max_(max_val), value_(min_val) {}

	void set_range(int16_t min_val, int16_t max_val)
	{
		min_ = min_val;
		max_ = max_val;
		if (value_ < min_)
			value_ = min_;
		if (value_ > max_)
			value_ = max_;
		invalidate();
	}

	void set_value(int16_t val)
	{
		if (val < min_)
			val = min_;
		if (val > max_)
			val = max_;
		if (val != value_)
		{
			value_ = val;
			invalidate();
		}
	}

	int16_t value() const { return value_; }
	int16_t min_value() const { return min_; }
	int16_t max_value() const { return max_; }

	void set_on_change(SliderCallback callback) { on_change_ = callback; }

	bool on_touch(const TouchEvent& event) override
	{
		if (event.type == TouchEvent::Type::Press || event.type == TouchEvent::Type::Move)
		{
			// Map touch X position to value
			int16_t rel_x = event.pos.x - bounds().x;
			if (rel_x < 0)
				rel_x = 0;
			if (rel_x > static_cast<int16_t>(bounds().w))
				rel_x = static_cast<int16_t>(bounds().w);

			int16_t new_val = min_ + static_cast<int16_t>((static_cast<int32_t>(rel_x) * (max_ - min_)) / bounds().w);
			set_value(new_val);

			if (on_change_)
			{
				on_change_(value_);
			}
			return true;
		}
		return false;
	}

	void draw(gfx::Canvas<ActivePixFmt>& canvas) override
	{
		// Track background
		canvas.fill_rect(bounds(), Color::rgb(50, 50, 60));
		canvas.draw_rect(bounds(), Color::rgb(70, 70, 80));

		// Fill bar
		int32_t range = max_ - min_;
		uint16_t fill_w =
			(range > 0) ? static_cast<uint16_t>((static_cast<int32_t>(value_ - min_) * bounds().w) / range) : 0;
		if (fill_w > 0)
		{
			Rect fill_rect{bounds().x, bounds().y, fill_w, bounds().h};
			canvas.fill_rect(fill_rect, Color::rgb(80, 160, 220));
		}

		// Knob
		int16_t knob_x = bounds().x + static_cast<int16_t>(fill_w) - 4;
		if (knob_x < bounds().x)
			knob_x = bounds().x;
		Rect knob{knob_x, static_cast<int16_t>(bounds().y - 2), 8, static_cast<uint16_t>(bounds().h + 4)};
		canvas.fill_rect(knob, Color::white());
	}

  private:
	int16_t min_			  = 0;
	int16_t max_			  = 100;
	int16_t value_			  = 0;
	SliderCallback on_change_ = nullptr;
};

} // namespace lumen::ui
