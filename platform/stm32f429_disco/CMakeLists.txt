add_library(stm32f429_disco STATIC
    startup.cpp
    ${CMAKE_SOURCE_DIR}/src/gfx/font_6x8.cpp
)

target_include_directories(stm32f429_disco PUBLIC ${CMAKE_SOURCE_DIR})

target_compile_options(stm32f429_disco PUBLIC
    -mcpu=cortex-m4
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
    -mthumb
    -fno-exceptions
    -fno-rtti
    -ffunction-sections
    -fdata-sections
)

target_link_options(stm32f429_disco PUBLIC
    -mcpu=cortex-m4
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
    -mthumb
    -T${CMAKE_CURRENT_SOURCE_DIR}/stm32f429zi.ld
    -Wl,--gc-sections
    -lnosys
    -lc
    -Wl,--no-warn-rwx-segments
)
