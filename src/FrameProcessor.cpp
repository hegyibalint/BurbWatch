//
// Created by baprof on 4/18/21.
//

#include <zconf.h>
#include "FrameProcessor.h"

#define VPI_SAFE_CALL(func)                                    \
     {                                                         \
         VPIStatus status = (func);                            \
         if (status != VPI_SUCCESS)                            \
         {                                                     \
             char buffer[VPI_MAX_STATUS_MESSAGE_LENGTH];       \
             vpiGetLastStatusMessage(buffer, sizeof(buffer));  \
             std::ostringstream ss;                            \
             ss << "Error at line " << __LINE__ << " ";        \
             ss << vpiStatusGetName(status) << ": " << buffer; \
             throw std::runtime_error(ss.str());               \
         }                                                     \
     }

FrameProcessor::FrameProcessor(OutputStream *camera_output_stream) : camera_output_stream{camera_output_stream} {
  frame_consumer = UniqueObj<FrameConsumer>(FrameConsumer::create(camera_output_stream));
}

void FrameProcessor::terminate() {
  terminated = true;
}

void FrameProcessor::start() {
  auto iOutputStream = interface_cast<IEGLOutputStream>(camera_output_stream);
  auto iFrameConsumer = interface_cast<IFrameConsumer>(frame_consumer);

  std::cout << "Waiting to be connected..." << std::endl;
  if (iOutputStream->waitUntilConnected()!=STATUS_OK) {
    throw std::runtime_error{"Cannot connect to stream"};
  };

  auto res = iOutputStream->getResolution();

  VPIStream stream;
  VPI_SAFE_CALL(vpiStreamCreate(0, &stream))

  VPIImage bgr_image;
  VPI_SAFE_CALL(vpiImageCreate(
      res.width(), res.height(),
      VPI_IMAGE_FORMAT_U8, 0, &bgr_image
  ));



  size_t frame_count = 0;
  while (!terminated) {
    std::cout << "Acquiring frame #" << std::to_string(frame_count) << std::endl;
    UniqueObj<Frame> frame(iFrameConsumer->acquireFrame());
    auto iFrame = interface_cast<IFrame>(frame);
    if (!iFrame) {
      throw std::runtime_error{"Cannot get frame"};
    }

    auto image = iFrame->getImage();
    auto iImageNativeBuffer = interface_cast<NV::IImageNativeBuffer>(image);
    if (!iImageNativeBuffer) {
      throw std::runtime_error{"Cannot get image native buffer"};
    }

    int fd = iImageNativeBuffer->createNvBuffer(
        res,
        NvBufferColorFormat_NV12_ER,
        NvBufferLayout_Pitch);

    VPIImage input_image;
    VPI_SAFE_CALL(vpiImageCreateNvBufferWrapper(fd, nullptr, 0, &input_image))

//    VPIConvertImageFormatParams convert_params = {
//        VPI_CONVERSION_CLAMP,
//        1, 0,
//        0,
//        VPI_INTERP_NEAREST, VPI_INTERP_NEAREST
//    };
    VPI_SAFE_CALL(vpiSubmitConvertImageFormat(stream, VPI_BACKEND_CUDA, input_image, bgr_image, nullptr))
    VPI_SAFE_CALL(vpiStreamSync(stream))

    cv::Mat cv_image;
    VPIImageData bgr_img_data;
    VPI_SAFE_CALL(vpiImageLock(bgr_image, VPI_LOCK_READ, &bgr_img_data))
    VPI_SAFE_CALL(vpiImageDataExportOpenCVMat(bgr_img_data, &cv_image))
    VPI_SAFE_CALL(vpiImageUnlock(bgr_image))

    background_subtractor->apply(cv_image, foregroundMask);
    cv::imshow("FG Mask", foregroundMask);

    cv::waitKey(20);

    vpiImageDestroy(input_image);
    frame_count++;
  }

  vpiStreamDestroy(stream);
  vpiImageDestroy(bgr_image);
}
