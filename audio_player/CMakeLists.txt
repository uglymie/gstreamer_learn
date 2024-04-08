

include_directories(include ${GStreamer_INCLUDE_DIR})

add_executable(audio_player playbin_way.c)   # audio_player.c    

target_link_directories(audio_player PRIVATE ${GStreamer_LIBRARY_DIR})

target_link_libraries(audio_player PRIVATE ${GStreamer_LIBS})
