# Lumen

An embedded GUI framework for microcontrollers, designed to be clean,
fast, and portable.

```cpp
#include <lumen/lumen.hpp>
#include "my_board.hpp"

class HelloScreen : public lumen::ui::Screen {
    lumen::ui::Label  title_{"Hello, Lumen!"};
    lumen::ui::Button btn_{"Click me"};

public:
    HelloScreen() {
        title_.set_bounds({10, 10, 200, 30});
        btn_.set_bounds({10, 60, 120, 40});
        add(title_);
        add(btn_);
    }
};

int main() {
    MyBoard board;
    lumen::Application<MyBoard> app(board);

    HelloScreen screen;
    app.navigate_to(screen);
    app.run();
}
```

Same code runs on STM32, Raspberry Pi, or your desktop (SDL2 simulator).

## Features

- **Cortex-M0 to M7** — scales from 32KB RAM to 1MB+
- **60 FPS on STM32F429** — 16-bit SPI with band rendering
- **Cooperative scheduler** — no RTOS required, but supports FreeRTOS
- **Dirty-rectangle rendering** — only redraws what changed
- **Zero-overhead drivers** — templates, not virtual classes
- **Single BoardConfig** — one struct wires all hardware
- **Desktop simulator** — develop with SDL2, deploy to hardware
- **No heap on small MCUs** — all widgets are static
- **SDF fonts** — one atlas renders at any size with smooth edges
- **Animation system** — 10 easing functions, property tweens
- **Live reload** — update widget properties over UART at runtime
- **DMA2D (Chrom-ART)** — hardware-accelerated fills on STM32F4/F7/H7
- **AI-assisted design** — Claude Code as visual designer

## Supported Hardware

### Boards

| Board | MCU | Display | Touch | Status |
|-------|-----|---------|-------|--------|
| STM32F429-DISCO | Cortex-M4 180MHz | 240x320 ILI9341 SPI | STMPE811 resistive | **Working** (60 FPS, touch, animations) |
| STM32F769-DISCO | Cortex-M7 216MHz | 800x480 OTM8009A DSI | FT6206 capacitive | **In progress** (SDRAM + clocks working, DSI WIP) |
| Desktop (SDL2) | Any | Configurable window | Mouse | **Working** |
| STM32H7B3-DISCO | Cortex-M7 280MHz | 480x272 RGB/LTDC | FT5336 capacitive | Planned |
| STM32H745I-DISCO | Dual M7+M4 480MHz | 800x480 RGB/LTDC | Capacitive | Planned |
| Raspberry Pi | ARM64 Linux | HDMI or SPI TFT | USB/touchscreen | Planned |

### Display Drivers

| Driver | Interface | Status |
|--------|-----------|--------|
| ILI9341 | SPI (16-bit DMA) | **Working** |
| OTM8009A | MIPI-DSI + LTDC | In progress |
| SDL2 simulator | Desktop | **Working** |
| LTDC (parallel RGB) | FMC | Planned |
| Linux framebuffer | /dev/fb0 | Planned |

### Touch Drivers

| Driver | Interface | Status |
|--------|-----------|--------|
| STMPE811 | I2C (resistive) | **Working** |
| FT6206 | I2C (capacitive) | Driver ready, untested |
| SDL2 mouse | Desktop | **Working** |

### Peripheral Support

| Feature | Status |
|---------|--------|
| 16-bit SPI pixel transfer | **Working** |
| DMA SPI (non-blocking) | Written, needs hardware debug |
| DMA2D (Chrom-ART) fills | **Working** (F429) |
| DMA2D alpha blending | Driver ready |
| I2C with bus recovery | **Working** |
| UART (live reload) | **Working** |
| SDRAM (FMC) | **Working** (F769) |
| FPU enabled | **Working** (M4 + M7) |

## Widgets

Label, Button, Checkbox, Toggle, Slider, ProgressBar — all with
bitmap and SDF font support, dirty tracking, and touch input.

## Font System

- **Bitmap fonts** — 1bpp packed, tiny footprint (1-3 KB per size), exact-size rendering
- **SDF fonts** — 8-bit distance fields, one atlas renders at any size (~4px to ~2x base)
- **Font converter** — `lumen-tools font` (TTF to bitmap) and `lumen-tools sdf-font` (TTF to SDF)
- **Built-in** — 6x8 monospace (760 bytes, always available)

## Quick Start

```bash
# Desktop simulator
cmake -B build -DLUMEN_BUILD_SIMULATOR=ON -DLUMEN_BUILD_EXAMPLES=ON
cmake --build build
./build/examples/hello_button/hello_button

# STM32F429-DISCO
cmake -B build-arm -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake -DLUMEN_TARGET_F429=ON
cmake --build build-arm
probe-rs run --chip STM32F429ZITx build-arm/stm32f429_hello

# STM32F769-DISCO
cmake -B build-f769 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake -DLUMEN_TARGET_F769=ON
cmake --build build-f769
probe-rs run --chip STM32F769NIHx build-f769/stm32f769_hello

# Generate fonts
cd tools && cargo build --release
./target/release/lumen-tools font /path/to/Font.ttf --sizes 10,14,20 --output ../lumen/gfx/fonts
./target/release/lumen-tools sdf-font /path/to/Font.ttf --base-size 16 --output ../lumen/gfx/fonts
```

## Documentation

- [Architecture](docs/01-architecture.md) — core concepts, scheduler, drivers
- [Building & Flashing](docs/02-building-and-flashing.md) — Linux, Windows, macOS, Raspberry Pi

## Inspiration

Built on ideas from:
- **TouchGFX** — hardware acceleration, designer tool, MVP pattern
- **LVGL** — portability, widget library, community
- **Embedded Wizard** — code generation, clean abstraction
- **Slint** — declarative UI, modern approach

## License

MIT
