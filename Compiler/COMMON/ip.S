/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2026, Johannes Fetz (johannesfetz@gmail.com)
** All rights reserved.
**
** IP.BIN generator: the system ID (first 256 bytes) is built from the makefile variables
** (JO_GAME_TITLE, JO_GAME_VERSION, etc), the rest comes from the IP.BIN template (security code, area code, etc).
**
** See https://segaretro.org/ROM_header#Saturn
**
** A value too long for its field stops the build with "attempt to move .org backwards"
*/

#define JO_IP_SPACE                 (0x20)

    .text
jo_ip_start:
    /* 0x00: Hardware identifier */
    .ascii  "SEGA SEGASATURN "
    /* 0x10: Maker ID (16 characters max) */
    .ascii  JO_IP_MAKER_ID
    .org    jo_ip_start + 0x20, JO_IP_SPACE
    /* 0x20: Product number (10 characters max) */
    .ascii  JO_IP_PRODUCT_NUMBER
    .org    jo_ip_start + 0x2A, JO_IP_SPACE
    /* 0x2A: Version (6 characters max, like "V1.000") */
    .ascii  JO_IP_VERSION
    .org    jo_ip_start + 0x30, JO_IP_SPACE
    /* 0x30: Release date (8 characters: YYYYMMDD) */
    .ascii  JO_IP_RELEASE_DATE
    .org    jo_ip_start + 0x38, JO_IP_SPACE
    /* 0x38: Device information and compatible areas: they must match the area code of the template */
    .incbin "IP.BIN", 0x38, 0x18
    /* 0x50: Compatible peripherals (16 characters max) */
    .ascii  JO_IP_PERIPHERALS
    .org    jo_ip_start + 0x60, JO_IP_SPACE
    /* 0x60: Game title (112 characters max) */
    .ascii  JO_IP_TITLE
    .org    jo_ip_start + 0xD0, JO_IP_SPACE
    /* 0xD0: Reserved, IP size, stack, first read address, etc. and the rest of the template */
    .incbin "IP.BIN", 0xD0
