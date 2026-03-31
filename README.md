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
- **Cooperative scheduler** — no RTOS required, but supports FreeRTOS
- **Dirty-rectangle rendering** — only redraws what changed
- **Zero-overhead drivers** — templates, not virtual classes
- **Single BoardConfig** — one struct wires all hardware
- **Desktop simulator** — develop with SDL2, deploy to hardware
- **No heap on small MCUs** — all widgets are static
- **AI-assisted design** — Claude Code as visual designer

## Supported Hardware

| Display | Interface | Status |
|---------|-----------|--------|
| ILI9341 | SPI | Planned |
| ST7789 | SPI | Planned |
| STM32 LTDC | RGB parallel | Planned |
| SDL2 | Desktop | In progress |
| Linux framebuffer | /dev/fb0 | Planned |

| Touch | Interface | Status |
|-------|-----------|--------|
| FT5336 | I2C (capacitive) | Planned |
| XPT2046 | SPI (resistive) | Planned |
| SDL2 mouse | Desktop | Planned |

## Quick Start

```bash
# Desktop simulator
cmake -B build -DLUMEN_BUILD_SIMULATOR=ON -DLUMEN_BUILD_EXAMPLES=ON
cmake --build build
./build/examples/hello_button/hello_button

# Cross-compile for STM32
cmake -B build-arm -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake
cmake --build build-arm
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
