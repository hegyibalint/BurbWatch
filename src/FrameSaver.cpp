//
// Created by baprof on 4/15/21.
//

#include "FrameSaver.hpp"

void FrameSaver::start(FrameQueue &queue) {
    vx_image image;

    while (true) {
        queue.pop(image, 10000);
        std::cout << "Popped image" << std::endl;

        NVXIO_SAFE_CALL(vxReleaseImage(&image));
    }
}
