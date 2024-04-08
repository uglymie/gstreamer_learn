cmake_minimum_required(VERSION 3.0.0)
project(basic_tutorial VERSION 0.1.0 LANGUAGES C CXX)

include(cmake/FindGStreamer.cmake)
LOAD_LIB_GStreamer()

add_subdirectory(hello_world)
add_subdirectory(manual_hello_world)
add_subdirectory(dynamic_hello_world)
add_subdirectory(seeking_example)
# add_subdirectory(media_player_in_GTK)

add_subdirectory(gstreamer_discoverer)
add_subdirectory(network_resilient)


add_subdirectory(multilingual_player)
add_subdirectory(subtitle_management)
add_subdirectory(playbin_waveform_generator)

add_subdirectory(audio_player)
add_subdirectory(direct_show)

add_subdirectory(gstCamera)
add_subdirectory(gstDecoder)