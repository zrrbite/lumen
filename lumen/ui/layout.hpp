#pragma once

#include "lumen/core/types.hpp"
#include "lumen/ui/container.hpp"

namespace lumen::ui {

enum class LayoutDir : uint8_t
{
	Horizontal,
	Vertical
};

/// BoxLayout — stacks children horizontally or vertically with spacing.
/// Applied once at setup time (not per-frame).
class BoxLayout
{
  public:
	BoxLayout(LayoutDir dir, uint8_t spacing = 4, uint8_t padding = 8) : dir_(dir), spacing_(spacing), padding_(padding)
	{}

	/// Apply layout to all children of a container within available area.
	void apply(Container& container, Rect area)
	{
		uint8_t count = container.child_count();
		if (count == 0)
			return;

		int16_t cursor_x  = area.x + padding_;
		int16_t cursor_y  = area.y + padding_;
		uint16_t usable_w = area.w - padding_ * 2;
		uint16_t usable_h = area.h - padding_ * 2;

		if (dir_ == LayoutDir::Vertical)
		{
			// Divide height equally among children
			uint16_t child_h = (usable_h - (count - 1) * spacing_) / count;
			for (uint8_t idx = 0; idx < count; ++idx)
			{
				Widget* child = container.child(idx);
				if (child != nullptr)
				{
					child->set_bounds({cursor_x, cursor_y, usable_w, child_h});
					cursor_y += child_h + spacing_;
				}
			}
		}
		else
		{
			// Divide width equally among children
			uint16_t child_w = (usable_w - (count - 1) * spacing_) / count;
			for (uint8_t idx = 0; idx < count; ++idx)
			{
				Widget* child = container.child(idx);
				if (child != nullptr)
				{
					child->set_bounds({cursor_x, cursor_y, child_w, usable_h});
					cursor_x += child_w + spacing_;
				}
			}
		}
	}

  private:
	LayoutDir dir_;
	uint8_t spacing_;
	uint8_t padding_;
};

/// GridLayout — arranges children in a rows x cols grid.
class GridLayout
{
  public:
	GridLayout(uint8_t cols, uint8_t rows, uint8_t spacing = 4, uint8_t padding = 8)
		: cols_(cols), rows_(rows), spacing_(spacing), padding_(padding)
	{}

	void apply(Container& container, Rect area)
	{
		uint8_t count = container.child_count();
		if (count == 0 || cols_ == 0 || rows_ == 0)
			return;

		uint16_t usable_w = area.w - padding_ * 2;
		uint16_t usable_h = area.h - padding_ * 2;
		uint16_t cell_w	  = (usable_w - (cols_ - 1) * spacing_) / cols_;
		uint16_t cell_h	  = (usable_h - (rows_ - 1) * spacing_) / rows_;

		for (uint8_t idx = 0; idx < count && idx < cols_ * rows_; ++idx)
		{
			uint8_t col = idx % cols_;
			uint8_t row = idx / cols_;

			int16_t cell_x = area.x + padding_ + col * (cell_w + spacing_);
			int16_t cell_y = area.y + padding_ + row * (cell_h + spacing_);

			Widget* child = container.child(idx);
			if (child != nullptr)
			{
				child->set_bounds({cell_x, cell_y, cell_w, cell_h});
			}
		}
	}

  private:
	uint8_t cols_;
	uint8_t rows_;
	uint8_t spacing_;
	uint8_t padding_;
};

} // namespace lumen::ui
