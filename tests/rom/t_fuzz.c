/*
** Jo Sega Saturn Engine - regression test ROM: fuzz tests of the parsers of files and stress of the APIs (the same random
** sequences at each run)
**
** - JOM models in memory: jo_3d_create_model_from_memory() and jo_3d_create_model_in_place() on mutated copies of valid
**   files, each model created is used (poses, search, draw), then freed
** - TGA images in memory: jo_tga_loader_from_stream(), jo_sprite_add_tga_from_stream() on mutated headers and pixels
** - The files of cd/FUZZ (written by gen_fuzz.py): JOM (jo_3d_load_model()), TGA (jo_sprite_add_tga()), WAV
**   (jo_audio_load_wav()), MAP (jo_map_load_from_file()), JOL (jo_level_load(), jo_entities_load(): with the level module)
** - The APIs: random sequences of jo_malloc() / jo_vdp2_malloc() / jo_sprite_add() / lists and their frees, sizes and ids out
**   of range
**
** Each iteration of the fuzz in memory has its own random seed (T_FUZZ_SEED(i)): an iteration is reproduced alone with
** T_FUZZ_FIRST / T_FUZZ_COUNT. The number of the iteration that fails is the "actual" value of the failed check.
** The progress is written on the screen (line 27): the last frame of a ROM that stops shows the input that stopped it.
**
** The bugs of the engine found by the fuzz (all fixed, sixth review), checked first by their own inputs (the K* and L* files
** of cd/FUZZ): t_fixed_bugs() (a failed check), t_fixed_crashes() (they crashed or hung the ROM: T_FUZZ_KNOWN_BUGS 10 + n
** runs the input n alone). No input of the fuzz is skipped.
**
** Variants (config.json): the others run a short fuzz; "fuzz" and "fuzz_release" (T_FUZZ_LONG, JO_DEBUG 0 for the second
** one) run the long one: run_tests.py --full, or add them to quick_variants
*/

#include "jt.h"
#include "tests.h"

/** @brief The long fuzz (T_FUZZ_LONG: the variants "fuzz" and "fuzz_release" of config.json), else a short one */
#ifdef T_FUZZ_LONG
# define T_FUZZ_JOM_ITERATIONS      (4000)
# define T_FUZZ_TGA_ITERATIONS      (1500)
# define T_FUZZ_API_OPERATIONS      (4000)
#else
# define T_FUZZ_JOM_ITERATIONS      (400)
# define T_FUZZ_TGA_ITERATIONS      (200)
# define T_FUZZ_API_OPERATIONS      (1000)
#endif

/** @brief The inputs of the bugs that crashed or hung the ROM (t_fixed_crashes()): 0 all, then the fuzz; 10 + n only the input
 *  n (to see one alone stop the ROM) */
#ifndef T_FUZZ_KNOWN_BUGS
# define T_FUZZ_KNOWN_BUGS          (0)
#endif

/** @brief First iteration and count (to reproduce one iteration: its number, 1) */
#ifndef T_FUZZ_FIRST
# define T_FUZZ_FIRST               (0)
#endif
#ifndef T_FUZZ_COUNT
# define T_FUZZ_COUNT               (0x7FFFFFFF)
#endif

/** @brief The files of cd/FUZZ: Jnn.JOM, Tnn.TGA, Wnn.WAV, Mnn.MAP, Lnn.JOL (gen_fuzz.py FUZZ_COUNT) */
#define T_FUZZ_FILES                (30)
#define T_FUZZ_DIR                  "FUZZ"

/*
** Random numbers
*/

static unsigned int         t_rng;

/** @brief Seed of an iteration (the same at each run) */
#define T_FUZZ_SEED(KIND, I)        (((unsigned int)(I) + 1u) * 0x9E3779B9u ^ (unsigned int)(KIND) * 0x85EBCA6Bu)

static unsigned int         t_rand(void)
{
    // xorshift32
    t_rng ^= t_rng << 13;
    t_rng ^= t_rng >> 17;
    t_rng ^= t_rng << 5;
    return (t_rng);
}

static unsigned int         t_below(const unsigned int n)
{
    return (n == 0 ? 0 : t_rand() % n);
}

static const unsigned int   t_u32_values[] = {0, 1, 2, 3, 4, 0x7F, 0x80, 0xFF, 0x100, 0x7FFF, 0x8000, 0xFFFF, 0x10000, 0x7FFFFFFF,
                                              0x80000000, 0xFFFFFFFF, 0xFFFFFFF8, 0xFFFFFFFC, 0x40000000, 0x15555556, 0x0CCCCCCD,
                                              0x0AAAAAAB};
static const unsigned short t_u16_values[] = {0, 1, 2, 7, 8, 9, 0x7F, 0x80, 0xFF, 0x100, 0x1F8, 0x200, 0x7FFF, 0x8000, 0x8001,
                                              0xFFFE, 0xFFFF};

/*
** Checks after each input
*/

/** @brief State of the checks of a group of inputs: the first input of each problem (-1: none) */
typedef struct
{
    int                     null_access;    // A NULL pointer access (JO_DEBUG: the trap of the engine)
    int                     vdp1_overflow;  // The VDP1 memory after the textures written
    int                     heap_corrupt;   // jo_memory_usage_percent() reports a corrupted block
    int                     leak;           // A block not freed (see t_heap_top())
    int                     tested;
    int                     accepted;
    int                     skipped;        // Inputs of known bugs not run
}                           t_fuzz_state;

#ifdef JO_DEBUG
extern volatile unsigned int __jo_core_null_access[3];
#endif

/** @brief The VDP1 memory just after the area of the textures (the gouraud tables of the sprites): a texture written after
 *  the end of its area changes it */
#define T_VDP1_GUARD                ((volatile unsigned int *)JO_VDP1_USER_AREA_END_ADDR)
#define T_VDP1_GUARD_WORDS          (64)
static unsigned int         t_vdp1_guard[T_VDP1_GUARD_WORDS];

static void                 t_state_init(t_fuzz_state * const s)
{
    int                     i;

    s->null_access = s->vdp1_overflow = s->heap_corrupt = s->leak = -1;
    s->tested = s->accepted = s->skipped = 0;
    for (i = 0; i < T_VDP1_GUARD_WORDS; ++i)
        t_vdp1_guard[i] = T_VDP1_GUARD[i];
}

/** @brief The first problem of the fuzz, on the screen (line 26): seen on the last frame of a ROM that stops later */
static void                 t_state_show(const char * const what, const int input)
{
    static bool             shown = false;

    if (shown)
        return;
    shown = true;
    jo_printf(0, 26, "FIRST %s %d       ", what, input);
}

/** @brief The checks after an input
 *  @param heap Walk the blocks of jo_malloc() (slow: not after each input)
 */
static void                 t_state_check(t_fuzz_state * const s, const int input, const bool heap)
{
    int                     i;
    bool                    changed;

#ifdef JO_DEBUG
    if (__jo_core_null_access[0] != 0 && s->null_access < 0)
    {
        s->null_access = input;
        t_state_show("NULL", input);
    }
    if (heap && s->heap_corrupt < 0)
    {
        // (the errors of the parsers until now are expected: only the ones of the walk count)
        jt_expect_errors(true);
        jo_memory_usage_percent();
        if (jt_expect_errors(true) > 0)
        {
            s->heap_corrupt = input;
            t_state_show("HEAP", input);
        }
    }
#else
    JO_UNUSED_ARG(heap);
#endif
    for (changed = false, i = 0; i < T_VDP1_GUARD_WORDS; ++i)
        if (T_VDP1_GUARD[i] != t_vdp1_guard[i])
        {
            changed = true;
            T_VDP1_GUARD[i] = t_vdp1_guard[i];
        }
    if (changed && s->vdp1_overflow < 0)
    {
        s->vdp1_overflow = input;
        t_state_show("VDP1", input);
    }
}

/** @brief The checks of a group: each problem is a failure with the first input as its actual value */
static void                 t_state_report(const t_fuzz_state * const s)
{
    JT_EQ(s->null_access, -1);
    JT_EQ(s->vdp1_overflow, -1);
    JT_EQ(s->heap_corrupt, -1);
    JT_EQ(s->leak, -1);
    JT_TRUE(s->tested > 0);
}

static void                 t_progress(const char * const what, const int i)
{
    jo_printf(0, 27, "FUZZ %s %d       ", what, i);
}

/** @brief A file of the fuzz directory (the seeds) */
static unsigned char        *t_read_fuzz_file(const char * const name, int * const len)
{
    unsigned char           *data;

    data = (unsigned char *)jo_fs_read_file_in_dir(name, T_FUZZ_DIR, len);
    JT_TRUE(data != JO_NULL);
    return (data);
}

/*
** JOM models
*/

static unsigned int         t_be32(const unsigned char * const p)
{
    return ((unsigned int)p[0] << 24 | (unsigned int)p[1] << 16 | (unsigned int)p[2] << 8 | p[3]);
}

static void                 t_put32(unsigned char * const p, const unsigned int v)
{
    p[0] = (unsigned char)(v >> 24);
    p[1] = (unsigned char)(v >> 16);
    p[2] = (unsigned char)(v >> 8);
    p[3] = (unsigned char)v;
}

static void                 t_put16(unsigned char * const p, const unsigned int v)
{
    p[0] = (unsigned char)(v >> 8);
    p[1] = (unsigned char)v;
}

#define T_JOM_MAX_CHUNKS            (24)

/** @brief The chunks of a JOM file (their offsets) */
static int                  t_jom_chunks(const unsigned char * const data, const unsigned int size, unsigned int * const offsets)
{
    unsigned int            p;
    unsigned int            chunk;
    int                     n;

    for (n = 0, p = 24; p + 8 <= size && n < T_JOM_MAX_CHUNKS; p += 8 + chunk)
    {
        offsets[n++] = p;
        chunk = t_be32(data + p + 4);
        if (jo_memcmp(data + p, "END ", 4) == 0 || chunk > size - p - 8)
            break;
    }
    return (n);
}

static const char           t_jom_tags[][5] = {"TIMG", "PNTS", "QUAD", "QMAT", "MATL", "TEXT", "NTAB", "CNRM", "VCOL", "GOUR",
                                               "GRUP", "BONE", "ANIM", "END ", "XXXX"};

/** @brief Mutate a JOM file
 *  @param data The file (capacity bytes)
 *  @param size Its size
 *  @return The new size
 */
static unsigned int         t_mutate_jom(unsigned char * const data, unsigned int size, const unsigned int capacity)
{
    unsigned int            offsets[T_JOM_MAX_CHUNKS];
    unsigned int            p;
    unsigned int            chunk;
    unsigned int            i;
    int                     count;

    count = t_jom_chunks(data, size, offsets);
    p = count > 0 ? offsets[t_below((unsigned int)count)] : 24;
    chunk = p + 8 <= size ? t_be32(data + p + 4) : 0;
    switch (t_below(12))
    {
    case 0:
        if (size > 0)
            data[t_below(size)] ^= (unsigned char)(1 << t_below(8));
        break;
    case 1:
        if (size > 0)
            data[t_below(size)] = (unsigned char)t_rand();
        break;
    case 2:
        if (size >= 4)
            t_put32(data + t_below(size / 4) * 4, t_u32_values[t_below(JT_COUNT(t_u32_values))]);
        break;
    case 3:
        if (size >= 2)
            t_put16(data + t_below(size / 2) * 2, t_u16_values[t_below(JT_COUNT(t_u16_values))]);
        break;
    case 4:
        if (size > 1)
            size = 1 + t_below(size - 1);
        break;
    case 5:
        // The count of a chunk
        if (p + 12 <= size)
            t_put32(data + p + 8, t_below(4) == 0 ? t_be32(data + p + 8) + 1 : t_u32_values[t_below(JT_COUNT(t_u32_values))]);
        break;
    case 6:
        // The size of a chunk
        if (p + 8 <= size)
            t_put32(data + p + 4, t_below(3) == 0 ? chunk + 4 * (t_below(3) - 1) + (t_below(2) ? 2 : 0) :
                    t_u32_values[t_below(JT_COUNT(t_u32_values))]);
        break;
    case 7:
        // The tag of a chunk
        if (p + 4 <= size)
            jo_memcpy(data + p, t_jom_tags[t_below(JT_COUNT(t_jom_tags))], 4);
        break;
    case 8:
        // A chunk twice
        if (p + 8 <= size && chunk <= size - p - 8 && size + 8 + chunk <= capacity)
        {
            for (i = size; i-- > p + 8 + chunk;)
                data[i + 8 + chunk] = data[i];
            jo_memcpy(data + p + 8 + chunk, data + p, 8 + chunk);
            size += 8 + chunk;
        }
        break;
    case 9:
    case 10:
        // A field of the elements of a chunk (aligned)
        if (chunk >= 8 && chunk <= size - p - 8)
        {
            i = p + 12 + t_below((chunk - 4) / 2) * 2;
            if (i + 2 <= size)
                t_put16(data + i, t_u16_values[t_below(JT_COUNT(t_u16_values))]);
        }
        break;
    default:
        if (chunk >= 8 && chunk <= size - p - 8)
        {
            i = p + 12 + t_below((chunk - 4) / 4) * 4;
            if (i + 4 <= size)
                t_put32(data + i, t_u32_values[t_below(JT_COUNT(t_u32_values))]);
        }
        break;
    }
    return (size);
}

/** @brief The top of the memory of jo_malloc() (after jo_reduce_memory_fragmentation(): the same address when every block
 *  allocated since was freed) */
static void                 *t_heap_top(void)
{
    void                    *p;

    jo_reduce_memory_fragmentation();
    p = jo_malloc(4);
    if (p != JO_NULL)
        jo_free(p);
    return (p);
}

static void                 t_bone_callback(const int bone, void * const param, const jo_fixed (* const matrix)[3])
{
    if (bone < 0 || bone >= JO_3D_MAX_BONES || matrix == JO_NULL)
        ++*(int *)param;
}

static jo_camera            t_fuzz_camera;

/** @brief Use a model as a game does */
static int                  t_use_model(jo_3d_model * const model)
{
    static const int        times[] = {0, 1, 7, 59, 1000, 100000, 0x7FFFFF, 0x7FFFFFFF, -3};
    jo_3d_pose              pose;
    jo_3d_pose              other;
    int                     bad;
    int                     a;
    int                     t;

    bad = 0;
    for (a = -1; a < (int)model->animation_count && a < 6; ++a)
    {
        jo_3d_model_animation_duration(model, a);
        jo_3d_model_is_animation(model, a, "walk");
        for (t = 0; t < JT_COUNT(times); ++t)
            jo_3d_model_get_pose(model, a, times[t], &pose);
    }
    jo_3d_model_get_pose(model, 0, 3, &pose);
    jo_3d_model_get_pose(model, 1, 5, &other);
    jo_3d_pose_blend(model, &pose, &other, 128);
    jo_3d_model_find_bone(model, "arm");
    jo_3d_model_find_group(model, "ARM");
    jo_3d_model_find_animation(model, "jump");
    jo_3d_push_matrix();
    {
        jo_3d_camera_look_at(&t_fuzz_camera);
        jo_3d_translate_matrix(0, 0, 3000);
        if (model->bone_count > 0)
        {
            jo_3d_model_draw_pose(model, &pose, t_bone_callback, &bad);
            jo_3d_model_set_hidden_bones(2);
            jo_3d_model_draw_pose(model, JO_NULL, JO_NULL, JO_NULL);
            jo_3d_model_set_hidden_bones(0);
        }
        else
            jo_3d_model_draw(model);
    }
    jo_3d_pop_matrix();
    return (bad);
}

/** @brief The models: mutated copies of SKEL.JOM, CAP.JOM, GOOD.JOM created in memory (a copy or in place) */
static void                 t_fuzz_jom_memory(void)
{
    static const char       *names[] = {"SKEL.JOM", "CAP.JOM"};
    unsigned char           *seeds[3];
    int                     sizes[3];
    unsigned char           *buffer;
    unsigned int            capacity;
    unsigned int            size;
    jo_3d_model             *model;
    void                    *top;
    t_fuzz_state            state;
    int                     base;
    int                     usage;
    int                     bad_bones;
    int                     i;
    int                     k;
    int                     m;
    bool                    in_place;

    jt_group("fuzz.jom_memory");
    usage = JT_MEMORY_USAGE();
    for (k = 0; k < 2; ++k)
        seeds[k] = t_read_fuzz_file(names[k], &sizes[k]);
    seeds[2] = (unsigned char *)jo_fs_read_file("GOOD.JOM", &sizes[2]);
    if (seeds[0] == JO_NULL || seeds[1] == JO_NULL || seeds[2] == JO_NULL)
        return;
    // The seeds are valid: their models are created
    for (k = 0; k < 3; ++k)
    {
        model = jo_3d_create_model_from_memory(seeds[k], (unsigned int)sizes[k]);
        JT_TRUE(model != JO_NULL);
        if (k == 0 && model != JO_NULL)
        {
            JT_EQ(model->bone_count, 2);
            JT_EQ(model->animation_count, 2);
            JT_EQ(model->texture_count, 2);
        }
        if (model != JO_NULL)
            jo_3d_free_model(model);
    }
    capacity = 0;
    for (k = 0; k < 3; ++k)
        capacity = JO_MAX(capacity, (unsigned int)sizes[k]);
    capacity = capacity * 3 + 64;
    buffer = (unsigned char *)jo_malloc(capacity);
    JT_TRUE(buffer != JO_NULL);
    if (buffer == JO_NULL)
        return;
    jo_3d_camera_init(&t_fuzz_camera);
    base = jo_sprite_count();
    bad_bones = 0;
    t_state_init(&state);
    jt_expect_errors(true);
    for (i = T_FUZZ_FIRST; i < T_FUZZ_JOM_ITERATIONS && i - T_FUZZ_FIRST < T_FUZZ_COUNT; ++i)
    {
        t_progress("JOM", i);
        t_rng = T_FUZZ_SEED(1, i);
        k = (int)t_below(3);
        jo_memcpy(buffer, seeds[k], sizes[k]);
        size = (unsigned int)sizes[k];
        for (m = 1 + (int)t_below(3); m > 0; --m)
            size = t_mutate_jom(buffer, size, capacity);
        in_place = (i & 1) != 0;
        ++state.tested;
        top = t_heap_top();
        model = in_place ? jo_3d_create_model_in_place(buffer, size) : jo_3d_create_model_from_memory(buffer, size);
        if (model != JO_NULL)
        {
            ++state.accepted;
            bad_bones += t_use_model(model);
            jo_3d_free_model(model);
        }
        // The textures of the model (embedded, gouraud tables, smaller copies) are freed
        jo_3d_model_free_textures_from(base);
        if (t_heap_top() != top && state.leak < 0)
        {
            state.leak = i;
            t_state_show("LEAK", i);
        }
        t_state_check(&state, i, (i & 31) == 31);
    }
    jt_expect_errors(false);
    t_state_check(&state, i, true);
    t_state_report(&state);
    JT_EQ(bad_bones, 0);
    JT_EQ(jo_sprite_count(), base);
    jo_free(buffer);
    for (k = 0; k < 3; ++k)
        jo_free(seeds[k]);
    JT_EQ(JT_MEMORY_USAGE(), usage);
    jt_value("fuzz.jom_memory.accepted", (unsigned int)state.accepted);
    jt_value("fuzz.jom_memory.skipped", (unsigned int)state.skipped);
}

/*
** TGA images
*/

/** @brief The header of a TGA file: little endian 16 bits */
static unsigned int         t_le16(const unsigned char * const p)
{
    return (p[0] | (unsigned int)p[1] << 8);
}

static void                 t_mutate_tga(unsigned char * const data, unsigned int * const size)
{
    static const unsigned char  offsets8[] = {0, 1, 2, 7, 16, 17};
    static const unsigned char  values8[] = {0, 1, 2, 3, 8, 9, 10, 16, 24, 32, 0x20, 0x7F, 0x80, 0xFF};
    static const unsigned char  offsets16[] = {3, 5, 12, 14};
    static const unsigned short values16[] = {0, 1, 7, 8, 9, 255, 256, 504, 512, 0x7FFF, 0x8000, 0xFFFF};
    unsigned int            v;
    unsigned int            o;

    switch (t_below(6))
    {
    case 0:
        o = offsets8[t_below(sizeof(offsets8))];
        data[o] = values8[t_below(sizeof(values8))];
        break;
    case 1:
    case 2:
        o = offsets16[t_below(sizeof(offsets16))];
        v = values16[t_below(JT_COUNT(values16))];
        data[o] = (unsigned char)v;
        data[o + 1] = (unsigned char)(v >> 8);
        break;
    case 3:
        data[t_below(*size)] ^= (unsigned char)(1 << t_below(8));
        break;
    case 4:
        data[t_below(*size)] = (unsigned char)t_rand();
        break;
    default:
        // (the stream has no size: a short file is followed by zeros)
        o = 1 + t_below(*size - 1);
        jo_memset(data + o, 0, *size - o);
        *size = o;
        break;
    }
}

/** @brief Pixels of a TGA image (its size in the header: width x height) */
static unsigned int         t_tga_pixels(const unsigned char * const data)
{
    return (t_le16(data + 12) * t_le16(data + 14));
}

/** @brief The images: mutated copies of S24.TGA, S32.TGA, S8.TGA, ID4.TGA read from memory */
static void                 t_fuzz_tga_memory(void)
{
    static const char       *names[] = {"S24.TGA", "S32.TGA", "S8.TGA"};
    unsigned char           *seeds[4];
    int                     sizes[4];
    unsigned char           *buffer;
    unsigned int            capacity;
    unsigned int            size;
    jo_palette              palette;
    jo_img                  img;
    t_fuzz_state            state;
    int                     base;
    int                     usage;
    int                     id;
    int                     i;
    int                     k;
    int                     m;

    jt_group("fuzz.tga_memory");
    usage = JT_MEMORY_USAGE();
    for (k = 0; k < 3; ++k)
        seeds[k] = t_read_fuzz_file(names[k], &sizes[k]);
    seeds[3] = (unsigned char *)jo_fs_read_file("ID4.TGA", &sizes[3]);
    if (seeds[0] == JO_NULL || seeds[1] == JO_NULL || seeds[2] == JO_NULL || seeds[3] == JO_NULL)
        return;
    // (the stream has no size: the pixels of a header bigger than the file are read in the zeros after it)
    capacity = 16384;
    buffer = (unsigned char *)jo_malloc(capacity);
    JT_TRUE(buffer != JO_NULL);
    if (buffer == JO_NULL)
        return;
    jo_create_palette(&palette);
    jo_set_tga_default_palette(&palette);
    base = jo_sprite_count();
    t_state_init(&state);
    jt_expect_errors(true);
    for (i = T_FUZZ_FIRST; i < T_FUZZ_TGA_ITERATIONS && i - T_FUZZ_FIRST < T_FUZZ_COUNT; ++i)
    {
        if ((i & 15) == 0)
            t_progress("TGA", i);
        t_rng = T_FUZZ_SEED(2, i);
        k = (int)t_below(4);
        jo_memset(buffer, 0, capacity);
        jo_memcpy(buffer, seeds[k], sizes[k]);
        size = (unsigned int)sizes[k];
        for (m = 1 + (int)t_below(2); m > 0; --m)
            t_mutate_tga(buffer, &size);
        // (a big image: rarely, it takes seconds to convert)
        if (t_tga_pixels(buffer) > 256 * 256 && t_below(16) != 0)
            continue;
        ++state.tested;
        img.data = JO_NULL;
        if (jo_tga_loader_from_stream(&img, (char *)buffer, JO_COLOR_Black) == JO_TGA_OK)
        {
            ++state.accepted;
            jo_free(img.data);
        }
        id = jo_sprite_add_tga_from_stream((char *)buffer, JO_COLOR_Transparent);
        if (id >= 0)
            jo_sprite_free_from(base);
        t_state_check(&state, i, (i & 15) == 15);
    }
    jt_expect_errors(false);
    t_state_check(&state, i, true);
    t_state_report(&state);
    jo_set_tga_default_palette(JO_NULL);
    JT_EQ(jo_sprite_count(), base);
    jo_free(buffer);
    for (k = 0; k < 4; ++k)
        jo_free(seeds[k]);
    JT_EQ(JT_MEMORY_USAGE(), usage);
    jt_value("fuzz.tga_memory.accepted", (unsigned int)state.accepted);
    jt_value("fuzz.tga_memory.skipped", (unsigned int)state.skipped);
}

/*
** The files of cd/FUZZ
*/

static void                 t_fuzz_files_jom(void)
{
    char                    name[16];
    jo_3d_model             *model;
    t_fuzz_state            state;
    int                     base;
    int                     usage;
    int                     bad_bones;
    int                     i;

    jt_group("fuzz.files_jom");
    base = jo_sprite_count();
    usage = JT_MEMORY_USAGE();
    bad_bones = 0;
    t_state_init(&state);
    jo_fs_cd(T_FUZZ_DIR);
    jt_expect_errors(true);
    for (i = 0; i < T_FUZZ_FILES; ++i)
    {
        jo_snprintf(name, sizeof(name), "J%02d.JOM", i);
        t_progress(name, i);
        ++state.tested;
        model = jo_3d_load_model(name);
        if (model != JO_NULL)
        {
            ++state.accepted;
            bad_bones += t_use_model(model);
            jo_3d_free_model(model);
        }
        jo_3d_model_free_textures_from(base);
        t_state_check(&state, i, true);
    }
    jt_expect_errors(false);
    jo_fs_cd(JO_PARENT_DIR);
    t_state_report(&state);
    JT_EQ(bad_bones, 0);
    JT_EQ(JT_MEMORY_USAGE(), usage);
    jt_value("fuzz.files_jom.accepted", (unsigned int)state.accepted);
}

static void                 t_fuzz_files_tga(void)
{
    char                    name[16];
    jo_palette              palette;
    jo_img                  img;
    t_fuzz_state            state;
    int                     base;
    int                     usage;
    int                     id;
    int                     i;

    jt_group("fuzz.files_tga");
    base = jo_sprite_count();
    usage = JT_MEMORY_USAGE();
    jo_create_palette(&palette);
    jo_set_tga_default_palette(&palette);
    t_state_init(&state);
    jt_expect_errors(true);
    for (i = 0; i < T_FUZZ_FILES; ++i)
    {
        jo_snprintf(name, sizeof(name), "T%02d.TGA", i);
        t_progress(name, i);
        ++state.tested;
        id = jo_sprite_add_tga(T_FUZZ_DIR, name, JO_COLOR_Transparent);
        if (id >= 0)
        {
            ++state.accepted;
            jo_sprite_free_from(base);
        }
        img.data = JO_NULL;
        if (jo_tga_loader(&img, T_FUZZ_DIR, name, JO_COLOR_Black) == JO_TGA_OK && img.data != JO_NULL)
            jo_free(img.data);
        t_state_check(&state, i, true);
    }
    jt_expect_errors(false);
    jo_set_tga_default_palette(JO_NULL);
    t_state_report(&state);
    JT_EQ(jo_sprite_count(), base);
    JT_EQ(JT_MEMORY_USAGE(), usage);
    jt_value("fuzz.files_tga.accepted", (unsigned int)state.accepted);
}

static void                 t_fuzz_files_wav(void)
{
    char                    name[16];
    jo_sound                sound;
    t_fuzz_state            state;
    int                     usage;
    int                     bad;
    int                     i;

    jt_group("fuzz.files_wav");
    usage = JT_MEMORY_USAGE();
    bad = 0;
    t_state_init(&state);
    jo_fs_cd(T_FUZZ_DIR);
    jt_expect_errors(true);
    for (i = 0; i < T_FUZZ_FILES; ++i)
    {
        jo_snprintf(name, sizeof(name), "W%02d.WAV", i);
        t_progress(name, i);
        ++state.tested;
        jo_memset(&sound, 0, sizeof(sound));
        if (jo_audio_load_wav(name, &sound))
        {
            ++state.accepted;
            // (the samples are in the file: not more than its size, 256 bytes at most here)
            if (sound.data == JO_NULL || sound.data_length > 256)
                ++bad;
            if (sound.data != JO_NULL && sound.data_length > 0 && sound.sample_rate > 0)
            {
                jo_audio_play_sound_on_channel(&sound, 0);
                jo_audio_stop_sound(&sound);
            }
            if (sound.data != JO_NULL)
                jo_free(sound.data);
        }
        t_state_check(&state, i, true);
    }
    jt_expect_errors(false);
    jo_fs_cd(JO_PARENT_DIR);
    t_state_report(&state);
    JT_EQ(bad, 0);
    JT_EQ(JT_MEMORY_USAGE(), usage);
    jt_value("fuzz.files_wav.accepted", (unsigned int)state.accepted);
}

static void                 t_fuzz_files_map(void)
{
    char                    name[16];
    t_fuzz_state            state;
    int                     base;
    int                     usage;
    int                     anim;
    int                     id;
    int                     i;

    jt_group("fuzz.files_map");
    base = jo_sprite_count();
    // The sprite of the tiles (its name in the maps) and an animation (@0: the first one)
    id = jo_sprite_add_tga(T_FUZZ_DIR, "S8.TGA", JO_COLOR_Transparent);
    JT_TRUE(id >= 0);
    anim = jo_create_sprite_anim((unsigned short)id, 1, 4);
    usage = JT_MEMORY_USAGE();
    t_state_init(&state);
    jt_expect_errors(true);
    for (i = 0; i < T_FUZZ_FILES; ++i)
    {
        jo_snprintf(name, sizeof(name), "M%02d.MAP", i);
        t_progress(name, i);
        ++state.tested;
        if (jo_map_load_from_file(0, 500, T_FUZZ_DIR, name))
        {
            ++state.accepted;
            jo_map_draw(0, 0, 0);
            jo_map_hitbox_detection_custom_boundaries(0, 10, 10, 16, 16);
            jo_map_free(0);
        }
        t_state_check(&state, i, true);
    }
    jt_expect_errors(false);
    t_state_report(&state);
    JT_EQ(JT_MEMORY_USAGE(), usage);
    if (anim >= 0)
        jo_remove_sprite_anim(anim);
    jo_sprite_free_from(base);
    jt_value("fuzz.files_map.accepted", (unsigned int)state.accepted);
}

#ifdef JO_COMPILE_WITH_LEVEL_SUPPORT

/** @brief The sections with records and the size of their records */
static const unsigned int   t_level_sections[][2] =
{
    {JO_LEVEL_SECTION_PLNT, sizeof(jo_level_plnt_record)},
    {JO_LEVEL_SECTION_MODL, sizeof(jo_level_modl_record)},
    {JO_LEVEL_SECTION_MBOX, sizeof(jo_level_mbox_record)},
    {JO_LEVEL_SECTION_INST, sizeof(jo_level_inst_record)},
    {JO_LEVEL_SECTION_DECL, sizeof(jo_level_decl_record)},
    {JO_LEVEL_SECTION_ENTI, sizeof(jo_level_enti_record)},
    {JO_LEVEL_SECTION_PROP, sizeof(jo_level_prop_record)},
    {JO_LEVEL_SECTION_LINK, sizeof(jo_level_link_record)},
    {JO_LEVEL_SECTION_CELL, sizeof(jo_level_cell_record)},
    {JO_LEVEL_SECTION_PRTL, sizeof(jo_level_prtl_record)},
    {JO_LEVEL_SECTION_WEAP, sizeof(jo_level_weap_record)},
    {JO_LEVEL_SECTION_INFO, 16},
    {JO_LEVEL_SECTION_STRS, 4},
};

static void                 t_fuzz_files_jol(void)
{
    static unsigned int     defaults[64];
    char                    name[16];
    jo_level                *level;
    t_fuzz_state            state;
    unsigned int            count;
    int                     usage;
    int                     i;
    int                     k;

    jt_group("fuzz.files_jol");
    usage = JT_MEMORY_USAGE();
    t_state_init(&state);
    jo_fs_cd(T_FUZZ_DIR);
    jt_expect_errors(true);
    for (i = -2; i < T_FUZZ_FILES; ++i)
    {
        // (the seeds first)
        if (i < 0)
            jo_snprintf(name, sizeof(name), "%s", i == -2 ? "DEFS.JOL" : "LEVEL.JOL");
        else
            jo_snprintf(name, sizeof(name), "L%02d.JOL", i);
        t_progress(name, i);
        ++state.tested;
        level = jo_level_load(name);
        if (i < 0)
            JT_TRUE(level != JO_NULL);
        if (level == JO_NULL)
            continue;
        ++state.accepted;
        for (k = 0; k < JT_COUNT(t_level_sections); ++k)
        {
            jo_level_get_records(level, t_level_sections[k][0], t_level_sections[k][1], defaults, &count);
            // (a smaller and a bigger record than the one of the file: an older or a newer engine)
            jo_level_get_records(level, t_level_sections[k][0], t_level_sections[k][1] / 2 + 4, defaults, &count);
            jo_level_get_records(level, t_level_sections[k][0], t_level_sections[k][1] + 8, defaults, &count);
            jo_level_get_header(level, t_level_sections[k][0], 16, defaults);
            jo_level_get_array(level, t_level_sections[k][0], 4);
            jo_level_get_array(level, t_level_sections[k][0], 0xFFFFFFF0u);
        }
        jo_level_string(level, 0);
        jo_level_string(level, 0xFFFFFFFFu);
        jo_entities_load(level);
        jo_entities_free();
        jo_level_free(level);
        t_state_check(&state, i, true);
    }
    jt_expect_errors(false);
    jo_fs_cd(JO_PARENT_DIR);
    t_state_report(&state);
    JT_EQ(JT_MEMORY_USAGE(), usage);
    jt_value("fuzz.files_jol.accepted", (unsigned int)state.accepted);
}

#endif /* JO_COMPILE_WITH_LEVEL_SUPPORT */

/*
** APIs
*/

/** @brief jo_vdp2_malloc() of a huge size (a negative number given as a size): refused, like jo_malloc() (see t_3d_bugs()) */
static void                 t_api_vdp2_malloc_sizes(void)
{
    unsigned char           *a;
    unsigned char           *b;
    unsigned char           *c;
    unsigned char           *first;

    jt_group("fuzz.api_vdp2_sizes");
    jt_expect_errors(true);
    first = (unsigned char *)jo_vdp2_malloc(JO_VDP2_RAM_USER, 16);
    jo_vdp2_free(first);
    // A new block at the top of the bank
    a = (unsigned char *)jo_vdp2_malloc(JO_VDP2_RAM_USER, 0xFFFFFF00u);
    JT_TRUE(a == JO_NULL);
    b = (unsigned char *)jo_vdp2_malloc(JO_VDP2_RAM_USER, 16);
    // (the next block after the first one, in the bank B1)
    JT_TRUE(b == first);
    if (b != JO_NULL)
        jo_vdp2_free(b);
    if (a != JO_NULL)
        jo_vdp2_free(a);
    // A free block in the middle of the bank (not at its top)
    a = (unsigned char *)jo_vdp2_malloc(JO_VDP2_RAM_USER, 256);
    b = (unsigned char *)jo_vdp2_malloc(JO_VDP2_RAM_USER, 256);
    jo_vdp2_free(a);
    c = (unsigned char *)jo_vdp2_malloc(JO_VDP2_RAM_USER, 0xFFFFFF00u);
    JT_TRUE(c == JO_NULL);
    if (c != JO_NULL)
        jo_vdp2_free(c);
    jo_vdp2_free(b);
    // Everything given back
    a = (unsigned char *)jo_vdp2_malloc(JO_VDP2_RAM_USER, 16);
    JT_TRUE(a == first);
    jo_vdp2_free(a);
    jt_expect_errors(false);
}

/** @brief jo_sprite_add() of a texture bigger than the VRAM of the VDP1 (512 KB): refused, not written after the area of the
 *  textures (the size of the texture was masked with 0x7FFE0 by the check of the free memory) */
/** @brief The VDP1 memory after the area of the textures (the gouraud tables): saved before a texture that may be written
 *  after the end of the area */
static unsigned int         t_vdp1_saved[(0x80000 - (JO_VDP1_USER_AREA_END_ADDR - JO_VDP1_VRAM)) / 4];

static void                 t_vdp1_save(const bool restore)
{
    unsigned int            k;

    for (k = 0; k < JT_COUNT(t_vdp1_saved); ++k)
        if (restore)
            ((volatile unsigned int *)JO_VDP1_USER_AREA_END_ADDR)[k] = t_vdp1_saved[k];
        else
            t_vdp1_saved[k] = ((volatile unsigned int *)JO_VDP1_USER_AREA_END_ADDR)[k];
}

static void                 t_api_sprite_sizes(void)
{
    t_fuzz_state            state;
    jo_img                  img;
    int                     base;
    int                     id;

    jt_group("fuzz.api_sprite_sizes");
    // (the gouraud tables after the textures: saved, they are written again if the sprite is accepted)
    t_vdp1_save(false);
    base = jo_sprite_count();
    t_state_init(&state);
    ++state.tested;
    // 0x7FFF x 8 pixels (RGB): 0x7FFF0 bytes, more than the VRAM; the pixels are read in the work RAM
    img.width = 0x7FFF;
    img.height = 8;
    img.data = (jo_color *)0x06000000;
    jt_expect_errors(true);
    id = jo_sprite_add(&img);
    jt_expect_errors(false);
    JT_EQ(id, -1);
    if (id >= 0)
        jo_sprite_free_from(base);
    t_state_check(&state, 0, false);
    t_state_report(&state);
    t_vdp1_save(true);
    JT_EQ(jo_sprite_count(), base);
}

#define T_API_BLOCKS                (24)

/** @brief A block of the random sequence */
typedef struct
{
    unsigned char           *p;
    unsigned int            size;
    int                     kind;       // 0: jo_malloc(), 1: jo_vdp2_malloc(), 2: a sprite
    int                     sprite;
}                           t_api_block;

static unsigned char        t_api_pattern(const t_api_block * const b, const unsigned int k, const unsigned int i)
{
    return ((unsigned char)(k * 29 + i * 7 + b->size));
}

/** @brief The contents of a block (written at its allocation) */
static bool                 t_api_block_ok(const t_api_block * const b, const unsigned int k)
{
    unsigned int            i;
    volatile unsigned short *w;

    if (b->kind == 1)
    {
        // (the VRAM of the VDP2: 16 bits accesses)
        for (w = (volatile unsigned short *)b->p, i = 0; i < b->size / 2; ++i)
            if (w[i] != (unsigned short)(t_api_pattern(b, k, i) * 257))
                return (false);
        return (true);
    }
    if (b->kind == 2)
    {
        w = (volatile unsigned short *)(JO_VDP1_VRAM + JO_MULT_BY_8(__jo_sprite_def[b->sprite].adr));
        for (i = 0; i < b->size / 2; ++i)
            if (w[i] != (unsigned short)(0x8000 | t_api_pattern(b, k, i)))
                return (false);
        return (true);
    }
    for (i = 0; i < b->size; ++i)
        if (b->p[i] != t_api_pattern(b, k, i))
            return (false);
    return (true);
}

/** @brief Random sequences of jo_malloc() (the behaviours), jo_vdp2_malloc(), sprites and their frees, lists with indexes out
 *  of range: the contents of each block are checked after each operation */
static void                 t_api_sequences(void)
{
    static unsigned short   pixels[16 * 32];
    static const jo_malloc_behaviour behaviours[] = {JO_FAST_ALLOCATION, JO_MALLOC_TRY_REUSE_BLOCK,
                                                     JO_MALLOC_TRY_REUSE_SAME_BLOCK_SIZE};
    t_api_block             blocks[T_API_BLOCKS];
    jo_list                 list;
    jo_node                 *node;
    int                     model[32];
    int                     count;
    jo_img                  img;
    int                     corrupted;
    int                     first_corrupted;
    int                     overlaps;
    int                     failed;
    int                     bad_list;
    int                     base;
    int                     usage;
    int                     sprites;
    unsigned char           *vdp2_first;
    unsigned int            k;
    unsigned int            j;
    unsigned int            i;
    int                     op;
    int                     index;
    t_api_block             *b;

    jt_group("fuzz.api_sequences");
    jo_memset(blocks, 0, sizeof(blocks));
    jo_list_init(&list);
    count = 0;
    corrupted = overlaps = failed = bad_list = sprites = 0;
    first_corrupted = -1;
    base = jo_sprite_count();
    usage = JT_MEMORY_USAGE();
    vdp2_first = (unsigned char *)jo_vdp2_malloc(JO_VDP2_RAM_USER, 16);
    jo_vdp2_free(vdp2_first);
    t_rng = T_FUZZ_SEED(3, 0);
    jt_expect_errors(true);
    for (op = 0; op < T_FUZZ_API_OPERATIONS; ++op)
    {
        if ((op & 63) == 0)
            t_progress("API", op);
        k = t_below(T_API_BLOCKS);
        b = &blocks[k];
        switch (t_below(4))
        {
        case 0:
        case 1:
            if (b->p != JO_NULL)
            {
                if (!t_api_block_ok(b, k) && first_corrupted < 0)
                    first_corrupted = op;
                if (b->kind == 0)
                    jo_free(b->p);
                else if (b->kind == 1)
                    jo_vdp2_free(b->p);
                else
                {
                    // (the sprites after it are freed too)
                    for (j = 0; j < T_API_BLOCKS; ++j)
                        if (j != k && blocks[j].kind == 2 && blocks[j].p != JO_NULL && blocks[j].sprite > b->sprite)
                            blocks[j].p = JO_NULL;
                    jo_sprite_free_from(b->sprite);
                }
                b->p = JO_NULL;
                break;
            }
            b->kind = (int)t_below(3);
            if (b->kind == 0)
            {
                b->size = 1 + (t_below(4) == 0 ? t_below(8000) : t_below(100));
                b->p = (unsigned char *)jo_malloc_with_behaviour(b->size, behaviours[t_below(JT_COUNT(behaviours))]);
                if (b->p == JO_NULL)
                {
                    ++failed;
                    break;
                }
                if ((unsigned int)b->p & 3)
                    ++corrupted;
                for (i = 0; i < b->size; ++i)
                    b->p[i] = t_api_pattern(b, k, i);
            }
            else if (b->kind == 1)
            {
                b->size = 2 * (1 + t_below(t_below(4) == 0 ? 2048 : 64));
                b->p = (unsigned char *)jo_vdp2_malloc(JO_VDP2_RAM_USER, b->size);
                // (the bank B1 is small, 12 blocks: refused when full)
                if (b->p == JO_NULL)
                    break;
                for (i = 0; i < b->size / 2; ++i)
                    ((volatile unsigned short *)b->p)[i] = (unsigned short)(t_api_pattern(b, k, i) * 257);
            }
            else
            {
                img.width = (unsigned short)(8 * (1 + t_below(2)));
                img.height = (unsigned short)(1 + t_below(32));
                b->size = img.width * img.height * 2;
                for (i = 0; i < b->size / 2; ++i)
                    pixels[i] = (unsigned short)(0x8000 | t_api_pattern(b, k, i));
                img.data = pixels;
                b->sprite = jo_sprite_add(&img);
                if (b->sprite < 0)
                    break;
                b->p = (unsigned char *)1;
                ++sprites;
            }
            // No overlap with the other blocks of the same memory
            for (j = 0; j < T_API_BLOCKS; ++j)
                if (j != k && blocks[j].p != JO_NULL && blocks[j].kind == b->kind && b->kind != 2 &&
                    blocks[j].p < b->p + b->size && b->p < blocks[j].p + blocks[j].size)
                    ++overlaps;
            break;
        case 2:
            // A list: indexes out of range too (insert: at the start or at the end, remove: nothing)
            index = (int)t_below((unsigned int)count + 4) - 2;
            if (t_below(2) == 0 && count < JT_COUNT(model))
            {
                jo_list_insert_at(&list, (jo_list_data)(void *)op, index);
                index = index < 0 ? 0 : index > count ? count : index;
                for (i = (unsigned int)count; (int)i > index; --i)
                    model[i] = model[i - 1];
                model[index] = op;
                ++count;
            }
            else
            {
                jo_list_remove_at(&list, index);
                if (index >= 0 && index < count)
                {
                    for (i = (unsigned int)index; (int)i < count - 1; ++i)
                        model[i] = model[i + 1];
                    --count;
                }
                else if (index < 0 && count > 0)
                {
                    // (jo_list_at(): the first node for a negative index)
                    for (i = 0; (int)i < count - 1; ++i)
                        model[i] = model[i + 1];
                    --count;
                }
            }
            if (list.count != count)
                ++bad_list;
            for (i = 0, node = list.first; node != JO_NULL && (int)i < count; node = node->next, ++i)
                if ((int)node->data.ptr != model[i])
                    ++bad_list;
            if ((int)i != count || node != JO_NULL)
                ++bad_list;
            break;
        default:
            if ((op & 7) == 0)
                jo_reduce_memory_fragmentation();
            break;
        }
        // The contents of all the blocks
        for (j = 0; j < T_API_BLOCKS; ++j)
            if (blocks[j].p != JO_NULL && !t_api_block_ok(&blocks[j], j))
            {
                ++corrupted;
                if (first_corrupted < 0)
                    first_corrupted = op;
            }
    }
    for (j = 0; j < T_API_BLOCKS; ++j)
        if (blocks[j].p != JO_NULL)
        {
            if (blocks[j].kind == 0)
                jo_free(blocks[j].p);
            else if (blocks[j].kind == 1)
                jo_vdp2_free(blocks[j].p);
        }
    jo_sprite_free_from(base);
    jo_list_clear(&list);
    jo_reduce_memory_fragmentation();
    jt_expect_errors(false);
    JT_EQ(first_corrupted, -1);
    JT_EQ(corrupted, 0);
    JT_EQ(overlaps, 0);
    JT_EQ(failed, 0);
    JT_EQ(bad_list, 0);
    JT_TRUE(sprites > 0);
    JT_EQ(jo_sprite_count(), base);
    JT_EQ(JT_MEMORY_USAGE(), usage);
    // The VDP2 memory given back
    b = &blocks[0];
    b->p = (unsigned char *)jo_vdp2_malloc(JO_VDP2_RAM_USER, 16);
    JT_TRUE(b->p == vdp2_first);
    jo_vdp2_free(b->p);
}

/** @brief Ids and indexes out of range: the documented results (all the builds) */
static void                 t_api_ids(void)
{
    jo_3d_model             *model;
    jo_3d_pose              pose;
    unsigned char           *data;
    int                     len;
    int                     last;

    jt_group("fuzz.api_ids");
    last = jo_get_last_sprite_id();
    jo_sprite_free_from(last + 1);
    jo_sprite_free_from(JO_MAX_SPRITE + 10);
    jo_sprite_free_from(-7);
    JT_EQ(jo_get_last_sprite_id(), last);
    JT_TRUE(!jo_sprite_add_end_codes(-1));
    JT_TRUE(!jo_sprite_add_end_codes(last + 1));
    JT_TRUE(!jo_sprite_add_end_codes(JO_MAX_SPRITE + 10));
    jt_expect_errors(true);
    JT_EQ(jo_replace_sprite_anim(JO_MAX_SPRITE_ANIM + 3, 0, 1, 1), -1);
    JT_EQ(jo_replace_sprite_anim(-2, 0, 1, 1), -1);
    jo_remove_sprite_anim(-1);
    jo_remove_sprite_anim(JO_MAX_SPRITE_ANIM + 3);
    jo_audio_stop_channel(200);
    jt_expect_errors(false);
    data = t_read_fuzz_file("SKEL.JOM", &len);
    if (data == JO_NULL)
        return;
    model = jo_3d_create_model_from_memory(data, (unsigned int)len);
    JT_TRUE(model != JO_NULL);
    if (model != JO_NULL)
    {
        pose.root.x = 1;
        jo_3d_model_get_pose(model, 1000, 5, &pose);
        JT_EQ(pose.root.x, 0);
        pose.root.x = 1;
        jo_3d_model_get_pose(model, -2, 5, &pose);
        JT_EQ(pose.root.x, 0);
        JT_EQ(jo_3d_model_animation_duration(model, 1000), 0);
        JT_EQ(jo_3d_model_animation_duration(model, -5), 0);
        JT_TRUE(!jo_3d_model_is_animation(model, 1000, "walk"));
        JT_TRUE(!jo_3d_model_is_animation(model, -1, "walk"));
        JT_EQ(jo_3d_model_find_bone(model, "nope"), -1);
        JT_EQ(jo_3d_model_find_animation(model, "nope"), -1);
        JT_TRUE(jo_3d_model_find_group(model, "nope") == JO_NULL);
        jo_3d_free_model(model);
    }
    jo_free(data);
}


/*
** The inputs of the known bugs (cd/FUZZ/K*, made by gen_fuzz.py)
*/

#define T_HEAP_PROBE                (8192)

/** @brief The free memory at the top of the heap filled with a pattern (jo_malloc() allocates there): a block written after its
 *  end changes the pattern (see t_heap_probe_check())
 *  @return The start of the area */
static unsigned char        *t_heap_probe_fill(void)
{
    unsigned char           *p;

    p = (unsigned char *)jo_malloc(T_HEAP_PROBE);
    if (p == JO_NULL)
        return (JO_NULL);
    jo_memset(p, 0xA5, T_HEAP_PROBE);
    // (the last block: given back to the zone, the next block starts at the same address)
    jo_free(p);
    return (p);
}

/** @brief The pattern after a block allocated in the probe area
 *  @return The bytes changed after the end of the block */
static int                  t_heap_probe_check(const unsigned char * const probe, const void * const block)
{
    const unsigned char     *p;
    int                     changed;

    if (probe == JO_NULL || block == JO_NULL)
        return (0);
    // (the header of a block: 2 bytes for the zone, 4 bytes for its size, header included; see malloc.c)
    p = (const unsigned char *)block + ((const unsigned int *)block)[-1] - 8;
    for (changed = 0; p < probe + T_HEAP_PROBE; ++p)
        if (*p != 0xA5)
            ++changed;
    return (changed);
}

/** @brief The bugs found by the fuzz that didn't stop the ROM: each check failed before its fix */
static void                 t_fixed_bugs(void)
{
    t_fuzz_state            state;
    jo_3d_model             *model;
    jo_3d_pose              pose;
    unsigned char           *data;
    unsigned char           *probe;
    void                    *top;
    int                     len;
    int                     base;
    int                     k;

    jt_group("fuzz.fixed_bugs");
    base = jo_sprite_count();
    jt_expect_errors(true);
    // KBIG.JOM: an embedded texture of 0x7FFF x 8 pixels (512 KB) accepted, written after the VRAM of the textures
    if ((data = t_read_fuzz_file("KBIG.JOM", &len)) != JO_NULL)
    {
        t_vdp1_save(false);
        t_state_init(&state);
        ++state.tested;
        model = jo_3d_create_model_from_memory(data, (unsigned int)len);
        if (model != JO_NULL)
            jo_3d_free_model(model);
        jo_3d_model_free_textures_from(base);
        t_state_check(&state, 0, false);
        JT_EQ(state.vdp1_overflow, -1);
        t_vdp1_save(true);
        jo_free(data);
    }
    // KLONG.JOM: 200 keys of 0xFFFF ticks, the root at x = 1000 in each key: the pose of any time has x = 1000
    if ((data = t_read_fuzz_file("KLONG.JOM", &len)) != JO_NULL)
    {
        model = jo_3d_create_model_from_memory(data, (unsigned int)len);
        JT_TRUE(model != JO_NULL);
        if (model != JO_NULL)
        {
            jo_3d_model_get_pose(model, 0, 8000000, &pose);
            JT_EQ(pose.root.x, 1000);
            // (time x 256 over 2^31)
            jo_3d_model_get_pose(model, 0, 9000000, &pose);
            JT_EQ(pose.root.x, 1000);
            jo_3d_free_model(model);
        }
        jo_3d_model_free_textures_from(base);
        jo_free(data);
    }
    // KDUP.JOM: a MATL chunk twice, copied twice by jo_3d_create_model_from_memory(): the first copy was not freed (a chunk
    // twice is now a wrong file: refused, nothing left)
    if ((data = t_read_fuzz_file("KDUP.JOM", &len)) != JO_NULL)
    {
        top = t_heap_top();
        model = jo_3d_create_model_from_memory(data, (unsigned int)len);
        JT_TRUE(model == JO_NULL);
        if (model != JO_NULL)
            jo_3d_free_model(model);
        jo_3d_model_free_textures_from(base);
        JT_TRUE(t_heap_top() == top);
        jo_free(data);
    }
    // KTEXT.JOM in place: 2 TEXT chunks (40 names, then 1): the block of the model holds 1 texture id, 40 are written after
    // it (KTEXT1.JOM: the same model with the first TEXT chunk only, nothing written after its block). KTEXT.JOM is now
    // refused (a chunk twice)
    for (k = 0; k < 2; ++k)
        if ((data = t_read_fuzz_file(k == 0 ? "KTEXT1.JOM" : "KTEXT.JOM", &len)) != JO_NULL)
        {
            probe = t_heap_probe_fill();
            model = jo_3d_create_model_in_place(data, (unsigned int)len);
            JT_EQ(model != JO_NULL, k == 0);
            JT_EQ(t_heap_probe_check(probe, model), 0);
            if (model != JO_NULL)
                jo_3d_free_model(model);
            jo_3d_model_free_textures_from(base);
            jo_free(data);
        }
    jt_expect_errors(false);
}

/** @brief The bugs found by the fuzz that crashed or hung the ROM (T_FUZZ_KNOWN_BUGS: 0 all, 10 + n the input n): reached and
 *  refused once fixed */
static void                 t_fixed_crashes(void)
{
    static const char       *names[] = {"KTGA.TGA", "KANIM.JOM", "KTIMG.JOM", "L08.JOL", "L09.JOL", "L17.JOL", "L19.JOL"};
    jo_3d_model             *model;
    jo_3d_pose              pose;
    jo_img                  img;
    unsigned char           *data;
    int                     len;
    int                     k;

    jt_group("fuzz.fixed_crashes");
    jt_expect_errors(true);
    for (k = 0; k < JT_COUNT(names); ++k)
    {
        if (T_FUZZ_KNOWN_BUGS != 0 && T_FUZZ_KNOWN_BUGS != 10 + k)
            continue;
        t_progress(names[k], k);
#ifdef JO_COMPILE_WITH_LEVEL_SUPPORT
        if (k >= 3)
        {
            // A section with a record_size of 0: jo_level_get_records() of a record of 24 bytes (an older engine)
            unsigned int    defaults[16];
            unsigned int    count;
            unsigned int    id;
            jo_level        *level;

            jo_memset(defaults, 0, sizeof(defaults));
            jo_fs_cd(T_FUZZ_DIR);
            // (refused since the fix: a section with a record_size of 0 and records)
            level = jo_level_load(names[k]);
            jo_fs_cd(JO_PARENT_DIR);
            JT_TRUE(level == JO_NULL);
            if (level != JO_NULL)
            {
                id = k <= 4 ? (k == 3 ? JO_LEVEL_SECTION_INFO : JO_LEVEL_SECTION_PRTL) : k == 5 ? JO_LEVEL_SECTION_PRTL : JO_LEVEL_SECTION_INFO;
                jo_level_get_records(level, id, k == 6 ? 16 : k == 3 ? 24 : 60, defaults, &count);
                jo_level_free(level);
            }
            JT_TRUE(true);
            continue;
        }
#else
        if (k >= 3)
            continue;
#endif
        if ((data = t_read_fuzz_file(names[k], &len)) == JO_NULL)
            continue;
        if (k == 0)
        {
            // 0xFFFF x 0x8001 pixels: 65534 bytes allocated (the size wraps around 32 bits), 2^31 pixels written
            img.data = JO_NULL;
            if (jo_tga_loader_from_stream(&img, (char *)data, JO_COLOR_Transparent) == JO_TGA_OK)
                jo_free(img.data);
        }
        else
        {
            model = jo_3d_create_model_from_memory(data, (unsigned int)len);
            if (model != JO_NULL)
            {
                // KANIM: the keys at an odd address (an address error of the SH-2)
                jo_3d_model_get_pose(model, 0, 5, &pose);
                jo_3d_free_model(model);
            }
        }
        jo_free(data);
        // (reached: the bug is fixed)
        JT_TRUE(true);
    }
    jt_expect_errors(false);
}

void                        t_fuzz(void)
{
    t_fixed_crashes();
#if T_FUZZ_KNOWN_BUGS
    // (one input of the crashes only)
    return;
#endif
    t_fixed_bugs();
    t_api_ids();
    t_api_vdp2_malloc_sizes();
    t_api_sprite_sizes();
    t_api_sequences();
    t_fuzz_jom_memory();
    t_fuzz_tga_memory();
    t_fuzz_files_jom();
    t_fuzz_files_tga();
    t_fuzz_files_wav();
    t_fuzz_files_map();
#ifdef JO_COMPILE_WITH_LEVEL_SUPPORT
    t_fuzz_files_jol();
#endif
    jo_clear_screen_line(27);
}
