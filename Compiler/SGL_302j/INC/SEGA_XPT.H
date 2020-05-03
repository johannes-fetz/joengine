/*
 * System name: Software library
 *
 * Module symbol name: sega_xpt.h
 *
 * Module name: Library common header file
 *
 * Purpose: (1) To simplify porting to other machines
 *       (2) To make the program easier to analyze
 *
 * Scope: General programming (used by both library developers and library users)
 *           Can be used)
 *
 * Prerequisites: Declarations and definitions that have the same meaning as the identifiers defined here, and definitions are defined in other files.
 *           Don't do it.
 * 
 * AUTHOR Ver1.00  N.T 1993/12/22
 *         Ver1.01  N.T 1993/12/27
 *         Ver1.02  N.T 1994/01/06
 *         Ver1.03  N.T 1994/02/23
 *         Ver1.04  N.T 1994/02/23
 *
 * DATE 1994-07-29
 */


#ifndef SEGA_XPT_H
#define SEGA_XPT_H

/*****************************************************************************/
/****************************** Basic data type declaration *****************************/
/*****************************************************************************/

typedef unsigned    char    Uint8;      /* Unsigned 1-byte integer */
typedef signed      char    Sint8;      /* Signed 1-byte integer */
typedef unsigned    short   Uint16;     /* Unsigned SmallInt */
typedef signed      short   Sint16;     /* Signed SmallInt */
typedef unsigned    long    Uint32;     /* Unsigned 4-byte integer */
typedef signed      long    Sint32;     /* Signed 4-byte integer */
typedef float               Float32;    /* 4 bytes Real */
typedef double              Float64;    /* 8 bytes Real */

typedef int                 Int;        /* Int type (for tool)            */

typedef int                 Bool;       /* Logical type (takes logical constants into values) */

/*****************************************************************************/
/********************************* Constant macros ********************************/
/*****************************************************************************/

#ifndef NULL
#define NULL    ((void *)0)             /* NULL */
#endif  /* NULL */

/*****************************************************************************/
/********************************** Enumeration constants *********************************/
/*****************************************************************************/

enum BooleanLogic {                     /* Logical constant 1 (false, true) */
    FALSE = 0,
    TRUE  = 1
};

enum BooleanSwitch {                    /* Logical Constant 2 (Switch) */
    OFF = 0,
    ON  = 1
};

enum Judgement {                        /* Conclusions constants (success, failure) */
    OK = 0,                             /* Success */
    NG = -1                             /* Failed */
};

/*****************************************************************************/
/********************************* Processing macros ********************************/
/*****************************************************************************/

#define MAX(x, y)       ((x) > (y) ? (x) : (y)) /* Maximum value */
#define MIN(x, y)       ((x) < (y) ? (x) : (y)) /* Minimum value */
#define ABS(x)          ((x) < 0 ? -(x) : (x))  /* Absolute value */

#endif  /* SEGA_XPT_H */
