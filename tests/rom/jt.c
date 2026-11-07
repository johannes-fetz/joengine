/*
** Jo Sega Saturn Engine - regression test ROM: records and barcode
**
** Barcode: blocks of 4 x 4 pixels from (0, 0), 80 blocks per line, 28 lines (280 bytes, the most significant bit first,
** white = 1). Frame: "JOT1", kind, stage, page, pages, length (2 bytes, little endian), payload, CRC-16 (CCITT) of
** everything before it. Kinds: 'R' a page of a report, 'K' the marker of a scene, 'Z' the end
** Records of a report: 'G' group, 'F' failure, 'V' value, 'M' metric, 'B' blob, 'I' info, 'S' summary (see jt_add_*())
*/

#include "jt.h"

#define JT_BLOCK                (4)
#define JT_COLUMNS              (80)
#define JT_LINES                (28)
#define JT_FRAME_BYTES          (JT_COLUMNS * JT_LINES / 8)
#define JT_HEADER_BYTES         (10)
#define JT_PAGE_BYTES           (JT_FRAME_BYTES - JT_HEADER_BYTES - 2)
#define JT_REPORT_SIZE          (12 * 1024)
#define JT_FRAMES_PER_PAGE      (3)
#define JT_MARKER_FRAMES        (3)

static unsigned char            jt_report[JT_REPORT_SIZE];
static int                      jt_report_length;
static bool                     jt_report_overflow;
static int                      jt_checks;
static int                      jt_failures;
static int                      jt_total_checks;
static int                      jt_total_failures;
static int                      jt_stage;
static unsigned char            jt_frame[JT_FRAME_BYTES];

static const jt_step            *jt_steps;
static int                      jt_step_count;
static int                      jt_step_index;
static int                      jt_step_frame;

/*
** Records
*/

static void                     jt_put(const void * const data, const int size)
{
    if (jt_report_length + size > JT_REPORT_SIZE - 16)
    {
        jt_report_overflow = true;
        return ;
    }
    jo_memcpy(jt_report + jt_report_length, data, size);
    jt_report_length += size;
}

static void                     jt_put_u8(const unsigned int value)
{
    unsigned char               b;

    b = (unsigned char)value;
    jt_put(&b, 1);
}

static void                     jt_put_u16(const unsigned int value)
{
    jt_put_u8(value);
    jt_put_u8(value >> 8);
}

static void                     jt_put_u32(const unsigned int value)
{
    jt_put_u16(value);
    jt_put_u16(value >> 16);
}

static void                     jt_put_string(const char * const text)
{
    int                         len;

    len = jo_strlen(text);
    if (len > 63)
        len = 63;
    jt_put_u8(len);
    jt_put(text, len);
}

unsigned int                    jt_hash(unsigned int hash, const void * const data, int size)
{
    const unsigned char         *p;

    for (p = (const unsigned char *)data; size > 0; --size, ++p)
        hash = (hash ^ *p) * 16777619u;
    return (hash);
}

void                            jt_group(const char * const name)
{
    jt_put_u8('G');
    jt_put_string(name);
}

void                            jt_check(const bool ok, const char * const text, const int line, const int actual, const int expected)
{
    ++jt_checks;
    if (ok)
        return ;
    ++jt_failures;
    jt_put_u8('F');
    jt_put_u16(line);
    jt_put_u32(actual);
    jt_put_u32(expected);
    jt_put_string(text);
}

void                            jt_value(const char * const name, const unsigned int value)
{
    jt_put_u8('V');
    jt_put_u32(value);
    jt_put_string(name);
}

void                            jt_metric(const char * const name, const unsigned int value)
{
    jt_put_u8('M');
    jt_put_u32(value);
    jt_put_string(name);
}

void                            jt_blob(const char * const name, const void * const data, const int size)
{
    jt_put_u8('B');
    jt_put_u16(size);
    jt_put(data, size);
    jt_put_string(name);
}

static bool                     jt_errors_expected;
static int                      jt_errors;

int                             jt_expect_errors(const bool expected)
{
    int                         count;

    count = jt_errors;
    jt_errors = 0;
    jt_errors_expected = expected;
    return (count);
}

#ifdef JO_DEBUG
static void                     jt_error_handler(const char *message, const char *function)
{
    char                        text[64];

    ++jt_errors;
    if (jt_errors_expected)
        return ;
    ++jt_checks;
    ++jt_failures;
    jo_snprintf(text, sizeof(text), "jo_core_error %s: %s", function, message);
    jt_put_u8('F');
    jt_put_u16(0);
    jt_put_u32(0);
    jt_put_u32(0);
    jt_put_string(text);
}
#endif

void                            jt_info(const char * const text)
{
    jt_put_u8('I');
    jt_put_string(text);
}

/*
** Barcode
*/

static unsigned short           jt_crc16(const unsigned char *data, int size)
{
    unsigned int                crc;
    int                         bit;

    for (crc = 0xFFFF; size > 0; --size, ++data)
    {
        crc ^= (unsigned int)*data << 8;
        for (bit = 0; bit < 8; ++bit)
            crc = (crc & 0x8000) ? ((crc << 1) ^ 0x1021) : (crc << 1);
    }
    return ((unsigned short)crc);
}

void                            jt_restore_nbg1(void)
{
    jo_vdp2_set_nbg1_16bits_bitmap();
    jo_clear_background(JO_COLOR_Black);
}

static void                     jt_draw_frame(const int kind, const int page, const int pages, const unsigned char * const payload, const int length)
{
    unsigned short              crc;
    int                         i;
    int                         x;
    int                         y;
    int                         bit;
    jo_color                    color;

    JO_ZERO(i);
    jo_memset(jt_frame, 0, sizeof(jt_frame));
    jo_memcpy(jt_frame, "JOT1", 4);
    jt_frame[4] = (unsigned char)kind;
    jt_frame[5] = (unsigned char)jt_stage;
    jt_frame[6] = (unsigned char)page;
    jt_frame[7] = (unsigned char)pages;
    jt_frame[8] = (unsigned char)length;
    jt_frame[9] = (unsigned char)(length >> 8);
    jo_memcpy(jt_frame + JT_HEADER_BYTES, payload, length);
    crc = jt_crc16(jt_frame, JT_HEADER_BYTES + length);
    jt_frame[JT_HEADER_BYTES + length] = (unsigned char)(crc >> 8);
    jt_frame[JT_HEADER_BYTES + length + 1] = (unsigned char)crc;
    for (bit = 0; bit < JT_FRAME_BYTES * 8; ++bit)
    {
        color = (jt_frame[bit >> 3] & (0x80 >> (bit & 7))) ? JO_COLOR_White : JO_COLOR_Black;
        x = (bit % JT_COLUMNS) * JT_BLOCK;
        y = (bit / JT_COLUMNS) * JT_BLOCK;
        for (i = 0; i < JT_BLOCK * JT_BLOCK; ++i)
            jo_vdp2_put_pixel_bitmap_nbg1(x + (i & 3), y + (i >> 2), color);
    }
}

static void                     jt_clear_text(void)
{
    int                         line;

    for (line = 14; line < 28; ++line)
        jo_clear_screen_line(line);
}

/*
** Steps
*/

static void                     jt_print_summary(void)
{
    int                         p;
    int                         line;
    int                         len;
    int                         text_len;
    char                        text[40];

    jt_clear_text();
    jo_printf(1, 14, "STAGE %d: %d CHECKS, %d FAILED  ", jt_stage, jt_checks, jt_failures);
    jo_printf(1, 15, "TOTAL: %d CHECKS, %d FAILED  ", jt_total_checks, jt_total_failures);
    // The first failures
    for (p = 0, line = 17; p < jt_report_length && line < 27;)
    {
        switch (jt_report[p])
        {
        case 'G':
        case 'I':
            p += 2 + jt_report[p + 1];
            break;
        case 'F':
            text_len = jt_report[p + 11];
            len = text_len > 36 ? 36 : text_len;
            jo_memcpy(text, jt_report + p + 12, len);
            text[len] = '\0';
            jo_printf(1, line++, "%s", text);
            p += 12 + text_len;
            break;
        case 'V':
        case 'M':
            p += 6 + jt_report[p + 5];
            break;
        case 'B':
            len = jt_report[p + 1] | (jt_report[p + 2] << 8);
            p += 3 + len;
            p += 1 + jt_report[p];
            break;
        default:
            p = jt_report_length;
            break;
        }
    }
}

static bool                     jt_report_step(const int frame)
{
    int                         pages;
    int                         page;

    if (frame == 0)
    {
        jt_total_checks += jt_checks;
        jt_total_failures += jt_failures;
        if (jt_report_overflow)
            jt_info("report overflow");
        jt_put_u8('S');
        jt_put_u32(jt_checks);
        jt_put_u32(jt_failures);
        jt_print_summary();
    }
    pages = (jt_report_length + JT_PAGE_BYTES - 1) / JT_PAGE_BYTES;
    // Twice each page
    if (frame >= 2 * pages * JT_FRAMES_PER_PAGE)
    {
        jt_report_length = 0;
        jt_report_overflow = false;
        jt_checks = 0;
        jt_failures = 0;
        ++jt_stage;
        return (true);
    }
    if (frame % JT_FRAMES_PER_PAGE == 0)
    {
        page = (frame / JT_FRAMES_PER_PAGE) % pages;
        jt_draw_frame('R', page, pages, jt_report + page * JT_PAGE_BYTES,
                      JO_MIN(JT_PAGE_BYTES, jt_report_length - page * JT_PAGE_BYTES));
    }
    return (false);
}

static void                     jt_marker(const char * const name)
{
    unsigned char               payload[72];
    int                         len;

    len = jo_strlen(name);
    if (len > 63)
        len = 63;
    payload[0] = (unsigned char)len;
    jo_memcpy(payload + 1, name, len);
    payload[1 + len] = JT_CAPTURE_FRAME;
    jt_draw_frame('K', 0, 1, payload, len + 2);
}

static void                     jt_tick(void)
{
    const jt_step               *step;
    bool                        done;
    int                         frame;

    if (jt_step_index >= jt_step_count)
    {
        if (jt_step_frame++ == 0)
        {
            jt_draw_frame('Z', 0, 1, (const unsigned char *)"END", 3);
            jt_clear_text();
            jo_printf(1, 14, "ALL STEPS DONE: %d CHECKS, %d FAILED  ", jt_total_checks, jt_total_failures);
        }
        return ;
    }
    step = &jt_steps[jt_step_index];
    frame = jt_step_frame++;
    done = false;
    switch (step->kind)
    {
    case JT_STEP_RUN:
        if (step->setup != JO_NULL)
            step->setup();
        done = true;
        break;
    case JT_STEP_REPORT:
        done = jt_report_step(frame);
        break;
    case JT_STEP_SCENE:
        if (frame < JT_MARKER_FRAMES)
        {
            if (frame == 0)
            {
                jt_marker(step->name);
                jt_clear_text();
                jo_printf(1, 14, "SCENE %s  ", step->name);
            }
            break;
        }
        if (frame == JT_MARKER_FRAMES)
        {
            jo_clear_background(JO_COLOR_Black);
            jt_clear_text();
            if (step->setup != JO_NULL)
                step->setup();
        }
        if (step->draw != JO_NULL)
            step->draw();
        if (frame >= JT_MARKER_FRAMES + (step->frames > 0 ? step->frames : JT_SCENE_FRAMES))
        {
            if (step->cleanup != JO_NULL)
                step->cleanup();
            done = true;
        }
        break;
    case JT_STEP_BENCH:
        if (frame == 0)
        {
            jt_clear_text();
            jo_printf(1, 14, "BENCH %s  ", step->name);
            if (step->setup != JO_NULL)
                step->setup();
        }
        if (step->draw != JO_NULL)
            step->draw();
        if (frame >= step->frames)
        {
            if (step->measure != JO_NULL)
                step->measure();
            if (step->cleanup != JO_NULL)
                step->cleanup();
            done = true;
        }
        break;
    }
    if (done)
    {
        ++jt_step_index;
        jt_step_frame = 0;
    }
}

void                            jt_run(const jt_step * const steps, const int count)
{
    jt_steps = steps;
    jt_step_count = count;
#ifdef JO_DEBUG
    // (an error of the engine is a failure of the report, the next steps run)
    jo_core_set_error_handler(jt_error_handler);
#endif
    jt_restore_nbg1();
    jo_core_add_callback(jt_tick);
    jo_core_run();
}
