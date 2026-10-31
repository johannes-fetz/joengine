/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2026, Johannes Fetz (johannesfetz@gmail.com)
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the Johannes Fetz nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL Johannes Fetz BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
** The sound driver of the 68000 (JO_COMPILE_WITH_AUDIO_DRIVER_MODULE): it runs the commands of the SH-2 (see
** protocol.h) and the glides of the pitches (each millisecond: timer A of the SCSP).
**
** Built by sound_driver/build.sh (m68k-linux-gnu-gcc): the engine includes the binary (audio_driver_68k.h), a
** game doesn't need a 68000 compiler.
*/

#include "protocol.h"

/** @brief The registers of the SCSP, seen by the 68000 */
#define SCSP_SLOT(SLOT, OFFSET)     (*(volatile unsigned short *)(0x100000 + ((SLOT) << 5) + (OFFSET)))
#define SCSP_COMMON(OFFSET)         (*(volatile unsigned short *)(0x100400 + (OFFSET)))
#define RAM16(OFFSET)               (*(volatile unsigned short *)(OFFSET))

#define SLOTS                       (32)
#define KEY_ON_EXECUTE              (0x1000)
#define KEY_ON                      (0x0800)
#define KEY_MASK                    (0x07FF)
#define SLOT_PITCH                  (0x10)
/** @brief Timer A: its counter (bits 7-0) and its prescaler (bits 10-8: 1 count every 2^n samples) */
#define COMMON_TIMER_A              (0x18)
/** @brief Pending interrupts of the sound CPU (read), reset (write 1) */
#define COMMON_SCIPD                (0x20)
#define COMMON_SCIRE                (0x22)
#define SCIPD_TIMER_A               (0x0040)
#define SCIPD_SAMPLE                (0x0400)
/** @brief Timer A overflows after 44 samples (about 1 ms): it counts from this value to 0xFF */
#define TIMER_A_RELOAD              (0x100 - 44)

/** @brief The glides of the slots */
static short                        glide_pitch[SLOTS];
static short                        glide_target[SLOTS];
static short                        glide_speed[SLOTS];
static unsigned int                 glide_active;

/** @brief Wait for the start of 2 samples: the keys written before are run by the SCSP */
static void                         wait_samples(void)
{
    int                             k;

    for (k = 0; k < 2; ++k)
    {
        SCSP_COMMON(COMMON_SCIRE) = SCIPD_SAMPLE;
        while (!(SCSP_COMMON(COMMON_SCIPD) & SCIPD_SAMPLE))
            ;
    }
}

/** @brief The keys of all the slots run at the next sample */
static void                         key_execute(void)
{
    SCSP_SLOT(0, 0) = SCSP_SLOT(0, 0) | KEY_ON_EXECUTE;
}

static void                         glide_stop(const unsigned int slot)
{
    glide_active &= ~(1U << slot);
}

/** @brief Run the commands written by the SH-2 since the last call */
static void                         run_commands(void)
{
    unsigned int                    head;
    unsigned int                    tail;
    unsigned int                    i;
    unsigned int                    slot;
    unsigned int                    k;
    volatile unsigned short         *cmd;
    int                             keys;

    head = RAM16(JO_DRIVER_HEAD) & (JO_DRIVER_QUEUE_LENGTH - 1);
    tail = RAM16(JO_DRIVER_TAIL);
    if (head == tail)
        return;
    // The slots to play are keyed off first (a slot that played its sound to the end is still keyed on: a key on would
    // be ignored), and the SCSP must run the key off before the key on
    keys = 0;
    for (i = tail; i != head; i = (i + 1) & (JO_DRIVER_QUEUE_LENGTH - 1))
    {
        cmd = (volatile unsigned short *)(JO_DRIVER_QUEUE + i * JO_DRIVER_COMMAND_SIZE);
        if ((cmd[0] >> 8) == JO_DRIVER_KEY_ON)
        {
            slot = cmd[0] & 0x1F;
            SCSP_SLOT(slot, 0) = SCSP_SLOT(slot, 0) & KEY_MASK;
            keys = 1;
        }
    }
    if (keys)
    {
        key_execute();
        wait_samples();
    }
    keys = 0;
    for (i = tail; i != head; i = (i + 1) & (JO_DRIVER_QUEUE_LENGTH - 1))
    {
        cmd = (volatile unsigned short *)(JO_DRIVER_QUEUE + i * JO_DRIVER_COMMAND_SIZE);
        slot = cmd[0] & 0x1F;
        switch (cmd[0] >> 8)
        {
            case JO_DRIVER_KEY_ON:
                glide_stop(slot);
                for (k = 1; k < 12; ++k)
                    SCSP_SLOT(slot, k * 2) = cmd[k + 1];
                SCSP_SLOT(slot, 0) = (cmd[1] & KEY_MASK) | KEY_ON;
                keys = 1;
                break;
            case JO_DRIVER_KEY_OFF:
                glide_stop(slot);
                SCSP_SLOT(slot, 0) = SCSP_SLOT(slot, 0) & KEY_MASK;
                keys = 1;
                break;
            case JO_DRIVER_WRITE:
                if (cmd[1] == SLOT_PITCH)
                    glide_stop(slot);
                SCSP_SLOT(slot, cmd[1] & 0x1E) = cmd[2];
                break;
            case JO_DRIVER_GLIDE:
                glide_target[slot] = (short)cmd[1];
                glide_speed[slot] = (short)cmd[2];
                glide_pitch[slot] = (short)cmd[3];
                glide_active |= 1U << slot;
                SCSP_SLOT(slot, SLOT_PITCH) = JO_DRIVER_PITCH_REGISTER(glide_pitch[slot]);
                break;
        }
    }
    if (keys)
        key_execute();
    RAM16(JO_DRIVER_TAIL) = head;
}

/** @brief Each millisecond: the glides */
static void                         tick(void)
{
    unsigned int                    slot;
    short                           diff;
    short                           step;

    SCSP_COMMON(COMMON_TIMER_A) = TIMER_A_RELOAD;
    SCSP_COMMON(COMMON_SCIRE) = SCIPD_TIMER_A;
    RAM16(JO_DRIVER_TICKS) = RAM16(JO_DRIVER_TICKS) + 1;
    if (!glide_active)
        return;
    for (slot = 0; slot < SLOTS; ++slot)
    {
        if (!(glide_active & (1U << slot)))
            continue;
        diff = glide_target[slot] - glide_pitch[slot];
        // (16 x 16 bits: muls)
        step = (short)(((int)diff * (int)glide_speed[slot]) >> 15);
        if (step == 0)
            step = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
        glide_pitch[slot] += step;
        if (glide_pitch[slot] == glide_target[slot])
            glide_stop(slot);
        SCSP_SLOT(slot, SLOT_PITCH) = JO_DRIVER_PITCH_REGISTER(glide_pitch[slot]);
    }
}

void                                driver_main(void)
{
    glide_active = 0;
    SCSP_COMMON(COMMON_TIMER_A) = TIMER_A_RELOAD;
    SCSP_COMMON(COMMON_SCIRE) = SCIPD_TIMER_A;
    RAM16(JO_DRIVER_STATE) = JO_DRIVER_MAGIC;
    for (;;)
    {
        run_commands();
        if (SCSP_COMMON(COMMON_SCIPD) & SCIPD_TIMER_A)
            tick();
    }
}

/*
** END OF FILE
*/
