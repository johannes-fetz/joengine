/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2017, Johannes Fetz (johannesfetz@gmail.com)
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
#include <jo/jo.h>

void			my_draw(void)
{
    jo_printf_with_color(0, 0, JO_COLOR_INDEX_Red, jo_get_last_error());
    jo_printf_with_color(13, 1, JO_COLOR_INDEX_White, "*Backup demo*");
    jo_printf_with_color(2, 3, JO_COLOR_INDEX_White, "(It's not a blue screen of death ^^)");
}

void            play_with_backup_device(jo_backup_device dev)
{
    char        *contents;

    /* Mount the internal backup memory */
	if (!jo_backup_mount(dev))
        return ;

	/* Create a new file */
	if (jo_backup_save_file_contents(dev, "TEST.TXT", "MY TEST", "HELLO WORLD!", 12))
    {
        /* Check if the exists */
        if (jo_backup_file_exists(dev, "TEST.TXT"))
            jo_printf_with_color(0, 8, JO_COLOR_INDEX_Green, "File exists: YES");
        else
            jo_printf_with_color(0, 8, JO_COLOR_INDEX_Red, "File exists: NO");

        /* Load file contents */
        contents = (char *)jo_backup_load_file_contents(dev, "TEST.TXT", JO_NULL);
        if (contents != JO_NULL)
            jo_printf_with_color(0, 10, JO_COLOR_INDEX_Green, "File contents: %s", contents);
        else
            jo_printf_with_color(0, 10, JO_COLOR_INDEX_Red, "Fail to get file contents!");

        /* Always free the contents when you don't use it */
        jo_free(contents);

        /* Delete file */
        jo_backup_delete_file(dev, "TEST.TXT");
    }
    else
        jo_printf_with_color(0, 8, JO_COLOR_INDEX_Red, "Fail to create file (reason=%d)", jo_backup_get_last_status(dev));

    /* Unmount the device */
    jo_backup_unmount(dev);
}

void			jo_main(void)
{
	jo_core_init(JO_COLOR_MediumBlue);

	/* You can pass other values to test like JoCartridgeMemoryBackup and JoExternalDeviceBackup */
    play_with_backup_device(JoInternalMemoryBackup);

	jo_core_add_callback(my_draw);
	jo_core_run();
}

/*
** END OF FILE
*/
