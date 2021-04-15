#include <iostream>
#include <thread>

#include <NVX/Application.hpp>
#include <NVX/ThreadSafeQueue.hpp>

#include "Common.hpp"
#include "MotionDetector.hpp"
#include "FrameSaver.hpp"

int main(int argc, char **argv) {
    nvxio::Application &app = nvxio::Application::get();
    app.init(argc, argv);

    FrameQueue queue{10};
    MotionDetector motionDetector;
    FrameSaver frameSaver;

    std::thread motionDetectorThread{&MotionDetector::start, std::ref(motionDetector), std::ref(queue)};
    std::thread frameSaverThread{&FrameSaver::start, std::ref(frameSaver), std::ref(queue)};

    motionDetectorThread.join();
    frameSaverThread.join();
}