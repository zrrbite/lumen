#pragma once

#include "lumen/core/types.hpp"
#include "lumen/gfx/canvas.hpp"
#include "lumen/hal/touch_driver.hpp"

namespace lumen::ui {

/// Touch event passed to widgets.
struct TouchEvent
{
	enum class Type : uint8_t
	{
		Press,
		Release,
		Move
	};

	Type type;
	Point pos; // In screen coordinates
};

/// Base class for all visible UI elements.
class Widget
{
  public:
	virtual ~Widget() = default;

	// --- Geometry ---
	Rect bounds() const { return bounds_; }
	void set_bounds(Rect r)
	{
		if (r.x != bounds_.x || r.y != bounds_.y || r.w != bounds_.w || r.h != bounds_.h)
		{
			invalidate();
			bounds_ = r;
			invalidate();
		}
	}

	// --- Rendering ---
	/// Draw this widget into the canvas. Canvas is clipped to the dirty region.
	virtual void draw(gfx::Canvas<Rgb565>& canvas) = 0;

	// --- Dirty tracking ---
	void invalidate() { dirty_ = true; }
	bool is_dirty() const { return dirty_; }
	void mark_clean() { dirty_ = false; }

	// --- Input ---
	/// Handle a touch event. Return true if consumed.
	virtual bool on_touch(const TouchEvent& /*event*/) { return false; }

	// --- Hierarchy ---
	Widget* parent() const { return parent_; }

	// --- Visibility ---
	void set_visible(bool v)
	{
		if (v != visible_)
		{
			visible_ = v;
			invalidate();
		}
	}
	bool is_visible() const { return visible_; }

	/// Override in Container to support tree traversal without RTTI.
	virtual bool is_container() const { return false; }

  protected:
	Rect bounds_{};
	Widget* parent_ = nullptr;
	bool dirty_		= true;
	bool visible_	= true;

	friend class Container;
};

} // namespace lumen::ui
