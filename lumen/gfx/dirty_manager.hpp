#pragma once

#include <array>

#include "lumen/core/types.hpp"

namespace lumen::gfx {

/// Tracks dirty (invalidated) screen regions for partial redraw.
/// When full, merges the two rects with smallest wasted area.
class DirtyManager
{
  public:
	static constexpr uint8_t MAX_RECTS = 8;

	void add(Rect rect)
	{
		if (rect.empty())
			return;

		if (count_ < MAX_RECTS)
		{
			rects_[count_++] = rect;
		}
		else
		{
			// Merge: find pair with smallest wasted area
			merge_smallest();
			rects_[count_++] = rect;
		}
	}

	void clear() { count_ = 0; }
	bool has_dirty() const { return count_ > 0; }
	uint8_t count() const { return count_; }
	const Rect& rect(uint8_t idx) const { return rects_[idx]; }

  private:
	void merge_smallest()
	{
		if (count_ < 2)
			return;

		// Find pair whose bounding union wastes least extra area
		uint32_t best_waste = UINT32_MAX;
		uint8_t best_a		= 0;
		uint8_t best_b		= 1;

		for (uint8_t a = 0; a < count_; ++a)
		{
			for (uint8_t b = a + 1; b < count_; ++b)
			{
				Rect merged	   = rects_[a].bounding_union(rects_[b]);
				uint32_t waste = merged.area() - rects_[a].area() - rects_[b].area();
				Rect inter	   = rects_[a].intersection(rects_[b]);
				waste += inter.area(); // Don't double-count overlap
				if (waste < best_waste)
				{
					best_waste = waste;
					best_a	   = a;
					best_b	   = b;
				}
			}
		}

		// Replace pair with their bounding union
		rects_[best_a] = rects_[best_a].bounding_union(rects_[best_b]);
		// Remove best_b by shifting
		for (uint8_t i = best_b; i < count_ - 1; ++i)
		{
			rects_[i] = rects_[i + 1];
		}
		--count_;
	}

	std::array<Rect, MAX_RECTS> rects_{};
	uint8_t count_ = 0;
};

} // namespace lumen::gfx
