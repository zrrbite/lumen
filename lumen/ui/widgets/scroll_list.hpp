#pragma once

#include <cstring>

#include "lumen/core/pixel_format.hpp"
#include "lumen/gfx/font_face.hpp"
#include "lumen/ui/widget.hpp"

namespace lumen::ui {

/// Callback invoked when the user selects a list item.
using SelectCallback = void (*)(uint8_t index);

/// A vertically scrollable list of text items with touch-drag scrolling
/// and keyboard/encoder navigation.
class ScrollList : public Widget
{
  public:
	static constexpr uint8_t MAX_ITEMS	  = 32;
	static constexpr uint8_t MAX_TEXT_LEN = 32;

	ScrollList() { set_focusable(true); }

	// --- Item management ---

	/// Add an item to the list. Returns false if full.
	bool add_item(const char* text)
	{
		if (item_count_ >= MAX_ITEMS)
			return false;
		std::strncpy(items_[item_count_].text, text, MAX_TEXT_LEN - 1);
		items_[item_count_].text[MAX_TEXT_LEN - 1] = '\0';
		items_[item_count_].selected			   = false;
		++item_count_;
		invalidate();
		return true;
	}

	/// Remove all items and reset state.
	void clear_items()
	{
		item_count_		= 0;
		selected_index_ = -1;
		scroll_offset_	= 0;
		invalidate();
	}

	uint8_t item_count() const { return item_count_; }

	const char* item_text(uint8_t index) const
	{
		if (index >= item_count_)
			return "";
		return items_[index].text;
	}

	// --- Selection ---

	int16_t selected_index() const { return selected_index_; }

	void set_selected(int16_t index)
	{
		if (index < -1 || index >= item_count_)
			return;
		if (index != selected_index_)
		{
			if (selected_index_ >= 0 && selected_index_ < item_count_)
				items_[selected_index_].selected = false;
			selected_index_ = index;
			if (selected_index_ >= 0)
				items_[selected_index_].selected = true;
			invalidate();
		}
	}

	void set_on_select(SelectCallback callback) { on_select_ = callback; }

	// --- Appearance ---

	void set_item_height(uint16_t h)
	{
		item_height_ = h;
		invalidate();
	}

	uint16_t item_height() const { return item_height_; }

	void set_font(const gfx::BitmapFont* font) { font_ = gfx::FontFace(font); }
	void set_font(const gfx::SdfFont* font, uint16_t size = 14) { font_ = gfx::FontFace(font, size); }

	void set_colors(Color bg, Color fg, Color sel_bg, Color sel_fg)
	{
		bg_color_	  = bg;
		fg_color_	  = fg;
		sel_bg_color_ = sel_bg;
		sel_fg_color_ = sel_fg;
		invalidate();
	}

	// --- Scroll ---

	int16_t scroll_offset() const { return scroll_offset_; }

	void set_scroll_offset(int16_t offset)
	{
		offset		   = clamp_scroll(offset);
		scroll_offset_ = offset;
		invalidate();
	}

	// --- Input handling ---

	bool on_touch(const TouchEvent& event) override
	{
		switch (event.type)
		{
		case TouchEvent::Type::Press:
			dragging_		   = true;
			drag_start_y_	   = event.pos.y;
			drag_start_scroll_ = scroll_offset_;
			return true;

		case TouchEvent::Type::Move:
			if (dragging_)
			{
				int16_t delta  = event.pos.y - drag_start_y_;
				int16_t offset = static_cast<int16_t>(drag_start_scroll_ - delta);
				set_scroll_offset(offset);
			}
			return true;

		case TouchEvent::Type::Release:
			if (dragging_)
			{
				dragging_ = false;
				// If minimal drag, treat as tap-to-select
				int16_t delta = event.pos.y - drag_start_y_;
				if (delta > -TAP_THRESHOLD && delta < TAP_THRESHOLD)
				{
					int16_t local_y = event.pos.y - bounds().y + scroll_offset_;
					int16_t index	= local_y / item_height_;
					if (index >= 0 && index < item_count_)
					{
						set_selected(index);
						if (on_select_)
							on_select_(static_cast<uint8_t>(index));
					}
				}
			}
			return true;
		}
		return false;
	}

	bool on_input(InputAction action) override
	{
		switch (action)
		{
		case InputAction::FocusNext:
			if (item_count_ == 0)
				return false;
			set_selected(static_cast<int16_t>(
				selected_index_ < 0 ? 0 : (selected_index_ + 1 < item_count_ ? selected_index_ + 1 : selected_index_)));
			ensure_visible(selected_index_);
			return true;

		case InputAction::FocusPrev:
			if (item_count_ == 0)
				return false;
			set_selected(static_cast<int16_t>(selected_index_ <= 0 ? 0 : selected_index_ - 1));
			ensure_visible(selected_index_);
			return true;

		case InputAction::Activate:
			if (selected_index_ >= 0 && on_select_)
				on_select_(static_cast<uint8_t>(selected_index_));
			return true;

		default:
			return false;
		}
	}

	// --- Drawing ---

	void draw(gfx::Canvas<ActivePixFmt>& canvas) override
	{
		// Background
		canvas.fill_rect(bounds(), bg_color_);

		// Save clip and restrict to widget bounds
		Rect old_clip = bounds(); // We'll set clip to our bounds
		canvas.set_clip(bounds());

		// Calculate visible range
		int16_t first_visible = scroll_offset_ / item_height_;
		if (first_visible < 0)
			first_visible = 0;

		int16_t last_visible = (scroll_offset_ + bounds().h + item_height_ - 1) / item_height_;
		if (last_visible > item_count_)
			last_visible = item_count_;

		for (int16_t i = first_visible; i < last_visible; ++i)
		{
			int16_t item_y = bounds().y + (i * item_height_) - scroll_offset_;

			// Item rect
			Rect item_rect{bounds().x, item_y, bounds().w, item_height_};

			// Highlight selected
			if (i == selected_index_)
			{
				canvas.fill_rect(item_rect, sel_bg_color_);
			}

			// Draw text
			if (font_.is_valid() && items_[i].text[0] != '\0')
			{
				Color text_color = (i == selected_index_) ? sel_fg_color_ : fg_color_;
				int16_t text_x	 = bounds().x + TEXT_PADDING;
				int16_t text_y	 = item_y + (item_height_ - font_.height()) / 2;
				font_.draw_string(canvas, text_x, text_y, items_[i].text, text_color);
			}
		}

		// Draw focus indicator
		if (is_focused())
		{
			canvas.draw_rect(bounds(), Color::white());
		}
	}

  private:
	struct Item
	{
		char text[MAX_TEXT_LEN] = "";
		bool selected			= false;
	};

	static constexpr int16_t TAP_THRESHOLD = 4;
	static constexpr int16_t TEXT_PADDING  = 2;

	Item items_[MAX_ITEMS]{};
	uint8_t item_count_		   = 0;
	int16_t selected_index_	   = -1;
	int16_t scroll_offset_	   = 0;
	uint16_t item_height_	   = 16;
	bool dragging_			   = false;
	int16_t drag_start_y_	   = 0;
	int16_t drag_start_scroll_ = 0;
	SelectCallback on_select_  = nullptr;
	gfx::FontFace font_{&gfx::font_6x8};

	Color bg_color_		= Color::rgb(20, 20, 30);
	Color fg_color_		= Color::white();
	Color sel_bg_color_ = Color::rgb(60, 120, 200);
	Color sel_fg_color_ = Color::white();

	/// Clamp scroll offset to valid range.
	int16_t clamp_scroll(int16_t offset) const
	{
		int16_t max_scroll = static_cast<int16_t>(item_count_ * item_height_) - bounds().h;
		if (max_scroll < 0)
			max_scroll = 0;
		if (offset < 0)
			offset = 0;
		if (offset > max_scroll)
			offset = max_scroll;
		return offset;
	}

	/// Scroll to make the given item index visible.
	void ensure_visible(int16_t index)
	{
		if (index < 0 || index >= item_count_)
			return;
		int16_t item_top	= static_cast<int16_t>(index * item_height_);
		int16_t item_bottom = static_cast<int16_t>(item_top + item_height_);

		if (item_top < scroll_offset_)
			set_scroll_offset(item_top);
		else if (item_bottom > scroll_offset_ + bounds().h)
			set_scroll_offset(static_cast<int16_t>(item_bottom - bounds().h));
	}
};

} // namespace lumen::ui
