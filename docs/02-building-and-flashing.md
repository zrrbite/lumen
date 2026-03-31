# Building and Flashing

## Desktop Simulator (Linux/macOS/Windows)

### Prerequisites

```bash
# Linux (Arch)
sudo pacman -S cmake sdl2

# Linux (Ubuntu/Debian)
sudo apt install cmake build-essential libsdl2-dev

# macOS
brew install cmake sdl2

# Windows (vcpkg)
vcpkg install sdl2
```

### Build

```bash
cd lumen
cmake -B build -DLUMEN_BUILD_SIMULATOR=ON -DLUMEN_BUILD_EXAMPLES=ON
cmake --build build
```

### Run examples

```bash
./build/examples/hello_rectangle/hello_rectangle   # Bouncing boxes
./build/examples/hello_button/hello_button         # Interactive button
./build/examples/widget_showcase/widget_showcase    # All widgets
./build/examples/dashboard/dashboard               # IoT dashboard
```

---

## STM32 Cross-Compilation

### Step 1: Install ARM toolchain

```bash
# Linux (Arch)
sudo pacman -S arm-none-eabi-gcc arm-none-eabi-newlib

# Linux (Ubuntu/Debian)
sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi

# macOS
brew install --cask gcc-arm-embedded

# Windows
# Download from https://developer.arm.com/downloads/-/gnu-rm
# Add bin/ to your PATH
```

Verify:
```bash
arm-none-eabi-gcc --version
```

### Step 2: Install flashing tools

```bash
# Linux (Arch) — pick one:
sudo pacman -S stlink               # st-flash (simplest)
sudo pacman -S openocd               # OpenOCD (most versatile)
cargo install probe-rs-tools         # probe-rs (modern, Rust-based)

# Linux (Ubuntu/Debian):
sudo apt install stlink-tools        # st-flash
sudo apt install openocd             # OpenOCD
cargo install probe-rs-tools         # probe-rs

# macOS:
brew install stlink                  # st-flash
brew install openocd                 # OpenOCD
cargo install probe-rs-tools         # probe-rs

# Windows:
# STM32CubeProgrammer: https://www.st.com/en/development-tools/stm32cubeprog.html
# Or use OpenOCD via MSYS2
```

### Step 3: Build for STM32F429-DISCO

```bash
cd lumen
cmake -B build-arm \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake \
  -DLUMEN_TARGET_F429=ON

cmake --build build-arm
```

This produces:
- `build-arm/stm32f429_hello` — ELF binary
- `build-arm/stm32f429_hello.bin` — raw binary for flashing

### Step 4: Flash

Connect the STM32F429-DISCO via USB (the built-in ST-Link debugger).

```bash
# Option A: st-flash (simplest)
st-flash write build-arm/stm32f429_hello.bin 0x08000000

# Option B: OpenOCD
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program build-arm/stm32f429_hello verify reset exit"

# Option C: probe-rs (shows RTT output too)
probe-rs run --chip STM32F429ZITx build-arm/stm32f429_hello
```

You should see 3 colored rectangles and "Hello, Lumen!" on the display.

### Adjusting for other boards

The CPU flags are set in the platform CMakeLists. For other MCUs:

| Board | MCU Flag | FPU Flag |
|-------|----------|----------|
| STM32F429-DISCO | `-mcpu=cortex-m4` | `-mfpu=fpv4-sp-d16 -mfloat-abi=hard` |
| STM32F769-DISCO | `-mcpu=cortex-m7` | `-mfpu=fpv5-d16 -mfloat-abi=hard` |
| STM32H7B3-DISCO | `-mcpu=cortex-m7` | `-mfpu=fpv5-d16 -mfloat-abi=hard` |
| STM32H745I-DISCO | `-mcpu=cortex-m7` | `-mfpu=fpv5-d16 -mfloat-abi=hard` |

---

## Raspberry Pi

### Native compilation (on the Pi)

```bash
sudo apt install cmake libsdl2-dev g++
cd lumen
cmake -B build -DLUMEN_BUILD_SIMULATOR=ON -DLUMEN_BUILD_EXAMPLES=ON
cmake --build build
./build/examples/dashboard/dashboard
```

### Cross-compilation (from PC)

```bash
# Linux (Arch)
sudo pacman -S aarch64-linux-gnu-gcc

# Linux (Ubuntu)
sudo apt install g++-aarch64-linux-gnu

# Build
cmake -B build-pi \
  -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
  -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc

cmake --build build-pi

# Deploy
scp build-pi/examples/dashboard/dashboard pi@raspberrypi:~/
ssh pi@raspberrypi ./dashboard
```

### Direct framebuffer (headless Pi with TFT)

For a Raspberry Pi with an SPI TFT display (no X11/Wayland), Lumen can
render to `/dev/fb0` or use DRM/KMS. A `LinuxFramebuffer` display driver
is planned.

---

## Troubleshooting

### "arm-none-eabi-gcc: command not found"
The toolchain isn't installed or not in PATH. See Step 1 above.

### "No ST-Link detected"
- Is the USB cable connected? Use the ST-Link USB port (not the user USB).
- On Linux, you may need udev rules:
  ```bash
  sudo cp /usr/share/stlink/stlink-udev-rules/49-stlink.rules /etc/udev/rules.d/
  sudo udevadm control --reload-rules
  ```

### "Flash write failed"
- Make sure the board isn't running a program that locks flash.
- Try holding the RESET button while starting the flash.
- Check that the `.bin` file isn't too large for the Flash (2MB for F429).

### Build fails with "undefined reference to ..."
- Make sure you're using the cross-compilation toolchain, not the host compiler.
- Check that `arm-none-eabi-newlib` is installed (provides libc for bare metal).
