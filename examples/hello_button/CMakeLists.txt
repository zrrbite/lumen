add_executable(hello_button main.cpp)
target_link_libraries(hello_button PRIVATE lumen_sim)
target_include_directories(hello_button PRIVATE ${CMAKE_SOURCE_DIR})
