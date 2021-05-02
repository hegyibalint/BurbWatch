//
// Created by baprof on 4/18/21.
//

#pragma once

#include <iostream>
#include <iomanip>
#include <exception>

#include <npp.h>
#include <cuda_runtime.h>
#include <cuda_egl_interop.h>

#include <Argus/Argus.h>
#include <EGLStream/EGLStream.h>
#include <EGLStream/NV/ImageNativeBuffer.h>

#include <NvBuffer.h>
#include <NvEglRenderer.h>

#include <vpi/VPI.h>
#include <vpi/algo/Rescale.h>
#include <vpi/algo/ConvertImageFormat.h>
#include <vpi/NvBufferInterop.h>
#include <vpi/OpenCVInterop.hpp>
#include <vpi/CUDAInterop.h>

#include <opencv4/opencv2/opencv.hpp>

#include "image/Image.h"

using namespace Argus;
using namespace EGLStream;

class FrameProcessor {
 public:
  explicit FrameProcessor(OutputStream* stream);
  void start();
  void terminate();

 private:
  bool terminated = false;
  OutputStream* camera_output_stream;
  UniqueObj<FrameConsumer> frame_consumer;
};

#define VPI_SAFE_CALL(func)                                                    \
  ({                                                                           \
     VPIStatus status = (func);                                                \
     if (status != VPI_SUCCESS)                                                \
     {                                                                         \
         char buffer[VPI_MAX_STATUS_MESSAGE_LENGTH];                           \
         vpiGetLastStatusMessage(buffer, sizeof(buffer));                      \
         std::ostringstream ss;                                                \
         ss << "Error at line " << __LINE__ << " ";                            \
         ss << vpiStatusGetName(status) << ": " << buffer;                     \
         throw std::runtime_error(ss.str());                                   \
     }                                                                         \
  })

#define NPP_SAFE_CALL(func)                                                    \
  ({                                                                           \
    NppStatus status = (func);                                                 \
      if (status != NPP_SUCCESS) {                                             \
      std::ostringstream ss;                                                   \
      ss << "Error code";                                                      \
      ss << status << " at line " << __LINE__;                                 \
      throw std::runtime_error{ss.str()};                                      \
    }                                                                          \
  })