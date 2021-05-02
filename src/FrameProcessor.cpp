//
// Created by baprof on 4/18/21.
//

#include "FrameProcessor.h"

void npp32FImshow(Npp32f *image, int stride, NppiSize frameSize) {
  auto gpu_mat = cv::cuda::GpuMat{
      cv::Size{frameSize.width, frameSize.height},
      CV_32FC1,
      image,
      static_cast<size_t>(stride)
  };
  cv::Mat cpu_mat{gpu_mat};

  uint8_t value = cpu_mat.at<uint8_t>(0, 0);
  std::cout << "Value @ 0,0: " << std::to_string(value) << std::endl;

  cv::imshow("Test", cpu_mat);
  cv::waitKey(5);
}

void npp8UImshow(Npp8u *image, int stride, NppiSize frameSize) {
  auto gpu_mat = cv::cuda::GpuMat{
      cv::Size{frameSize.width, frameSize.height},
      CV_8UC1,
      image,
      static_cast<size_t>(stride)
  };
  cv::Mat cpu_mat{gpu_mat};

  float32_t value = cpu_mat.at<float32_t>(0, 0);
  std::cout << "Value @ 0,0: " << std::to_string(value) << std::endl;

  cv::imshow("Test", cpu_mat);
  cv::waitKey(5);
}

VPIStatus initCuda8UImage(
    VPIImage &image,
    Npp8u *&image_data,
    int &image_step,
    Size2D<uint32_t> frame_size) {
  image_data = nppiMalloc_8u_C1(frame_size.width(), frame_size.height(), &image_step);
  if (image_data==nullptr) {
    throw std::runtime_error{"Cannot allocate CUDA memory"};
  }

  VPIImageData vpi_image_data;
  memset(&vpi_image_data, 0, sizeof(image_data));
  vpi_image_data.numPlanes = 1;
  vpi_image_data.format = VPI_IMAGE_FORMAT_U8;
  vpi_image_data.planes[0].width = frame_size.width();
  vpi_image_data.planes[0].height = frame_size.height();
  vpi_image_data.planes[0].pixelType = VPI_PIXEL_TYPE_U8;
  vpi_image_data.planes[0].pitchBytes = image_step;
  vpi_image_data.planes[0].data = image_data;

  return vpiImageCreateCUDAMemWrapper(&vpi_image_data, 0, &image);
}

VPIStatus initCuda32FImage(
    VPIImage &image,
    Npp32f *&image_data,
    int &image_step,
    Size2D<uint32_t> frame_size) {
  image_data = nppiMalloc_32f_C1(frame_size.width(), frame_size.height(), &image_step);
  if (image_data==nullptr) {
    throw std::runtime_error{"Cannot allocate CUDA memory"};
  }

  VPIImageData vpi_image_data;
  memset(&vpi_image_data, 0, sizeof(image_data));
  vpi_image_data.numPlanes = 1;
  vpi_image_data.format = VPI_IMAGE_FORMAT_F32;
  vpi_image_data.planes[0].width = frame_size.width();
  vpi_image_data.planes[0].height = frame_size.height();
  vpi_image_data.planes[0].pixelType = VPI_PIXEL_TYPE_F32;
  vpi_image_data.planes[0].pitchBytes = image_step;
  vpi_image_data.planes[0].data = image_data;

  return vpiImageCreateCUDAMemWrapper(&vpi_image_data, 0, &image);
}

FrameProcessor::FrameProcessor(OutputStream *camera_output_stream) : camera_output_stream{camera_output_stream} {
  frame_consumer = UniqueObj<FrameConsumer>(FrameConsumer::create(camera_output_stream));
}

void FrameProcessor::start() {
  auto iOutputStream = interface_cast<IEGLOutputStream>(camera_output_stream);
  auto iFrameConsumer = interface_cast<IFrameConsumer>(frame_consumer);

  std::cout << "Waiting to be connected..." << std::endl;
  if (iOutputStream->waitUntilConnected()!=STATUS_OK) {
    throw std::runtime_error{"Cannot connect to stream"};
  };
  std::cout << "Connected" << std::endl;

  auto res = iOutputStream->getResolution();

  VPIStream vpi_stream;

  int input_fd = -1;
  VPIImage input_image;

  VPIImage rescaled_image;
  VPI_SAFE_CALL(vpiImageCreate(640, 480, VPI_IMAGE_FORMAT_NV12_ER, 0, &rescaled_image));

  int gray_image_step;
  Npp8u* gray_image_data;
  VPIImage gray_image;

  int bg_image_step;
  Npp32f* bg_image_data;
  VPIImage bg_image;

  int diff_image_step;
  Npp8u* diff_image_data;
  VPIImage diff_image;

  int eroded_image_step;
  Npp8u* eroded_image_data;
  VPIImage eroded_image;

  int dilated_image_step;
  Npp8u* dilated_image_data;
  VPIImage dilated_image;

  auto rescaled_res = Size2D<uint32_t> {
    16, 9
  };
  auto npp_res = NppiSize{
      static_cast<int>(res.width()),
      static_cast<int>(res.height())
  };

  VPI_SAFE_CALL(vpiStreamCreate(0, &vpi_stream));

  VPI_SAFE_CALL(initCuda32FImage(bg_image, bg_image_data, bg_image_step, rescaled_res));
  VPI_SAFE_CALL(initCuda32FImage(bg_image, bg_image_data, bg_image_step, rescaled_res));
  VPI_SAFE_CALL(initCuda8UImage(diff_image, diff_image_data, diff_image_step, rescaled_res));
  VPI_SAFE_CALL(initCuda8UImage(eroded_image, eroded_image_data, eroded_image_step, rescaled_res));
  VPI_SAFE_CALL(initCuda8UImage(dilated_image, dilated_image_data, dilated_image_step, rescaled_res));

  int label_buffer_size = -1;
  NPP_SAFE_CALL(nppiLabelMarkersGetBufferSize_8u_C1R(npp_res, &label_buffer_size));
  auto label_buffer = nppsMalloc_8u(label_buffer_size);

  cv::Mat cv_image;
  VPIImageData cv_image_data;

  //nppiSet_32f_C1R(128, bg_image_data, bg_image_step, nppRes);

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

    if (input_fd==-1) {
      input_fd = iImageNativeBuffer->createNvBuffer(
          res,
          NvBufferColorFormat_NV12_ER,
          NvBufferLayout_Pitch
      );
    } else {
      iImageNativeBuffer->copyToNvBuffer(input_fd);
    }

    VPI_SAFE_CALL(vpiImageCreateNvBufferWrapper(input_fd, nullptr, 0, &input_image));
    VPI_SAFE_CALL(vpiSubmitRescale(vpi_stream, VPI_BACKEND_VIC, input_image, rescaled_image, VPI_INTERP_LINEAR, VPI_BORDER_CLAMP, 0));
    VPI_SAFE_CALL(vpiSubmitConvertImageFormat(vpi_stream, VPI_BACKEND_CUDA, rescaled_image, gray_image, nullptr));
    VPI_SAFE_CALL(vpiStreamSync(vpi_stream));

//    nppiDivC_32f_C1IR(
//        255.0f,
//        gray_image_data, gray_image_step,
//        npp_res
//    );
//
//    if (frame_count==0) {
//      nppiCopy_32f_C1R(
//          gray_image_data, gray_image_step,
//          bg_image_data, bg_image_step,
//          npp_res
//      );
//    }
//
//    float alpha = 0.1f;
////    nppiMulC_32f_C1IR(
////        1 - alpha,
////        bg_image_data, bg_image_step,
////        nppRes
////    );
//    NPP_SAFE_CALL(nppiAddWeighted_32f_C1IR(
//        gray_image_data, gray_image_step,
//        bg_image_data, bg_image_step,
//        npp_res, alpha
//    ));
//    NPP_SAFE_CALL(nppiCompareEqualEps_32f_C1R(
//        gray_image_data, gray_image_step,
//        bg_image_data, bg_image_step,
//        diff_image_data, diff_image_step,
//        npp_res, 0.1
//    ));
//    NPP_SAFE_CALL(nppiErode3x3_8u_C1R(
//        diff_image_data, diff_image_step,
//        eroded_image_data, eroded_image_step,
//        npp_res
//    ));
//    NPP_SAFE_CALL(nppiDilate3x3_8u_C1R(
//        eroded_image_data, eroded_image_step,
//        dilated_image_data, dilated_image_step,
//        npp_res
//    ));
//    nppiThreshold_LTValGTVal_32f_C1IR(
//        diff_image_data, diff_image_step,
//        nppRes,
//        0.25, 0,
//        0.25, 1.0
//    );
//    nppiMulC_32f_C1IR(
//        10,
//        diff_image_data, diff_image_step,
//        nppRes
//    );
//    int label_count;
//    NPP_SAFE_CALL(nppiLabelMarkers_8u_C1IR(
//        dilated_image_data, dilated_image_step,
//        npp_res,
//        1, nppiNormInf,
//        &label_count, label_buffer
//    ));

//    std::cout << "\tLabel count: " << std::to_string(label_count) << std::endl;

    //npp32FImshow(gray_image_data, gray_image_step, nppRes);
//    npp32FImshow(bg_image_data, bg_image_step, nppRes);
//    npp8UImshow(dilated_image_data, dilated_image_step, nppRes);


    vpiImageDestroy(input_image);
    frame_count++;
  }

  NvReleaseFd(input_fd);
}

void FrameProcessor::terminate() {
  terminated = true;
}