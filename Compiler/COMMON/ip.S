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
** A value too long for its field stops the build with an error that names the makefile variable and its limit
*/

#define JO_IP_SPACE                     (0x20)
#define JO_IP_MAKER_ID_OFFSET           (0x10)
#define JO_IP_PRODUCT_NUMBER_OFFSET     (0x20)
#define JO_IP_VERSION_OFFSET            (0x2A)
#define JO_IP_RELEASE_DATE_OFFSET       (0x30)
#define JO_IP_DEVICE_INFO_OFFSET        (0x38)
#define JO_IP_DEVICE_INFO_SIZE          (0x18)
#define JO_IP_PERIPHERALS_OFFSET        (0x50)
#define JO_IP_TITLE_OFFSET              (0x60)
#define JO_IP_RESERVED_OFFSET           (0xD0)
#define JO_IP_MAKER_ID_SIZE             (JO_IP_PRODUCT_NUMBER_OFFSET - JO_IP_MAKER_ID_OFFSET)
#define JO_IP_PRODUCT_NUMBER_SIZE       (JO_IP_VERSION_OFFSET - JO_IP_PRODUCT_NUMBER_OFFSET)
#define JO_IP_VERSION_SIZE              (JO_IP_RELEASE_DATE_OFFSET - JO_IP_VERSION_OFFSET)
#define JO_IP_RELEASE_DATE_SIZE         (JO_IP_DEVICE_INFO_OFFSET - JO_IP_RELEASE_DATE_OFFSET)
#define JO_IP_PERIPHERALS_SIZE          (JO_IP_TITLE_OFFSET - JO_IP_PERIPHERALS_OFFSET)
#define JO_IP_TITLE_SIZE                (JO_IP_RESERVED_OFFSET - JO_IP_TITLE_OFFSET)

    .text
jo_ip_start:
    /* 0x00: Hardware identifier */
    .ascii  "SEGA SEGASATURN "
    /* 0x10: Maker ID (16 characters max) */
jo_ip_maker_id_start:
    .ascii  JO_IP_MAKER_ID
jo_ip_maker_id_end:
    .if     (jo_ip_maker_id_end - jo_ip_maker_id_start) > JO_IP_MAKER_ID_SIZE
    .error  "JO_GAME_MAKER_ID is too long: 16 characters max"
    .endif
    .org    jo_ip_start + JO_IP_PRODUCT_NUMBER_OFFSET, JO_IP_SPACE
    /* 0x20: Product number (10 characters max) */
jo_ip_product_number_start:
    .ascii  JO_IP_PRODUCT_NUMBER
jo_ip_product_number_end:
    .if     (jo_ip_product_number_end - jo_ip_product_number_start) > JO_IP_PRODUCT_NUMBER_SIZE
    .error  "JO_GAME_PRODUCT_NUMBER is too long: 10 characters max"
    .endif
    .org    jo_ip_start + JO_IP_VERSION_OFFSET, JO_IP_SPACE
    /* 0x2A: Version (6 characters max, like "V1.000") */
jo_ip_version_start:
    .ascii  JO_IP_VERSION
jo_ip_version_end:
    .if     (jo_ip_version_end - jo_ip_version_start) > JO_IP_VERSION_SIZE
    .error  "JO_GAME_VERSION is too long: 6 characters max (like V1.000)"
    .endif
    .org    jo_ip_start + JO_IP_RELEASE_DATE_OFFSET, JO_IP_SPACE
    /* 0x30: Release date (8 characters: YYYYMMDD) */
jo_ip_release_date_start:
    .ascii  JO_IP_RELEASE_DATE
jo_ip_release_date_end:
    .if     (jo_ip_release_date_end - jo_ip_release_date_start) > JO_IP_RELEASE_DATE_SIZE
    .error  "JO_GAME_RELEASE_DATE is too long: 8 characters max (YYYYMMDD)"
    .endif
    .org    jo_ip_start + JO_IP_DEVICE_INFO_OFFSET, JO_IP_SPACE
    /* 0x38: Device information and compatible areas: they must match the area code of the template */
    .incbin "IP.BIN", JO_IP_DEVICE_INFO_OFFSET, JO_IP_DEVICE_INFO_SIZE
    /* 0x50: Compatible peripherals (16 characters max) */
jo_ip_peripherals_start:
    .ascii  JO_IP_PERIPHERALS
jo_ip_peripherals_end:
    .if     (jo_ip_peripherals_end - jo_ip_peripherals_start) > JO_IP_PERIPHERALS_SIZE
    .error  "JO_GAME_PERIPHERALS is too long: 16 characters max"
    .endif
    .org    jo_ip_start + JO_IP_TITLE_OFFSET, JO_IP_SPACE
    /* 0x60: Game title (112 characters max) */
jo_ip_title_start:
    .ascii  JO_IP_TITLE
jo_ip_title_end:
    .if     (jo_ip_title_end - jo_ip_title_start) > JO_IP_TITLE_SIZE
    .error  "JO_GAME_TITLE is too long: 112 characters max"
    .endif
    .org    jo_ip_start + JO_IP_RESERVED_OFFSET, JO_IP_SPACE
    /* 0xD0: Reserved, IP size, stack, first read address, etc. and the rest of the template */
    .incbin "IP.BIN", JO_IP_RESERVED_OFFSET
