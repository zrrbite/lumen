# Building and Flashing

## Desktop Simulator (Linux/macOS/Windows)

### Prerequisites

- CMake 3.20+
- C++17 compiler (gcc, clang, MSVC)
- SDL2 development libraries

```bash
# Linux (Ubuntu/Debian)
sudo apt install cmake build-essential libsdl2-dev

# Linux (Arch)
sudo pacman -S cmake sdl2

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

### Run

```bash
./build/examples/hello_button/hello_button
```

## Cross-Compilation for ARM (STM32, etc.)

### Prerequisites

- `arm-none-eabi-gcc` toolchain
- CMake 3.20+

```bash
# Linux (Ubuntu/Debian)
sudo apt install gcc-arm-none-eabi

# Linux (Arch)
sudo pacman -S arm-none-eabi-gcc arm-none-eabi-newlib

# macOS
brew install --cask gcc-arm-embedded

# Windows
# Download from https://developer.arm.com/downloads/-/gnu-rm
```

### Build

```bash
cmake -B build-arm \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake \
  -DCMAKE_C_FLAGS="-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard" \
  -DCMAKE_CXX_FLAGS="-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard"

cmake --build build-arm
```

Adjust `-mcpu` for your target:
- Cortex-M0: `-mcpu=cortex-m0`
- Cortex-M4: `-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard`
- Cortex-M7: `-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard`

### Flashing

#### Linux — OpenOCD

```bash
# Install
sudo apt install openocd

# Flash (ST-Link, STM32F4)
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program build-arm/my_app.elf verify reset exit"
```

#### Linux — st-flash

```bash
# Install
sudo apt install stlink-tools

# Flash
st-flash write build-arm/my_app.bin 0x08000000
```

#### Linux/macOS — probe-rs (Rust-based, modern)

```bash
# Install
cargo install probe-rs-tools

# Flash
probe-rs run --chip STM32F429ZITx build-arm/my_app.elf
```

#### Windows — STM32CubeProgrammer

1. Download from [st.com](https://www.st.com/en/development-tools/stm32cubeprog.html)
2. Connect ST-Link USB
3. Open STM32CubeProgrammer → Open File → select `.bin` or `.elf`
4. Click "Download"

#### Windows — OpenOCD (via MSYS2/WSL)

```bash
# Same as Linux commands, run in MSYS2 or WSL
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program my_app.elf verify reset exit"
```

## Raspberry Pi

### Native compilation (on the Pi)

```bash
sudo apt install cmake libsdl2-dev
cmake -B build -DLUMEN_BUILD_SIMULATOR=ON
cmake --build build
```

### Cross-compilation (from a PC)

```bash
# Install cross compiler
sudo apt install g++-aarch64-linux-gnu

cmake -B build-pi \
  -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
  -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc

cmake --build build-pi

# Copy to Pi
scp build-pi/my_app pi@raspberrypi:~/
ssh pi@raspberrypi ./my_app
```

### Direct framebuffer (no X11)

For headless Pi with a TFT display, Lumen can write directly to
`/dev/fb0` or use DRM/KMS. A `LinuxFramebuffer` display driver
will be provided for this use case.

## Build Configurations

| Config | Target | Display | Notes |
|--------|--------|---------|-------|
| Desktop SDL2 | x86_64 | SDL2 window | Development & testing |
| STM32F4 bare metal | Cortex-M4 | SPI TFT | Typical embedded |
| STM32H7 FreeRTOS | Cortex-M7 | RGB parallel | High-end embedded |
| Raspberry Pi | ARM64 Linux | Framebuffer/SDL2 | Linux embedded |
