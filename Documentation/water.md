# Water and sun {#water_guide}

The water of the engine (`jo/water.h`): a sea with waves around the camera, objects that float on it or are seen through
it, and the sun in the sky with its lens flare (`jo/sun.h`). It needs the 3D module and the 2 CPUs (the dual CPU support,
on by default). As with every module, a game links only the functions it calls.

- Sample: `Samples/demo - 3D water`: the sea, crates that fall in it and float, the looks and the swells.

Units: 1 unit = 1/16 m. Y goes down: the calm surface is at y = 0 and a crest at y < 0. Heights are given up (> 0: a
crest). Time is counted in frames at 60 per second (`jo_core_get_vblank_count()`), whatever the frame rate of the game.

## The sea

```c
jo_water_config config;
jo_water_default_config(&config);                       // 4 levels of quads, the swell JO_WATER_SWELL_CHOPPY
jo_water_config_look(&config, JO_WATER_LOOK_TROPICAL_TEAL);
jo_water *sea = jo_water_create(&config);

// each frame
jo_water_update(sea, jo_core_get_vblank_count(), cam_x, cam_y, cam_z, heading);
jo_3d_camera_look_at(&cam);
jo_water_draw_with(sea, draw_on_master, draw_on_slave); // or jo_water_draw(sea)
```

- **The grid.** Levels of quads around the camera, from small quads near it to bigger ones farther away
  (`jo_water_level`), then a flat skirt to the horizon.
  - The first levels are computed and drawn by the slave CPU.
  - The far levels are updated every 2 or 4 frames.
  - `jo_water_draw_with()` draws objects on the master CPU while the slave CPU draws the water.
- **The swell.** A sum of waves (`jo_water_wave`: length, direction, height, period).
  - `jo_water_swell()` fills a standard one: `GENTLE`, `CHOPPY` or `STORM`.
  - `jo_water_set_waves()` and `jo_water_set_swell()` change it at any time.
- **The look.** The color of the water and its light: the face of a wave toward the camera, the low angle far away, the
  slopes toward and away from the sun, glints, the spot of the sun, caustics and reflections of the clouds.
  - `jo_water_config_look()` sets one of 11 standard looks, at no memory cost (inline values).
  - `jo_water_set_look()` changes the look of a water already made.
- **Seen through.** `jo_water_blend_with_bottom(jo_water_look_blend(look))` makes the VDP2 mix the RGB pixels of the
  sprites (the water) with the layer behind them (a bottom of the sea on a rotating plane).
  - Opaque objects must be in palette colors, with the MSB of their colors cleared.
- **The surface for the game.** These give the same waves as the drawn surface:
  - `jo_water_height()` and `jo_water_sample()`: the height, plus the slopes for `sample` ;
  - `jo_water_wave_height()`: the waves only, faster ;
  - `jo_water_drawn_height()`: interpolated from the drawn vertices.
- **Events.**
  - `jo_water_drop()`: a ring of waves where something falls.
  - `jo_water_wave_front()`: a straight wave that goes away.
  - `jo_water_foam()`: a trail, a wake or a splash, in a grid of cells around the camera.
- **The shore.** `jo_water_set_depth_map()` gives a map of depths (0: the shore, 255: deep). With it:
  - the shallow water is lighter ;
  - the waves break into surf ;
  - the bottom is seen through the water near the shore (`config.bottom`, `config.see_through`).
- **The patterns.** The foam and bottom textures are made from patterns built into the engine (`jo_engine/water_patterns.h`,
  22 KB, linked only with the water).
  - `tools/make_water_patterns.py` regenerates them. With `--jol FILE`, it writes them into a level file instead (the
    game section `'foam'`), and `config.patterns_file = "FILE.JOL"` reads them from there.
  - `config.grain` reads `GRAIN.TGA` (TGA module).

## Floating objects

```c
jo_water_floating crate;
jo_water_floating_init(&crate, x, z, 60, 12, 9);       // 60 units above the sea, half its height 12, sinks 9
// each frame, after jo_water_update()
jo_water_float(sea, &crate);
// draw at (crate.x, -crate.y, crate.z), rotated by crate.tilt_x then crate.tilt_z
```

The object falls with gravity, then splashes into the water: a ring of waves as high as its speed. After that it floats:
a damped spring pulls it toward the surface under it, and it tilts with the slope of the waves. Draw it with a small
negative depth bias, so that the water around it does not hide its part under the surface.

## Objects under the water

`jo_water_add_model()` (one object per group of the model) and `jo_water_add_mesh()` put rocks, wrecks or pillars under
the water. These objects are:

- **seen through the water.** Their colors become their colors seen through the water. Palette colors, 16-color textures
  (a 32-byte table each) and RGB textures (copied) all work.
- **sorted with the water.** Each object is sorted where its line of sight goes into the water, and drawn scaled around
  the camera so that its polygons keep their order. An object stays behind what covers it above the water:
  `jo_water_add_above()`, called each frame for a jet ski or a buoy.

`jo_water_place()`, `jo_water_show()`, `jo_water_reshape()` and `jo_water_set_far()` move, hide, update or limit them.

`jo_water_add_dip()` and `jo_water_dip_mesh()` handle a floating hull: the part of the hull under the surface takes the
colors seen through the water, more of them deeper.

The code of the objects is in `jo_engine/water_objects.c`. A game that adds no object links none of it.

## The sun and its lens flare (`jo/sun.h`)

```c
jo_sun sun;
jo_sun_init(&sun, JO_DEG_TO_ANGLE(200), JO_DEG_TO_ANGLE(13));
sun.halo_strips = 4;   sun.halo[k] = ...;   sun.core = ...;   sun.flare[k] = ...;

// each frame
jo_sun_update(&sun, heading, look_down, is_sun_hidden);   // look_down: tangent of the angle (jo_fixed)
jo_sun_draw(&sun);                                        // in the frame of the 3D: behind everything
jo_sun_draw_flare(&sun);                                  // after the rest, before the HUD
```

- The sun is drawn where its direction is seen from the camera:
  - its halo in strips, its core over the middle ;
  - sort key: `z + bias`, which puts it behind every polygon.
- The lens flare is a set of pieces on the line from the sun through the middle of the screen. By default there are 7
  pieces, made from 3 sprites: a ring, a disc and a hexagon (`jo_sun.flares`).
  - It fades in, one step per frame, when the sun comes near the middle of the screen, and fades out when it leaves.
  - It does not show when `is_sun_hidden()` (called only when needed) says something is between the camera and the sun.
  - Under the horizon, the pieces can be drawn half transparent by the VDP1 from RGB copies of the same pixels
    (`jo_sun.flare_rgb[]`).
- `Compiler/COMMON/jo_level/sun.py` makes the sprites in 16 colors (the TX16 section of a level file).
  - `halo(sky, height, opacity)` and `core()`: each pixel gets the most see-through blend ratio of the VDP2
    (`jo_vdp2_set_sprite_blend_ratios()`) that can show its light, and a color that gives that light over the sky behind
    it. `sky(h)` is the clear sky h pixels above the horizon.
  - `flare(kind, color)`: the pieces, dithered between the blend ratios.
  - `pack16(sprites)`: one palette and the textures, ready for `Level.add_textures16()`.

