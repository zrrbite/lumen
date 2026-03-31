#pragma once

#include "lumen/ui/widget.hpp"

namespace lumen::ui {

using CheckboxCallback = void (*)(bool checked);

/// A toggleable checkbox.
class Checkbox : public Widget
{
  public:
	Checkbox() = default;

	void set_checked(bool checked)
	{
		if (checked != checked_)
		{
			checked_ = checked;
			invalidate();
		}
	}

	bool is_checked() const { return checked_; }
	void toggle() { set_checked(!checked_); }

	void set_on_change(CheckboxCallback callback) { on_change_ = callback; }

	bool on_touch(const TouchEvent& event) override
	{
		if (event.type == TouchEvent::Type::Press)
		{
			toggle();
			if (on_change_)
			{
				on_change_(checked_);
			}
			return true;
		}
		return false;
	}

	void draw(gfx::Canvas<Rgb565>& canvas) override
	{
		Color bg	 = checked_ ? Color::rgb(50, 160, 80) : Color::rgb(60, 60, 70);
		Color border = checked_ ? Color::rgb(70, 180, 100) : Color::rgb(100, 100, 110);

		canvas.fill_rect(bounds(), bg);
		canvas.draw_rect(bounds(), border);

		// Checkmark: draw an X-like mark when checked
		if (checked_)
		{
			int16_t cx	= bounds().x + bounds().w / 4;
			int16_t cy	= bounds().y + bounds().h / 4;
			uint16_t cw = bounds().w / 2;
			uint16_t ch = bounds().h / 2;
			canvas.fill_rect({cx, cy, cw, ch}, Color::white());
		}
	}

  private:
	bool checked_				= false;
	CheckboxCallback on_change_ = nullptr;
};

} // namespace lumen::ui
