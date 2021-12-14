# Edge Impulse Example: classifying RGB565 frame buffer data

The Edge Impulse inferencing SDK expects a `signal_t` structure that contains sensor data - this is done so you don't need to load the full data into memory, but rather can page data in when needed. This `signal_t` structure expects data to be laid out in RGB888 format, but many cameras output data in RGB565 format instead. Additionally the classifier expects data in set dimensions (defined in `EI_CLASSIFIER_INPUT_WIDTH` / `EI_CLASSIFIER_INPUT_HEIGHT`) which might not be a native resolution of the camera. This example shows how to directly interact with an RGB565 frame buffer, by converting the data on the fly, and by creating a cutout when the resolutions don't match.

## How to use this example

This is a full demonstration application that runs on macOS and Linux, but you'll only need the `r565_to_rgb` and `cutout_get_data` functions, plus the defines from `main.cpp`, on your embedded device. These functions have no external dependencies and build on any system where the Edge Impulse classifier runs.

To run this application:

1. Install the dependencies listed in [Running your impulse locally on your desktop computer](https://docs.edgeimpulse.com/docs/running-your-impulse-locally).
1. Build the application:

    ```
    $ sh build.sh
    ```

1. Run the application:

    ```
    $ ./build/edge-impulse-standalone
    ```

This has now created two files:

* [framebuffer.bmp](framebuffer.bmp) - a 'fake' framebuffer that was created in RGB565 format.
* [from_signal.bmp](from_signal.bmp) - the image as received by the classifier. This went through RGB565->RGB888 conversion, and through the cutout.

### How to test squashing/resizing
Instead of just cutting out part of the image, you can also just crop down to the same aspect ratio, then squash/resize (bilinear interpolation) the rest of the image.  This preserves more of your frame. To try this, run the following
```
make squash
```

Then run the application, same as above.  
* [framebuffer.bmp](framebuffer.bmp) - The original, non squashed image
* [squashed_from_signal.bmp](squashed_from_signal.bmp) - the image as received by the classifier. This was cropped, then squashed
## Questions?

Let us know on the [forums](https://forum.edgeimpulse.com).
