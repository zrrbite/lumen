#pragma once

#include "lumen/core/types.hpp"

namespace lumen::ui {

/// Visual style for a widget — plain struct, no inheritance.
struct Style
{
	Color bg_color		  = Color::rgb(40, 40, 50);
	Color fg_color		  = Color::white();
	Color border_color	  = Color::rgb(80, 80, 90);
	uint8_t border_width  = 1;
	uint8_t corner_radius = 0; // TODO: rounded corners
	uint8_t padding		  = 4;
};

/// Theme — a collection of styles for all widget types and states.
struct Theme
{
	// General
	Color background = Color::rgb(20, 20, 30);

	// Label
	Style label = {Color::rgb(40, 40, 50), Color::white(), {}, 0, 0, 2};

	// Button
	Style button_normal	 = {Color::rgb(60, 120, 200), Color::white(), Color::rgb(80, 140, 220), 1, 0, 4};
	Style button_pressed = {Color::rgb(40, 80, 150), Color::white(), Color::rgb(60, 100, 170), 1, 0, 4};

	// Checkbox
	Style checkbox_off = {Color::rgb(60, 60, 70), Color::white(), Color::rgb(100, 100, 110), 2, 0, 0};
	Style checkbox_on  = {Color::rgb(50, 160, 80), Color::white(), Color::rgb(70, 180, 100), 2, 0, 0};

	// Slider
	Style slider_track = {Color::rgb(50, 50, 60), {}, Color::rgb(70, 70, 80), 1, 0, 0};
	Style slider_knob  = {Color::rgb(80, 160, 220), {}, Color::white(), 1, 0, 0};

	// ProgressBar
	Style progress_bg	= {Color::rgb(50, 50, 60), {}, Color::rgb(70, 70, 80), 1, 0, 0};
	Style progress_fill = {Color::rgb(50, 180, 80), {}, {}, 0, 0, 0};

	// Toggle
	Style toggle_off = {Color::rgb(80, 80, 90), Color::rgb(180, 180, 180), {}, 0, 0, 0};
	Style toggle_on	 = {Color::rgb(50, 160, 80), Color::white(), {}, 0, 0, 0};

	/// Default dark theme.
	static Theme dark() { return {}; }
};

} // namespace lumen::ui
