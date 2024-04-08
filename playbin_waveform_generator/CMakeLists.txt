

include_directories(include ${GStreamer_INCLUDE_DIR})

add_executable(playbin_waveform_generator playback-tutorial-3.c)

target_link_directories(playbin_waveform_generator PRIVATE ${GStreamer_LIBRARY_DIR})

target_link_libraries(playbin_waveform_generator PRIVATE ${GStreamer_LIBS})
