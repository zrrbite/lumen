#pragma once

/// Lumen — Embedded GUI Framework
/// Single convenience header.

// Core
#include "lumen/core/scheduler.hpp"
#include "lumen/core/types.hpp"

// HAL
#include "lumen/hal/display_driver.hpp"
#include "lumen/hal/input_driver.hpp"
#include "lumen/hal/tick_source.hpp"
#include "lumen/hal/touch_driver.hpp"

// OS
#include "lumen/hal/os/bare_metal.hpp"

// Platform
#include "lumen/platform/board_config.hpp"
