//
// Created by baprof on 4/15/21.
//

#pragma once

#include <NVX/nvx.h>
#include <NVX/ThreadSafeQueue.hpp>

#include <OVX/RenderOVX.hpp>
#include <OVX/UtilityOVX.hpp>

#include "Common.hpp"

class FrameSaver {
private:

public:
    FrameSaver() = default;
    FrameSaver(FrameSaver& other) = delete;

    void start(FrameQueue& queue);
};