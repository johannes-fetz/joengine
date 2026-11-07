/*
** Jo Sega Saturn Engine - regression test ROM: the test modules
*/

#ifndef __TESTS_H__
# define __TESTS_H__

/** @brief Pure logic: math, strings, lists, memory (t_logic.c) */
void    t_logic(void);

/** @brief State of the engine and of the hardware after the API calls (t_state.c) */
void    t_state(void);

/** @brief More math and lists (t_more.c) */
void    t_more(void);

/** @brief The bugs found by the review of the engine (t_bugs.c) */
void    t_bugs(void);

/** @brief Random sequences of allocations, lists and sprites (t_stress.c) */
void    t_stress(void);

/** @brief Leaks: cycles of create, use and free of each resource (t_leaks.c) */
void    t_leaks(void);

/** @brief The steps of the leak checks (t_leaks.c: after the unit checks, before their report) */
extern const jt_step    t_leak_steps[];

/** @brief Count of t_leak_steps */
extern const int        t_leak_step_count;

/** @brief The steps of the scenes and of the measures (scenes.c) */
extern const jt_step    t_scene_steps[];

/** @brief Count of t_scene_steps */
extern const int        t_scene_step_count;

/** @brief Initialize the resources of the scenes (scenes.c) */
void    t_scenes_init(void);

#endif /* !__TESTS_H__ */
