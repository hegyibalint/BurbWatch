#include <iostream>
#include <thread>
#include <exception>

#include "FrameSource.h"

int main(int argc, char **argv) {
//  auto renderer = NvEglRenderer::createEglRenderer(
//      "renderer0", 1280, 720, 0, 0
//  );
//  if (!renderer) {
//    throw std::runtime_error{"Cannot create renderer"};
//  }

  FrameSource frameSource;
  frameSource.start();

//  delete renderer;
}