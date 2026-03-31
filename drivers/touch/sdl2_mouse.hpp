#pragma once

#ifdef LUMEN_PLATFORM_DESKTOP

#include <SDL2/SDL.h>

#include "lumen/hal/touch_driver.hpp"

namespace lumen::drivers {

/// SDL2 mouse input as a touch driver for desktop simulation.
template <typename Display> class Sdl2Mouse
{
  public:
	explicit Sdl2Mouse(Display& display) : display_(display) {}

	void init() {}

	bool poll(hal::TouchPoint& out)
	{
		Point pos;
		bool pressed = display_.get_mouse(pos);

		// Only report when state changes or while pressed
		if (pressed || was_pressed_)
		{
			out.pos		 = pos;
			out.pressed	 = pressed;
			out.pressure = pressed ? 255 : 0;
			was_pressed_ = pressed;
			return true;
		}
		return false;
	}

	uint8_t max_points() const { return 1; }

  private:
	Display& display_;
	bool was_pressed_ = false;
};

} // namespace lumen::drivers

#endif // LUMEN_PLATFORM_DESKTOP
