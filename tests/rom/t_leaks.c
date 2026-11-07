/*
** Jo Sega Saturn Engine - regression test ROM: leaks. Each resource is created, used and freed during N cycles, after a
** first cycle (the caches of the engine filled once: the textures of the models, the pool of the gouraud tables, the
** buffers kept by a module): the free memory must be exactly the same after the N cycles as after the first one.
**
** The counters: the free bytes of jo_malloc() (all the zones, block headers included, measured by allocating the
** largest blocks until the memory is full), the free bytes of the VDP1 sprite area and the last sprite id, the top of the
** VDP2 banks (a block allocated then freed in each bank), the palettes of the color RAM, the free file handles.
** A difference is a failure of the group, with the leak per cycle in a message (jt_info()).
*/

#include "jt.h"
#include "tests.h"

/** @brief Cycles after the first one (a leak of a few bytes per cycle: N times bigger) */
#define T_CYCLES                (8)

/** @brief Blocks of the measure of the free memory */
#define T_MAX_CHUNKS            (768)

/* (internal functions and values of the engine, not in its headers) */
unsigned int                    jo_sprite_free_memory(void);
extern int                      __jo_current_palette_id;
int                             __jo_fs_get_file_id(const char * const filename);
void                            *__jo_fs_open(const int file_id);
void                            __jo_fs_close(void *handle);

typedef struct
{
    int                         heap;       // Free bytes of jo_malloc()
    int                         vdp1;       // Free bytes of the sprite area of the VRAM
    int                         sprite;     // Last sprite id
    int                         b1;         // Top of the VDP2 banks (JO_VDP2_RAM_USER: B1, MAP_NBG1: B0, LINE_SCROLL: A0)
    int                         b0;
    int                         a0;
    int                         palette;    // Last palette of the color RAM
    int                         files;      // Free file handles
    int                         pool;       // Address of a table added to the pool of the gouraud tables (see t_pool_probe())
}                               t_snap;

static void                     *t_chunks[T_MAX_CHUNKS];
static bool                     t_chunks_overflow;

/** @brief The size of the block of a pointer of jo_malloc() (its header: short zone, unsigned int size) */
static  __jo_force_inline int   t_block_size(const void * const p)
{
    return ((int)((const unsigned int *)p)[-1]);
}

/** @brief The free bytes of jo_malloc(): the largest blocks allocated until the memory is full (blocks included), then freed */
static int                      t_heap_free(void)
{
    unsigned int                lo;
    unsigned int                hi;
    unsigned int                mid;
    void                        *p;
    int                         n;
    int                         total;

    n = 0;
    total = 0;
    // (the blocks are found from the largest one: the next one is not bigger)
    hi = 1024 * 1024;
    while (n < T_MAX_CHUNKS)
    {
        if ((p = jo_malloc(4)) == JO_NULL)
            break;
        jo_free(p);
        // The largest block (lo: allocated, hi: refused)
        lo = 4;
        hi += 4;
        while (hi - lo > 4)
        {
            mid = ((lo + hi) / 2) & ~3u;
            if (mid <= lo)
                mid = lo + 4;
            if ((p = jo_malloc(mid)) != JO_NULL)
            {
                jo_free(p);
                lo = mid;
            }
            else
                hi = mid;
        }
        if ((p = jo_malloc(lo)) == JO_NULL)
            break;
        hi = lo;
        t_chunks[n++] = p;
        total += t_block_size(p);
    }
    t_chunks_overflow = n >= T_MAX_CHUNKS;
    while (n > 0)
        jo_free(t_chunks[--n]);
    return (total);
}

/** @brief A block of a VDP2 bank allocated then freed: the top of the bank (or the first free block) */
static int                      t_vdp2_probe(const jo_vdp2_ram_usage usage)
{
    void                        *p;

    // (a full bank: 0, not an error)
    jt_expect_errors(true);
    p = jo_vdp2_malloc(usage, 32);
    jt_expect_errors(false);
    if (p == JO_NULL)
        return (0);
    jo_vdp2_free(p);
    return ((int)p);
}

/** @brief The free file handles */
static int                      t_free_files(void)
{
    void                        *handles[16];
    int                         id;
    int                         n;

    id = __jo_fs_get_file_id("TEST.TXT");
    if (id < 0)
        return (-1);
    for (n = 0; n < 16 && (handles[n] = __jo_fs_open(id)) != JO_NULL; ++n)
        ;
    for (id = 0; id < n; ++id)
        __jo_fs_close(handles[id]);
    return (n);
}

/** @brief The pool of the small sets of gouraud tables (jo_3d_add_gouraud_tables(), in a shared sprite): no counter, the
 *  address of one more table (a line: 2 units of 8 bytes). Its line is used: the next probe is 2 units after it when
 *  nothing else used the pool */
static int                      t_pool_probe(void)
{
#ifdef JO_COMPILE_WITH_3D_SUPPORT
    static jo_color             table[8];

    return ((int)jo_3d_add_gouraud_tables(table, 2));
#else
    return (0);
#endif
}

static void                     t_snap_take(t_snap * const s)
{
    // (first: a new sprite of the pool is counted by this snapshot)
    s->pool = t_pool_probe();
    s->heap = t_heap_free();
    s->vdp1 = (int)jo_sprite_free_memory();
    s->sprite = jo_get_last_sprite_id();
    s->b1 = t_vdp2_probe(JO_VDP2_RAM_USER);
    s->b0 = t_vdp2_probe(JO_VDP2_RAM_MAP_NBG1);
    s->a0 = t_vdp2_probe(JO_VDP2_RAM_LINE_SCROLL);
    s->palette = __jo_current_palette_id;
    s->files = t_free_files();
}

/** @brief The counters after the cycles are the same as before them (the message: the leak per cycle) */
static void                     t_leak_check(const char * const name, const t_snap * const a, const t_snap * const b, const int cycles)
{
    char                        text[96];
    int                         pool;

    jt_group(name);
    // (the lines of the pool used by the cycles, when they added no sprite: the probe of a used one)
    pool = b->sprite == a->sprite ? (b->pool - a->pool) / 2 - 1 : -1;
    if (a->heap != b->heap || a->vdp1 != b->vdp1 || a->sprite != b->sprite || pool != 0)
    {
        // (63 characters at most: the values per cycle x 100, then the name)
        jo_snprintf(text, sizeof(text), "h%d v%d s%d p%d/c100 %s", (a->heap - b->heap) * 100 / cycles,
                    (a->vdp1 - b->vdp1) * 100 / cycles, (b->sprite - a->sprite) * 100 / cycles, pool * 100 / cycles, name);
        jt_info(text);
    }
    JT_TRUE(!t_chunks_overflow);
    JT_EQ(b->heap, a->heap);
    JT_EQ(b->vdp1, a->vdp1);
    JT_EQ(b->sprite, a->sprite);
    JT_EQ(b->b1, a->b1);
    JT_EQ(b->b0, a->b0);
    JT_EQ(b->a0, a->a0);
    JT_EQ(b->palette, a->palette);
    JT_EQ(b->files, a->files);
    JT_EQ(b->pool, a->pool + 2);
}

/** @brief Like t_cycles(): the work RAM and the files only (the sprites are kept by design) */
static void                     t_cycles_heap(const char * const name, void (*cycle)(void))
{
    t_snap                      a;
    t_snap                      b;
    int                         k;

    jt_group(name);
    cycle();
    t_snap_take(&a);
    for (k = 0; k < T_CYCLES; ++k)
        cycle();
    t_snap_take(&b);
    JT_TRUE(!t_chunks_overflow);
    JT_EQ(b.heap, a.heap);
    JT_EQ(b.files, a.files);
}

/** @brief A cycle once (the caches), then T_CYCLES cycles, checked */
static void                     t_cycles(const char * const name, void (*cycle)(void))
{
    t_snap                      a;
    t_snap                      b;
    int                         k;

    jt_group(name);
    cycle();
    t_snap_take(&a);
    for (k = 0; k < T_CYCLES; ++k)
        cycle();
    t_snap_take(&b);
    t_leak_check(name, &a, &b, T_CYCLES);
}

/*
** Files
*/

static char                     *t_file;
static int                      t_file_len;

/** @brief A file of the CD in a block (JO_NULL: not found, a failure) */
static char                     *t_read(const char * const name, int * const len)
{
    char                        *data;

    data = jo_fs_read_file(name, len);
    JT_TRUE(data != JO_NULL);
    return (data);
}

static void                     c_fs_read(void)
{
    char                        *d;
    int                         len;

    if ((d = jo_fs_read_file("TEST.TXT", &len)) != JO_NULL)
        jo_free(d);
    if ((d = jo_fs_read_file_in_dir("PATTERN.BIN", "DATA", &len)) != JO_NULL)
        jo_free(d);
}

static void                     c_fs_open(void)
{
    jo_file                     file;
    char                        buf[16];

    if (jo_fs_open(&file, "TEST.TXT"))
    {
        jo_fs_read_next_bytes(&file, buf, sizeof(buf));
        jo_fs_close(&file);
    }
    // (a file not found: nothing kept)
    jt_expect_errors(true);
    if (jo_fs_open(&file, "NOPE.TXT"))
        jo_fs_close(&file);
    jt_expect_errors(false);
}

static void                     c_fs_read_errors(void)
{
    char                        *d;
    int                         len;

    jt_expect_errors(true);
    d = jo_fs_read_file_in_dir("TEST.TXT", "NOPE", &len);
    if (d != JO_NULL)
        jo_free(d);
    d = jo_fs_read_file("NOPE.TXT", &len);
    if (d != JO_NULL)
        jo_free(d);
    jt_expect_errors(false);
}

/*
** Sprites and images
*/

static void                     c_sprite_tga(void)
{
    int                         first;

    first = jo_sprite_add_tga(JO_ROOT_DIR, "T16.TGA", JO_COLOR_Transparent);
    JT_TRUE(first >= 0);
    jo_sprite_add_bin(JO_ROOT_DIR, "FIRE1.BIN", JO_COLOR_Transparent);
    jo_sprite_free_from(first);
}

static void                     c_sprite_tileset(void)
{
    static const jo_tile        tiles[2] = {{0, 0, 8, 8}, {8, 0, 8, 8}};
    int                         first;

    first = jo_sprite_add_tga_tileset(JO_ROOT_DIR, "T16.TGA", JO_COLOR_Transparent, tiles, 2);
    JT_TRUE(first >= 0);
    jo_sprite_free_from(first);
}

static jo_palette               t_p8_palette;

static jo_palette               *t_p8_palette_handler(void)
{
    return (&t_p8_palette);
}

static void                     c_sprite_tga_8bits(void)
{
    int                         first;

    // (a palette created once, see jo_set_tga_palette_handling(): the color RAM has no free)
    jo_set_tga_palette_handling(t_p8_palette_handler);
    first = jo_sprite_add_tga(JO_ROOT_DIR, "P8.TGA", JO_COLOR_Transparent);
    JT_TRUE(first >= 0);
    jo_set_tga_palette_handling(JO_NULL);
    jo_sprite_free_from(first);
}

static void                     c_sprite_anim(void)
{
    int                         a;
    int                         b;

    a = jo_create_sprite_anim(0, 2, 4);
    b = jo_create_sprite_anim(0, 2, 4);
    JT_TRUE(a >= 0 && b >= 0);
    jo_remove_sprite_anim(a);
    jo_remove_sprite_anim(b);
}

/** @brief A font: jo_font_free() frees the font, not its sprites (no API) */
static void                     c_font(void)
{
    jo_font                     *font;

    font = jo_font_load(JO_ROOT_DIR, "T16.TGA", JO_COLOR_Transparent, 8, 8, 0, "AB");
    JT_TRUE(font != JO_NULL);
    if (font != JO_NULL)
        jo_font_free(font);
}

static void                     c_font_and_sprites(void)
{
    jo_font                     *font;
    int                         base;

    base = jo_get_last_sprite_id();
    font = jo_font_load(JO_ROOT_DIR, "T16.TGA", JO_COLOR_Transparent, 8, 8, 0, "AB");
    if (font != JO_NULL)
        jo_font_free(font);
    jo_sprite_free_from(base + 1);
}

static void                     c_map(void)
{
    JT_TRUE(jo_map_create(0, 32, 500));
    jo_map_add_tile(0, 0, 0, 0, 0);
    // (created again: the tiles before are freed)
    JT_TRUE(jo_map_create(0, 64, 500));
    jo_map_free(0);
}

static void                     c_list(void)
{
    jo_list                     list;
    int                         k;

    jo_list_init(&list);
    for (k = 0; k < 20; ++k)
        jo_list_add_ptr(&list, (void *)(k + 1));
    jo_list_remove_at(&list, 3);
    jo_list_remove_first(&list);
    jo_list_clear(&list);
    jo_list_init(&list);
    for (k = 0; k < 4; ++k)
        jo_list_add_ptr(&list, jo_malloc(24));
    jo_list_free_and_clear(&list);
}

static void                     c_vdp2_image(void)
{
    static unsigned char        cells[16 * 16];
    jo_img_8bits                img;

    img.width = 16;
    img.height = 16;
    img.data = cells;
    jo_vdp2_set_nbg2_8bits_image(&img, t_p8_palette.id, false, false);
}

/*
** Audio
*/

#ifdef JO_COMPILE_WITH_AUDIO_SUPPORT
static void                     c_audio(void)
{
    jo_sound                    sound;

    jo_memset(&sound, 0, sizeof(sound));
    JT_TRUE(jo_audio_load_pcm("TPCM.PCM", JoSoundMono8Bit, &sound));
    jo_audio_free_pcm(&sound);
    jo_memset(&sound, 0, sizeof(sound));
    JT_TRUE(jo_audio_load_wav("STRM.WAV", &sound));
    jo_audio_free_pcm(&sound);
    // (a wrong file: nothing kept)
    jo_memset(&sound, 0, sizeof(sound));
    jt_expect_errors(true);
    jo_audio_load_wav("HANG.WAV", &sound);
    jt_expect_errors(false);
    jo_audio_free_pcm(&sound);
}

static void                     c_audio_play(void)
{
    jo_sound                    sound;

    jo_memset(&sound, 0, sizeof(sound));
    if (jo_audio_load_pcm("TPCM.PCM", JoSoundMono8Bit, &sound))
    {
        sound.volume = 0;
        jo_audio_play_sound_on_channel(&sound, 2);
        jo_audio_stop_sound(&sound);
        jo_audio_free_pcm(&sound);
    }
}

static void                     c_audio_music(void)
{
    jt_expect_errors(true);
    jo_audio_play_music("STRM.WAV", false);
    jt_expect_errors(false);
    jo_audio_stop_music();
}
#endif

/*
** Modules
*/

#ifdef JO_COMPILE_WITH_STORYBOARD_SUPPORT
static jo_3d_object_attributes  t_story_object;

static void                     c_storyboard(void)
{
    jo_storyboard               *s;

    s = jo_storyboard_create(false, false);
    JT_TRUE(s != JO_NULL);
    if (s == JO_NULL)
        return;
    jo_storyboard_add_object(s, &t_story_object);
    jo_storyboard_create_rotation_animation(s, 2, 10);
    jo_storyboard_create_translation_animation(s, 45, 2, 10);
    jo_storyboard_destroy(s);
}
#endif

#ifdef JO_COMPILE_WITH_PSEUDO_MODE7_SUPPORT
static void                     c_mode7(void)
{
    jo_mode7                    params;
    jo_img                      out;

    jo_memset(&params, 0, sizeof(params));
    params.space_z = 50;
    params.horizon = 4;
    params.scale_x = 100;
    params.scale_y = 100;
    out.width = 64;
    out.height = 32;
    out.data = JO_NULL;
    JT_TRUE(jo_init_mode7_for_image_output(&params, 32, 32, &out));
    // (initialized again with another height: its cache is freed first)
    out.height = 16;
    JT_TRUE(jo_init_mode7_for_image_output(&params, 32, 32, &out));
    jo_free_mode7(&params);
    if (out.data != JO_NULL)
        jo_free(out.data);
}
#endif

/*
** 3D
*/

#ifdef JO_COMPILE_WITH_3D_SUPPORT

static void                     c_mesh(void)
{
    jo_3d_mesh                  *m;
    jo_3d_ray_plane             *planes;

    m = jo_3d_create_mesh(6);
    JT_TRUE(m != JO_NULL);
    if (m != JO_NULL)
        jo_3d_free_mesh(m);
    m = jo_3d_create_sphere(100, 8, 6);
    JT_TRUE(m != JO_NULL);
    if (m != JO_NULL)
    {
        planes = jo_3d_mesh_create_ray_planes(m);
        if (planes != JO_NULL)
            jo_free(planes);
        jo_3d_free_mesh(m);
    }
    m = jo_3d_create_indexed_mesh(8, 6);
    JT_TRUE(m != JO_NULL);
    if (m != JO_NULL)
        jo_3d_free_mesh(m);
}

/** @brief The model files of the tests: read once */
static char                     *t_cap;
static int                      t_cap_len;
static char                     *t_sci;
static int                      t_sci_len;
static char                     *t_duct;
static int                      t_duct_len;
static char                     *t_barrel;
static int                      t_barrel_len;

/** @brief A model created from memory, used (pose), freed */
static void                     t_use_model(jo_3d_model * const model)
{
    jo_3d_pose                  pose;

    if (model == JO_NULL)
        return;
    if (model->animation_count > 0)
    {
        jo_3d_model_get_pose(model, 0, 10, &pose);
        jo_3d_model_get_pose(model, (int)model->animation_count - 1, 1000, &pose);
    }
}

static void                     c_model_memory(void)
{
    jo_3d_model                 *m;

    JT_TRUE((m = jo_3d_create_model_from_memory((const unsigned char *)t_file, (unsigned int)t_file_len)) != JO_NULL);
    t_use_model(m);
    if (m != JO_NULL)
        jo_3d_free_model(m);
}

static void                     c_model_in_place(void)
{
    jo_3d_model                 *m;

    JT_TRUE((m = jo_3d_create_model_in_place((const unsigned char *)t_file, (unsigned int)t_file_len)) != JO_NULL);
    t_use_model(m);
    if (m != JO_NULL)
        jo_3d_free_model(m);
}

static const char               *t_model_name;

static void                     c_model_load(void)
{
    jo_3d_model                 *m;

    JT_TRUE((m = jo_3d_load_model(t_model_name)) != JO_NULL);
    t_use_model(m);
    if (m != JO_NULL)
        jo_3d_free_model(m);
}

/** @brief A model and all its sprites (textures, gouraud tables, mipmaps, tiles) freed: what a level does */
static void                     c_model_load_and_sprites(void)
{
    int                         base;

    base = jo_get_last_sprite_id();
    c_model_load();
    jo_3d_model_free_textures_from(base + 1);
}

/** @brief The wrong files: nothing kept */
static void                     c_model_errors(void)
{
    jo_3d_model                 *m;
    char                        *d;
    int                         len;
    static const char           *names[3] = {"BADV.JOM", "BADC.JOM", "HANG.JOM"};
    int                         k;

    for (k = 0; k < 3; ++k)
    {
        d = jo_fs_read_file(names[k], &len);
        if (d == JO_NULL)
            continue;
        jt_expect_errors(true);
        m = jo_3d_create_model_from_memory((const unsigned char *)d, (unsigned int)len);
        if (m != JO_NULL)
            jo_3d_free_model(m);
        m = jo_3d_create_model_in_place((const unsigned char *)d, (unsigned int)len);
        if (m != JO_NULL)
            jo_3d_free_model(m);
        m = jo_3d_load_model(names[k]);
        if (m != JO_NULL)
            jo_3d_free_model(m);
        jt_expect_errors(false);
        jo_free(d);
    }
}

/** @brief A model with gouraud tables tinted (jo_3d_prepare_tint()), then freed */
static void                     c_model_tint(void)
{
    jo_3d_model                 *m;

    m = jo_3d_create_model_from_memory((const unsigned char *)t_cap, (unsigned int)t_cap_len);
    JT_TRUE(m != JO_NULL);
    if (m == JO_NULL)
        return;
    jo_3d_set_tint(10, -6, 0);
    jo_3d_prepare_tint(m->mesh);
    jo_3d_set_tint(0, 0, 0);
    jo_3d_free_model(m);
}

/** @brief A mesh with gouraud tables (the same tables at each cycle), tinted then freed */
static jo_3d_mesh               *t_mesh_with_gouraud(const int polygons)
{
    jo_3d_mesh                  *m;
    static jo_color             tables[4 * 8];
    static unsigned short       base = 0;
    int                         k;

    m = jo_3d_create_mesh(polygons);
    if (m == JO_NULL)
        return (JO_NULL);
    // (the tables once: jo_3d_add_gouraud_tables() has no free, see c_gouraud_tables())
    if (base == 0)
    {
        for (k = 0; k < 4 * 8; ++k)
            tables[k] = JO_COLOR_RGB(k * 8, 128, 255 - k * 8);
        base = jo_3d_add_gouraud_tables(tables, 2);
    }
    for (k = 0; k < polygons; ++k)
    {
        m->data.attributes[k].draw_mode |= JO_VDP1_COLOR_CALC_GOURAUD;
        m->data.attributes[k].gouraud_table = (unsigned short)(base + (k & 1));
    }
    return (m);
}

static void                     c_mesh_tint(void)
{
    jo_3d_mesh                  *m;

    m = t_mesh_with_gouraud(2);
    if (m == JO_NULL)
        return;
    jo_3d_set_tint(8, 8, 8);
    jo_3d_prepare_tint(m);
    jo_3d_set_tint(0, 0, 0);
    jo_3d_free_mesh(m);
}

/** @brief The cache of the tints keyed by the address of the polygons of a mesh: a mesh freed, a new mesh at the same
 *  address gets the copies of the old one (another count of tables, other colors) */
static void                     t_tint_stale_key(void)
{
    jo_3d_mesh                  *a;
    jo_3d_mesh                  *b;
    const void                  *key;
    int                         free_vdp1;

    jt_group("leaks.3d.tint_stale_key");
    a = jo_3d_create_mesh(2);
    if (a == JO_NULL)
        return;
    {
        static jo_color         tables[8];
        unsigned short          base;

        jo_memset(tables, 0, sizeof(tables));
        base = jo_3d_add_gouraud_tables(tables, 2);
        a->data.attributes[0].draw_mode |= JO_VDP1_COLOR_CALC_GOURAUD;
        a->data.attributes[0].gouraud_table = base;
    }
    jo_3d_set_tint(6, 0, 0);
    jo_3d_prepare_tint(a);
    key = &a->data;
    jo_3d_free_mesh(a);
    b = jo_3d_create_mesh(2);
    if (b == JO_NULL)
        return;
    if (&b->data != key)
    {
        jt_info("leaks.3d.tint_stale_key: the new mesh is at another address, not checked");
        jo_3d_free_mesh(b);
        jo_3d_set_tint(0, 0, 0);
        return;
    }
    {
        static jo_color         tables[4 * 4];
        unsigned short          base;
        int                     k;

        for (k = 0; k < 16; ++k)
            tables[k] = JO_COLOR_White;
        base = jo_3d_add_gouraud_tables(tables, 4);
        b->data.attributes[0].draw_mode |= JO_VDP1_COLOR_CALC_GOURAUD;
        b->data.attributes[0].gouraud_table = (unsigned short)(base + 3);
    }
    // A new mesh in the cache: its copies are new tables (a sprite). The stale entry: no new tables
    free_vdp1 = (int)jo_sprite_free_memory();
    jo_3d_prepare_tint(b);
    jo_3d_set_tint(0, 0, 0);
    JT_TRUE((int)jo_sprite_free_memory() != free_vdp1);
    jo_3d_free_mesh(b);
}

/** @brief An environment map set twice on a mesh, the mesh freed */
static void                     c_environment(void)
{
    static jo_color             pixels[16 * 16];
    jo_img                      img;
    jo_3d_mesh                  *m;
    int                         id;
    int                         k;

    for (k = 0; k < 16 * 16; ++k)
        pixels[k] = JO_COLOR_RGB(k, 255 - k, 128);
    img.width = 16;
    img.height = 16;
    img.data = pixels;
    id = jo_sprite_add(&img);
    m = jo_3d_create_sphere(100, 6, 4);
    if (m != JO_NULL && id >= 0)
    {
        JT_TRUE(jo_3d_set_mesh_environment(m, id, 2));
        JT_TRUE(jo_3d_set_mesh_environment(m, id, 2));
        jo_3d_set_mesh_polygon_environment(m, 0);
    }
    if (m != JO_NULL)
        jo_3d_free_mesh(m);
    jo_sprite_free_from(id);
}

/** @brief An environment map set once, the mesh freed, the texture kept (8 maps at most: the slots of the freed meshes) */
static void                     c_environment_slots(void)
{
    static int                  texture = -1;
    jo_3d_mesh                  *m;
    static jo_color             pixels[16 * 16];
    jo_img                      img;
    int                         base;

    if (texture < 0)
    {
        img.width = 16;
        img.height = 16;
        img.data = pixels;
        texture = jo_sprite_add(&img);
    }
    base = jo_get_last_sprite_id();
    m = jo_3d_create_sphere(100, 6, 4);
    if (m == JO_NULL)
        return;
    JT_TRUE(jo_3d_set_mesh_environment(m, texture, 2));
    jo_3d_free_mesh(m);
    // (the tiles freed: the slot is forgotten with them, see __jo_3d_forget_sprites_from())
    jo_sprite_free_from(base + 1);
}

/** @brief An environment map set twice on a mesh: the tiles of the first one stay in the VRAM; the meshes freed keep
 *  their slot (8 at most): the map of a new mesh is refused */
static void                     t_environment_stale(void)
{
    static jo_color             pixels[16 * 16];
    jo_img                      img;
    jo_3d_mesh                  *m;
    int                         texture;
    int                         last;
    int                         k;
    int                         refused;

    jt_group("leaks.3d.environment_twice");
    img.width = 16;
    img.height = 16;
    img.data = pixels;
    texture = jo_sprite_add(&img);
    m = jo_3d_create_sphere(100, 6, 4);
    if (m == JO_NULL || texture < 0)
        return;
    JT_TRUE(jo_3d_set_mesh_environment(m, texture, 2));
    last = jo_get_last_sprite_id();
    JT_TRUE(jo_3d_set_mesh_environment(m, texture, 2));
    // (the tiles of the first call replaced: no new sprite expected)
    JT_EQ(jo_get_last_sprite_id(), last);
    jo_3d_free_mesh(m);
    jt_group("leaks.3d.environment_freed_meshes");
    refused = 0;
    for (k = 0; k < 10; ++k)
    {
        m = jo_3d_create_sphere(100 + k, 6, 4);
        if (m == JO_NULL)
            break;
        if (!jo_3d_set_mesh_environment(m, texture, 2))
            ++refused;
        jo_3d_free_mesh(m);
    }
    JT_EQ(refused, 0);
    jo_sprite_free_from(texture);
}

/** @brief Mipmaps and tiles of RGB and 16 colors textures, set twice, the textures freed */
static void                     c_texture_options(void)
{
    static jo_color             pixels[32 * 32];
    static unsigned char        pixels4[32 * 32 / 2];
    static jo_color             lut[16];
    jo_img                      img;
    int                         id;
    int                         id4;
    int                         k;

    for (k = 0; k < 32 * 32; ++k)
        pixels[k] = JO_COLOR_RGB(k & 255, (k >> 2) & 255, 99);
    for (k = 0; k < 32 * 32 / 2; ++k)
        pixels4[k] = (unsigned char)(k * 7);
    for (k = 0; k < 16; ++k)
        lut[k] = JO_COLOR_RGB(k * 16, 0, 255 - k * 16);
    img.width = 32;
    img.height = 32;
    img.data = pixels;
    id = jo_sprite_add(&img);
    id4 = jo_sprite_add_4bits_lut(pixels4, 32, 32, lut);
    JT_TRUE(id >= 0 && id4 >= 0);
    JT_TRUE(jo_3d_set_texture_mipmaps(id));
    JT_TRUE(jo_3d_set_texture_subdivision(id));
    JT_TRUE(jo_3d_set_texture_mipmaps(id));
    JT_TRUE(jo_3d_set_texture_subdivision(id));
    JT_TRUE(jo_3d_set_texture_mipmaps(id4));
    JT_TRUE(jo_3d_set_texture_subdivision_near_only(id4));
    jo_3d_set_texture_subdivision(id4);
    jo_sprite_free_from(id);
}

/** @brief Gouraud tables added, then given back (jo_3d_free_gouraud_tables(), in the reverse order) */
static void                     c_gouraud_tables(void)
{
    static jo_color             tables[4 * 4];
    unsigned short              a;

    a = jo_3d_add_gouraud_tables(tables, 4);
    JT_TRUE(a != 0);
    jo_3d_free_gouraud_tables(a, 4);
}

static void                     c_gouraud_tables_and_sprites(void)
{
    static jo_color             tables[4 * 200];
    unsigned short              a;
    unsigned short              b;

    a = jo_3d_add_gouraud_tables(tables, 4);
    JT_TRUE(a != 0);
    // (a big set: its own sprite)
    b = jo_3d_add_gouraud_tables(tables, 200);
    JT_TRUE(b != 0);
    jo_3d_free_gouraud_tables(b, 200);
    jo_3d_free_gouraud_tables(a, 4);
}

static void                     c_sprite_quad(void)
{
    static jo_color             pixels[16 * 8];
    jo_img                      img;
    int                         id;

    img.width = 16;
    img.height = 8;
    img.data = pixels;
    id = jo_sprite_add(&img);
    JT_TRUE(jo_3d_create_sprite_quad(id));
    jo_sprite_free_from(id);
}

/** @brief The quad of a sprite created twice (the first one is replaced, not freed) */
static void                     c_sprite_quad_twice(void)
{
    static jo_color             pixels[16 * 8];
    jo_img                      img;
    int                         id;

    img.width = 16;
    img.height = 8;
    img.data = pixels;
    id = jo_sprite_add(&img);
    JT_TRUE(jo_3d_create_sprite_quad(id));
    JT_TRUE(jo_3d_create_sprite_quad(id));
    jo_sprite_free_from(id);
}

/** @brief Point lights on the mesh of a model, computed, cleared, the model freed */
static jo_3d_model              *t_light_model;

static void                     c_point_lights(void)
{
    jo_pos3D_fixed              p;
    int                         id;

    if (t_light_model == JO_NULL)
        return;
    JT_TRUE(jo_3d_point_lights_add_mesh(t_light_model->mesh));
    p.x = 0;
    p.y = jo_int2fixed(-118);
    p.z = jo_int2fixed(-20);
    id = jo_3d_point_light_add(&p, 255, 200, 100, 100, 256);
    JT_TRUE(id >= 0);
    jo_3d_point_light_add(&p, 0, 200, 255, 50, 128);
    jo_3d_point_lights_update();
    p.z = jo_int2fixed(20);
    jo_3d_point_light_set_position(id, &p);
    jo_3d_point_lights_update();
    jo_3d_point_lights_clear();
}

static void                     c_point_lights_model(void)
{
    t_light_model = jo_3d_create_model_from_memory((const unsigned char *)t_cap, (unsigned int)t_cap_len);
    c_point_lights();
    if (t_light_model != JO_NULL)
        jo_3d_free_model(t_light_model);
    t_light_model = JO_NULL;
}

#ifdef JO_COMPILE_WITH_LEVEL_SUPPORT
static void                     c_glows(void)
{
    jo_pos3D_fixed              p;

    p.x = 0;
    p.y = 0;
    p.z = jo_int2fixed(100);
    JT_TRUE(jo_3d_glow_add(&p, 255, 255, 0, 20, 256) >= 0);
    jo_3d_glows_clear();
}
#endif

#ifdef JO_COMPILE_WITH_EFFECTS_SUPPORT
static void                     c_effects(void)
{
    jo_effect_def               def;
    jo_pos3D_fixed              p;
    int                         k;

    jo_memset(&def, 0, sizeof(def));
    def.frame_count = 1;
    def.life = 4;
    def.size_start = 4;
    def.size_end = 8;
    p.x = p.y = p.z = 0;
    for (k = 0; k < 40; ++k)
        jo_effect_add(&def, &p, 0);
    jo_effects_update();
    jo_effects_clear();
}
#endif

#ifdef JO_COMPILE_WITH_LEVEL_SUPPORT

/** @brief A level: its cells (models for the lights, mirrors), its entities (an actor, an explosive), the point lights of
 *  the cells, everything freed */
static void                     c_level(void)
{
    jo_level                    *level;

    level = jo_level_load("TINY.JOL");
    JT_TRUE(level != JO_NULL);
    if (level == JO_NULL)
        return;
    JT_EQ(jo_cells_load(level), 2);
    JT_EQ(jo_entities_load(level), 3);
    jo_level_free(level);
    jo_cells_add_point_light_meshes(true);
    jo_entities_free();
    jo_cells_free();
}

/** @brief The level and the sprites loaded with it freed */
static void                     c_level_and_sprites(void)
{
    int                         base;

    base = jo_get_last_sprite_id();
    c_level();
    jo_3d_model_free_textures_from(base + 1);
}

static void                     c_level_errors(void)
{
    jo_level                    *level;

    jt_expect_errors(true);
    level = jo_level_load("NOPE.JOL");
    if (level != JO_NULL)
        jo_level_free(level);
    // (not a level file)
    level = jo_level_load("TEST.TXT");
    if (level != JO_NULL)
        jo_level_free(level);
    jt_expect_errors(false);
}
#endif

#endif /* JO_COMPILE_WITH_3D_SUPPORT */

/*
** The checks without drawing
*/

void                            t_leaks(void)
{
    t_snap                      a;
    t_snap                      b;

    jt_group("leaks.measure");
    // The measure itself: the same twice
    t_snap_take(&a);
    t_snap_take(&b);
    t_leak_check("leaks.measure", &a, &b, 1);
    t_cycles("leaks.fs.read", c_fs_read);
    t_cycles("leaks.fs.open", c_fs_open);
    t_cycles("leaks.fs.errors", c_fs_read_errors);
    t_cycles("leaks.sprite.tga_bin", c_sprite_tga);
    t_cycles("leaks.sprite.tileset", c_sprite_tileset);
    jo_create_palette(&t_p8_palette);
    t_cycles("leaks.sprite.tga_8bits", c_sprite_tga_8bits);
    t_cycles("leaks.sprite.anim", c_sprite_anim);
    t_cycles("leaks.font", c_font);
    t_cycles("leaks.font_and_sprites", c_font_and_sprites);
    t_cycles("leaks.map", c_map);
    t_cycles("leaks.list", c_list);
    t_cycles("leaks.vdp2.nbg2_image", c_vdp2_image);
#ifdef JO_COMPILE_WITH_AUDIO_SUPPORT
    t_cycles("leaks.audio.load", c_audio);
    t_cycles("leaks.audio.play", c_audio_play);
    t_cycles("leaks.audio.music", c_audio_music);
#endif
#ifdef JO_COMPILE_WITH_STORYBOARD_SUPPORT
    t_cycles("leaks.storyboard", c_storyboard);
#endif
#ifdef JO_COMPILE_WITH_PSEUDO_MODE7_SUPPORT
    t_cycles("leaks.mode7", c_mode7);
#endif
#ifdef JO_COMPILE_WITH_3D_SUPPORT
    t_cycles("leaks.3d.mesh", c_mesh);
    t_cap = t_read("CAP.JOM", &t_cap_len);
    t_sci = t_read("SCIENTF.JOM", &t_sci_len);
    t_duct = t_read("DUCTEND.JOM", &t_duct_len);
    t_barrel = t_read("BARRELD.JOM", &t_barrel_len);
    t_file = t_read("GOOD.JOM", &t_file_len);
    t_cycles("leaks.3d.model_memory.good", c_model_memory);
    t_cycles("leaks.3d.model_in_place.good", c_model_in_place);
    jo_free(t_file);
    t_file = t_cap;
    t_file_len = t_cap_len;
    t_cycles("leaks.3d.model_memory.gouraud", c_model_memory);
    t_cycles("leaks.3d.model_in_place.gouraud", c_model_in_place);
    t_file = t_sci;
    t_file_len = t_sci_len;
    t_cycles("leaks.3d.model_memory.skeleton", c_model_memory);
    t_cycles("leaks.3d.model_in_place.skeleton", c_model_in_place);
    t_file = t_duct;
    t_file_len = t_duct_len;
    t_cycles("leaks.3d.model_memory.subdivision", c_model_memory);
    t_file = t_barrel;
    t_file_len = t_barrel_len;
    t_cycles("leaks.3d.model_memory.mipmaps", c_model_memory);
    t_model_name = "SCIENTF.JOM";
    t_cycles("leaks.3d.model_load.skeleton", c_model_load);
    t_cycles("leaks.3d.model_load_and_sprites.skeleton", c_model_load_and_sprites);
    t_model_name = "CAP.JOM";
    t_cycles("leaks.3d.model_load.gouraud", c_model_load);
    t_cycles("leaks.3d.model_load_and_sprites.gouraud", c_model_load_and_sprites);
    t_cycles("leaks.3d.model_errors", c_model_errors);
    t_cycles("leaks.3d.model_tint", c_model_tint);
    t_cycles("leaks.3d.mesh_tint", c_mesh_tint);
    t_tint_stale_key();
    t_environment_stale();
    t_cycles("leaks.3d.environment", c_environment);
    t_cycles("leaks.3d.environment_slots", c_environment_slots);
    t_cycles("leaks.3d.texture_options", c_texture_options);
    t_cycles("leaks.3d.gouraud_tables", c_gouraud_tables);
    t_cycles("leaks.3d.gouraud_tables_and_sprites", c_gouraud_tables_and_sprites);
    t_cycles("leaks.3d.sprite_quad", c_sprite_quad);
    t_cycles("leaks.3d.sprite_quad_twice", c_sprite_quad_twice);
    t_light_model = jo_3d_create_model_from_memory((const unsigned char *)t_cap, (unsigned int)t_cap_len);
    t_cycles("leaks.3d.point_lights", c_point_lights);
    if (t_light_model != JO_NULL)
        jo_3d_free_model(t_light_model);
    t_light_model = JO_NULL;
    t_cycles("leaks.3d.point_lights_model", c_point_lights_model);
#ifdef JO_COMPILE_WITH_EFFECTS_SUPPORT
    t_cycles("leaks.3d.effects", c_effects);
#endif
#ifdef JO_COMPILE_WITH_LEVEL_SUPPORT
    t_cycles("leaks.3d.glows", c_glows);
    t_cycles("leaks.level.errors", c_level_errors);
    // (the textures of the models stay loaded by design, see jo_3d_free_model(): the work RAM only)
    t_cycles_heap("leaks.level", c_level);
    t_cycles("leaks.level_and_sprites", c_level_and_sprites);
#endif
#endif
}

/*
** The checks with drawing: a model created each frame, drawn in a pose and with a tint, freed the next frame (the commands
** of a frame are written at its end)
*/

#ifdef JO_COMPILE_WITH_3D_SUPPORT

static t_snap                   t_draw_before;
static jo_3d_model              *t_draw_model;
static int                      t_draw_frames;
static jo_camera                t_draw_camera;

#define T_DRAW_FRAMES           (12)

static void                     t_draw_one(void)
{
    jo_3d_pose                  pose;

    if (t_draw_model != JO_NULL)
        jo_3d_free_model(t_draw_model);
    t_draw_model = JO_NULL;
    // (the last frame: the model of the frame before freed, nothing drawn)
    if (t_draw_frames >= T_DRAW_FRAMES)
        return;
    t_draw_model = jo_3d_create_model_from_memory((const unsigned char *)t_sci, (unsigned int)t_sci_len);
    if (t_draw_model == JO_NULL)
        return;
    jo_3d_camera_set_viewpoint(&t_draw_camera, 0, -60, -220);
    jo_3d_camera_set_target(&t_draw_camera, 0, -60, 0);
    jo_3d_camera_look_at(&t_draw_camera);
    jo_3d_push_matrix();
    {
        jo_3d_model_get_pose(t_draw_model, 0, t_draw_frames * 5, &pose);
        jo_3d_set_tint(8 + (t_draw_frames & 7) * 2, 0, 0);
        jo_3d_model_draw_pose(t_draw_model, &pose, JO_NULL, JO_NULL);
        jo_3d_set_tint(0, 0, 0);
        jo_3d_translate_matrix(80, 0, 0);
        jo_3d_mesh_draw(t_draw_model->mesh);
    }
    jo_3d_pop_matrix();
    ++t_draw_frames;
}

static void                     s_leaks_draw_setup(void)
{
    jo_3d_camera_init(&t_draw_camera);
    t_draw_frames = 0;
    t_draw_model = JO_NULL;
    if (t_sci == JO_NULL)
        t_sci = jo_fs_read_file("SCIENTF.JOM", &t_sci_len);
    // (a model first, not drawn: the caches)
    t_draw_model = jo_3d_create_model_from_memory((const unsigned char *)t_sci, (unsigned int)t_sci_len);
    if (t_draw_model != JO_NULL)
    {
        jo_3d_set_tint(8, 0, 0);
        jo_3d_prepare_tint(t_draw_model->mesh);
        jo_3d_set_tint(0, 0, 0);
        jo_3d_free_model(t_draw_model);
    }
    t_draw_model = JO_NULL;
    t_snap_take(&t_draw_before);
}

static void                     s_leaks_draw_measure(void)
{
    t_snap                      after;

    if (t_draw_model != JO_NULL)
        jo_3d_free_model(t_draw_model);
    t_draw_model = JO_NULL;
    t_snap_take(&after);
    t_leak_check("leaks.3d.draw_pose_tint", &t_draw_before, &after, t_draw_frames > 0 ? t_draw_frames : 1);
}

const jt_step                   t_leak_steps[] =
{
    {JT_STEP_RUN, "leaks", t_leaks, JO_NULL, JO_NULL, JO_NULL, 0},
    {JT_STEP_BENCH, "leaks_draw", s_leaks_draw_setup, t_draw_one, s_leaks_draw_measure, JO_NULL, T_DRAW_FRAMES},
};
#else
const jt_step                   t_leak_steps[] =
{
    {JT_STEP_RUN, "leaks", t_leaks, JO_NULL, JO_NULL, JO_NULL, 0},
};
#endif

const int                       t_leak_step_count = JT_COUNT(t_leak_steps);
