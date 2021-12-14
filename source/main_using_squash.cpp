#include <stdio.h>
#include <iostream>
#include <sstream>
#include "model-parameters/model_metadata.h"
#include "edge-impulse-sdk/dsp/numpy_types.h"
#include "bitmap_helpers.h"
#include "edge-impulse-sdk/dsp/image/processing.hpp"

// raw frame buffer from the camera
#define FRAME_BUFFER_COLS 320
#define FRAME_BUFFER_ROWS 240
float image[FRAME_BUFFER_COLS * FRAME_BUFFER_ROWS] = {0};
uint8_t image_rgb888_packed[FRAME_BUFFER_COLS * FRAME_BUFFER_ROWS * 3] = {0};

constexpr int R_OFFSET = 0, G_OFFSET = 1, B_OFFSET = 2;
/**
 * This function is called by the classifier to get data
 * We don't want to have a separate copy of the cutout here, so we'll read from the frame buffer dynamically
 */
int image_get_data(size_t offset, size_t length, float *out_ptr) {
    uint32_t out_ptr_ix = 0;

    //change offset from float pointer (size 4) to packed RGB (1B ptr)
    offset = offset*3;

    while (length != 0) {
        // clang-format off
        out_ptr[out_ptr_ix] =
            (image_rgb888_packed[offset + R_OFFSET] << 16) +
            (image_rgb888_packed[offset + G_OFFSET] << 8) +
            image_rgb888_packed[offset + B_OFFSET];
        // clang-format on

        // and go to the next pixel
        out_ptr_ix++;
        offset+=3;
        length--;
    }

    // and done!
    return 0;
}

int main(int argc, char **argv)
{
    // fill frame buffer with some example data... This is normally done by the camera.
    for (size_t row = 0; row < FRAME_BUFFER_ROWS; row++)
    {
        for (size_t col = 0; col < FRAME_BUFFER_COLS; col++)
        {
            // change color a bit (light -> dark from top->down, so we know if the image looks good quickly)
            uint8_t blue_intensity = (uint8_t)((255.0f / (float)(FRAME_BUFFER_ROWS)) * (float)(row));
            uint8_t green_intensity = (uint8_t)((255.0f / (float)(FRAME_BUFFER_COLS)) * (float)(col));

            image[(row * FRAME_BUFFER_COLS) + col] = blue_intensity | green_intensity << 8;
            image_rgb888_packed[((row * FRAME_BUFFER_COLS) + col)*3 + R_OFFSET] = 0; //red is zero for test
            image_rgb888_packed[((row * FRAME_BUFFER_COLS) + col)*3 + G_OFFSET] = green_intensity;
            image_rgb888_packed[((row * FRAME_BUFFER_COLS) + col)*3 + B_OFFSET] = blue_intensity;
        }
    }

    // write to a .bmp file so we can debug
    int b = create_bitmap_file("framebuffer.bmp", image, FRAME_BUFFER_COLS, FRAME_BUFFER_ROWS);
    printf("created framebuffer.bmp? %d\n", b);

    // In place squash the image 
    std::cout << "Squashing to: " << EI_CLASSIFIER_INPUT_WIDTH << " x " << EI_CLASSIFIER_INPUT_HEIGHT << std::endl;

    ei::image::processing::crop_and_interpolate_rgb888(
        image_rgb888_packed, // const uint8_t *srcImage,
        FRAME_BUFFER_COLS, // int srcWidth,
        FRAME_BUFFER_ROWS, //int srcHeight,
        image_rgb888_packed, // uint8_t *dstImage,
        EI_CLASSIFIER_INPUT_WIDTH, // int dstWidth,
        EI_CLASSIFIER_INPUT_HEIGHT); // int dstHeight);

    // set up a signal to read from the frame buffer
    ei::signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &image_get_data;

    float output[signal.total_length];

    // read through the signal buffered, like the classifier lib also does
    for (size_t ix = 0; ix < signal.total_length; ix += 1024) {
        size_t bytes_to_read = 1024;
        if (ix + bytes_to_read > signal.total_length) {
            bytes_to_read = signal.total_length - ix;
        }

        int r = signal.get_data(ix, bytes_to_read, output + ix);
        if (r != 0) {
            printf("Failed to read from signal at ix=%lu (len=%lu) -> (%d)\n", ix, bytes_to_read, r);
        }
    }

    // and write the output bmp file
    b = create_bitmap_file("squashed_from_signal.bmp", output, EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT);
    printf("created squashed_from_signal.bmp? %d\n", b);

    printf("Done\n");
}
