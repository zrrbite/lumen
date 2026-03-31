add_executable(hello_rectangle main.cpp)
target_link_libraries(hello_rectangle PRIVATE lumen_sim)
target_include_directories(hello_rectangle PRIVATE ${CMAKE_SOURCE_DIR})
