

include_directories(include ${GStreamer_INCLUDE_DIR})

add_executable(multilingual_player playback-tutorial-1.c)

target_link_directories(multilingual_player PRIVATE ${GStreamer_LIBRARY_DIR})

target_link_libraries(multilingual_player PRIVATE ${GStreamer_LIBS})
