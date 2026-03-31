add_executable(dashboard main.cpp)
target_link_libraries(dashboard PRIVATE lumen_sim)
target_include_directories(dashboard PRIVATE ${CMAKE_SOURCE_DIR})
