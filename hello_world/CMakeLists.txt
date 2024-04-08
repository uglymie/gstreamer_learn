

include_directories(include ${GStreamer_INCLUDE_DIR})

add_executable(hello_world basic-tutorial-1.c)

target_link_directories(hello_world PRIVATE ${GStreamer_LIBRARY_DIR})

target_link_libraries(hello_world PRIVATE ${GStreamer_LIBS})
