/*
** Jo Sega Saturn Engine - regression test ROM
**
** Run by tests/run_tests.py (see tests/README.md): the checks of the engine (stage 0), the measures (stage 1), then the
** scenes compared with the golden images
*/

#include "jt.h"
#include "tests.h"

static jt_step          steps[64];

static void             unit_tests(void)
{
    // (t_state() first: its values are the state of the engine after jo_core_init(), not after the other checks)
    t_logic();
    t_state();
    t_more();
    t_bugs();
    t_stress();
}

static void             memory_metrics(void)
{
    jt_metric("ram.heap_percent", JT_MEMORY_USAGE());
    jt_metric("ram.heap_fragments", JT_MEMORY_FRAGMENTS());
    jt_metric("ram.sprite_percent", jo_sprite_usage_percent());
    jt_metric("ram.sprite_count", jo_sprite_count());
}

void                    jo_main(void)
{
    int                 count;
    int                 i;

    jo_core_init(JO_COLOR_Black);
    t_scenes_init();
    count = 0;
    steps[count++] = (jt_step){JT_STEP_RUN, "unit", unit_tests, JO_NULL, JO_NULL, JO_NULL, 0};
    for (i = 0; i < t_leak_step_count; ++i)
        steps[count++] = t_leak_steps[i];
    steps[count++] = (jt_step){JT_STEP_REPORT, "unit", JO_NULL, JO_NULL, JO_NULL, JO_NULL, 0};
    steps[count++] = (jt_step){JT_STEP_RUN, "memory", memory_metrics, JO_NULL, JO_NULL, JO_NULL, 0};
    for (i = 0; i < t_scene_step_count && count < JT_COUNT(steps); ++i)
        steps[count++] = t_scene_steps[i];
    jt_run(steps, count);
}
