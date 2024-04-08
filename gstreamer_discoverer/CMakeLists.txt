

include_directories(include ${GStreamer_INCLUDE_DIR})

add_executable(gstreamer_discoverer basic-tutorial-9.c)

target_link_directories(gstreamer_discoverer PRIVATE ${GStreamer_LIBRARY_DIR})

target_link_libraries(gstreamer_discoverer PRIVATE ${GStreamer_LIBS})
