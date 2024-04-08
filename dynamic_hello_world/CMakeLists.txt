

include_directories(include ${GStreamer_INCLUDE_DIR})

add_executable(dynamic_hello_world basic-tutorial-3.c)

target_link_directories(dynamic_hello_world PRIVATE ${GStreamer_LIBRARY_DIR})

target_link_libraries(dynamic_hello_world PRIVATE ${GStreamer_LIBS})
