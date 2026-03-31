#pragma once

/// BoardConfig concept for Lumen.
///
/// A BoardConfig must provide:
///
///   Type aliases:
///     using Display  = <display driver>;
///     using Touch    = <touch driver>;
///     using Input    = <input driver>;
///     using Tick     = <tick source>;
///     using OS       = <OS abstraction>;
///
///   Constants:
///     static constexpr size_t framebuffer_count;     // 0, 1, or 2
///     static constexpr size_t scratch_buffer_size;    // bytes
///     static constexpr bool   use_external_ram;
///
///   Instances (owned by the config):
///     Display display;
///     Touch   touch;
///     Input   input;
///     Tick    tick;
///
///   Methods:
///     void init_hardware();
///
/// Example:
///   struct MyBoard {
///       using Display = drivers::Ili9341Spi<...>;
///       using Touch   = hal::NullTouch;
///       using Input   = hal::NullInput;
///       using Tick    = hal::StdChronoTick;
///       using OS      = hal::BareMetalOS;
///
///       static constexpr size_t framebuffer_count = 0;
///       static constexpr size_t scratch_buffer_size = 512;
///       static constexpr bool   use_external_ram = false;
///
///       Display display;
///       Touch   touch;
///       Input   input;
///       Tick    tick;
///
///       void init_hardware() { /* board-specific init */ }
///   };

#include <cstddef>
#include <type_traits>

namespace lumen::platform {

template <typename B> struct BoardConfigCheck
{
	static_assert(sizeof(typename B::Display) > 0, "BoardConfig must have Display type");
	static_assert(sizeof(typename B::Touch) > 0, "BoardConfig must have Touch type");
	static_assert(sizeof(typename B::Input) > 0, "BoardConfig must have Input type");
	static_assert(sizeof(typename B::Tick) > 0, "BoardConfig must have Tick type");
	static_assert(sizeof(typename B::OS) > 0, "BoardConfig must have OS type");
	static constexpr bool value = true;
};

} // namespace lumen::platform
