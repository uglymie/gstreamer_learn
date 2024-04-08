

include_directories(include ${GStreamer_INCLUDE_DIR})

add_executable(seeking_example basic-tutorial-4.c)

target_link_directories(seeking_example PRIVATE ${GStreamer_LIBRARY_DIR})

target_link_libraries(seeking_example PRIVATE ${GStreamer_LIBS})
