cmake_minimum_required(VERSION 3.10)
project(BurbCapture LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
set(CMAKE_CXX_STANDARD 17)
set(TegraMM_ROOT /usr/src/jetson_multimedia_api)

find_package(TegraMM REQUIRED)
find_package(vpi REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)

include_directories(
        /usr/src/jetson_multimedia_api/include
        /usr/src/jetson_multimedia_api/argus/include
        /usr/src/jetson_multimedia_api/include/libjpeg-8b
)
link_directories(
        /usr/lib/aarch64-linux-gnu
        /usr/lib/aarch64-linux-gnu/tegra
)

add_executable(BurbCapture
        src/main.cpp
        src/FrameSource.cpp
        src/FrameProcessor.cpp)
target_link_libraries(BurbCapture
        tegra_mm
        vpi
        Threads::Threads
        ${OpenCV_LIBRARIES})
