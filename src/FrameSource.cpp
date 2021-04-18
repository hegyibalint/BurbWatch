//
// Created by baprof on 4/16/21.
//

#include "FrameSource.h"

#define SENSOR_MODE 1

using namespace Argus;
using namespace EGLStream;

void FrameSource::start() {
  auto cameraProvider = UniqueObj{CameraProvider::create()};
  auto *iCameraProvider = interface_cast<ICameraProvider>(cameraProvider);
  if (!iCameraProvider) {
    throw std::runtime_error{"Cannot get camera provider"};
  }

  std::vector<CameraDevice*> cameraDevices;
  iCameraProvider->getCameraDevices(&cameraDevices);
  if (cameraDevices.empty()) {
    throw std::runtime_error{"No cameras available"};
  }

  auto iCameraProperties = interface_cast<ICameraProperties>(cameraDevices[0]);
  if (!iCameraProperties) {
    throw std::runtime_error{"Cannot get camera properties"};
  }

  std::vector<SensorMode*> sensorModes;
  iCameraProperties->getBasicSensorModes(&sensorModes);
  for (const auto &mode : sensorModes) {
    auto iSensorMode = interface_cast<ISensorMode>(mode);
    std::cout << iSensorMode->getResolution().width() << "x" << iSensorMode->getResolution().height() << std::endl;
  }
  auto iSensorMode = interface_cast<ISensorMode>(sensorModes[SENSOR_MODE]);
  if (!iSensorMode) {
    throw std::runtime_error{"Cannot get source settings"};
  }

  UniqueObj<CaptureSession> captureSession{iCameraProvider->createCaptureSession(cameraDevices[0])};
  auto iCaptureSession = interface_cast<ICaptureSession>(captureSession);
  if (!iCaptureSession){
    throw std::runtime_error{"Cannot get capture session"};
  }

  UniqueObj<OutputStreamSettings> streamSettings{iCaptureSession->createOutputStreamSettings(STREAM_TYPE_EGL)};
  auto iEglStreamSettings = interface_cast<IEGLOutputStreamSettings>(streamSettings);
  if (!iEglStreamSettings) {
    throw std::runtime_error{"Cannot get output stream settings"};
  }
  iEglStreamSettings->setPixelFormat(PIXEL_FMT_YCbCr_420_888);
  iEglStreamSettings->setResolution(iSensorMode->getResolution());
  iEglStreamSettings->setMetadataEnable(true);

  UniqueObj<OutputStream> outputStream{iCaptureSession->createOutputStream(streamSettings.get())};

  FrameProcessor frame_processor{outputStream.get()};
  std::thread processor_thread{&FrameProcessor::start, std::ref(frame_processor)};

  UniqueObj<Request> request(iCaptureSession->createRequest(CAPTURE_INTENT_VIDEO_RECORD));
  auto iRequest = interface_cast<IRequest>(request);
  if (!iRequest) {
    throw std::runtime_error{"Cannot get output stream settings"};
  }
  iRequest->enableOutputStream(outputStream.get());

  auto iSourceSettings = interface_cast<ISourceSettings>(iRequest->getSourceSettings());
  iSourceSettings->setSensorMode(sensorModes[SENSOR_MODE]);

  std::cout << "Submitting capture request" << std::endl;
  if (iCaptureSession->repeat(request.get()) != STATUS_OK) {
    throw std::runtime_error{"Cannot submit capture request"};
  }

  std::this_thread::sleep_for(std::chrono::seconds(60));
  iCaptureSession->stopRepeat();
  iCaptureSession->waitForIdle();

  std::cout << "Terminating processor" << std::endl;
  frame_processor.terminate();
  std::cout << "Joining thread" << std::endl;
  processor_thread.join();

  outputStream.reset();
  cameraProvider.reset();
}


