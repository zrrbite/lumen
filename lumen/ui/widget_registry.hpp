#pragma once

/// Widget registry — maps string names to widget pointers.
/// Used for live property updates and scripting.
/// Fixed-size, no heap allocation.

#include <cstring>

#include "lumen/ui/widget.hpp"
#include "lumen/ui/widgets/button.hpp"
#include "lumen/ui/widgets/label.hpp"
#include "lumen/ui/widgets/progress_bar.hpp"

namespace lumen::ui {

struct WidgetEntry
{
	const char* name = nullptr;
	Widget* widget	 = nullptr;

	enum class Type : uint8_t
	{
		Unknown,
		Label,
		Button,
		ProgressBar,
	} type = Type::Unknown;
};

/// Registry of named widgets for runtime access.
class WidgetRegistry
{
  public:
	static constexpr uint8_t MAX_WIDGETS = 32;

	/// Register a label with a name.
	void add(const char* name, Label& widget) { add_entry(name, &widget, WidgetEntry::Type::Label); }

	/// Register a button with a name.
	void add(const char* name, Button& widget) { add_entry(name, &widget, WidgetEntry::Type::Button); }

	/// Register a progress bar with a name.
	void add(const char* name, ProgressBar& widget) { add_entry(name, &widget, WidgetEntry::Type::ProgressBar); }

	/// Find a widget by name.
	const WidgetEntry* find(const char* name) const
	{
		for (uint8_t i = 0; i < count_; ++i)
		{
			if (std::strcmp(entries_[i].name, name) == 0)
				return &entries_[i];
		}
		return nullptr;
	}

	/// Set a property on a named widget. Returns true on success.
	/// Supported properties depend on widget type:
	///   Label:       text, color_r/g/b, bg_r/g/b
	///   Button:      text (label), color_r/g/b
	///   ProgressBar: value, fill_r/g/b
	///   All:         x, y, w, h, visible
	bool set_property(const char* widget_name, const char* prop, const char* value)
	{
		const WidgetEntry* entry = find(widget_name);
		if (entry == nullptr)
			return false;

		// Common properties (all widget types)
		if (std::strcmp(prop, "x") == 0)
		{
			Rect b = entry->widget->bounds();
			b.x	   = static_cast<int16_t>(parse_int(value));
			entry->widget->set_bounds(b);
			return true;
		}
		if (std::strcmp(prop, "y") == 0)
		{
			Rect b = entry->widget->bounds();
			b.y	   = static_cast<int16_t>(parse_int(value));
			entry->widget->set_bounds(b);
			return true;
		}
		if (std::strcmp(prop, "w") == 0)
		{
			Rect b = entry->widget->bounds();
			b.w	   = static_cast<uint16_t>(parse_int(value));
			entry->widget->set_bounds(b);
			return true;
		}
		if (std::strcmp(prop, "h") == 0)
		{
			Rect b = entry->widget->bounds();
			b.h	   = static_cast<uint16_t>(parse_int(value));
			entry->widget->set_bounds(b);
			return true;
		}
		if (std::strcmp(prop, "visible") == 0)
		{
			entry->widget->set_visible(parse_int(value) != 0);
			return true;
		}

		// Type-specific properties
		switch (entry->type)
		{
		case WidgetEntry::Type::Label:
			return set_label_prop(static_cast<Label*>(entry->widget), prop, value);
		case WidgetEntry::Type::Button:
			return set_button_prop(static_cast<Button*>(entry->widget), prop, value);
		case WidgetEntry::Type::ProgressBar:
			return set_bar_prop(static_cast<ProgressBar*>(entry->widget), prop, value);
		default:
			return false;
		}
	}

	uint8_t count() const { return count_; }

  private:
	WidgetEntry entries_[MAX_WIDGETS]{};
	uint8_t count_ = 0;

	void add_entry(const char* name, Widget* widget, WidgetEntry::Type type)
	{
		if (count_ < MAX_WIDGETS)
		{
			entries_[count_] = {name, widget, type};
			++count_;
		}
	}

	static int parse_int(const char* str)
	{
		int result = 0;
		bool neg   = false;
		if (*str == '-')
		{
			neg = true;
			++str;
		}
		while (*str >= '0' && *str <= '9')
		{
			result = result * 10 + (*str - '0');
			++str;
		}
		return neg ? -result : result;
	}

	static bool set_label_prop(Label* label, const char* prop, const char* value)
	{
		if (std::strcmp(prop, "text") == 0)
		{
			label->set_text(value);
			return true;
		}
		if (std::strcmp(prop, "color") == 0)
		{
			label->set_color(parse_color(value));
			return true;
		}
		if (std::strcmp(prop, "bg") == 0)
		{
			label->set_bg_color(parse_color(value));
			return true;
		}
		return false;
	}

	static bool set_button_prop(Button* button, const char* prop, const char* value)
	{
		if (std::strcmp(prop, "text") == 0)
		{
			button->set_label(value);
			return true;
		}
		return false;
	}

	static bool set_bar_prop(ProgressBar* bar, const char* prop, const char* value)
	{
		if (std::strcmp(prop, "value") == 0)
		{
			bar->set_value(static_cast<uint8_t>(parse_int(value)));
			return true;
		}
		return false;
	}

	/// Parse "r,g,b" into a Color.
	static Color parse_color(const char* str)
	{
		int vals[3] = {0, 0, 0};
		int idx		= 0;
		while (*str && idx < 3)
		{
			if (*str >= '0' && *str <= '9')
			{
				vals[idx] = vals[idx] * 10 + (*str - '0');
			}
			else if (*str == ',')
			{
				++idx;
			}
			++str;
		}
		return Color::rgb(static_cast<uint8_t>(vals[0]), static_cast<uint8_t>(vals[1]), static_cast<uint8_t>(vals[2]));
	}
};

} // namespace lumen::ui
