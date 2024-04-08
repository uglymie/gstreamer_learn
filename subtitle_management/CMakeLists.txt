

include_directories(include ${GStreamer_INCLUDE_DIR})

add_executable(subtitle_management playback-tutorial-2.c)

target_link_directories(subtitle_management PRIVATE ${GStreamer_LIBRARY_DIR})

target_link_libraries(subtitle_management PRIVATE ${GStreamer_LIBS})
