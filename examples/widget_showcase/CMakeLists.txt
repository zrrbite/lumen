add_executable(widget_showcase main.cpp)
target_link_libraries(widget_showcase PRIVATE lumen_sim)
target_include_directories(widget_showcase PRIVATE ${CMAKE_SOURCE_DIR})
