/*
** Jo Sega Saturn Engine - regression test ROM: random sequences (the same at each run) of allocations, lists and sprites,
** their contents checked after each step
*/

#include "jt.h"
#include "tests.h"

#define T_BLOCKS                (48)

static unsigned int             t_seed = 12345;

static unsigned int             t_next(const unsigned int max)
{
    t_seed = t_seed * 1103515245u + 12345u;
    return ((t_seed >> 12) % max);
}

static void                     t_malloc_stress(void)
{
    static unsigned char        *blocks[T_BLOCKS];
    static unsigned int         sizes[T_BLOCKS];
    unsigned int                op;
    unsigned int                k;
    unsigned int                i;
    unsigned int                j;
    int                         before;
    int                         corrupted;
    int                         overlaps;
    int                         unaligned;
    int                         failed;

    jt_group("stress.malloc");
    before = JT_MEMORY_USAGE();
    corrupted = overlaps = unaligned = failed = 0;
    for (op = 0; op < 3000; ++op)
    {
        k = t_next(T_BLOCKS);
        if (blocks[k] != JO_NULL)
        {
            // The contents written at its allocation
            for (i = 0; i < sizes[k]; ++i)
                if (blocks[k][i] != (unsigned char)(k * 31 + i))
                {
                    ++corrupted;
                    break;
                }
            jo_free(blocks[k]);
            blocks[k] = JO_NULL;
            continue;
        }
        // Sizes from 1 byte (odd sizes) to 6 KB
        sizes[k] = 1 + (t_next(4) == 0 ? t_next(6000) : t_next(200));
        blocks[k] = (unsigned char *)jo_malloc(sizes[k]);
        if (blocks[k] == JO_NULL)
        {
            ++failed;
            continue;
        }
        if ((unsigned int)blocks[k] & 3)
            ++unaligned;
        for (i = 0; i < sizes[k]; ++i)
            blocks[k][i] = (unsigned char)(k * 31 + i);
        for (j = 0; j < T_BLOCKS; ++j)
            if (j != k && blocks[j] != JO_NULL && blocks[j] < blocks[k] + sizes[k] && blocks[k] < blocks[j] + sizes[j])
                ++overlaps;
        if ((op & 255) == 0)
            jo_reduce_memory_fragmentation();
    }
    for (k = 0; k < T_BLOCKS; ++k)
    {
        if (blocks[k] == JO_NULL)
            continue;
        for (i = 0; i < sizes[k]; ++i)
            if (blocks[k][i] != (unsigned char)(k * 31 + i))
            {
                ++corrupted;
                break;
            }
        jo_free(blocks[k]);
        blocks[k] = JO_NULL;
    }
    jo_reduce_memory_fragmentation();
    JT_EQ(corrupted, 0);
    JT_EQ(overlaps, 0);
    JT_EQ(unaligned, 0);
    JT_EQ(failed, 0);
    JT_EQ(JT_MEMORY_USAGE(), before);
    blocks[0] = (unsigned char *)jo_malloc(16);
    JT_TRUE(blocks[0] != JO_NULL);
    jo_free(blocks[0]);
    blocks[0] = JO_NULL;
}

static void                     t_list_stress(void)
{
    jo_list                     list;
    jo_node                     *node;
    int                         model[64];
    int                         count;
    int                         op;
    int                         i;
    int                         bad;
    int                         value;

    jt_group("stress.list");
    jo_list_init(&list);
    count = 0;
    bad = 0;
    for (op = 0; op < 2000; ++op)
    {
        switch (t_next(5))
        {
        case 0:
        case 1:
            if (count < 64)
            {
                i = (int)t_next((unsigned int)count + 1);
                value = op;
                jo_list_insert_at(&list, (jo_list_data)(void *)value, i);
                for (value = count; value > i; --value)
                    model[value] = model[value - 1];
                model[i] = op;
                ++count;
            }
            break;
        case 2:
            if (count > 0)
            {
                i = (int)t_next((unsigned int)count);
                jo_list_remove_at(&list, i);
                for (; i < count - 1; ++i)
                    model[i] = model[i + 1];
                --count;
            }
            break;
        case 3:
            if (count > 0)
            {
                jo_list_remove_first(&list);
                for (i = 0; i < count - 1; ++i)
                    model[i] = model[i + 1];
                --count;
            }
            break;
        default:
            if (count > 0)
            {
                jo_list_remove_last(&list);
                --count;
            }
            break;
        }
        // The list against the model, forward and backward
        if (list.count != count)
            ++bad;
        for (i = 0, node = list.first; node != JO_NULL && i < count; node = node->next, ++i)
            if ((int)node->data.ptr != model[i])
                ++bad;
        if (i != count || node != JO_NULL)
            ++bad;
        for (i = count - 1, node = list.last; node != JO_NULL && i >= 0; node = node->prev, --i)
            if ((int)node->data.ptr != model[i])
                ++bad;
        if (i != -1 || node != JO_NULL)
            ++bad;
    }
    JT_EQ(bad, 0);
    jo_list_clear(&list);
    JT_EQ(list.count, 0);
}

static void                     t_sprite_stress(void)
{
    static unsigned short       pixels[32 * 32];
    jo_img                      img;
    int                         ids[16];
    int                         base;
    int                         count;
    int                         k;
    int                         i;
    int                         bad;
    int                         usage;
    unsigned short              *vram;

    jt_group("stress.sprites");
    base = jo_get_last_sprite_id();
    usage = jo_sprite_usage_percent();
    bad = 0;
    for (k = 0; k < 40; ++k)
    {
        // Add sprites of several sizes, check the texture of each one in the VDP1 memory, free from a random one
        count = 1 + (int)t_next(16);
        for (i = 0; i < count; ++i)
        {
            img.width = (unsigned short)(8 * (1 + t_next(4)));
            img.height = (unsigned short)(1 + t_next(32));
            for (int p = 0; p < img.width * img.height; ++p)
                pixels[p] = (unsigned short)(0x8000 | ((p * 7 + i * 13 + k) & 0x7FFF));
            img.data = pixels;
            ids[i] = jo_sprite_add(&img);
            if (ids[i] != base + 1 + i)
                ++bad;
            vram = (unsigned short *)(JO_VDP1_VRAM + JO_MULT_BY_8(__jo_sprite_def[ids[i]].adr));
            for (int p = 0; p < img.width * img.height; ++p)
                if (vram[p] != pixels[p])
                {
                    ++bad;
                    break;
                }
            if (jo_sprite_get_width(ids[i]) != img.width || jo_sprite_get_height(ids[i]) != img.height)
                ++bad;
        }
        jo_sprite_free_from(ids[t_next((unsigned int)count)]);
        jo_sprite_free_from(base + 1);
    }
    JT_EQ(bad, 0);
    JT_EQ(jo_get_last_sprite_id(), base);
    JT_EQ(jo_sprite_usage_percent(), usage);
}

void                            t_stress(void)
{
    t_malloc_stress();
    t_list_stress();
    t_sprite_stress();
}
