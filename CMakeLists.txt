cmake_minimum_required(VERSION 3.10)
project(BurbCapture LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
set(CMAKE_CXX_STANDARD 17)

find_package(OpenCV REQUIRED)
find_package(VisionWorks REQUIRED)
find_package(CUDA REQUIRED)

link_directories(/usr/lib/aarch64-linux-gnu/tegra)
link_directories(/usr/lib/aarch64-linux-gnu/tegra-egl)
link_directories(/usr/share/visionworks/sources/libs/aarch64/linux/release)
link_directories(/usr/share/visionworks/sources/3rdparty/freetype/libs/)

add_executable(BurbCapture
        src/main.cpp
        src/MotionDetector.cpp
        src/FrameSaver.cpp)
target_include_directories(BurbCapture PRIVATE /usr/share/visionworks/sources/nvxio/include)
target_link_libraries(BurbCapture
        -lnvx
        -lovx
        ${OpenCV_LIBRARIES}
        ${VisionWorks_LIBRARIES}
        ${CUDA_LIBRARIES}
        -lgstpbutils-1.0
        -lgstaudio-1.0
        -lgstvideo-1.0
        -lgstapp-1.0
        -lgstbase-1.0
        -lgstreamer-1.0
        -lgobject-2.0
        -lglib-2.0
        -lglfw
        -lfreetype
        -lEGL
        -lGLESv2
        /usr/lib/aarch64-linux-gnu/tegra-egl/libGLESv2_nvidia.so.2
        /usr/lib/aarch64-linux-gnu/tegra/libcuda.so)
