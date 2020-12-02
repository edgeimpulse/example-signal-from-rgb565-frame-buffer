#include <stdio.h>
#include <iostream>
#include <sstream>
#include "ei_run_classifier.h"
#include "bitmap_helpers.h"

// raw frame buffer from the camera
#define FRAME_BUFFER_COLS           160
#define FRAME_BUFFER_ROWS           120
uint16_t frame_buffer[FRAME_BUFFER_COLS * FRAME_BUFFER_ROWS] = { 0 };

// cutout that we want (this does not do a resize, which would also be an option, but you'll need some resize lib for that)
#define CUTOUT_COLS                 EI_CLASSIFIER_INPUT_WIDTH
#define CUTOUT_ROWS                 EI_CLASSIFIER_INPUT_HEIGHT
const int cutout_row_start = (FRAME_BUFFER_ROWS - CUTOUT_ROWS) / 2;
const int cutout_col_start = (FRAME_BUFFER_COLS - CUTOUT_COLS) / 2;

// helper methods to convert from rgb -> 565 and vice versa
uint16_t rgb_to_565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

void r565_to_rgb(uint16_t color, uint8_t *r, uint8_t *g, uint8_t *b) {
    *r = (color & 0xF800) >> 8;
    *g = (color & 0x07E0) >> 3;
    *b = (color & 0x1F) << 3;
}

/**
 * This function is called by the classifier to get data
 * We don't want to have a separate copy of the cutout here, so we'll read from the frame buffer dynamically
 */
int cutout_get_data(size_t offset, size_t length, float *out_ptr) {
    // so offset and length naturally operate on the *cutout*, so we need to cut it out from the real framebuffer
    size_t bytes_left = length;
    size_t out_ptr_ix = 0;

    // read byte for byte
    while (bytes_left != 0) {
        // find location of the byte in the cutout
        size_t cutout_row = floor(offset / CUTOUT_COLS);
        size_t cutout_col = offset - (cutout_row * CUTOUT_COLS);

        // then read the value from the real frame buffer
        size_t frame_buffer_row = cutout_row + cutout_row_start;
        size_t frame_buffer_col = cutout_col + cutout_col_start;

        // grab the value and convert to r/g/b
        uint16_t pixel = frame_buffer[(frame_buffer_row * FRAME_BUFFER_COLS) + frame_buffer_col];

        uint8_t r, g, b;
        r565_to_rgb(pixel, &r, &g, &b);

        // then convert to out_ptr format
        float pixel_f = (r << 16) + (g << 8) + b;
        out_ptr[out_ptr_ix] = pixel_f;

        // and go to the next pixel
        out_ptr_ix++;
        offset++;
        bytes_left--;
    }

    // and done!
    return 0;
}


int main(int argc, char **argv) {
    // fill frame buffer with some example data... This is normally done by the camera.
    for (size_t row = 0; row < FRAME_BUFFER_ROWS; row++) {
        for (size_t col = 0; col < FRAME_BUFFER_COLS; col++) {
            // cutout is the center of the frame buffer
            bool within_cout = (row >= cutout_row_start && row < cutout_row_start + CUTOUT_ROWS) &&
                (col >= cutout_col_start && col < cutout_col_start + CUTOUT_COLS);

            // change color a bit (light -> dark from top->down, so we know if the image looks good quickly)
            uint8_t intensity = (uint8_t)((255.0f / static_cast<float>(FRAME_BUFFER_ROWS)) * static_cast<float>(row));

            // if within the cutout make it blue
            if (within_cout) {
                frame_buffer[(row * FRAME_BUFFER_COLS) + col] = rgb_to_565(0, 0, intensity);
            }
            // otherwise red
            else {
                frame_buffer[(row * FRAME_BUFFER_COLS) + col] = rgb_to_565(intensity, 0, 0);
            }
        }
    }

    // write to a .bmp file so we can debug
    int b = create_bitmap_file("framebuffer.bmp", frame_buffer, FRAME_BUFFER_COLS, FRAME_BUFFER_ROWS);
    printf("created framebuffer.bmp? %d\n", b);

    // set up a signal to read from the frame buffer
    signal_t signal;
    signal.total_length = CUTOUT_COLS * CUTOUT_ROWS;
    signal.get_data = &cutout_get_data;

    // this is where we'll write all the output
    float cutout_output[CUTOUT_COLS * CUTOUT_ROWS];

    // read through the signal buffered, like the classifier lib also does
    for (size_t ix = 0; ix < signal.total_length; ix += 1024) {
        size_t bytes_to_read = 1024;
        if (ix + bytes_to_read > signal.total_length) {
            bytes_to_read = signal.total_length - ix;
        }

        int r = signal.get_data(ix, bytes_to_read, cutout_output + ix);
        if (r != 0) {
            printf("Failed to read from signal at ix=%lu (len=%lu) -> (%d)\n", ix, bytes_to_read, r);
        }
    }

    // and write the output bmp file
    b = create_bitmap_file("from_signal.bmp", cutout_output, CUTOUT_COLS, CUTOUT_ROWS);
    printf("created from_signal.bmp? %d\n", b);

    printf("Done\n");
}
