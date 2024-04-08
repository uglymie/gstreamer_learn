

include_directories(include ${GStreamer_INCLUDE_DIR})

add_executable(network_resilient basic-tutorial-12.c)

target_link_directories(network_resilient PRIVATE ${GStreamer_LIBRARY_DIR})

target_link_libraries(network_resilient PRIVATE ${GStreamer_LIBS})
