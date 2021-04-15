//
// Created by baprof on 4/15/21.
//
#pragma once

#include <iostream>
#include <chrono>

#include <NVX/nvx.h>
#include <NVX/ThreadSafeQueue.hpp>

#include <OVX/RenderOVX.hpp>
#include <OVX/FrameSourceOVX.hpp>
#include <OVX/UtilityOVX.hpp>

#include "Common.hpp"

class MotionDetector {
private:

public:
    MotionDetector() = default;
    MotionDetector(MotionDetector& other) = delete;

    void start(FrameQueue& queue);
};
