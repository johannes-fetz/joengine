/*
** Jo Sega Saturn Engine - regression test ROM: more math and lists, the vertex animations of the models
*/

#include "jt.h"
#include "tests.h"

#define T_F(X)                  ((int)((X) * 1000.0f))

static void             t_math_more(void)
{
    jo_vectorf          a;
    jo_vectorf          b;
    jo_vectorf          r;
    jo_matrixf          m;
    jo_matrixf          inv;
    jo_matrixf          p;
    jo_matrixf          t;
    jo_vector_fixed     p0;
    jo_vector_fixed     p1;
    jo_vector_fixed     p2;
    jo_vector_fixed     p3;
    jo_vector_fixed     bz;
    int                 i;
    int                 bad;

    jt_group("math.misc");
    JT_EQ(jo_fixed_pow(JO_FIXED_2, JO_FIXED_3), jo_int2fixed(8));
    JT_EQ(jo_fixed_pow(JO_FIXED_3, 0), JO_FIXED_1);
    JT_EQ(jo_fixed_pow(JO_FIXED_1_DIV_2, JO_FIXED_2), JO_FIXED_1 / 4);
    JT_EQ(jo_highest_bit(0), 0);
    JT_EQ(jo_highest_bit(1), 0);
    JT_EQ(jo_highest_bit(2), 1);
    JT_EQ(jo_highest_bit(0x80000000u), 31);
    for (bad = 0, i = 0; i < 31; ++i)
        if (jo_highest_bit((1u << i) | ((1u << i) - 1)) != (unsigned int)i)
            ++bad;
    JT_EQ(bad, 0);
    JT_EQ(jo_turn_toward(0, 90, 10), 10);
    JT_EQ(jo_turn_toward(10, 350, 30), 350);
    JT_EQ(jo_turn_toward(350, 10, 5), 355);
    JT_EQ(jo_turn_toward(5, 300, 10), 355);
    JT_EQ(jo_turn_toward(100, 100, 10), 100);
    JT_NEAR(jo_fixed_rad2deg(jo_fixed_deg2rad(jo_int2fixed(45))), jo_int2fixed(45), 1024);
    JT_NEAR(jo_fixed_wrap_to_pi(jo_float2fixed(4.0f)), jo_float2fixed(4.0f - 6.2831853f), 64);
    JT_NEAR(jo_tan(45), JO_FIXED_1, 64);
    JT_NEAR(jo_tan(-45), -JO_FIXED_1, 64);
    JT_TRUE(jo_square_intersect(0, 0, 10, 10, 5, 5, 10, 10));
    JT_TRUE(!jo_square_intersect(0, 0, 10, 10, 20, 20, 10, 10));
    // Bezier: t = 0 -> p0, t = 1 -> p3
    p0.x = 0; p0.y = 0; p0.z = 0;
    p1.x = jo_int2fixed(10); p1.y = 0; p1.z = 0;
    p2.x = jo_int2fixed(10); p2.y = jo_int2fixed(10); p2.z = 0;
    p3.x = jo_int2fixed(20); p3.y = jo_int2fixed(10); p3.z = jo_int2fixed(5);
    jo_vector_fixed_compute_bezier_point(0, p0, p1, p2, p3, &bz);
    JT_NEAR(bz.x, 0, 16);
    JT_NEAR(bz.y, 0, 16);
    jo_vector_fixed_compute_bezier_point(JO_FIXED_1, p0, p1, p2, p3, &bz);
    JT_NEAR(bz.x, p3.x, 64);
    JT_NEAR(bz.y, p3.y, 64);
    JT_NEAR(bz.z, p3.z, 64);

    jt_group("math.vectorf");
    a.x = 3.0f; a.y = 4.0f; a.z = 0.0f;
    JT_NEAR(T_F(jo_vectorf_length(&a)), 5000, 5);
    jo_vectorf_normalize(&a, &r);
    JT_NEAR(T_F(r.x), 600, 5);
    JT_NEAR(T_F(r.y), 800, 5);
    b.x = 0.0f; b.y = 0.0f; b.z = 1.0f;
    JT_NEAR(T_F(jo_vectorf_dot(&a, &b)), 0, 1);
    // Parallel vectors: 0 (not negative), opposite: PI
    JT_NEAR(T_F(jo_vectorf_angle_between_radf(&a, &a)), 0, 1);
    r.x = -a.x; r.y = -a.y; r.z = -a.z;
    JT_NEAR(T_F(jo_vectorf_angle_between_radf(&a, &r)), 3142, 2);
    jo_vectorf_cross(&a, &b, &r);
    JT_NEAR(T_F(r.x), 4000, 5);
    JT_NEAR(T_F(r.y), -3000, 5);
    JT_NEAR(T_F(r.z), 0, 5);

    jt_group("math.matrixf");
    // inverse(M) * M = identity, M = T * R
    jo_matrixf_rotation_y_rad(0.7f, &m);
    b.x = 10.0f; b.y = -5.0f; b.z = 3.0f;
    jo_matrixf_translation(&b, &t);
    jo_matrixf_mul(&m, &t, &p);
    jo_matrixf_invert_affine(&p, &inv);
    jo_matrixf_mul(&p, &inv, &m);
    JT_NEAR(T_F(m.m00), 1000, 5);
    JT_NEAR(T_F(m.m11), 1000, 5);
    JT_NEAR(T_F(m.m22), 1000, 5);
    JT_NEAR(T_F(m.m33), 1000, 5);
    JT_NEAR(T_F(m.m01), 0, 5);
    JT_NEAR(T_F(m.m02), 0, 5);
    JT_NEAR(T_F(m.m30), 0, 5);
    JT_NEAR(T_F(m.m31), 0, 5);
    JT_NEAR(T_F(m.m32), 0, 5);
    // A point moved by M then by the inverse comes back
    a.x = 1.0f; a.y = 2.0f; a.z = 3.0f;
    jo_matrixf_mul_pos(&p, &a, &r);
    jo_matrixf_mul_pos(&inv, &r, &b);
    JT_NEAR(T_F(b.x), 1000, 5);
    JT_NEAR(T_F(b.y), 2000, 5);
    JT_NEAR(T_F(b.z), 3000, 5);
}

static int              t_count_30;

static bool             t_is_30(jo_node *node, void *extra)
{
    (void)extra;
    if ((int)node->data.ptr == 30)
        ++t_count_30;
    return ((int)node->data.ptr == 30);
}

static void             t_list_more(void)
{
    jo_list             list;
    int                 i;

    jt_group("list.more");
    jo_list_init(&list);
    for (i = 0; i < 6; ++i)
        jo_list_add_ptr(&list, (void *)(i * 10));
    jo_list_remove_first(&list);
    JT_EQ(list.count, 5);
    JT_EQ((int)jo_list_first(&list)->data.ptr, 10);
    jo_list_remove_last(&list);
    JT_EQ(list.count, 4);
    JT_EQ((int)jo_list_last(&list)->data.ptr, 40);
    JT_TRUE(jo_list_any(&list, t_is_30, JO_NULL));
    JT_EQ(t_count_30, 1);
    jo_list_add_ptr(&list, (void *)30);
    jo_list_add_ptr(&list, (void *)30);
    jo_list_remove_all_value(&list, (jo_list_data)(void *)30);
    JT_EQ(list.count, 3);
    JT_TRUE(!jo_list_any(&list, t_is_30, JO_NULL));
    // Removing at the edges: the first and the last links stay right
    jo_list_remove_at(&list, 0);
    jo_list_remove_at(&list, list.count - 1);
    JT_EQ(list.count, 1);
    JT_TRUE(list.first == list.last);
    JT_TRUE(list.first != JO_NULL && list.first->prev == JO_NULL && list.first->next == JO_NULL);
    jo_list_remove_at(&list, 0);
    JT_EQ(list.count, 0);
    JT_TRUE(list.first == JO_NULL && list.last == JO_NULL);
    jo_list_clear(&list);
}

/** @brief Light gun API without a gun (the test ROM runs with a pad on the port 1): no gun found, no position, safe on
 *  every port; a peripheral with the ID of a gun is typed JoLightGun but has no position until its sensor latches */
static void             t_gun_more(void)
{
    int                 x;
    int                 y;
    int                 port;
    unsigned int        sr;
    jo_peripheral       saved;
    bool                typed;
    bool                seen;

    jt_group("input.gun");
    JT_EQ(JO_GUN_TRIGGER, JO_KEY_A);
    JT_EQ(JO_GUN_START_BUTTON, JO_KEY_START);
    JT_TRUE(jo_get_input_type(0) != JoLightGun);
    for (port = 0; port < JO_INPUT_MAX_DEVICE; ++port)
    {
        x = 123;
        y = 456;
        seen = jo_get_input_gun_position(port, &x, &y);
        JT_TRUE(!seen);
        JT_EQ(x, -1);
        JT_EQ(y, -1);
        JT_TRUE(!jo_calibrate_input_gun(port, 160, 120));
    }
    // (no gun: the HV counters are not latched by the external signal)
    JT_EQ(JO_VDP2_EXTEN & JO_VDP2_EXTEN_EXLTEN, 0);
    // A gun ID on the port 1 for a moment (the VBLANK copies the peripherals: interrupts masked)
    sr = __jo_core_disable_interrupts();
    saved = jo_inputs[0];
    jo_inputs[0].id = JO_PERIPHERAL_ID_GUN;
    typed = jo_get_input_type(0) == JoLightGun;
    seen = jo_get_input_gun_position(0, &x, &y);
    jo_inputs[0] = saved;
    __jo_core_restore_interrupts(sr);
    JT_TRUE(typed);
    JT_TRUE(!seen);
    JT_EQ(x, -1);
}

/* The models of cd/ made by gen_models.py: a square of 4 points and its vertex animation "wave" of 2 frames, a skeleton of
   2 bones and its skin */
#define T_MORPH_FRAMES          (2)
#define T_MORPH_TICKS           (8)
#define T_MORPH_HALF            (T_MORPH_TICKS / 2)
/* The moves of the points of the frame 1 (x 2^shift: 16.16 fixed) */
#define T_MORPH_MOVE_X          (3)
#define T_MORPH_MOVE_Y          (-2)
/* SKIN.JOM: its skin (the points of the child near the joint, the nearest first), the time of the bent pose */
#define T_SKIN_POINTS           (4)
#define T_SKIN_FIRST            (4)
#define T_SKIN_FAR              (6)
#define T_SKIN_BENT             (JO_3D_ANIMATION_TICKS_PER_SECOND)

/** @brief A model of the CD loaded, its wrong chunks ignored (an error expected) */
static jo_3d_model      *t_model_file(const char * const name, const bool wrong)
{
    jo_3d_model         *model;

    jt_expect_errors(wrong);
    model = jo_3d_load_model(name);
    jt_expect_errors(false);
    return (model);
}

/** @brief The point 1 of the square (1, -1) x 2^shift drawn at a time of the vertex animation: moved by the fraction of
 *  the frame 1 */
static bool             t_morph_point(const jo_3d_model * const model, const int time, const int shift, const int num,
                                      const int den)
{
    const jo_pos3D_fixed *p;

    jo_3d_push_matrix();
    jo_3d_model_draw_morph(model, jo_3d_model_find_morph(model, "WAVE"), time);
    jo_3d_pop_matrix();
    p = &model->morph_draw[0]->points[1];
    // (x 2^shift then / den: exact, the fractions are halves)
    return (p->x == (den + T_MORPH_MOVE_X * num) * (1 << shift) / den &&
            p->y == (-den + T_MORPH_MOVE_Y * num) * (1 << shift) / den && p->z == 0);
}

/** @brief How far a point of the skin moved from the point of the model */
static int              t_skin_move(const jo_3d_model * const model, const int point)
{
    const jo_pos3D_fixed *a;
    const jo_pos3D_fixed *b;

    a = &model->mesh->data.points[point];
    b = &model->morph_draw[0]->points[point];
    return (JO_ABS(b->x - a->x) + JO_ABS(b->y - a->y) + JO_ABS(b->z - a->z));
}

/** @brief Vertex animations (morph targets, "MRPH"): the frames interpolated, both paths of the shift (>= 8: a
 *  multiplication, < 8: a multiplication then a shift by 8), the loop, the wrong chunks dropped. The skin of a skeleton
 *  ("SKIN"): the points near the joint follow the parent, the nearest the most, the others rigid */
static void             t_morph_more(void)
{
    static const char * const files[] = {"MRPH12.JOM", "MRPH2.JOM"};
    static const int    shifts[] = {12, 2};
    static const char * const wrong[] = {"MRPHB1.JOM", "MRPHB2.JOM", "MRPHB3.JOM"};
    /* Time, then the fraction of the move of the frame 1 (numerator, denominator) */
    static const int    times[][3] = {{0, 0, 1}, {T_MORPH_HALF, 1, 2}, {T_MORPH_TICKS, 1, 1},
                                      {T_MORPH_TICKS + T_MORPH_HALF, 1, 2}, {T_MORPH_FRAMES * T_MORPH_TICKS, 0, 1}};
    jo_3d_model         *model;
    jo_3d_pose          pose;
    unsigned int        k;
    unsigned int        t;

    jt_group("model.morph");
    for (k = 0; k < sizeof(shifts) / sizeof(shifts[0]); ++k)
    {
        model = t_model_file(files[k], false);
        JT_TRUE(model != JO_NULL && model->morph_count == 1);
        if (model == JO_NULL)
            continue;
        JT_EQ(jo_3d_model_find_morph(model, "walk"), -1);
        // (a loop: the last frame goes back to the first one)
        JT_EQ(jo_3d_model_morph_duration(model, 0), T_MORPH_FRAMES * T_MORPH_TICKS);
        for (t = 0; t < sizeof(times) / sizeof(times[0]); ++t)
            JT_TRUE(t_morph_point(model, times[t][0], shifts[k], times[t][1], times[t][2]));
        // (the model itself doesn't move: the points are interpolated in its buffer)
        JT_EQ(model->mesh->data.points[1].x, 1 << shifts[k]);
        jo_3d_free_model(model);
    }
    // The model is loaded without a wrong vertex animation
    for (k = 0; k < sizeof(wrong) / sizeof(wrong[0]); ++k)
    {
        model = t_model_file(wrong[k], true);
        JT_TRUE(model != JO_NULL && model->morph_count == 0);
        if (model != JO_NULL)
            jo_3d_free_model(model);
    }
    jt_group("model.skin");
    model = t_model_file("SKIN.JOM", false);
    JT_TRUE(model != JO_NULL && model->skin_count == T_SKIN_POINTS);
    if (model != JO_NULL)
    {
        jo_3d_model_get_pose(model, 0, T_SKIN_BENT, &pose);
        jo_3d_push_matrix();
        jo_3d_model_draw_pose(model, &pose, JO_NULL, JO_NULL);
        jo_3d_pop_matrix();
        JT_EQ(t_skin_move(model, 0), 0);
        JT_TRUE(t_skin_move(model, T_SKIN_FIRST) > t_skin_move(model, T_SKIN_FAR));
        JT_TRUE(t_skin_move(model, T_SKIN_FAR) > 0);
        jo_3d_free_model(model);
    }
    model = t_model_file("SKINB.JOM", true);
    JT_TRUE(model != JO_NULL && model->skin_count == 0);
    if (model != JO_NULL)
        jo_3d_free_model(model);
}

void                    t_more(void)
{
    t_math_more();
    t_list_more();
    t_gun_more();
    t_morph_more();
}
