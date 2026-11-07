/*
** Jo Sega Saturn Engine - regression test ROM
**
** The results are drawn as a barcode at the top of the screen (NBG1 bitmap); tests/run_tests.py reads it from the video
** recorded by Mednafen. See tests/README.md
*/

#ifndef __JT_H__
# define __JT_H__

# include <jo/jo.h>

/** @brief Count of the elements of an array */
# define JT_COUNT(ARRAY)            ((int)(sizeof(ARRAY) / sizeof((ARRAY)[0])))

/** @brief Start a group of checks (the name of the failures) */
void            jt_group(const char * const name);

/** @brief Count a check, record a failure (Internal usage, see JT_TRUE(), JT_EQ(), JT_NEAR()) */
void            jt_check(const bool ok, const char * const text, const int line, const int actual, const int expected);

/** @brief The condition must be true */
# define JT_TRUE(COND)              do { bool __ok = !!(COND); jt_check(__ok, #COND, __LINE__, __ok, 1); } while (0)

/** @brief Two integers must be equal */
# define JT_EQ(A, B)                do { int __a = (A); int __b = (B); jt_check(__a == __b, #A " == " #B, __LINE__, __a, __b); } while (0)

/** @brief Two integers must be equal +/- TOLERANCE */
# define JT_NEAR(A, B, TOLERANCE)   do { int __a = (A); int __b = (B); jt_check(JO_ABS(__a - __b) <= (TOLERANCE), #A " ~ " #B, __LINE__, __a, __b); } while (0)

/** @brief A value compared with tests/golden/rom_values.json (characterization: --approve records the new value) */
void            jt_value(const char * const name, const unsigned int value);

/** @brief A measure compared with the budgets of tests/golden/budgets.json (perf.*: CPU ticks, ram.*: bytes or %) */
void            jt_metric(const char * const name, const unsigned int value);

/** @brief Bytes compared with tests/golden/rom_values.json (the VDP2 registers: the report names the registers changed) */
void            jt_blob(const char * const name, const void * const data, const int size);

/** @brief The errors of jo_core_error() are expected (counted) or not (a failure of the report): the ROM goes on, no error
 *  screen
 *  @return The errors counted since the last call */
int             jt_expect_errors(const bool expected);

/** @brief A message shown in the report */
void            jt_info(const char * const text);

/** @brief Memory usage of jo_malloc() (JO_DEBUG only in the engine: 0 without it) */
# ifdef JO_DEBUG
#  define JT_MEMORY_USAGE()         jo_memory_usage_percent()
#  define JT_MEMORY_FRAGMENTS()     jo_memory_fragmentation()
# else
#  define JT_MEMORY_USAGE()         (0)
#  define JT_MEMORY_FRAGMENTS()     (0)
# endif

/** @brief FNV-1a hash */
unsigned int    jt_hash(unsigned int hash, const void * const data, int size);

/** @brief Initial value of jt_hash() */
# define JT_HASH_INIT               (2166136261u)

/** @brief Kind of step */
typedef enum
{
    /** @brief run() once (checks, values), no frame */
    JT_STEP_RUN,
    /** @brief Show the records of the previous steps (a report), then clear them */
    JT_STEP_REPORT,
    /** @brief A picture compared with tests/golden/rom/<variant>/<name>.png: a marker (the name), setup(), then draw() each
     *  frame; the harness captures the frame JT_CAPTURE_FRAME after the marker. cleanup() at the end */
    JT_STEP_SCENE,
    /** @brief A measure: setup(), draw() during `frames` frames, then measure() (jt_metric()) and cleanup() */
    JT_STEP_BENCH
}               jt_step_kind;

/** @brief A step of the test ROM */
typedef struct
{
    jt_step_kind    kind;
    const char      *name;
    void            (*setup)(void);
    void            (*draw)(void);
    void            (*measure)(void);
    void            (*cleanup)(void);
    int             frames;
}               jt_step;

/** @brief Frames of a scene after its marker */
# define JT_SCENE_FRAMES            (24)

/** @brief Frame captured by the harness (after the last frame of the marker) */
# define JT_CAPTURE_FRAME           (20)

/** @brief Run the steps (calls jo_core_run()) */
void            jt_run(const jt_step * const steps, const int count);

/** @brief The barcode is drawn on the NBG1 bitmap: a scene that changes NBG1 calls it in its cleanup */
void            jt_restore_nbg1(void);

#endif /* !__JT_H__ */
