#pragma once

#include "lumen/core/types.hpp"
#include "lumen/ui/widget.hpp"

namespace lumen::ui {

/// Recognized gesture types.
enum class GestureType : uint8_t
{
	None,
	Tap,
	LongPress,
	SwipeLeft,
	SwipeRight,
	SwipeUp,
	SwipeDown,
};

/// Event emitted when a gesture is recognized.
struct GestureEvent
{
	GestureType type = GestureType::None;
	Point start{};
	Point end{};
	TickMs duration = 0;
};

/// Callback invoked when a gesture is detected.
using GestureCallback = void (*)(const GestureEvent&);

/// Detects tap, long-press, and swipe gestures from raw touch events.
///
/// Feed touch events via on_touch() and call update() periodically so that
/// long-press timeouts fire even when no new touch events arrive.
/// No heap allocations — all state is inline.
class GestureDetector
{
  public:
	/// Minimum hold time (ms) before a long press fires.
	TickMs long_press_ms = 500;

	/// Minimum movement (pixels) to classify as a swipe rather than a tap.
	int16_t swipe_threshold = 30;

	/// Register a callback for recognized gestures.
	void set_on_gesture(GestureCallback cb) { callback_ = cb; }

	/// Feed a raw touch event. Call this from a widget's on_touch().
	void on_touch(const TouchEvent& event, TickMs now)
	{
		switch (event.type)
		{
		case TouchEvent::Type::Press:
			active_		  = true;
			moved_		  = false;
			long_pressed_ = false;
			start_		  = event.pos;
			last_		  = event.pos;
			press_time_	  = now;
			break;

		case TouchEvent::Type::Move:
			if (!active_)
				break;
			last_	   = event.pos;
			int16_t dx = static_cast<int16_t>(last_.x - start_.x);
			int16_t dy = static_cast<int16_t>(last_.y - start_.y);
			if (abs_i16(dx) > swipe_threshold || abs_i16(dy) > swipe_threshold)
				moved_ = true;
			break;

		case TouchEvent::Type::Release:
			if (!active_)
				break;
			last_		   = event.pos;
			TickMs elapsed = now - press_time_;

			if (!moved_ && !long_pressed_)
			{
				fire({GestureType::Tap, start_, last_, elapsed});
			}
			else if (moved_)
			{
				int16_t dx = static_cast<int16_t>(last_.x - start_.x);
				int16_t dy = static_cast<int16_t>(last_.y - start_.y);
				fire({swipe_direction(dx, dy), start_, last_, elapsed});
			}
			active_ = false;
			break;
		}
	}

	/// Call periodically (e.g. each frame) to detect long-press timeouts.
	void update(TickMs now)
	{
		if (!active_ || moved_ || long_pressed_)
			return;

		if (now - press_time_ >= long_press_ms)
		{
			long_pressed_  = true;
			TickMs elapsed = now - press_time_;
			fire({GestureType::LongPress, start_, last_, elapsed});
		}
	}

  private:
	GestureCallback callback_ = nullptr;

	Point start_{};
	Point last_{};
	TickMs press_time_ = 0;
	bool active_	   = false;
	bool moved_		   = false;
	bool long_pressed_ = false;

	void fire(const GestureEvent& event)
	{
		if (callback_)
			callback_(event);
	}

	static constexpr int16_t abs_i16(int16_t v) { return v < 0 ? static_cast<int16_t>(-v) : v; }

	static constexpr GestureType swipe_direction(int16_t dx, int16_t dy)
	{
		if (abs_i16(dx) >= abs_i16(dy))
			return dx < 0 ? GestureType::SwipeLeft : GestureType::SwipeRight;
		return dy < 0 ? GestureType::SwipeUp : GestureType::SwipeDown;
	}
};

} // namespace lumen::ui
