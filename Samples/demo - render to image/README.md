![preview](https://github.com/johannes-fetz/joengine/tree/master/Samples/demo%20-%20render%20to%20image/preview.jpg?raw=true)
# demo - render to image
This demo shows VDP1 rendering a rotating cube offscreen and then putting the rendered image as a texture on another rotating cube.

It uses the render to image of the engine (`jo/render_to_image.h`): `jo_3d_render_to_image_init()`, `jo_3d_render_to_image_add_sprite()`, then `jo_3d_render_to_image_begin()` and `jo_3d_render_to_image_end()` around the drawings of each frame.

## Note
This demo might not work in some emulators, testing with mednafen or on real HW is recommended

##Credits
Demo made by [ReyeMe](https://reye.me/)

## Special thanks to
XL2, mrkotfw and hitomi2500