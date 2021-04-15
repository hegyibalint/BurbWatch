//
// Created by baprof on 4/15/21.
//

#include "MotionDetector.hpp"

vx_uint32 rs_w = 16;
vx_uint32 rs_h = 9;
vx_int32 thresold_value = 10;

void MotionDetector::start(FrameQueue &queue) {
    ovxio::ContextGuard context;
    vxDirective(context, VX_DIRECTIVE_ENABLE_PERFORMANCE);

    ovxio::FrameSource::Parameters requested_config;
    requested_config.frameWidth = 1920;
    requested_config.frameHeight = 1080;

    auto frameSource = ovxio::createDefaultFrameSource(context, "device:///nvcamera");
    frameSource->setConfiguration(requested_config);
    if (!frameSource->open()) {
        throw std::runtime_error{"Couldn't open frame source"};
    }

    auto config = frameSource->getConfiguration();
    auto renderer{ovxio::createDefaultRender(context, "Demo", config.frameWidth, config.frameHeight)};

    vx_graph graph = vxCreateGraph(context);
    NVXIO_CHECK_REFERENCE(graph);

    vx_image input_image = vxCreateImage(context, config.frameWidth, config.frameHeight, config.format);
    NVXIO_CHECK_REFERENCE(input_image);

    vx_image input_yuv = vxCreateImage(context, config.frameWidth, config.frameHeight, VX_DF_IMAGE_IYUV);
    NVXIO_CHECK_REFERENCE(input_yuv);

    vx_image gray_image = vxCreateImage(context, config.frameWidth, config.frameHeight, VX_DF_IMAGE_U8);
    NVXIO_CHECK_REFERENCE(gray_image);

    vx_image delay_exemplar = vxCreateImage(context, rs_w, rs_h, VX_DF_IMAGE_U8);
    NVXIO_CHECK_REFERENCE(delay_exemplar);
    vx_delay delay = vxCreateDelay(context, (vx_reference) delay_exemplar, 2);
    NVXIO_CHECK_REFERENCE(delay);
    NVXIO_SAFE_CALL(vxReleaseImage(&delay_exemplar));
    auto current_scaled = (vx_image) vxGetReferenceFromDelay(delay, 0);
    NVXIO_CHECK_REFERENCE(current_scaled);
    auto previous_scaled = (vx_image) vxGetReferenceFromDelay(delay, -1);
    NVXIO_CHECK_REFERENCE(previous_scaled);

    vx_image diff_image = vxCreateImage(context, rs_w, rs_h, VX_DF_IMAGE_U8);
    NVXIO_CHECK_REFERENCE(diff_image);

    vx_threshold threshold = vxCreateThreshold(context, VX_THRESHOLD_TYPE_BINARY, VX_TYPE_UINT8);
    NVXIO_CHECK_REFERENCE(threshold);
    NVXIO_SAFE_CALL(
            vxSetThresholdAttribute(threshold, VX_THRESHOLD_THRESHOLD_VALUE, &thresold_value, sizeof(thresold_value)));

    vx_image thresold_image = vxCreateImage(context, rs_w, rs_h, VX_DF_IMAGE_U8);
    NVXIO_CHECK_REFERENCE(diff_image);

    vx_image eroded_thresold_image = vxCreateImage(context, rs_w, rs_h, VX_DF_IMAGE_U8);
    NVXIO_CHECK_REFERENCE(diff_image);

    vx_node nodes[] = {
            vxColorConvertNode(graph, input_image, input_yuv),
            vxChannelExtractNode(graph, input_yuv, VX_CHANNEL_Y, gray_image),
            vxScaleImageNode(graph, gray_image, current_scaled, VX_INTERPOLATION_TYPE_AREA),
            vxAbsDiffNode(graph, previous_scaled, current_scaled, diff_image),
            vxThresholdNode(graph, diff_image, threshold, thresold_image),
            vxErode3x3Node(graph, thresold_image, eroded_thresold_image)
    };
    for (auto &node : nodes) {
        NVXIO_CHECK_REFERENCE(node);
        NVXIO_SAFE_CALL(vxReleaseNode(&node));
    }
    NVXIO_SAFE_CALL(vxVerifyGraph(graph));

    size_t frame_index = 0;
    size_t pixel_count = config.frameWidth / 80 * config.frameHeight / 80;
    std::unique_ptr<vx_uint8[]> buffer{new vx_uint8[pixel_count]};
    vx_perf_t perf;
    while (true) {
//        std::cout << "Processsing frame " << frame_index << std::endl;
        frameSource->fetch(input_image);

        NVXIO_SAFE_CALL(vxProcessGraph(graph));

//        renderer->putImage(eroded_thresold_image);

        NVXIO_SAFE_CALL(vxQueryGraph(graph, VX_GRAPH_ATTRIBUTE_PERFORMANCE, &perf, sizeof(perf)));
//        std::cout << "Graph Time : " << static_cast<double>(perf.tmp) / 1000000.0 << " ms" << std::endl;

        vx_uint32 eroded_height, eroded_width;
        NVXIO_SAFE_CALL(
                vxQueryImage(eroded_thresold_image, VX_IMAGE_ATTRIBUTE_WIDTH, &eroded_width, sizeof(eroded_width)));
        NVXIO_SAFE_CALL(
                vxQueryImage(eroded_thresold_image, VX_IMAGE_ATTRIBUTE_HEIGHT, &eroded_height, sizeof(eroded_height)));

        vx_rectangle_t rect = {0, 0, eroded_width, eroded_height};
        vx_imagepatch_addressing_t addr{
                eroded_width, eroded_height,
                sizeof(vx_uint8),
                static_cast<vx_int32>(eroded_width * sizeof(vx_uint8))
        };
        NVXIO_SAFE_CALL(vxCopyImagePatch(eroded_thresold_image, &rect, 0, &addr, buffer.get(), VX_READ_ONLY,
                                         VX_MEMORY_TYPE_HOST));

        vx_uint32 active_blocks = 0;
        for (vx_uint32 i = 0; i < eroded_width * eroded_height; ++i) {
            active_blocks = buffer[i] > 0 ? active_blocks + 1 : active_blocks;
        }
//        std::cout << "Active blocks: " << std::to_string(active_blocks) << std::endl;

        if (active_blocks > 0) {

        }

        renderer->flush();
        NVXIO_SAFE_CALL(vxAgeDelay(delay));

        frame_index++;
    }

    NVXIO_SAFE_CALL(vxReleaseGraph(&graph));
    NVXIO_SAFE_CALL(vxReleaseThreshold(&threshold));
    NVXIO_SAFE_CALL(vxReleaseImage(&input_image));
    NVXIO_SAFE_CALL(vxReleaseImage(&previous_scaled));
    NVXIO_SAFE_CALL(vxReleaseImage(&current_scaled));
    NVXIO_SAFE_CALL(vxReleaseImage(&thresold_image));
    NVXIO_SAFE_CALL(vxReleaseImage(&eroded_thresold_image));
    NVXIO_SAFE_CALL(vxReleaseDelay(&delay));
}
