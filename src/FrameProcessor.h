//
// Created by baprof on 4/18/21.
//

#pragma once

#include <iostream>
#include <iomanip>
#include <exception>

#include <Argus/Argus.h>
#include <EGLStream/EGLStream.h>
#include <EGLStream/NV/ImageNativeBuffer.h>

#include <opencv4/opencv2/opencv.hpp>
#include <vpi/VPI.h>
#include <vpi/algo/ConvertImageFormat.h>
#include <vpi/NvBufferInterop.h>
#include <vpi/OpenCVInterop.hpp>

using namespace Argus;
using namespace EGLStream;

class FrameProcessor {
 public:
  FrameProcessor(OutputStream* stream);
  void start();
  void terminate();

 private:
  bool terminated = false;
  OutputStream* camera_output_stream;
  UniqueObj<FrameConsumer> frame_consumer;
};