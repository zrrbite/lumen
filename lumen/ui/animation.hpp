#pragma once

/// Animation system for Lumen.
/// Provides easing functions, property tweens, and an animation manager.
/// Zero heap allocation — all animations stored in fixed-size arrays.

#include <cstdint>

#include "lumen/core/types.hpp"

namespace lumen::ui {

/// Standard easing functions. All take t in [0,1] and return [0,1].
namespace ease {

inline float linear(float t)
{
	return t;
}

inline float in_quad(float t)
{
	return t * t;
}
inline float out_quad(float t)
{
	return t * (2.0f - t);
}
inline float in_out_quad(float t)
{
	return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

inline float in_cubic(float t)
{
	return t * t * t;
}
inline float out_cubic(float t)
{
	float u = t - 1.0f;
	return u * u * u + 1.0f;
}
inline float in_out_cubic(float t)
{
	return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
}

inline float in_expo(float t)
{
	return t == 0.0f ? 0.0f : (1 << static_cast<int>(10 * (t - 1))) / 1024.0f;
}
inline float out_expo(float t)
{
	return t == 1.0f ? 1.0f : 1.0f - (1 << static_cast<int>(-10 * t)) / 1024.0f;
}

inline float out_back(float t)
{
	const float c = 1.70158f;
	float u		  = t - 1.0f;
	return 1.0f + (c + 1.0f) * u * u * u + c * u * u;
}

inline float out_bounce(float t)
{
	if (t < 1.0f / 2.75f)
	{
		return 7.5625f * t * t;
	}
	if (t < 2.0f / 2.75f)
	{
		float u = t - 1.5f / 2.75f;
		return 7.5625f * u * u + 0.75f;
	}
	if (t < 2.5f / 2.75f)
	{
		float u = t - 2.25f / 2.75f;
		return 7.5625f * u * u + 0.9375f;
	}
	float u = t - 2.625f / 2.75f;
	return 7.5625f * u * u + 0.984375f;
}

} // namespace ease

/// Easing function pointer type.
using EaseFn = float (*)(float);

/// A single property animation (tween).
struct Tween
{
	float from		= 0.0f;
	float to		= 0.0f;
	TickMs duration = 0;
	TickMs start	= 0;
	TickMs delay	= 0;
	EaseFn easing	= ease::linear;
	float* target	= nullptr; ///< Pointer to the value being animated
	bool active		= false;
	bool started	= false;

	/// Update the tween. Returns true while active.
	bool update(TickMs now)
	{
		if (!active || target == nullptr)
			return false;

		if (!started)
		{
			if (now - start < delay)
				return true; // Still waiting
			start += delay;
			started = true;
		}

		TickMs elapsed = now - start;
		if (elapsed >= duration)
		{
			*target = to;
			active	= false;
			return false;
		}

		float t = static_cast<float>(elapsed) / static_cast<float>(duration);
		float e = easing(t);
		*target = from + (to - from) * e;
		return true;
	}
};

/// Manages a fixed set of concurrent animations.
class AnimationManager
{
  public:
	static constexpr uint8_t MAX_TWEENS = 16;

	/// Start a new tween. Returns the tween index, or -1 if full.
	int animate(float* target,
				float from,
				float to,
				TickMs duration,
				TickMs now,
				EaseFn easing = ease::linear,
				TickMs delay  = 0)
	{
		for (uint8_t i = 0; i < MAX_TWEENS; ++i)
		{
			if (!tweens_[i].active)
			{
				tweens_[i] = {from, to, duration, now, delay, easing, target, true, false};
				return i;
			}
		}
		return -1;
	}

	/// Cancel a tween by index.
	void cancel(int index)
	{
		if (index >= 0 && index < MAX_TWEENS)
			tweens_[index].active = false;
	}

	/// Cancel all tweens targeting a specific value.
	void cancel_target(float* target)
	{
		for (auto& tw : tweens_)
		{
			if (tw.target == target)
				tw.active = false;
		}
	}

	/// Tick all active tweens. Call this every frame.
	/// Returns true if any animation is still running.
	bool update(TickMs now)
	{
		bool any_active = false;
		for (auto& tw : tweens_)
		{
			if (tw.update(now))
				any_active = true;
		}
		return any_active;
	}

	/// Check if any animations are running.
	bool is_animating() const
	{
		for (const auto& tw : tweens_)
		{
			if (tw.active)
				return true;
		}
		return false;
	}

  private:
	Tween tweens_[MAX_TWEENS]{};
};

} // namespace lumen::ui
