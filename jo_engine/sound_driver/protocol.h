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
** The sound driver of the 68000 (JO_COMPILE_WITH_AUDIO_DRIVER_MODULE): what the SH-2 and the 68000 share in the
** sound RAM. Included by the driver (sound_driver/driver.c) and by the engine (audio.c).
**
** The sound RAM (offsets, the 68000 sees them at the same addresses):
**   0x0000 - 0x0AFF   the driver: its vectors, its code, its data, its stack
**   0x0B00 - 0x0B0F   the mailbox: the state of the driver, the indexes of the queue
**   0x0C00 - 0x0FFF   the queue of the commands: 32 commands of 32 bytes, written by the SH-2, run by the 68000
**   0x1000 - ...      the sounds (and the waveform of the tones)
**
** The SH-2 writes a command at the head of the queue, then the head; the 68000 runs the commands from the tail to the
** head, then writes the tail. The commands of a batch (written between 2 reads of the head) start together: the slots
** are keyed off, the 68000 waits for 2 samples (the SCSP runs the keys at the start of a sample), then they are
** programmed and keyed on at the same time (a stereo sound, the 2 slots of a FM tone).
*/

#ifndef __JO_SOUND_DRIVER_PROTOCOL_H__
# define __JO_SOUND_DRIVER_PROTOCOL_H__

/** @brief End of the driver (its stack is below) */
# define JO_DRIVER_CODE_END             (0x0B00)
/** @brief The driver writes JO_DRIVER_MAGIC there when it runs */
# define JO_DRIVER_STATE                (0x0B00)
# define JO_DRIVER_MAGIC                (0x4A6F)
/** @brief The head of the queue (written by the SH-2) */
# define JO_DRIVER_HEAD                 (0x0B02)
/** @brief The tail of the queue (written by the 68000) */
# define JO_DRIVER_TAIL                 (0x0B04)
/** @brief Milliseconds counted by the driver (its timer: the SH-2 can see that it runs) */
# define JO_DRIVER_TICKS                (0x0B06)
/** @brief The queue of the commands */
# define JO_DRIVER_QUEUE                (0x0C00)
# define JO_DRIVER_QUEUE_LENGTH         (32)
# define JO_DRIVER_COMMAND_SIZE         (32)
/** @brief The first byte of the sound RAM after the driver */
# define JO_DRIVER_AREA_END             (0x1000)

/*
** The commands: word 0: the command (bits 15-8) and the slot (bits 7-0)
*/
/** @brief Play a slot: words 1 to 12: the 12 registers of the slot (0x00 to 0x16, the key bits of the register 0x00
 *  are ignored). Its glide is stopped */
# define JO_DRIVER_KEY_ON               (1)
/** @brief Stop a slot (its glide too) */
# define JO_DRIVER_KEY_OFF              (2)
/** @brief Write a register of a slot: word 1: its offset (0x02 to 0x16), word 2: its value. A write of the pitch
 *  (0x10) stops the glide of the slot */
# define JO_DRIVER_WRITE                (3)
/** @brief The pitch of a slot glides (each millisecond): word 1: the target pitch, word 2: the speed (the part of the
 *  difference done each millisecond, 32768 = all of it), word 3: the start pitch. The pitches as octave * 1024 +
 *  frequency number (signed: -8192 to 8191; the register 0x10: octave in bits 14-11, frequency number in bits 9-0) */
# define JO_DRIVER_GLIDE                (4)

/** @brief A pitch register (0x10) -> a pitch of a glide (octave * 1024 + frequency number) */
# define JO_DRIVER_GLIDE_PITCH(REG)     ((short)((((short)((REG) << 1)) >> 12) * 1024 + ((REG) & 0x3FF)))
/** @brief A pitch of a glide -> a pitch register */
# define JO_DRIVER_PITCH_REGISTER(P)    ((unsigned short)(((((P) >> 10) & 0xF) << 11) | ((P) & 0x3FF)))

#endif /* !__JO_SOUND_DRIVER_PROTOCOL_H__ */

/*
** END OF FILE
*/
