/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2021, Johannes Fetz (johannesfetz@gmail.com)
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

void            test1(void)
{
    char        *file1_contents;

    // Easiest method to load the full contents of a file : jo_fs_read_file()
    // The last parameter of jo_fs_read_file() is a optional pointer to an integer that gives the length of the contents
    file1_contents = jo_fs_read_file("FILE1.TXT", JO_NULL);
    // jo_fs_read_file() automatically adds the null char at the end of the string
    jo_printf(0, 0, "test1: %s", file1_contents);
    // Don't forget to free the contents as soon as you no longer need it :)
    jo_free(file1_contents);
}

void            test2(void)
{
    char        buf[64 + 1];
    jo_file     file;
    int         length;

    jo_fs_open(&file, "FILE2.TXT");
    // Note: file.size gives the size of the file
    //       file.read gives the total of bytes read.
    //       You can seek into the file with jo_fs_seek_forward()
    length = jo_fs_read_next_bytes(&file, buf, 64); // Of course you can call multiple time this method
    buf[length] = '\0'; // Don't forget to add the null char at the end of the string
    jo_printf(0, 1, "test2: %s", buf);
    jo_fs_close(&file);
}

void            my_async_callback_for_test3(char *contents, int length, int optional_token)
{
    // jo_fs_read_file_async() automatically adds the null char at the end of the string
    jo_printf(0, 2, "test3: %s", contents);
    jo_free(contents);
}

void            test3(void)
{
    // my_async_callback_for_test3() will be called as soon as the contents of the file is ready
    jo_fs_read_file_async("FILE3.TXT", my_async_callback_for_test3, 0/*optional_token*/);
}

void			jo_main(void)
{
	jo_core_init(JO_COLOR_Black);
	test1();
    test2();
    test3();
    // Note: You can change the current working directory with jo_fs_cd()
	jo_core_run();
}

/*
** END OF FILE
*/
