# Lumen — Embedded GUI Framework

## Overview

Lumen is a C++17 GUI framework for microcontrollers (Cortex-M0 to M7).
It uses cooperative scheduling, dirty-rectangle rendering, and compile-time
hardware abstraction via templates.

## Building

```bash
# Desktop simulator
cmake -B build -DLUMEN_BUILD_SIMULATOR=ON -DLUMEN_BUILD_EXAMPLES=ON
cmake --build build

# ARM cross-compilation
cmake -B build-arm -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake
cmake --build build-arm
```

## Project Structure

```
lumen/                     # Public headers (include path)
  core/types.hpp           # Rect, Point, Size, Color, TickMs, Rgb565
  core/scheduler.hpp       # Task, Scheduler (cooperative)
  hal/display_driver.hpp   # Display driver concept
  hal/touch_driver.hpp     # Touch driver concept + NullTouch
  hal/input_driver.hpp     # Input driver concept + NullInput
  hal/tick_source.hpp      # StdChronoTick, SysTickSource
  hal/os/bare_metal.hpp    # BareMetalOS
  hal/os/freertos.hpp      # FreeRtosOS
  platform/board_config.hpp # BoardConfig concept checker
  gfx/                     # Canvas, DirtyManager, Font (coming)
  ui/                      # Widget, Container, Screen (coming)
  lumen.hpp                # Convenience include
drivers/                   # Display/touch/input driver implementations
platform/                  # Board support packages
tools/                     # Rust CLI for font/image conversion
examples/                  # Example applications
docs/                      # Documentation
```

## Key Types

- `lumen::Rect` — {x, y, w, h} with intersection/union/contains
- `lumen::Color` — {r, g, b, a} with rgb565/argb8888 conversion
- `lumen::Rgb565` / `lumen::Argb8888` — pixel format tags
- `lumen::TickMs` — uint32_t milliseconds since boot
- `lumen::Task` — base class with execute(), period_ms(), priority()
- `lumen::Scheduler` — cooperative task runner

## Architecture

App code → Application<BoardConfig> → Scheduler → BoardConfig → Hardware

BoardConfig is a struct with type aliases:
```cpp
struct MyBoard {
    using Display = drivers::Ili9341Spi<...>;
    using Touch   = hal::NullTouch;
    using Input   = hal::NullInput;
    using Tick    = hal::StdChronoTick;
    using OS      = hal::BareMetalOS;
    // ... instances and init_hardware()
};
```

## Code Style

- C++17, no exceptions, no RTTI on embedded
- snake_case for methods and variables
- PascalCase for types and classes
- ALL_CAPS for constants and macros
- Headers in `lumen/` use `#pragma once`
- No heap allocation on M0/M4

## Testing

Desktop simulator with SDL2 is the primary test environment.
Cross-compile to ARM for hardware testing.
