# Lumen Architecture

## Overview

Lumen is a GUI framework for microcontrollers (Cortex-M0 to M7). It uses a
cooperative scheduler, dirty-rectangle rendering, and compile-time hardware
abstraction.

```
Your App (Screens, Widgets)
       ↓
Application<BoardConfig>
       ↓
┌───────────────────────────────────┐
│ Cooperative Scheduler             │
│  InputTask → polls touch/buttons  │
│  RenderTask → dirty rect drawing  │
│  FlushTask → push to display      │
│  AnimationTask → tick animations  │
└───────────────────────────────────┘
       ↓
BoardConfig (compile-time wiring)
  Display, Touch, Input, Tick, OS
```

## Key Concepts

### BoardConfig

One struct per physical board. Defines all hardware via type aliases:

```cpp
struct MyBoard {
    using Display = drivers::Ili9341Spi<...>;
    using Touch   = hal::NullTouch;
    using Input   = hal::NullInput;
    using Tick    = hal::SysTickSource;
    using OS      = hal::BareMetalOS;

    static constexpr size_t framebuffer_count = 0;
    static constexpr size_t scratch_buffer_size = 512;

    Display display;
    Touch   touch;
    Input   input;
    Tick    tick;

    void init_hardware() { /* clocks, pins, peripherals */ }
};
```

Swap `MyBoard` for `DesktopSdl2Config` and the same app runs on your PC.

### Cooperative Scheduler

Replaces an RTOS for simple systems. Tasks run in priority order:

```cpp
class MyTask : public lumen::Task {
    void execute(TickMs now) override { /* do work */ }
    TickMs period_ms() const override { return 100; } // every 100ms
    uint8_t priority() const override { return 50; }
};
```

The scheduler calls `execute()` when the task is due. No preemption,
no stack per task, no RTOS required.

### With an RTOS

When FreeRTOS (or similar) is present, set `using OS = hal::FreeRtosOS` in
your BoardConfig. The scheduler runs inside one OS task and yields when idle.

### Dirty Rectangle Rendering

Only changed regions are redrawn:

1. Widget data changes → `widget.invalidate()`
2. DirtyManager collects dirty regions
3. RenderTask clips to dirty rect, walks widget tree
4. Only intersecting widgets are drawn
5. Result pushed to display

An idle screen uses zero CPU for rendering.

### Memory Model

| Target | RAM | Strategy |
|--------|-----|----------|
| Cortex-M0 | 32KB | No framebuffer, scratch buffer renders directly to display |
| Cortex-M4 | 128KB | Line buffer in SRAM |
| Cortex-M7 | 1MB+ | Double framebuffer in external SDRAM |

No heap allocation on M0/M4. Widgets are static objects.

### Driver Model

Drivers are templates (duck-typed), not virtual classes:

- **Zero overhead** — no vtable, everything inlines
- **Easy to add** — implement 4-5 methods, no inheritance
- **Compile-time checked** — `static_assert` validates driver interface

## Widget System

```cpp
class DashboardScreen : public lumen::ui::Screen {
    lumen::ui::Label  title_{"Dashboard"};
    lumen::ui::Button settings_btn_{"Settings"};

public:
    DashboardScreen() {
        title_.set_bounds({10, 10, 200, 30});
        settings_btn_.set_bounds({10, 200, 120, 40});
        add(title_);
        add(settings_btn_);
    }

    void update_model() override {
        title_.set_text(sensor::read_temp_string());
    }
};
```

Widgets are statically allocated, composed into screens, and only redraw
when their data changes.
