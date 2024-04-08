

include_directories(include ${GStreamer_INCLUDE_DIR})

add_executable(media_player_in_GTK basic-tutorial-5.c)

target_link_directories(media_player_in_GTK PRIVATE ${GStreamer_LIBRARY_DIR})

target_link_libraries(media_player_in_GTK PRIVATE ${GStreamer_LIBS})
