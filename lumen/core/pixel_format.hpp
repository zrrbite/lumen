#pragma once

/// Active pixel format selection.
/// The build system defines LUMEN_ARGB8888 for boards with framebuffers
/// that use 32-bit pixels (F769 LTDC, H7 boards). Default is RGB565
/// for SPI displays (F429, etc).

#include "lumen/core/types.hpp"

namespace lumen {

#if defined(LUMEN_ARGB8888)
using ActivePixFmt = Argb8888;
#else
using ActivePixFmt = Rgb565;
#endif

} // namespace lumen
