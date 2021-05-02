cmake_minimum_required(VERSION 3.10)
project(BurbCapture LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)
find_package(TegraMM REQUIRED)
find_package(vpi REQUIRED)

include_directories(
        # CUDA
        /usr/local/cuda-10.2/targets/aarch64-linux/include
        # TegraMM
        ${TegraMM_INCLUDE_DIRS}
        # VPI
        ${VPI_INCLUDE_DIRS}
        # OpenCV
        ${OpenCV_INCLUDE_DIRS}
)
link_directories(
        /usr/local/cuda-10.2/targets/aarch64-linux/lib
        /opt/nvidia/deepstream/deepstream-5.1/lib
        /usr/lib/aarch64-linux-gnu/tegra
)

add_executable(BurbCapture
        src/main.cpp
        src/FrameSource.cpp
        src/FrameProcessor.cpp
        src/Image.cpp)
target_link_libraries(BurbCapture
        cuda
        cudart
        vpi
        nppial
        nppisu
        nppidei
        nppitc
        nppim
        nppif
        npps
        ${OpenCV_LIBRARIES}
        tegra_mm)

