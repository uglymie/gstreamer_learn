
set(OpenCV_DIR "D:/opencv/build")
include(${OpenCV_DIR}/OpenCVConfig.cmake)
include(../cmake/nvidia_common.cmake)

find_package(OpenCV REQUIRED)

include_directories(include 
    ${GStreamer_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
    ${CUDA_INCLUDE_DIRS}
    )

file(GLOB SOURCES *.cpp)

add_executable(gstDecoder ${SOURCES})

target_link_directories(gstDecoder PRIVATE ${GStreamer_LIBRARY_DIR})

target_link_libraries(gstDecoder PRIVATE ${GStreamer_LIBS} ${OpenCV_LIBRARIES})
