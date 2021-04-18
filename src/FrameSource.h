//
// Created by baprof on 4/16/21.
//

#pragma once

#include <iostream>
#include <memory>
#include <exception>
#include <thread>

#include <Argus/Argus.h>
#include <EGLStream/EGLStream.h>

#include "FrameProcessor.h"

class FrameSource {
 public:
  void start();
};