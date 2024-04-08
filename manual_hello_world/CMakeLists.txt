

include_directories(include ${GStreamer_INCLUDE_DIR})

add_executable(manual_hello_world basic-tutorial-2.c)

target_link_directories(manual_hello_world PRIVATE ${GStreamer_LIBRARY_DIR})

target_link_libraries(manual_hello_world PRIVATE ${GStreamer_LIBS})
