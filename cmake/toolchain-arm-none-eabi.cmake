set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_SIZE arm-none-eabi-size)

set(CMAKE_CXX_FLAGS_INIT "-mthumb -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti")
set(CMAKE_C_FLAGS_INIT "-mthumb -ffunction-sections -fdata-sections")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,--gc-sections -specs=nosys.specs -specs=nano.specs")

# Prevent CMake from trying to link a test program (bare metal has no OS)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
