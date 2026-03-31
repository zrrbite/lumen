add_executable(stm32f429_hello main.cpp)
target_link_libraries(stm32f429_hello PRIVATE stm32f429_disco)

# Generate .bin and .hex for flashing
add_custom_command(TARGET stm32f429_hello POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:stm32f429_hello> stm32f429_hello.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:stm32f429_hello> stm32f429_hello.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:stm32f429_hello>
    COMMENT "Generating .bin and .hex, showing size"
)
