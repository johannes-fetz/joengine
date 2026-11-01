# Jo Engine 3D model file (JOM) {#model_format}

A model (`.JOM`) holds a mesh made with a 3D software: points, polygons, materials, textures, vertex colors, groups, and optionally a rigid skeleton with its animations. The engine loads it with `jo_3d_load_model()` (`JO_COMPILE_WITH_3D_SUPPORT`, see `jo_engine/jo/model.h`); the converter `Compiler/COMMON/jo_model.py` writes it from a Wavefront OBJ file (and its MTL materials). `jo_model.py --info FILE.JOM` prints the content of a file.

The data of a JOM file are in the memory format of the Saturn: the points and the polygons are copied as they are, nothing is converted when the file is loaded (`jo_3d_create_model_in_place()` even uses the content of the file in place).

## Rules of the format

1. The file is big endian (the SH-2 reads it directly). The chunks are aligned on 4 bytes: the size of the data of a chunk is a multiple of 4 (the writer pads with zeros).
2. The file starts with a header (24 bytes), then chunks until the `END ` chunk.
3. A chunk is an identifier of 4 characters, the size of its data (u32) and the data. The loader skips the chunks it doesn't know (a file written by a newer converter is read by an older engine).
4. The counts are `u32`, the indexes are `u16` (at most 65535 points, polygons, normals or materials).
5. The structures of the chunks `MATL`, `GRUP`, `BONE` and `ANIM` have the same layout as the C structures `jo_3d_material`, `jo_3d_model_group`, `jo_3d_bone` and `jo_3d_animation` of `model.h`.
6. A new kind of data is a new chunk: the layout of a chunk never changes (the version in the header is for the header itself and the rules above).

## Types

| Type | Size | Description |
|---|---|---|
| u8, i8, u16, i16, u32, i32 | 1, 2, 4 | Integers (big endian) |
| fixed | 4 | Q16.16 fixed point number (jo_fixed) |
| char[16] | 16 | Name, ended by a zero (truncated to 15 characters) |
| angle | 2 | Angle (jo_angle): 0x10000 = 360 degrees, stored as i16 |
| rgb555 | 2 | Color of the VDP1: bit 15 set, 5 bits per channel (blue in the high bits) |

## Axes and units

The converter transforms the OBJ (Y up, right handed) to the axes of the Saturn (X right, Y down, Z forward): a rotation of 180 degrees around X (`x, -y, -z`) and the vertex order of the polygons is reversed (the front faces are clockwise on the screen). `--saturn-axes` skips the conversion, `--scale S` multiplies the coordinates. The points, the normals and the pivots are fixed point numbers in the units of the model.

## File header

| Offset | Field | Type | Description |
|---|---|---|---|
| 0 | `magic` | char[4] | `JOM1` |
| 4 | `version` | u16 | 1 (`JO_3D_MODEL_VERSION`) |
| 6 | `reserved` | u16 | 0 |
| 8 | `center` | fixed[3] | Center of the bounding sphere (x, y, z), see `jo_3d_is_sphere_visible()` |
| 20 | `radius` | fixed | Radius of the bounding sphere |

Size: 24 bytes. The chunks follow.

## Chunk header

| Offset | Field | Type | Description |
|---|---|---|---|
| 0 | `tag` | char[4] | Identifier of the chunk (see below) |
| 4 | `size` | u32 | Size of the data in bytes (multiple of 4) |
| 8 | data | | `size` bytes |

Most chunks start with a `u32` count, then `count` records. A chunk of records of 2 bytes (`QMAT`, `CNRM`, `GOUR`) is padded with 2 bytes when its count is odd.

## Chunks

| Tag | Present | Content |
|---|---|---|
| `TIMG` | with `--embed-textures` (first chunk) | Embedded textures |
| `PNTS` | always | Points |
| `QUAD` | always | Polygons |
| `QMAT` | always | Material of each polygon |
| `MATL` | always | Materials |
| `TEXT` | when the model has textures | Names of the textures |
| `NTAB` | when all the faces of the OBJ have normals (`vn`) | Normals of the corners |
| `CNRM` | with `NTAB` | Normal of each corner of each polygon |
| `VCOL` | when the OBJ has vertex colors (`v x y z r g b`) | Vertex colors |
| `GOUR` | with `VCOL`, when a material has `jo_gouraud` | Gouraud colors of the corners |
| `GRUP` | when the OBJ has `o` or `g` | Groups |
| `BONE` | with `--skeleton` | Bones of the rigid skeleton |
| `ANIM` | with `--skeleton` | Animations of the skeleton |
| `END ` | always (last chunk, size 0) | End of the file |

### TIMG: embedded textures

`u32 count`, then `count` textures. The textures are in the order of the `TEXT` chunk: the texture `i` of the model is the entry `i`. `jo_3d_load_model()` reads them one at a time and copies them to the VRAM (a texture already loaded by another model, with the same name, is skipped); they don't stay in the work RAM. Without this chunk, the textures are files on the CD named by `TEXT` (`.BIN`, or `.TGA`).

| Offset | Field | Type | Description |
|---|---|---|---|
| 0 | `name` | char[16] | Name of the texture: the start of the file name, a hash of the pixels and `.BIN` (`WALL_1A2B3C.BIN`), the same in `TEXT`. The hash makes the name unique for each texture of all the models: a texture shared by several models is loaded once |
| 16 | `width` | u16 | Width in pixels (multiple of 8) |
| 18 | `height` | u16 | Height in pixels |
| 20 | `format` | u16 | 0: RGB 555, 1: 16 colors with a lookup table (`jo_lut 1` in the MTL) |
| 22 | `reserved` | u16 | 0 |
| 24 | pixels | | See below, padded to 4 bytes |

- Format 0: `width × height` rgb555 pixels, left to right then top to bottom; 0 is transparent.
- Format 1: 16 rgb555 colors (the color 0 is transparent), then 2 pixels per byte (the left pixel in the high 4 bits): `width × height / 2` bytes. See `jo_sprite_add_4bits_lut()`: 4 times less VRAM than RGB, the same rendering (gouraud, half transparency).

### PNTS: points

`u32 count`, then `count × (fixed x, fixed y, fixed z)` (12 bytes per point), in the axes of the Saturn (Y down). The engine transforms each point once per frame: by default the converter merges the points at the same position (`--no-weld` keeps them), only between points used by the same groups.

### QUAD: polygons

`u32 count`, then `count` polygons of 20 bytes (`jo_3d_polygon`):

| Offset | Field | Type | Description |
|---|---|---|---|
| 0 | `normal` | fixed[3] | Normal of the polygon (x, y, z), unit length |
| 12 | `v0`, `v1`, `v2`, `v3` | u16[4] | Indexes of the 4 corners A, B, C, D in `PNTS`. A triangle: `v3 = v2` |

The VDP1 maps the whole texture on a quad (no texture coordinates): the corner with the top left of the texture becomes A, the texture is drawn from the edge A-B to the edge D-C. A triangle is a quad (A, B, C, C) with its shortest edge in A-B (`--keep-triangles` keeps the order of the OBJ). A face of 5 points or more is cut in quads by the converter.

### QMAT: material of each polygon

`u32 count` (the number of polygons), then `count × u16`: the index of the material of each polygon in `MATL` (+ 2 bytes of padding if `count` is odd).

### MATL: materials

`u32 count`, then `count` materials of 32 bytes (`jo_3d_material`):

| Offset | Field | Type | Description |
|---|---|---|---|
| 0 | `name` | char[16] | MTL `newmtl` |
| 16 | `r`, `g`, `b` | u8[3] | Color (MTL `Kd`): the polygons without texture |
| 19 | `shine` | u8 | MTL `Ns` (0 to 255), for the program |
| 20 | `texture` | i16 | Index of the texture in `TEXT`, -1: no texture |
| 22 | `subdivision_error` | u16 | MTL `jo_subdivision_error`: distortion in pixels over which the polygons are drawn in parts (`jo_3d_set_texture_subdivision_error()`), 0: the default |
| 24 | `flags` | u32 | `JO_3D_MATERIAL_xxx`, see below |
| 28 | `user` | u32 | MTL `jo_value`, for the program |

Flags (`jo_3d_material.flags`): the bits 0 to 15 belong to the engine, the bits 16 to 31 to the program.

| Bit | Flag | MTL option | Description |
|---|---|---|---|
| 0 | `JO_3D_MATERIAL_DUAL_PLANE` | `jo_dual 1` | Drawn on both sides |
| 1 | `JO_3D_MATERIAL_SORT_MIN` | `jo_sort min` | Painter's sort with the nearest point |
| 2 | `JO_3D_MATERIAL_SORT_MAX` | `jo_sort max` | Painter's sort with the farthest point |
| 1+2 | `JO_3D_MATERIAL_SORT_PREVIOUS` | `jo_sort previous` | Sorted with the depth of the polygon before it in the model (a decal drawn over the face before it). Neither bit: the center of the polygon (`jo_sort center`, the default) |
| 3 | `JO_3D_MATERIAL_GOURAUD` | `jo_gouraud 1` | Gouraud shading with the vertex colors (`GOUR` chunk) |
| 4 | `JO_3D_MATERIAL_HALF_TRANSPARENT` | `jo_half_transparent 1` (or `d` < 1) | Half transparent |
| 5 | `JO_3D_MATERIAL_SCREEN_DOORS` | `jo_screen_doors 1` | Screen doors (mesh) |
| 6 | `JO_3D_MATERIAL_LIGHT` | `jo_light 1` | Lit by the light of the engine (`jo_3d_light()`) |
| 7 | `JO_3D_MATERIAL_FLIP_H` | `jo_flip h` | Texture flipped horizontally |
| 8 | `JO_3D_MATERIAL_FLIP_V` | `jo_flip v` | Texture flipped vertically (`jo_flip hv`: both) |
| 9 | `JO_3D_MATERIAL_END_CODES` | `jo_end_codes 1` | The texture stops each line after its last visible pixel (`jo_sprite_add_end_codes()`) |
| 10 | `JO_3D_MATERIAL_MIPMAPS` | `jo_mipmaps 1` | Smaller copies of the texture far away (`jo_3d_set_texture_mipmaps()`) |
| 11 | `JO_3D_MATERIAL_SUBDIVISION` | `jo_subdivision 1` | The polygons near the camera are drawn in parts (`jo_3d_set_texture_subdivision()`) |
| 12 | `JO_3D_MATERIAL_SUBDIVISION_NEAR` | `jo_subdivision 2` | Only the polygons that cross the near plane are drawn in parts (`jo_3d_set_texture_subdivision_near_only()`) |
| 16 to 31 | `JO_3D_MATERIAL_USER(N)` | `jo_user N [N...]` | Flags of the program (N from 0 to 15) |

`jo_lut 1` has no flag: it changes the format of the embedded texture (`TIMG`).

### TEXT: textures

`u32 count`, then `count × char[16]`: the name of each texture (MTL `map_Kd`), a file name of the CD (8.3, uppercase, with the extension: `WALL.BIN`). With `TIMG` the pixels are in the file (the names are those of `TIMG`); otherwise the loader reads the file from the CD: a `.BIN` jo image (written by the converter from the PNG in `--texture-dir`: width multiple of 8, RGB 555, alpha becomes transparent) or a `.TGA`.

### NTAB: normals

`u32 count`, then `count × (fixed x, fixed y, fixed z)`: the normals of the OBJ (`vn`), for the lighting of the corners. Written only when every face of the OBJ has normals.

### CNRM: normal of each corner

`u32 count` (4 × the number of polygons), then `count × u16` (+ 2 bytes of padding if `count` is odd): the index in `NTAB` of the normal of the corners A, B, C, D of each polygon, in the order of `QUAD`.

### VCOL: vertex colors

`u32 count` (the number of points), then `count × (fixed r, fixed g, fixed b)`: the color of each point of `PNTS` (OBJ `v x y z r g b`), 1.0 = 0x10000. When points are merged, the color of the first point is kept.

### GOUR: gouraud colors

`u32 count` (4 × the number of polygons), then `count × rgb555` (+ 2 bytes of padding if `count` is odd): the gouraud color of the corners A, B, C, D of each polygon, in the order of `QUAD`. A channel of 16 leaves the texture unchanged (the VDP1 adds `channel - 16` to each pixel), 0 for the polygons of a material without `JO_3D_MATERIAL_GOURAUD`. `jo_3d_load_model()` gives one gouraud table to the polygons with the same colors.

### GRUP: groups

`u32 count`, then `count` groups of 24 bytes (`jo_3d_model_group`). Each OBJ `o` or `g` is a group, for the animations of the program (and the bones): its polygons are together in the OBJ, and its points are the points used only by its polygons (a point shared with another group doesn't move with it).

| Offset | Field | Type | Description |
|---|---|---|---|
| 0 | `name` | char[16] | OBJ `o` or `g` |
| 16 | `first_point` | u16 | First point used only by the group |
| 18 | `point_count` | u16 | Number of points used only by the group, from `first_point` |
| 20 | `first_polygon` | u16 | First polygon of the group |
| 22 | `polygon_count` | u16 | Number of polygons of the group |

### BONE: rigid skeleton

`u32 count` (at most `JO_3D_MAX_BONES` = 32), then `count` bones of 32 bytes (`jo_3d_bone`). A parent is always before its children. Written from the JSON file of `--skeleton` (see the usage of `jo_model.py`).

| Offset | Field | Type | Description |
|---|---|---|---|
| 0 | `name` | char[16] | Name of the bone |
| 16 | `group` | u16 | Index in `GRUP` of the group moved by the bone, `JO_3D_BONE_NO_GROUP` (0xFFFF): no polygons |
| 18 | `parent` | i16 | Index of the parent bone, -1: root |
| 20 | `pivot` | fixed[3] | Joint (model space): the bone turns around it |

### ANIM: animations

`u32 count`, then `count` animations of 28 bytes (`jo_3d_animation`), then the keys of all the animations.

| Offset | Field | Type | Description |
|---|---|---|---|
| 0 | `name` | char[16] | Name of the animation (`jo_3d_model_is_animation()`) |
| 16 | `key_count` | u16 | Number of keys (at least 1) |
| 18 | `key_ticks` | u16 | Time between 2 keys in ticks (`JO_3D_ANIMATION_TICKS_PER_SECOND` = 60 per second) |
| 20 | `flags` | u16 | `JO_3D_ANIMATION_LOOP` (bit 0): the last key goes back to the first one |
| 22 | `duplicate` | u16 | 0 in the file (set by the loader: 1 when an animation before has the same name) |
| 24 | `offset` | u32 | Offset of the keys from the start of the data of the chunk (the `count`) |

A key (the keys of an animation follow each other):

| Offset | Field | Type | Description |
|---|---|---|---|
| 0 | `root` | fixed[3] | Offset of the whole model (a step, a fall) |
| 12 | `angles` | angle[bones][3] | For each bone of `BONE`, the angles around X, Y then Z of its parent |
| | pad | i16 | 2 bytes when the number of bones is odd |

The engine interpolates between the keys (`jo_3d_model_get_pose()`) and draws the pose (`jo_3d_model_draw_pose()`).

## Layout of a file

```
Header (24 bytes)
"TIMG" chunk (optional)
"PNTS" chunk
"QUAD" chunk
"QMAT" chunk
"MATL" chunk
"TEXT" chunk (optional)
"NTAB" chunk, "CNRM" chunk (optional)
"VCOL" chunk, "GOUR" chunk (optional)
"GRUP" chunk (optional)
"BONE" chunk, "ANIM" chunk (optional)
"END " chunk (size 0)
```
