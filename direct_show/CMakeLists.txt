

include_directories(include ${GStreamer_INCLUDE_DIR})

add_executable(direct_show direct_show.c)

target_link_directories(direct_show PRIVATE ${GStreamer_LIBRARY_DIR})

target_link_libraries(direct_show PRIVATE ${GStreamer_LIBS})
