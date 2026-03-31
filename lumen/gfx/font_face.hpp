#pragma once

/// Unified font interface — abstracts BitmapFont and SdfFont behind a
/// common draw_text / measure_text interface. Zero heap allocation.

#include "lumen/core/pixel_format.hpp"
#include "lumen/gfx/canvas.hpp"
#include "lumen/gfx/font.hpp"
#include "lumen/gfx/sdf_font.hpp"

namespace lumen::gfx {

/// A font reference that can hold either a BitmapFont or SdfFont.
/// Widgets use this instead of referencing font types directly.
class FontFace
{
  public:
	FontFace() = default;

	/// Construct from a bitmap font.
	explicit FontFace(const BitmapFont* bmp) : bitmap_(bmp), type_(Type::Bitmap) {}

	/// Construct from an SDF font with a target render size.
	FontFace(const SdfFont* sdf, uint16_t target_size) : sdf_(sdf), sdf_size_(target_size), type_(Type::Sdf) {}

	/// Draw text at the given position.
	void draw_string(Canvas<ActivePixFmt>& canvas, int16_t x, int16_t y, const char* text, Color color) const
	{
		switch (type_)
		{
		case Type::Bitmap:
			if (bitmap_)
				bitmap_->draw_string(canvas, x, y, text, color);
			break;
		case Type::Sdf:
			if (sdf_)
				sdf_->draw_string(canvas, x, y, text, color, sdf_size_);
			break;
		default:
			break;
		}
	}

	/// Measure text width in pixels.
	uint16_t string_width(const char* text) const
	{
		switch (type_)
		{
		case Type::Bitmap:
			return bitmap_ ? bitmap_->string_width(text) : 0;
		case Type::Sdf:
			return sdf_ ? sdf_->string_width(text, sdf_size_) : 0;
		default:
			return 0;
		}
	}

	/// Get the line height in pixels.
	uint16_t height() const
	{
		switch (type_)
		{
		case Type::Bitmap:
			return bitmap_ ? bitmap_->char_height : 0;
		case Type::Sdf:
			return sdf_ ? static_cast<uint16_t>((sdf_->cell_h * sdf_size_) / sdf_->base_size) : 0;
		default:
			return 0;
		}
	}

	bool is_valid() const { return type_ != Type::None; }

  private:
	enum class Type : uint8_t
	{
		None,
		Bitmap,
		Sdf
	};

	const BitmapFont* bitmap_ = nullptr;
	const SdfFont* sdf_		  = nullptr;
	uint16_t sdf_size_		  = 14;
	Type type_				  = Type::None;
};

} // namespace lumen::gfx
