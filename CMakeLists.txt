cmake_minimum_required(VERSION 3.20)
project(lumen VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(LUMEN_BUILD_EXAMPLES "Build example applications" OFF)
option(LUMEN_BUILD_SIMULATOR "Build SDL2 desktop simulator" OFF)

# Core library — mostly headers with a small compiled component for font data
add_library(lumen_core STATIC
    src/gfx/font_6x8.cpp
)

target_include_directories(lumen_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(lumen_core PUBLIC cxx_std_17)

# Header-only interface for template code
add_library(lumen INTERFACE)

target_include_directories(lumen INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(lumen INTERFACE cxx_std_17)

# No exceptions, no RTTI on embedded
if(CMAKE_CROSSCOMPILING)
    target_compile_options(lumen INTERFACE -fno-exceptions -fno-rtti -fno-threadsafe-statics)
endif()

# Desktop simulator support
if(LUMEN_BUILD_SIMULATOR)
    find_package(SDL2 REQUIRED)
    add_library(lumen_sim INTERFACE)
    target_link_libraries(lumen_sim INTERFACE lumen lumen_core SDL2::SDL2)
    target_compile_definitions(lumen_sim INTERFACE LUMEN_PLATFORM_DESKTOP=1)
endif()

# Examples
if(LUMEN_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# STM32F429 platform (cross-compilation only)
if(CMAKE_CROSSCOMPILING AND LUMEN_TARGET_F429)
    add_subdirectory(platform/stm32f429_disco)
    add_executable(stm32f429_hello examples/stm32f429_hello/main.cpp)
    target_link_libraries(stm32f429_hello PRIVATE stm32f429_disco)
    # Generate .bin
    add_custom_command(TARGET stm32f429_hello POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:stm32f429_hello> stm32f429_hello.bin
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:stm32f429_hello>
        COMMENT "Generating .bin, showing size"
    )
endif()

# STM32F769 platform (cross-compilation only)
if(CMAKE_CROSSCOMPILING AND LUMEN_TARGET_F769)
    add_subdirectory(platform/stm32f769_disco)
    add_executable(stm32f769_hello examples/stm32f769_hello/main.cpp)
    target_link_libraries(stm32f769_hello PRIVATE stm32f769_disco)
    add_custom_command(TARGET stm32f769_hello POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:stm32f769_hello> stm32f769_hello.bin
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:stm32f769_hello>
        COMMENT "Generating .bin, showing size"
    )
endif()
