#pragma once

/// Input driver concept for Lumen.
///
/// Any input driver must provide:
///   void init();
///   bool poll(InputState& out);

#include <cstdint>

namespace lumen::hal {

enum class InputEvent : uint8_t
{
	None,
	Press,
	Release,
	RotateCW,
	RotateCCW
};

struct InputState
{
	uint8_t id		 = 0;
	InputEvent event = InputEvent::None;
};

/// Null input driver — for boards without buttons/encoders.
struct NullInput
{
	void init() {}
	bool poll(InputState&) { return false; }
};

} // namespace lumen::hal
