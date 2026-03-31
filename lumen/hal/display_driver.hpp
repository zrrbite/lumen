#pragma once

/// Display driver concept for Lumen.
///
/// Any display driver must provide:
///
///   using PixelFormat = Rgb565 | Argb8888;
///   static constexpr uint16_t width();
///   static constexpr uint16_t height();
///   void init();
///   void set_window(Rect area);
///   void write_pixels(const pixel_t* data, uint32_t count);
///   void flush();
///
/// Optional DMA support:
///   bool supports_dma() const;
///   void write_pixels_dma(const pixel_t* data, uint32_t count,
///                         void(*on_complete)(void*), void* ctx);

#include <type_traits>

#include "lumen/core/types.hpp"

namespace lumen::hal {

/// Compile-time check that a type satisfies the display driver concept.
template <typename D> struct DisplayDriverCheck
{
	// Must have a PixelFormat type alias
	using PF = typename D::PixelFormat;

	// Must have static width() and height()
	static_assert(std::is_same_v<decltype(D::width()), uint16_t>, "Display must have static uint16_t width()");
	static_assert(std::is_same_v<decltype(D::height()), uint16_t>, "Display must have static uint16_t height()");

	static constexpr bool value = true;
};

} // namespace lumen::hal
