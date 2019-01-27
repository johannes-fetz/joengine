#ifndef GCC_TM_H
#define GCC_TM_H
#define TARGET_CPU_DEFAULT (SELECT_SH2)
#ifndef LIBC_GLIBC
# define LIBC_GLIBC 1
#endif
#ifndef LIBC_UCLIBC
# define LIBC_UCLIBC 2
#endif
#ifndef LIBC_BIONIC
# define LIBC_BIONIC 3
#endif
#ifndef LIBC_MUSL
# define LIBC_MUSL 4
#endif
#ifndef SH_MULTILIB_CPU_DEFAULT
# define SH_MULTILIB_CPU_DEFAULT "m2"
#endif
#ifndef SUPPORT_SH2
# define SUPPORT_SH2 1
#endif
#ifndef SUPPORT_SH1
# define SUPPORT_SH1 1
#endif
#ifndef SUPPORT_SH2
# define SUPPORT_SH2 1
#endif
#ifndef SUPPORT_SH2E
# define SUPPORT_SH2E 1
#endif
#ifndef SUPPORT_SH4
# define SUPPORT_SH4 1
#endif
#ifndef SUPPORT_SH4_SINGLE
# define SUPPORT_SH4_SINGLE 1
#endif
#ifndef SUPPORT_SH4_SINGLE_ONLY
# define SUPPORT_SH4_SINGLE_ONLY 1
#endif
#ifndef SUPPORT_SH2A
# define SUPPORT_SH2A 1
#endif
#ifndef SUPPORT_SH2A_SINGLE
# define SUPPORT_SH2A_SINGLE 1
#endif
#ifdef IN_GCC
# include "options.h"
# include "insn-constants.h"
# include "config/vxworks-dummy.h"
# include "config/sh/sh.h"
# include "config/dbxelf.h"
# include "config/elfos.h"
# include "config/sh/elf.h"
# include "config/sh/embed-elf.h"
# include "config/newlib-stdint.h"
# include "sysroot-suffix.h"
# include "config/initfini-array.h"
#endif
#if defined IN_GCC && !defined GENERATOR_FILE && !defined USED_FOR_TARGET
# include "insn-flags.h"
#endif
#if defined IN_GCC && !defined GENERATOR_FILE
# include "insn-modes.h"
#endif
# include "defaults.h"
#endif /* GCC_TM_H */
