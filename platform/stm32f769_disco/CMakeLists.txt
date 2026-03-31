add_library(stm32f769_disco STATIC
    startup.cpp
    ${CMAKE_SOURCE_DIR}/src/gfx/font_6x8.cpp
)

target_include_directories(stm32f769_disco PUBLIC ${CMAKE_SOURCE_DIR})

target_compile_options(stm32f769_disco PUBLIC
    -mcpu=cortex-m7
    -mfpu=fpv5-sp-d16
    -mfloat-abi=hard
    -mthumb
    -fno-exceptions
    -fno-rtti
    -ffunction-sections
    -fdata-sections
)

target_link_options(stm32f769_disco PUBLIC
    -mcpu=cortex-m7
    -mfpu=fpv5-sp-d16
    -mfloat-abi=hard
    -mthumb
    -T${CMAKE_CURRENT_SOURCE_DIR}/stm32f769ni.ld
    -Wl,--gc-sections
    -lnosys
    -lc
    -Wl,--no-warn-rwx-segments
)
