#pragma once

/// Touch driver concept for Lumen.
///
/// Any touch driver must provide:
///   void init();
///   bool poll(TouchPoint& out);
///   uint8_t max_points() const;

#include "lumen/core/types.hpp"

namespace lumen::hal {

struct TouchPoint
{
	Point pos;
	bool pressed	 = false;
	uint8_t pressure = 0; // 0-255, relevant for resistive
};

/// Null touch driver — for boards without a touchscreen.
struct NullTouch
{
	void init() {}
	bool poll(TouchPoint&) { return false; }
	uint8_t max_points() const { return 0; }
};

} // namespace lumen::hal
