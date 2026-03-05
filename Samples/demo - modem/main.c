/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2020, Johannes Fetz (johannesfetz@gmail.com)
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
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AN
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

#include "modem.h"
#include <jo/jo.h>
#include <string.h>

// heartbeat message to send periodically
const char g_heartbeat[] = "LOOC si sihT ";
unsigned int g_heartbeat_counter = 0;


unsigned char g_recv_buf[16] = {0};
unsigned char g_send_buf[16] = {0};

unsigned int g_frame = 0;
unsigned int g_sprite_id = 0;

// serial error callbacks
void my_overrun_callback(void);
void my_framming_callback(void);
void my_parity_callback(void);

void my_serial_recv(void);
void my_serial_send(void);
void my_gamepad(void);
void my_draw(void);
void my_frame_counter(void);
void prepend_array(unsigned char* array, unsigned int array_size, unsigned char val);
char convert_button_to_char(void);


bool is_present = false;

void			jo_main(void)
{
	jo_core_init(JO_COLOR_Black);

	is_present = jo_modem_init();

	// note: only tested with US and Jap Saturn. PAL saturn will likely be different speeds

	// initialize serial
	//jo_serial_async_init();

	// optionally set serial error callback functions
	// these will be called when the serial unit detects an error
	// the function prototype is: void (*jo_serial_error_callback)(void)
	//jo_serial_register_error_callbacks(my_overrun_callback, my_framming_callback, my_parity_callback);

	// Improvement: put send\recv serial callbacks on the Slave SH2 to avoid vblank issues
	// This sample is using only the Master SH2
	jo_core_add_callback(my_serial_send);
	jo_core_add_callback(my_serial_recv);

	jo_core_add_callback(my_draw);
	jo_core_add_callback(my_frame_counter);	
	jo_core_add_callback(my_gamepad);

	g_sprite_id = jo_sprite_add_tga(JO_ROOT_DIR, "MODEM.TGA", JO_COLOR_Green);

	jo_core_run();
}

void my_overrun_callback(void)
{
	jo_printf(1, 25, "Overrun (ORER) error detected");
}

void my_framming_callback(void)
{
	jo_printf(1, 26, "Framing (FER) error detected");
}

void my_parity_callback(void)
{
	jo_printf(1, 27, "Parity (PER) error detected");
}

// drawing callback
void			my_draw(void)
{
	unsigned int i = 0;

	jo_printf(11, 2, "Dial-Up Modem Demo");
	jo_sprite_draw3D(g_sprite_id, 0,-40, 500);

	jo_printf(11, 1, "present: %d", is_present);

	/*
	jo_printf(1, 18, "Send:");
	for(i = 0; i < sizeof(g_send_buf); i++)
	{
		jo_printf(7 + (i * 2), 18, "%c", g_send_buf[i]);
	}

	jo_printf(1, 20, "Recv:");
	for(i = 0; i < sizeof(g_recv_buf); i++)
	{
		jo_printf(7 + (i * 2), 20, "%c", g_recv_buf[i]);
	}

	jo_printf(1, 22, "Press buttons to send data");
	jo_printf(1, 23, "Sends 'This is COOL' char every");
    jo_printf(1, 24, "255 frames. 209954 baudrate 8N1");
	*/

	//jo_printf(1, 23, "H is a periodic heartbeat. 1800 B 8N1");
}

// callback to check controller 1 input
// if controller input, send via serial link
void			my_gamepad(void)
{
	unsigned char data = 0;
	int result = 0;

	if(!jo_is_input_available(0))
	{
		return;
	}

	data = convert_button_to_char();
	if(!data)
	{
		return;
	}

	/*
	result = jo_serial_send_byte(data);
	if(result != 0)
	{
		return;
	}
	*/

	prepend_array(g_send_buf, sizeof(g_send_buf), data);
}

// send a heartbeat character over the serial link every 255 frames
// messasge = "This is COOL"
void			my_serial_send(void)
{
	unsigned char data = 0;
	int result = 0;

	if ((g_frame & 255) != 0)
	{
		return;
	}

	data = g_heartbeat[g_heartbeat_counter];
	g_heartbeat_counter++;

	if(g_heartbeat_counter >= sizeof(g_heartbeat))
	{
		g_heartbeat_counter = 0;
	}	

	/*
	result = jo_serial_send_byte(data);
	if(result != 0)
	{
		return;
	}
	*/

	prepend_array(g_send_buf, sizeof(g_send_buf), data);

	// clear out the error messages
	jo_printf(1, 25, "                             ");
	jo_printf(1, 26, "                             ");
	jo_printf(1, 27, "                             ");
}

// callback to check if serial data is available
void			my_serial_recv(void)
{
	unsigned char data = 0;
	int result = 0;

	/*
	result = jo_serial_recv_byte(&data);
	if(result != 0)
	{
		return;
	}
	*/

	prepend_array(g_recv_buf, sizeof(g_recv_buf), data);
}

void			my_frame_counter(void)
{
	g_frame++;
}

void			prepend_array(unsigned char* array, unsigned int array_size, unsigned char val)
{
	memmove(array + 1, array, array_size -1);
	array[0] = val;
}

char			convert_button_to_char(void)
{
	if(jo_is_input_key_down(0, JO_KEY_RIGHT))
	{
		return 'r';
	}
	else if(jo_is_input_key_down(0, JO_KEY_LEFT))
	{
		return 'l';
	}
	else if(jo_is_input_key_down(0, JO_KEY_DOWN))
	{
		return 'd';
	}
	else if(jo_is_input_key_down(0, JO_KEY_UP))
	{
		return 'u';
	}
	else if(jo_is_input_key_down(0, JO_KEY_START))
	{
		return 'S';
	}
	else if(jo_is_input_key_down(0, JO_KEY_A))
	{
		return 'A';
	}
	else if(jo_is_input_key_down(0, JO_KEY_B))
	{
		return 'B';
	}
	else if(jo_is_input_key_down(0, JO_KEY_C))
	{
		return 'C';
	}
	else if(jo_is_input_key_down(0, JO_KEY_X))
	{
		return 'X';
	}
	else if(jo_is_input_key_down(0, JO_KEY_Y))
	{
		return 'Y';
	}
	else if(jo_is_input_key_down(0, JO_KEY_Z))
	{
		return 'Z';
	}
	else if(jo_is_input_key_down(0, JO_KEY_LEFT))
	{
		return 'L';
	}
	else if(jo_is_input_key_down(0, JO_KEY_RIGHT))
	{
		return 'R';
	}

	return 0;
}

/*
** END OF FILE
*/
