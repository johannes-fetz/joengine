/*
** Jo Sega Saturn Engine - regression test ROM: pure logic (math, strings, lists, memory)
*/

#include "jt.h"
#include "tests.h"

static void             t_fixed(void)
{
    jt_group("math.fixed");
    JT_EQ(jo_int2fixed(3), 3 * 65536);
    JT_EQ(jo_fixed2int(jo_int2fixed(-7)), -7);
    JT_EQ(jo_float2fixed(1.5f), 98304);
    JT_EQ(jo_fixed_mult(JO_FIXED_2, JO_FIXED_3), jo_int2fixed(6));
    JT_EQ(jo_fixed_mult(jo_int2fixed(-2), JO_FIXED_1_DIV_2), -JO_FIXED_1);
    JT_EQ(jo_fixed_mult(jo_int2fixed(300), jo_int2fixed(200)), jo_int2fixed(60000));
    JT_EQ(jo_fixed_div(jo_int2fixed(6), JO_FIXED_2), jo_int2fixed(3));
    JT_NEAR(jo_fixed_div(JO_FIXED_1, JO_FIXED_3), 21845, 1);
    JT_NEAR(jo_fixed_div(jo_int2fixed(-10), jo_int2fixed(4)), -163840, 1);
    JT_EQ(jo_int_div(100, 7), 14);
    JT_EQ(jo_int_div(-100, 7), -14);
    JT_EQ(jo_fixed_floor(jo_float2fixed(2.5f)), JO_FIXED_2);
    JT_EQ(jo_fixed_ceil(jo_float2fixed(2.5f)), JO_FIXED_3);
    JT_EQ(jo_fixed_floor(jo_float2fixed(-2.5f)), -JO_FIXED_3);
    JT_EQ(jo_lerp(0, JO_FIXED_2, JO_FIXED_1_DIV_2), JO_FIXED_1);
    JT_EQ(jo_fixed_wrap_to_180(jo_int2fixed(270)), jo_int2fixed(-90));
    JT_EQ(jo_fixed_wrap_to_180(jo_int2fixed(-450)), jo_int2fixed(-90));
    JT_EQ(jo_gcd(12, 18), 6);
    JT_EQ(jo_gcd(17, 5), 1);
}

static void             t_sqrt(void)
{
    unsigned int        hash;
    unsigned int        v;
    int                 i;
    int                 bad;

    jt_group("math.sqrt");
    JT_EQ(jo_sqrt(0), 0);
    JT_EQ(jo_sqrt(1), 1);
    JT_EQ(jo_sqrt(144), 12);
    JT_EQ(jo_sqrt(1000000), 1000);
    JT_NEAR(jo_fixed_sqrt(jo_int2fixed(2)), 92682, 16);
    JT_NEAR(jo_fixed_sqrt(jo_int2fixed(100)), jo_int2fixed(10), 16);
    JT_NEAR(jo_fixed_rsqrt(jo_int2fixed(4)), JO_FIXED_1_DIV_2, 128);
    // jo_sqrt(n * n) == n
    for (bad = 0, i = 0; i < 4000; i += 13)
        if (jo_sqrt((unsigned int)(i * i)) != (unsigned int)i)
            ++bad;
    JT_EQ(bad, 0);
    for (hash = JT_HASH_INIT, i = 1; i < 2000000; i += 7919)
    {
        v = jo_sqrt(i);
        hash = jt_hash(hash, &v, 4);
        v = (unsigned int)jo_fixed_sqrt(i);
        hash = jt_hash(hash, &v, 4);
        v = (unsigned int)jo_fixed_rsqrt(i);
        hash = jt_hash(hash, &v, 4);
    }
    jt_value("math.sqrt.sweep", hash);
}

static void             t_trig(void)
{
    unsigned int        hash;
    jo_fixed            s;
    jo_fixed            c;
    int                 deg;
    int                 bad;
    int                 v;

    jt_group("math.trig");
    JT_EQ(jo_sin(0), 0);
    JT_NEAR(jo_sin(90), JO_FIXED_1, 2);
    JT_NEAR(jo_sin(30), JO_FIXED_1_DIV_2, 64);
    JT_NEAR(jo_cos(180), -JO_FIXED_1, 2);
    JT_NEAR(jo_sin(-90), -JO_FIXED_1, 2);
    JT_NEAR(jo_sin(450), JO_FIXED_1, 2);
    JT_NEAR(jo_cos(60), JO_FIXED_1_DIV_2, 64);
    // sin^2 + cos^2 = 1, sincos = (sin, cos)
    for (bad = 0, deg = -360; deg < 720; deg += 7)
    {
        jo_sincos(deg, &s, &c);
        if (JO_ABS(jo_fixed_mult(s, s) + jo_fixed_mult(c, c) - JO_FIXED_1) > 64 || s != jo_sin(deg) || c != jo_cos(deg))
            ++bad;
    }
    JT_EQ(bad, 0);
    for (hash = JT_HASH_INIT, deg = 0; deg < 360; ++deg)
    {
        v = jo_sin(deg);
        hash = jt_hash(hash, &v, 4);
        v = jo_fixed_sin(jo_int2fixed(deg) / 57);
        hash = jt_hash(hash, &v, 4);
    }
    jt_value("math.sin.sweep", hash);
    for (hash = JT_HASH_INIT, deg = 0; deg < 360; deg += 5)
    {
        v = jo_atan2_degrees(jo_sin(deg), jo_cos(deg));
        hash = jt_hash(hash, &v, 4);
        v = jo_atan2_fixed(jo_sin(deg), jo_cos(deg));
        hash = jt_hash(hash, &v, 4);
    }
    jt_value("math.atan2.sweep", hash);
    JT_NEAR(jo_atan2_degrees(JO_FIXED_1, 0), 90, 1);
    JT_NEAR(jo_atan2_degrees(0, JO_FIXED_1), 0, 1);
    JT_NEAR(jo_atan2_degrees(jo_sin(45), jo_cos(45)), 45, 1);
    // tangent of a jo_angle: the same as jo_tan() on whole degrees, a fraction of degree, the infinite ones
    for (bad = 0, deg = -85; deg <= 85; deg += 5)
        if (jo_tan_angle(jo_DEGtoANG_int(deg)) != jo_tan(deg))
            ++bad;
    JT_EQ(bad, 0);
    JT_EQ(jo_tan_angle(0), 0);
    JT_NEAR(jo_tan_angle(JO_DEG_TO_ANGLE(45)), JO_FIXED_1, 64);
    JT_NEAR(jo_tan_angle(JO_DEG_TO_ANGLE(22.5)), 27146, 64);
    JT_EQ(jo_tan_angle(JO_DEG_TO_ANGLE(90)), JO_FIXED_MAX);
    JT_EQ(jo_tan_angle((jo_angle)JO_DEG_TO_ANGLE(270)), JO_FIXED_MIN);
}

static void             t_vectors(void)
{
    jo_vector4_fixed    a;
    jo_vector4_fixed    b;
    jo_vector4_fixed    r;
    jo_matrix           m;
    jo_matrix           n;
    jo_matrix           p;
    jo_vector_fixed     offset;
    unsigned int        hash;

    jt_group("math.vector");
    a.x = JO_FIXED_1; a.y = 0; a.z = 0; a.w = JO_FIXED_1;
    b.x = 0; b.y = JO_FIXED_1; b.z = 0; b.w = JO_FIXED_1;
    jo_vector4_fixed_cross(&a, &b, &r);
    JT_EQ(r.x, 0);
    JT_EQ(r.y, 0);
    JT_EQ(r.z, JO_FIXED_1);
    JT_EQ(jo_vector4_fixed_dot(&a, &b), 0);
    JT_EQ(jo_vector4_fixed_dot(&a, &a), JO_FIXED_1);
    jt_group("math.matrix");
    jo_matrix_identity(&m);
    JT_EQ(m.m00, JO_FIXED_1);
    JT_EQ(m.m11, JO_FIXED_1);
    JT_EQ(m.m01, 0);
    jo_matrix_rotation_z_rad(1.5707963f, &n);
    jo_matrix_mul(&m, &n, &p);
    hash = jt_hash(JT_HASH_INIT, &p, sizeof(p));
    jo_matrix_rotation_x_rad(0.3f, &n);
    jo_matrix_mul(&p, &n, &m);
    offset.x = jo_int2fixed(10);
    offset.y = jo_int2fixed(-20);
    offset.z = jo_int2fixed(30);
    jo_matrix_translation(&offset, &n);
    jo_matrix_mul(&m, &n, &p);
    hash = jt_hash(hash, &p, sizeof(p));
    jo_matrix_mul_vector4(&p, &a, &r);
    hash = jt_hash(hash, &r, sizeof(r));
    jt_value("math.matrix.chain", hash);
}

static void             t_strings(void)
{
    char                buf[96];
    unsigned char       bytes[32];
    int                 i;

    jt_group("tools.string");
    JT_EQ(jo_strlen(""), 0);
    JT_EQ(jo_strlen("hello"), 5);
    JT_EQ(jo_strcmp("abc", "abc"), 0);
    JT_TRUE(jo_strcmp("abc", "abd") < 0);
    JT_TRUE(jo_strcmp("b", "a") > 0);
    JT_TRUE(jo_string_equals("JO", "JO"));
    JT_TRUE(!jo_string_equals("JO", "JOE"));
    JT_TRUE(jo_endwith("MODEL.BIN", ".BIN"));
    JT_TRUE(!jo_endwith("MODEL.BIN", ".TGA"));
    JT_EQ(jo_tools_atoi("-123"), -123);
    JT_EQ(jo_tools_atoi("4567"), 4567);
    jt_group("tools.sprintf");
    jo_sprintf(buf, "%d|%u|%x|%X|%c|%s|%%", -42, 42u, 0xbeefu, 0xbeefu, 'Z', "ok");
    JT_EQ(jo_strcmp(buf, "-42|42|beef|BEEF|Z|ok|%"), 0);
    jo_sprintf(buf, "[%5d][%-5d][%05d][%3s]", 42, 42, -42, "a");
    JT_EQ(jo_strcmp(buf, "[   42][42   ][-0042][  a]"), 0);
    JT_EQ(jo_sprintf(buf, "%s", "abcdef"), 6);
    jt_group("tools.memory");
    for (i = 0; i < 32; ++i)
        bytes[i] = (unsigned char)i;
    jo_memcpy(buf, bytes, 32);
    JT_EQ(jo_memcmp(buf, bytes, 32), 0);
    jo_memset(buf + 8, 0xAA, 8);
    JT_EQ((unsigned char)buf[7], 7);
    JT_EQ((unsigned char)buf[8], 0xAA);
    JT_EQ((unsigned char)buf[15], 0xAA);
    JT_EQ((unsigned char)buf[16], 16);
    JT_TRUE(jo_memcmp(buf, bytes, 32) != 0);
    JT_EQ(jo_swap_endian_int(0x11223344), 0x44332211);
    JT_EQ(jo_swap_endian_ushort(0x1122), 0x2211);
}

static void             t_list(void)
{
    jo_list             list;
    jo_node             *node;
    int                 sum;
    int                 i;

    jt_group("list");
    jo_list_init(&list);
    JT_EQ(list.count, 0);
    for (i = 1; i <= 5; ++i)
        jo_list_add_ptr(&list, (void *)(i * 10));
    JT_EQ(list.count, 5);
    JT_EQ((int)jo_list_first(&list)->data.ptr, 10);
    JT_EQ((int)jo_list_last(&list)->data.ptr, 50);
    JT_EQ((int)jo_list_at(&list, 2)->data.ptr, 30);
    jo_list_remove_at(&list, 2);
    JT_EQ(list.count, 4);
    JT_EQ((int)jo_list_at(&list, 2)->data.ptr, 40);
    jo_list_insert_at(&list, (jo_list_data)(void *)25, 2);
    JT_EQ((int)jo_list_at(&list, 2)->data.ptr, 25);
    for (sum = 0, node = list.first; node != JO_NULL; node = node->next)
        sum += (int)node->data.ptr;
    JT_EQ(sum, 10 + 20 + 25 + 40 + 50);
    jo_list_clear(&list);
    JT_EQ(list.count, 0);
    JT_TRUE(list.first == JO_NULL);
}

static void             t_malloc(void)
{
    void                *blocks[24];
    int                 before;
    int                 i;
    unsigned char       *p;
    bool                ok;

    jt_group("malloc");
    before = JT_MEMORY_USAGE();
    p = (unsigned char *)jo_malloc(1000);
    JT_TRUE(p != JO_NULL);
    for (i = 0; i < 1000; ++i)
        p[i] = (unsigned char)i;
    for (ok = true, i = 0; i < 1000; ++i)
        ok = ok && p[i] == (unsigned char)i;
    JT_TRUE(ok);
    jo_free(p);
    // Allocations of different sizes, half freed, then all: the usage goes back to its value before
    for (i = 0; i < 24; ++i)
        blocks[i] = jo_malloc(64 + i * 97);
    for (ok = true, i = 0; i < 24; ++i)
        ok = ok && blocks[i] != JO_NULL;
    JT_TRUE(ok);
    for (i = 0; i < 24; i += 2)
        jo_free(blocks[i]);
    for (i = 0; i < 24; i += 2)
        blocks[i] = jo_malloc(32 + i * 13);
    for (i = 0; i < 24; ++i)
        jo_free(blocks[i]);
    jo_reduce_memory_fragmentation();
    JT_EQ(JT_MEMORY_USAGE(), before);
    // Two allocations don't overlap
    p = (unsigned char *)jo_malloc(256);
    blocks[0] = jo_malloc(256);
    JT_TRUE((unsigned char *)blocks[0] >= p + 256 || (unsigned char *)blocks[0] + 256 <= p);
    jo_free(blocks[0]);
    jo_free(p);
}

void                    t_logic(void)
{
    t_fixed();
    t_sqrt();
    t_trig();
    t_vectors();
    t_strings();
    t_list();
    t_malloc();
}
