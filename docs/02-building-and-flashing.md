# Building and Flashing

## Platforms Overview

| Platform | Display | Touch | Build | Status |
|----------|---------|-------|-------|--------|
| Desktop (SDL2) | SDL2 window | Mouse | Native | **Working** |
| STM32F429-DISCO | 240x320 ILI9341 SPI | STMPE811 I2C | Cross-compile | **Working** (60fps) |
| STM32F769-DISCO | 800x480 OTM8009A DSI | FT6206 I2C | Cross-compile | **WIP** (SDRAM + clocks working, DSI video WIP) |
| Raspberry Pi 4 | HDMI via DRM/KMS | evdev touchscreen | Native Linux | **Builds** (untested on hardware) |

---

## Desktop Simulator (SDL2)

Develop and test without hardware. Same widget code, different BoardConfig.

### Prerequisites

```bash
# Linux (Arch)
sudo pacman -S cmake sdl2

# Linux (Ubuntu/Debian)
sudo apt install cmake build-essential libsdl2-dev

# macOS
brew install cmake sdl2
```

### Build & Run

```bash
cmake -B build -DLUMEN_BUILD_SIMULATOR=ON -DLUMEN_BUILD_EXAMPLES=ON
cmake --build build

./build/examples/hello_button/hello_button
./build/examples/widget_showcase/widget_showcase
./build/examples/dashboard/dashboard
```

### Visual Tests

```bash
cmake --build build --target visual_test
./build/visual_test             # Compare against reference screenshots
./build/visual_test --update    # Update reference screenshots
```

---

## STM32F429-DISCO

240x320 ILI9341 SPI display + STMPE811 resistive touch. Bare-metal, 180MHz Cortex-M4.

### Hardware Setup

- Connect via USB to the ST-Link port (mini-USB, not the user USB)
- Display and touch are on-board — no wiring needed

### Prerequisites

```bash
# ARM toolchain
sudo pacman -S arm-none-eabi-gcc arm-none-eabi-newlib  # Arch
sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi  # Ubuntu

# Flashing tool
cargo install probe-rs-tools
```

### Build

```bash
cmake -B build-arm \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake \
  -DLUMEN_TARGET_F429=ON
cmake --build build-arm
```

Produces `build-arm/stm32f429_hello` (ELF, ~71KB).

### Flash & Run

```bash
probe-rs run --chip STM32F429ZITx build-arm/stm32f429_hello
```

### What's Working

- 60fps rendering with 16-bit SPI + DMA2D buffer clearing
- Resistive touch with TOUCH_DET-based press/release detection
- I2C bus recovery on startup (prevents stuck bus after crashes)
- FPU enabled for animation easing
- SDF + bitmap fonts, animated progress bar, screen transitions
- Two-screen demo with slide and wipe transitions

### Hardware Notes

- **FPU**: Enabled in startup.cpp (CPACR register). Without this, float ops crash.
- **snprintf**: Don't use — pulls in newlib mutex code that crashes from SysTick. Use custom `int_to_str`.
- **Touch axes**: No swap, Y inverted. Calibrated for the on-board STMPE811.
- **SPI clock**: Prescaler 2 = ~11MHz (APB2=90MHz / 8). ILI9341 max is ~15MHz.
- **Scratch buffer**: 4800 bytes = 10 scan lines for band rendering.

---

## STM32F769-DISCO

800x480 OTM8009A display via MIPI-DSI + LTDC. 16MB SDRAM. 216MHz Cortex-M7.

### Hardware Setup

- Connect via USB to the ST-Link port
- Display is on-board (4" LCD with capacitive touch)

### Build

```bash
cmake -B build-f769 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake \
  -DLUMEN_TARGET_F769=ON
cmake --build build-f769
```

### Flash & Run

```bash
probe-rs run --chip STM32F769NIHx build-f769/stm32f769_hello
```

### What's Working

- 216MHz system clock (HSE 25MHz + PLL, overdrive mode)
- FPU enabled (Cortex-M7 FPv5)
- SDRAM initialized (MT48LC4M32B2, FMC Bank 1 at 0xC0000000, CAS3, 108MHz)
- PLLSAI for LTDC clock (27.43MHz)
- DSI PLL locked, DSI regulator ready, PHY enabled
- OTM8009A commands sent via DSI (panel turns on)

### What's Not Working Yet

- DSI video stream: LTDC data reaches the panel but displays noise, not the framebuffer contents
- Needs reference register dump from a working CubeMX/TouchGFX project to diff against our init sequence
- FT6206 touch driver written but untested

### Hardware Notes

- **HSE crystal**: 25MHz (F429 uses 8MHz — different PLL config)
- **SDRAM**: CAS latency 3, burst read enabled, refresh count 0x0603 (from ST BSP)
- **DSI**: WRPCR at offset 0x430 (not 0x418), WCFGR at 0x400, PLL lock in WISR bit 8
- **UIX4**: PHY unit interval = 8. Without this the DSI PHY can't serialize data.
- **Display reset**: PJ15 (active low, 20ms pulse)
- **LED pins**: LD1 (red) = PJ13, LD2 (green) = PJ5

---

## Raspberry Pi 4

HDMI display via DRM/KMS + evdev touch input. Native Linux, no bare-metal.

### Hardware Setup

- RPi 4 with HDMI display, or official 7" touchscreen
- Any Linux distro (Raspberry Pi OS, Ubuntu, Arch ARM)

### Prerequisites (on the Pi)

```bash
sudo apt install cmake g++ libdrm-dev
# DRM headers needed for /dev/dri access
```

### Build (on the Pi)

```bash
cmake -B build-rpi -DLUMEN_TARGET_RPI=ON
cmake --build build-rpi
```

### Run

```bash
sudo ./build-rpi/rpi_hello
# Needs root for /dev/dri and /dev/input access
```

Or add your user to the required groups:
```bash
sudo usermod -aG video,input $USER
# Log out and back in, then:
./build-rpi/rpi_hello
```

### Cross-Compilation (from PC)

```bash
# Install cross-compiler
sudo pacman -S aarch64-linux-gnu-gcc  # Arch
sudo apt install g++-aarch64-linux-gnu  # Ubuntu

# Build
cmake -B build-rpi-cross \
  -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
  -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
  -DLUMEN_TARGET_RPI=ON
cmake --build build-rpi-cross

# Deploy
scp build-rpi-cross/rpi_hello pi@raspberrypi:~/
ssh pi@raspberrypi sudo ./rpi_hello
```

### Architecture

- **Display**: DRM/KMS with dumb buffers. Double-buffered with page flip for vsync.
  Auto-detects connected display and resolution from `/dev/dri/card0`.
- **Touch**: Linux evdev via `/dev/input/event*`. Auto-scans for devices with
  `ABS_MT_POSITION_X` (multitouch) or `ABS_X` (single touch).
- **Pixel format**: ARGB8888 (set via `LUMEN_ARGB8888` define)
- **Tick source**: `std::chrono::steady_clock` (same as desktop simulator)

### Hardware Notes

- **No SDL2 needed** — DRM/KMS talks directly to the GPU, no windowing system required
- **Works headless** — can run without X11/Wayland (console mode)
- **Vsync**: DRM page flip with `DRM_MODE_PAGE_FLIP_EVENT` flag

---

## Troubleshooting

### "arm-none-eabi-gcc: command not found"
The toolchain isn't installed or not in PATH. See ARM toolchain install above.

### "No ST-Link detected" / "No debug probes were found"
- Is the USB cable connected to the ST-Link port?
- On Linux, you may need udev rules:
  ```bash
  # For probe-rs:
  curl -sL https://probe.rs/files/69-probe-rs.rules | sudo tee /etc/udev/rules.d/69-probe-rs.rules
  sudo udevadm control --reload-rules
  ```

### STM32 touch not working after crash
The I2C bus can get stuck if firmware crashes mid-transfer. Power cycle the board
(unplug USB completely, wait 3 seconds, replug). The I2C bus recovery in startup
handles most cases, but a hard crash can leave the STMPE811 in a bad state.

### "Failed to initialize DRM display" (RPi)
- Make sure no other program is using the display (X11, Wayland, fbcon)
- Try running from a console (no desktop environment)
- Check `/dev/dri/card0` exists: `ls -l /dev/dri/`
- Run with `sudo` or add user to `video` group
