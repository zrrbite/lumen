#pragma once

#include "lumen/core/pixel_format.hpp"
#include "lumen/ui/widget.hpp"

namespace lumen::ui {

/// A widget that contains child widgets. Fixed-size array — no heap.
class Container : public Widget
{
  public:
	static constexpr uint8_t MAX_CHILDREN = 32;

	bool is_container() const override { return true; }

	void add(Widget& child)
	{
		if (count_ < MAX_CHILDREN)
		{
			children_[count_++] = &child;
			child.parent_		= this;
			child.invalidate();
		}
	}

	void remove(Widget& child)
	{
		for (uint8_t i = 0; i < count_; ++i)
		{
			if (children_[i] == &child)
			{
				child.parent_ = nullptr;
				// Shift remaining
				for (uint8_t j = i; j < count_ - 1; ++j)
				{
					children_[j] = children_[j + 1];
				}
				--count_;
				invalidate();
				return;
			}
		}
	}

	void draw(gfx::Canvas<ActivePixFmt>& canvas) override
	{
		for (uint8_t i = 0; i < count_; ++i)
		{
			if (children_[i]->is_visible())
			{
				children_[i]->draw(canvas);
			}
		}
	}

	bool on_touch(const TouchEvent& event) override
	{
		// Dispatch to children in reverse order (top-most first)
		for (int i = count_ - 1; i >= 0; --i)
		{
			if (children_[i]->is_visible() && children_[i]->bounds().contains(event.pos))
			{
				if (children_[i]->on_touch(event))
				{
					return true;
				}
			}
		}
		return false;
	}

	/// Handle input actions -- focus navigation.
	bool on_input(InputAction action) override
	{
		if (focus_idx_ < count_ && children_[focus_idx_]->on_input(action))
			return true;

		if (action == InputAction::FocusNext)
			return focus_move(1);
		if (action == InputAction::FocusPrev)
			return focus_move(-1);
		if (action == InputAction::Activate && focus_idx_ < count_)
		{
			TouchEvent press{TouchEvent::Type::Press, {0, 0}};
			TouchEvent release{TouchEvent::Type::Release, {0, 0}};
			children_[focus_idx_]->on_touch(press);
			children_[focus_idx_]->on_touch(release);
			return true;
		}
		return false;
	}

	uint8_t child_count() const { return count_; }
	Widget* child(uint8_t i) const { return (i < count_) ? children_[i] : nullptr; }

  private:
	Widget* children_[MAX_CHILDREN]{};
	uint8_t count_	   = 0;
	uint8_t focus_idx_ = 0;

	bool focus_move(int8_t dir)
	{
		if (count_ == 0)
			return false;
		if (focus_idx_ < count_)
			children_[focus_idx_]->set_focused(false);
		for (uint8_t i = 0; i < count_; ++i)
		{
			focus_idx_ = static_cast<uint8_t>((focus_idx_ + dir + count_) % count_);
			if (children_[focus_idx_]->is_focusable())
			{
				children_[focus_idx_]->set_focused(true);
				return true;
			}
		}
		return false;
	}
};

} // namespace lumen::ui
