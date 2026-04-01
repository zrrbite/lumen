#pragma once

/// Screen transitions: slide, wipe, instant.
/// Integrates with Application::navigate_to() to animate between screens.
/// Works with band rendering (no framebuffer needed).

#include <cstdint>

#include "lumen/core/types.hpp"
#include "lumen/ui/animation.hpp"

namespace lumen::ui {

/// Transition direction for slides.
enum class Direction : uint8_t
{
	Left,
	Right,
	Up,
	Down,
};

/// Transition type.
enum class TransitionType : uint8_t
{
	Instant,   ///< No animation, immediate switch
	SlideIn,   ///< New screen slides in, old stays
	SlideOut,  ///< Old screen slides out, new stays
	SlideBoth, ///< Both screens slide together
	WipeDown,  ///< Horizontal line sweeps down, revealing new screen
	WipeRight, ///< Vertical line sweeps right, revealing new screen
	Crossfade, ///< Alpha blend between screens (framebuffer boards only)
};

/// Describes a screen transition.
struct Transition
{
	TransitionType type = TransitionType::Instant;
	Direction direction = Direction::Left;
	TickMs duration		= 300;
	EaseFn easing		= ease::out_cubic;
};

/// Manages an active transition between two screens.
/// The Application ticks this each frame during a transition.
class TransitionState
{
  public:
	bool is_active() const { return active_; }

	/// Start a transition.
	void start(const Transition& trans, TickMs now, uint16_t disp_w, uint16_t disp_h)
	{
		trans_	  = trans;
		start_	  = now;
		progress_ = 0.0f;
		active_	  = true;
		disp_w_	  = disp_w;
		disp_h_	  = disp_h;
	}

	/// Update transition progress. Returns false when complete.
	bool update(TickMs now)
	{
		if (!active_)
			return false;

		TickMs elapsed = now - start_;
		if (elapsed >= trans_.duration)
		{
			progress_ = 1.0f;
			active_	  = false;
			return false;
		}

		float t	  = static_cast<float>(elapsed) / static_cast<float>(trans_.duration);
		progress_ = trans_.easing(t);
		return true;
	}

	/// Get the pixel offset for the outgoing screen.
	Point outgoing_offset() const
	{
		if (trans_.type == TransitionType::Instant)
			return {0, 0};

		if (trans_.type == TransitionType::WipeDown || trans_.type == TransitionType::WipeRight)
			return {0, 0}; // Outgoing doesn't move in wipe

		if (trans_.type == TransitionType::SlideIn)
			return {0, 0}; // Outgoing stays in place

		// SlideOut or SlideBoth: outgoing moves away
		return calc_offset(true);
	}

	/// Get the pixel offset for the incoming screen.
	Point incoming_offset() const
	{
		if (trans_.type == TransitionType::Instant)
			return {0, 0};

		if (trans_.type == TransitionType::WipeDown || trans_.type == TransitionType::WipeRight)
			return {0, 0}; // Incoming doesn't move in wipe

		if (trans_.type == TransitionType::SlideOut)
			return {0, 0}; // Incoming is already in place

		// SlideIn or SlideBoth: incoming slides from off-screen
		return calc_offset(false);
	}

	/// For wipe transitions: get the boundary line position.
	int16_t wipe_position() const
	{
		if (trans_.type == TransitionType::WipeDown)
			return static_cast<int16_t>(progress_ * disp_h_);
		if (trans_.type == TransitionType::WipeRight)
			return static_cast<int16_t>(progress_ * disp_w_);
		return 0;
	}

	/// Get the clip rect for the outgoing screen during wipe.
	Rect outgoing_clip() const
	{
		if (trans_.type == TransitionType::WipeDown)
		{
			int16_t wp = wipe_position();
			return {0, wp, disp_w_, static_cast<uint16_t>(disp_h_ - wp)};
		}
		if (trans_.type == TransitionType::WipeRight)
		{
			int16_t wp = wipe_position();
			return {wp, 0, static_cast<uint16_t>(disp_w_ - wp), disp_h_};
		}
		return {0, 0, disp_w_, disp_h_};
	}

	/// Get the clip rect for the incoming screen during wipe.
	Rect incoming_clip() const
	{
		if (trans_.type == TransitionType::WipeDown)
			return {0, 0, disp_w_, static_cast<uint16_t>(wipe_position())};
		if (trans_.type == TransitionType::WipeRight)
			return {0, 0, static_cast<uint16_t>(wipe_position()), disp_h_};
		return {0, 0, disp_w_, disp_h_};
	}

	const Transition& transition() const { return trans_; }
	float progress() const { return progress_; }

	/// For crossfade: alpha of incoming screen (0-255).
	/// Outgoing alpha = 255 - incoming alpha.
	uint8_t crossfade_alpha() const { return static_cast<uint8_t>(progress_ * 255.0f); }

  private:
	Transition trans_;
	TickMs start_	 = 0;
	float progress_	 = 0.0f;
	bool active_	 = false;
	uint16_t disp_w_ = 0;
	uint16_t disp_h_ = 0;

	Point calc_offset(bool outgoing) const
	{
		int16_t full_x = 0;
		int16_t full_y = 0;

		switch (trans_.direction)
		{
		case Direction::Left:
			full_x = -static_cast<int16_t>(disp_w_);
			break;
		case Direction::Right:
			full_x = static_cast<int16_t>(disp_w_);
			break;
		case Direction::Up:
			full_y = -static_cast<int16_t>(disp_h_);
			break;
		case Direction::Down:
			full_y = static_cast<int16_t>(disp_h_);
			break;
		}

		if (outgoing)
		{
			// Outgoing: starts at (0,0), moves to full offset
			return {static_cast<int16_t>(full_x * progress_), static_cast<int16_t>(full_y * progress_)};
		}
		else
		{
			// Incoming: starts at -full offset, moves to (0,0)
			return {static_cast<int16_t>(-full_x * (1.0f - progress_)),
					static_cast<int16_t>(-full_y * (1.0f - progress_))};
		}
	}
};

// Pre-built transition presets
namespace transitions {

inline constexpr Transition instant()
{
	return {TransitionType::Instant, Direction::Left, 0, ease::linear};
}

inline Transition slide_left(TickMs duration = 300)
{
	return {TransitionType::SlideBoth, Direction::Left, duration, ease::out_cubic};
}

inline Transition slide_right(TickMs duration = 300)
{
	return {TransitionType::SlideBoth, Direction::Right, duration, ease::out_cubic};
}

inline Transition slide_up(TickMs duration = 300)
{
	return {TransitionType::SlideBoth, Direction::Up, duration, ease::out_cubic};
}

inline Transition slide_down(TickMs duration = 300)
{
	return {TransitionType::SlideBoth, Direction::Down, duration, ease::out_cubic};
}

inline Transition wipe_down(TickMs duration = 400)
{
	return {TransitionType::WipeDown, Direction::Down, duration, ease::in_out_cubic};
}

inline Transition wipe_right(TickMs duration = 400)
{
	return {TransitionType::WipeRight, Direction::Right, duration, ease::in_out_cubic};
}

inline Transition slide_in_left(TickMs duration = 300)
{
	return {TransitionType::SlideIn, Direction::Left, duration, ease::out_cubic};
}

inline Transition slide_out_right(TickMs duration = 300)
{
	return {TransitionType::SlideOut, Direction::Right, duration, ease::out_cubic};
}

inline Transition crossfade(TickMs duration = 500)
{
	return {TransitionType::Crossfade, Direction::Left, duration, ease::in_out_quad};
}

} // namespace transitions

} // namespace lumen::ui
