#pragma once

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

	void draw(gfx::Canvas<Rgb565>& canvas) override
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

	uint8_t child_count() const { return count_; }
	Widget* child(uint8_t i) const { return (i < count_) ? children_[i] : nullptr; }

  private:
	Widget* children_[MAX_CHILDREN]{};
	uint8_t count_ = 0;
};

} // namespace lumen::ui
