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
** INCLUDES
*/
#include <stdbool.h>
#include "jo/conf.h"
#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/math.h"
#include "jo/core.h"
#include "jo/input.h"

/*
** The SMPC system status and the peripheral data are requested once per frame
** (INTBACK command at the end of the VBLANK-IN interrupt, like the Sega libraries). The SMPC raises an interrupt every time
** a batch of data is ready.
*/

/*
** INTERNAL MACROS
*/
# define JO_SMPC_MAX_PERIPHERAL         (JO_SMPC_PERIPHERAL_PER_PORT * 2)
# define JO_SMPC_INTBACK_BUFFER_SIZE    (32 * 16)
# define JO_SMPC_IREG(N)                (InputRegister0 + ((N) << 1))
# define JO_SMPC_OREG(N)                (OutputRegister0 + ((N) << 1))

/** @brief Output register count */
# define JO_SMPC_OUTPUT_REGISTER_COUNT  (32)
/** @brief Status flag: a command is running */
# define JO_SMPC_STATUS_FLAG_BUSY       (1 << 0)
/** @brief Status register: more peripheral data are available (INTBACK) */
# define JO_SMPC_STATUS_MORE_DATA       (1 << 5)
/** @brief Status register: the output registers contain peripheral data (0: system status) */
# define JO_SMPC_STATUS_PERIPHERAL_DATA (1 << 7)

/*
** INTBACK parameters
*/
/** @brief IREG0: get the system status */
# define JO_SMPC_INTBACK_SYSTEM_STATUS  (0x01)
/** @brief IREG0: continue the peripheral data transfer */
# define JO_SMPC_INTBACK_CONTINUE       (0x80)
/** @brief IREG1: get the peripheral data (15 bytes mode for both ports, OREG return) */
# define JO_SMPC_INTBACK_PERIPHERALS    (0x08)
/** @brief IREG1: no peripheral data */
# define JO_SMPC_INTBACK_NO_PERIPHERAL  (0x00)
/** @brief IREG2: end of the parameters */
# define JO_SMPC_INTBACK_END            (0xF0)

/*
** Peripheral data
*/
/** @brief Number of peripherals connected to a port (port status byte) */
# define JO_SMPC_PORT_PERIPHERAL_COUNT(STATUS)  ((STATUS) & 0x0f)
/** @brief Peripheral type (peripheral ID high nibble) */
# define JO_SMPC_PERIPHERAL_TYPE(ID)            ((ID) >> 4)
/** @brief Data size (peripheral ID low nibble) */
# define JO_SMPC_PERIPHERAL_SIZE(ID)            ((ID) & 0x0f)
/** @brief The data size is in the next byte */
# define JO_SMPC_PERIPHERAL_EXTENDED_SIZE       (0x0f)
/** @brief Peripheral type: digital (gamepad) */
# define JO_SMPC_PERIPHERAL_TYPE_DIGITAL        (0x0)
/** @brief Peripheral type: analog (mission stick, racing wheel, 3D pad) */
# define JO_SMPC_PERIPHERAL_TYPE_ANALOG         (0x1)
/** @brief Peripheral type: mouse */
# define JO_SMPC_PERIPHERAL_TYPE_MOUSE          (0x2)
/** @brief Peripheral type: keyboard */
# define JO_SMPC_PERIPHERAL_TYPE_KEYBOARD       (0x3)
/** @brief Peripheral type: shuttle mouse */
# define JO_SMPC_PERIPHERAL_TYPE_SHUTTLE_MOUSE  (0xE)
/** @brief No button pressed (buttons are active low) */
# define JO_SMPC_NO_BUTTON                      (0xffff)
/** @brief No button pressed in the low byte */
# define JO_SMPC_NO_BUTTON_LOW_BYTE             (0xff)

/*
** Mouse first data byte
*/
# define JO_SMPC_MOUSE_LEFT             (1 << 0)
# define JO_SMPC_MOUSE_RIGHT            (1 << 1)
# define JO_SMPC_MOUSE_MIDDLE           (1 << 2)
# define JO_SMPC_MOUSE_START            (1 << 3)
# define JO_SMPC_MOUSE_X_SIGN           (1 << 4)
# define JO_SMPC_MOUSE_Y_SIGN           (1 << 5)
# define JO_SMPC_MOUSE_X_OVERFLOW       (1 << 6)
# define JO_SMPC_MOUSE_Y_OVERFLOW       (1 << 7)

/*
** GLOBALS
*/
jo_peripheral                              __jo_smpc_peripherals[JO_SMPC_MAX_PERIPHERAL];
jo_smpc_status                          __jo_smpc_status;
static unsigned char                    __jo_smpc_intback_buffer[JO_SMPC_INTBACK_BUFFER_SIZE];
static unsigned int                     __jo_smpc_intback_length;
static volatile bool                    __jo_smpc_intback_busy;
static unsigned short                   __jo_smpc_previous_pressed[JO_SMPC_MAX_PERIPHERAL];
/** @brief End of the peripherals of each port filled by the last parse (the next ones are cleared already: the 2 x 15 entries
 *  were cleared at each VBLANK; the whole port the first time) */
static unsigned char                    __jo_smpc_port_end[2] = {JO_SMPC_PERIPHERAL_PER_PORT, JO_SMPC_PERIPHERAL_PER_PORT * 2};

/** @brief Mark a peripheral as not connected
 *  @param p Peripheral
 */
static void                             jo_smpc_clear_peripheral(jo_peripheral *p)
{
    // (already cleared: the ports without a peripheral, at each VBLANK)
    if (p->id == JO_PERIPHERAL_ID_NOT_CONNECTED && p->size == 0 && p->data == JO_SMPC_NO_BUTTON && p->push == JO_SMPC_NO_BUTTON &&
        p->pull == JO_SMPC_NO_BUTTON)
        return;
    p->id = JO_PERIPHERAL_ID_NOT_CONNECTED;
    p->size = 0;
    // Buttons are active low
    p->data = JO_SMPC_NO_BUTTON;
    p->push = JO_SMPC_NO_BUTTON;
    p->pull = JO_SMPC_NO_BUTTON;
    p->reserved[0] = 0;
    p->reserved[1] = 0;
    p->reserved[2] = 0;
    p->reserved[3] = 0;
}

/** @brief Update the button state of a peripheral
 *  @param index Peripheral index (0 to 29)
 *  @param active_low Button bits (0 = pressed)
 *  @remarks push and pull are computed from the previous state
 */
static void                             jo_smpc_set_buttons(const int index, const unsigned short active_low)
{
    jo_peripheral                          *p;
    unsigned short                      pressed;
    unsigned short                      previous;

    p = &__jo_smpc_peripherals[index];
    pressed = ~active_low;
    previous = __jo_smpc_previous_pressed[index];
    p->data = active_low;
    p->push = ~(pressed & ~previous);
    p->pull = ~(~pressed & previous);
    __jo_smpc_previous_pressed[index] = pressed;
}

/** @brief Decode the INTBACK data of a peripheral
 *  @param index Peripheral index (0 to 29)
 *  @param id Peripheral ID (JO_PERIPHERAL_ID_xxx)
 *  @param data Peripheral data
 *  @param size Data size in bytes
 */
static void                             jo_smpc_fill_peripheral(const int index, const unsigned char id, const unsigned char *data, const unsigned int size)
{
    jo_peripheral                          *p;
    jo_peripheral_analog                           *a;
    jo_peripheral_pointer                            *m;
    jo_peripheral_keyboard                         *k;
    unsigned short                      buttons;
    int                                 dx;
    int                                 dy;

    p = &__jo_smpc_peripherals[index];
    if (p->id != id)
    {
        jo_smpc_clear_peripheral(p);
        __jo_smpc_previous_pressed[index] = 0;
    }
    p->id = id;
    p->size = size;
    switch (JO_SMPC_PERIPHERAL_TYPE(id))
    {
        case JO_SMPC_PERIPHERAL_TYPE_SHUTTLE_MOUSE:
        case JO_SMPC_PERIPHERAL_TYPE_MOUSE:
            if (size < 3)
                break;
            JO_ZERO(buttons);
            if (data[0] & JO_SMPC_MOUSE_START)
                buttons |= JO_MOUSE_START_BUTTON;
            if (data[0] & JO_SMPC_MOUSE_LEFT)
                buttons |= JO_MOUSE_LEFT_BUTTON;
            if (data[0] & JO_SMPC_MOUSE_RIGHT)
                buttons |= JO_MOUSE_RIGHT_BUTTON;
            if (data[0] & JO_SMPC_MOUSE_MIDDLE)
                buttons |= JO_MOUSE_MIDDLE_BUTTON;
            jo_smpc_set_buttons(index, ~buttons);
            dx = data[1];
            dy = data[2];
            // 9 bits signed values
            if (data[0] & JO_SMPC_MOUSE_X_SIGN)
                dx -= 256;
            if (data[0] & JO_SMPC_MOUSE_Y_SIGN)
                dy -= 256;
            if (data[0] & JO_SMPC_MOUSE_X_OVERFLOW)
                JO_ZERO(dx);
            if (data[0] & JO_SMPC_MOUSE_Y_OVERFLOW)
                JO_ZERO(dy);
            m = (jo_peripheral_pointer *)p;
            m->x += dx;
            m->y -= dy;
            break;
        case JO_SMPC_PERIPHERAL_TYPE_ANALOG: // Mission stick, racing wheel, 3D pad
            if (size >= 2)
                jo_smpc_set_buttons(index, (data[0] << 8) | data[1]);
            a = (jo_peripheral_analog *)p;
            a->x = size > 2 ? data[2] : 0;
            a->y = size > 3 ? data[3] : 0;
            a->z = size > 4 ? data[4] : 0;
            a->w = size > 5 ? data[5] : 0;
            break;
        case JO_SMPC_PERIPHERAL_TYPE_KEYBOARD:
            if (size >= 2)
                jo_smpc_set_buttons(index, (data[0] << 8) | data[1]);
            k = (jo_peripheral_keyboard *)p;
            k->condition = size > 2 ? data[2] : 0;
            k->code = size > 3 ? data[3] : 0;
            break;
        default: // Digital devices
            if (size >= 2)
                jo_smpc_set_buttons(index, (data[0] << 8) | data[1]);
            else if (size == 1)
                jo_smpc_set_buttons(index, (data[0] << 8) | JO_SMPC_NO_BUTTON_LOW_BYTE);
            break;
    }
}

/** @brief Decode the INTBACK data of a port
 *  @param pos Position of the port data in the INTBACK buffer
 *  @param first_index Index of the first peripheral of the port (0 or 15)
 *  @return Position of the next port data
 *  @remarks Up to 15 peripherals per port with a multitap
 */
static unsigned int                     jo_smpc_parse_port(unsigned int pos, const int first_index)
{
    unsigned char                       port_status;
    unsigned int                        count;
    unsigned int                        i;
    unsigned char                       id;
    unsigned int                        size;
    int                                 index;
    int                                 last;
    unsigned char                       *end;

    end = &__jo_smpc_port_end[first_index == 0 ? 0 : 1];
    if (pos >= __jo_smpc_intback_length)
    {
        for (index = first_index; index < *end; ++index)
            jo_smpc_clear_peripheral(&__jo_smpc_peripherals[index]);
        *end = (unsigned char)first_index;
        return (pos);
    }
    port_status = __jo_smpc_intback_buffer[pos++];
    count = JO_SMPC_PORT_PERIPHERAL_COUNT(port_status);
    index = first_index;
    for (JO_ZERO(i); i < count && pos < __jo_smpc_intback_length; ++i)
    {
        id = __jo_smpc_intback_buffer[pos++];
        size = JO_SMPC_PERIPHERAL_SIZE(id);
        if (size == JO_SMPC_PERIPHERAL_EXTENDED_SIZE && pos < __jo_smpc_intback_length)
            size = __jo_smpc_intback_buffer[pos++];
        if (id == JO_PERIPHERAL_ID_NOT_CONNECTED)
            JO_ZERO(size);
        if (index < first_index + JO_SMPC_PERIPHERAL_PER_PORT)
        {
            if (id == JO_PERIPHERAL_ID_NOT_CONNECTED)
                jo_smpc_clear_peripheral(&__jo_smpc_peripherals[index]);
            else
                jo_smpc_fill_peripheral(index, id, &__jo_smpc_intback_buffer[pos], size);
            ++index;
        }
        pos += size;
    }
    // (the ones filled by the last parse after these: cleared, the next ones are cleared already)
    last = index;
    for (; index < *end; ++index)
        jo_smpc_clear_peripheral(&__jo_smpc_peripherals[index]);
    *end = (unsigned char)last;
    return (pos);
}

/** @brief Copy the system status from the SMPC output registers */
static void                             jo_smpc_read_status(void)
{
    __jo_smpc_status.cond = jo_smpc_read_byte(JO_SMPC_OREG(0));
    __jo_smpc_status.year = (jo_smpc_read_byte(JO_SMPC_OREG(1)) << 8) | jo_smpc_read_byte(JO_SMPC_OREG(2));
    __jo_smpc_status.month = jo_smpc_read_byte(JO_SMPC_OREG(3));
    __jo_smpc_status.day = jo_smpc_read_byte(JO_SMPC_OREG(4));
    __jo_smpc_status.hour = jo_smpc_read_byte(JO_SMPC_OREG(5));
    __jo_smpc_status.minute = jo_smpc_read_byte(JO_SMPC_OREG(6));
    __jo_smpc_status.second = jo_smpc_read_byte(JO_SMPC_OREG(7));
    __jo_smpc_status.cartridge_code = jo_smpc_read_byte(JO_SMPC_OREG(8));
    __jo_smpc_status.area_code = jo_smpc_read_byte(JO_SMPC_OREG(9));
    __jo_smpc_status.system_status = (jo_smpc_read_byte(JO_SMPC_OREG(10)) << 8) | jo_smpc_read_byte(JO_SMPC_OREG(11));
    __jo_smpc_status.smpc_memory = (jo_smpc_read_byte(JO_SMPC_OREG(12)) << 24) | (jo_smpc_read_byte(JO_SMPC_OREG(13)) << 16) |
                                   (jo_smpc_read_byte(JO_SMPC_OREG(14)) << 8) | jo_smpc_read_byte(JO_SMPC_OREG(15));
}

/** @brief SMPC interrupt handler: an INTBACK batch is ready
 *  @remarks The status register tells if the batch contains the system status or the peripheral data
 *  @remarks Some emulators (Yabause 0.9.14) don't send the system status when the previous INTBACK is not finished
 *           at the beginning of the frame, so the first batch can contain the peripheral data
 */
static void __attribute__((interrupt_handler)) jo_smpc_handler(void)
{
    register unsigned int               i;
    unsigned char                       sr;

    sr = jo_smpc_read_byte(StatusRegister);
    if (!(sr & JO_SMPC_STATUS_PERIPHERAL_DATA))
        jo_smpc_read_status();
    else
    {
        for (JO_ZERO(i); i < JO_SMPC_OUTPUT_REGISTER_COUNT && __jo_smpc_intback_length < JO_SMPC_INTBACK_BUFFER_SIZE; ++i)
            __jo_smpc_intback_buffer[__jo_smpc_intback_length++] = jo_smpc_read_byte(JO_SMPC_OREG(i));
    }
    if (sr & JO_SMPC_STATUS_MORE_DATA)
    {
        // More peripheral data: continue
        jo_smpc_write_byte(InputRegister0, JO_SMPC_INTBACK_CONTINUE);
    }
    else
    {
        jo_smpc_parse_port(jo_smpc_parse_port(0, 0), JO_SMPC_PERIPHERAL_PER_PORT);
        __jo_smpc_intback_busy = false;
    }
    // The SCU masked its interrupts when this one was taken
    __jo_core_restore_scu_mask();
}

/** @brief Request the system status and the peripheral data (Internal usage)
 *  @remarks Called at the end of the VBLANK-IN interrupt. Does nothing if the previous request is not complete
 */
void                                    __jo_smpc_vblank_in(void)
{
    if (__jo_smpc_intback_busy || (jo_smpc_read_byte(StatusFlag) & JO_SMPC_STATUS_FLAG_BUSY))
        return;
    __jo_smpc_intback_busy = true;
    JO_ZERO(__jo_smpc_intback_length);
    jo_smpc_write_byte(StatusFlag, JO_SMPC_STATUS_FLAG_BUSY);
    jo_smpc_write_byte(InputRegister0, JO_SMPC_INTBACK_SYSTEM_STATUS);
    jo_smpc_write_byte(InputRegister1, JO_SMPC_INTBACK_PERIPHERALS);
    jo_smpc_write_byte(InputRegister2, JO_SMPC_INTBACK_END);
    jo_smpc_write_byte(CommandRegister, FetchStatusAndPeripheralData);
}

/** @brief Initialize the peripherals and read the system status once (Internal usage) */
void                                    __jo_smpc_init(void)
{
    register unsigned int               i;

    for (JO_ZERO(i); i < JO_SMPC_MAX_PERIPHERAL; ++i)
    {
        jo_smpc_clear_peripheral(&__jo_smpc_peripherals[i]);
        JO_ZERO(__jo_smpc_previous_pressed[i]);
    }
    __jo_smpc_intback_busy = false;
    // Both ports are read by the SMPC (INTBACK), not directly by the CPU (the BIOS can change these registers)
    jo_smpc_write_byte(DataDirectionRegister1, 0);
    jo_smpc_write_byte(DataDirectionRegister2, 0);
    jo_smpc_write_byte(InputOutputSelectRegister, 0);
    jo_smpc_write_byte(ExternalLatchEnableRegister, 0);
    // Get the system status once (clock, language, area...)
    jo_smpc_begin_command();
    jo_smpc_write_byte(InputRegister0, JO_SMPC_INTBACK_SYSTEM_STATUS);
    jo_smpc_write_byte(InputRegister1, JO_SMPC_INTBACK_NO_PERIPHERAL);
    jo_smpc_write_byte(InputRegister2, JO_SMPC_INTBACK_END);
    jo_smpc_end_command(FetchStatusAndPeripheralData);
    jo_smpc_read_status();
    __jo_core_set_interrupt_handler(JO_VECTOR_SMPC, jo_smpc_handler);
}

void                                    __jo_smpc_command(const jo_smpc_command command)
{
    // Wait for the INTBACK of the current frame
    while (__jo_smpc_intback_busy)
        ;
    jo_smpc_begin_command();
    jo_smpc_end_command(command);
}

/*
** END OF FILE
*/
