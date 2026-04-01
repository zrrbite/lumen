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

## Font & Image Pipeline

Lumen supports two font formats: **Bitmap** (small, fast, fixed-size) and **SDF** (larger, scalable to any size).

### Bitmap Fonts (BitmapFont)

1bpp packed bitmaps. One file per size. Fast rendering, tiny footprint.

```bash
cd tools && cargo build --release
# Generate bitmap fonts at specific sizes
./target/release/lumen-tools font Font.ttf --sizes 10,14,20 --output ../lumen/gfx/fonts
```

```cpp
#include "lumen/gfx/fonts/liberation_sans_14.hpp"
label.set_font(&lumen::gfx::liberation_sans_14); // Renders at exactly 14px
```

| Pros | Cons |
|------|------|
| Tiny (1-3 KB per size) | Fixed size — need separate file per size |
| Fast (1 bit per pixel check) | Jagged at non-native sizes |
| No float math | Can't scale at runtime |

Best for: status text, counters, small labels where you know the exact size at compile time.

### SDF Fonts (SdfFont)

8-bit signed distance fields. One file renders at **any size** with smooth edges.

```bash
# Generate SDF font (one atlas, renders at any size)
./target/release/lumen-tools sdf-font Font.ttf --base-size 16 --spread 2 --output ../lumen/gfx/fonts
# Larger base = more detail, more flash. Smaller = less detail, less flash.
# --spread controls the smoothing radius (2-4 typical)
```

```cpp
#include "lumen/gfx/fonts/liberation_sans_sdf16.hpp"
// Label accepts both bitmap and SDF fonts via FontFace:
label.set_font(&lumen::gfx::liberation_sans_sdf16, 20); // SDF at 20px
label.set_font(&lumen::gfx::liberation_sans_14);         // Bitmap at 14px
```

| Pros | Cons |
|------|------|
| One atlas → any size (~4px to ~2× base) | Larger (42 KB for 16px base, 95 glyphs) |
| Smooth edges at all scales | Slower (distance sample per pixel) |
| Supports effects (outline, glow, shadow) | Needs FPU for spread calculation |

Best for: titles, dynamic UI where text size changes, or when you want one font to cover all sizes.

### Choosing Between Them

- **RAM-constrained (M0, <64KB)**: bitmap only, 6x8 built-in + one generated size
- **Standard (M4, 128-256KB)**: bitmap for body text, SDF for titles
- **Full (M7, 512KB+)**: SDF everywhere, larger base sizes for detail

### Image Converter & Widget

Convert PNGs to C++ headers, display with the Image widget:

```bash
./target/release/lumen-tools image icon.png --output ../lumen/gfx/images/icon.hpp
./target/release/lumen-tools image photo.png --format argb8888 --output ../lumen/gfx/images/photo.hpp
```

```cpp
#include "lumen/gfx/images/icon.hpp"
#include "lumen/ui/widgets/image.hpp"

lumen::ui::Image icon;
icon.set_image_rgb565(lumen::assets::ICON_DATA, lumen::assets::ICON_W, lumen::assets::ICON_H);
icon.set_bounds({10, 10, 50, 50});  // Image centered within bounds
```

Built-in fonts: `font_6x8` (6x8 monospace, 760 bytes — always available).

## Animation System

```cpp
#include "lumen/ui/animation.hpp"
lumen::ui::AnimationManager anim;
// Animate a float from current value to target over 300ms with easing
anim.animate(&my_value, from, to, 300, tick.now(), lumen::ui::ease::out_cubic);
// Tick in update_model()
anim.update(tick.now());
```

Easing functions: `linear`, `in_quad`, `out_quad`, `in_out_quad`, `in_cubic`, `out_cubic`, `in_out_cubic`, `in_expo`, `out_expo`, `out_back`, `out_bounce`.

## Pixel Format

All widgets use `Canvas<ActivePixFmt>`. The pixel format is selected at compile time:

- **RGB565** (default): 16-bit, used by SPI displays (F429-DISCO, ILI9341)
- **ARGB8888**: 32-bit, used by LTDC framebuffer displays (F769-DISCO, H7 boards)

To switch: define `LUMEN_ARGB8888` in CMake for the target. All widgets, fonts, and rendering automatically adapt.

## Focus & Input

Widgets support keyboard/encoder navigation in addition to touch:

```cpp
btn.set_focusable(true);  // Widget can receive focus
screen.on_input(InputAction::FocusNext);  // Tab to next focusable widget
screen.on_input(InputAction::Activate);   // Press/release focused widget
```

`InputAction` values: `FocusNext`, `FocusPrev`, `Activate`, `Back`.
Container automatically manages focus chain among its children.

## Live Reload (UART)

Update widget properties at runtime over serial (115200 8N1):

```
SET title.text Hello World!
SET title.color 100,200,255
SET bar.value 75
SET btn.visible 0
PING
```

Requires WidgetRegistry with named widgets. See `examples/stm32f429_hello/`.

## Scripting Engine

Lumen includes a tiny expression evaluator for field-updatable UI logic. No heap, runs on MCU.

### Variables and expressions
```
RUN counter = 0
RUN counter += 1
RUN x = counter * 10 + 5
RUN result = (a + b) / 2
```

### Widget property access
```
RUN bar.value = counter * 10
RUN title.text = "Hello!"
RUN title.color = 255,100,50
RUN btn.visible = 0
```
Widget names come from the WidgetRegistry — register widgets with `registry.add("name", widget)`.

### Conditionals
```
RUN if counter > 10 { counter = 0 }
RUN if x == 0 { title.text = "Zero" } else { title.text = "Nonzero" }
```

### Operators
Arithmetic: `+`, `-`, `*`, `/`, `%`
Comparison: `==`, `!=`, `<`, `>`, `<=`, `>=`
Logic: `&&`, `||`, `!`
Assignment: `=`, `+=`, `-=`, `*=`

### Button script binding
```cpp
// Define a runner that forwards to your ScriptEngine
static ScriptEngine* g_script;
static void run_script(const char* s) { g_script->exec(s); }

// Bind script to button — runs on each click
btn.set_on_click_script("counter += 1; bar.value = counter * 10", run_script);
```

### Over UART (live reload)
```
RUN counter = 42
RUN bar.value = 100
RUN if counter > 0 { title.text = "Active" }
```

### Limits
- 32 variables max (fixed-size, no heap)
- 128 chars max per statement
- String literals use a single static buffer (one string value at a time)
- No loops, no functions — just sequential statements

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
