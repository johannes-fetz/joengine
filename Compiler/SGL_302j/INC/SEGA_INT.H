/*-----------------------------------------------------------------------------
 *  FILE: sega_int.h
 *
 *      Copyright(c) 1994 SEGA.
 *
 *  PURPOSE:
 *
 *      INT library header file.
 *
 *  DESCRIPTION:
 *
 *  AUTHOR(S)
 *
 *      1994-07-05  N.T Ver.1.00
 *
 *  MOD HISTORY:
 *
 *-----------------------------------------------------------------------------
 */

#ifndef SEGA_INT_H
#define SEGA_INT_H
/*
 * C VIRTUAL TYPES DEFINITIONS
 */
#include "SEGA_SYS.H"

/*
 * GLOBAL DEFINES/MACROS DEFINES
 */

/* Constants */
/**** Interrupt address register ***********************************************/
#define INT_REG_IST     ((Uint32 *)0x25fe00a4)  /* Interrupt status register*/
#define INT_REG_IACK    ((Uint32 *)0x25fe00bc)  /* A-Bus Interrupt Acknowledgment */
/***** Interrupt mask bit value constant ********************************************/
#define INT_MSK_NULL      0             /* None specified                          */
#define INT_MSK_ALL      (INT_MSK_ABUS     |    /* Specify All                 */\
                          INT_MSK_SPR      |\
                          INT_MSK_DMAI     |\
                          INT_MSK_DMA0     |\
                          INT_MSK_DMA1     |\
                          INT_MSK_DMA2     |\
                          INT_MSK_PAD      |\
                          INT_MSK_SYS      |\
                          INT_MSK_SND      |\
                          INT_MSK_DSP      |\
                          INT_MSK_TIM1     |\
                          INT_MSK_TIM0     |\
                          INT_MSK_HBLK_IN  |\
                          INT_MSK_VBLK_OUT |\
                          INT_MSK_VBLK_IN  )
#define INT_MSK_ABUS     (1 << 15)      /* A-Bus                             */
#define INT_MSK_SPR      (1 << 13)      /* Sprite drawing finished                */
#define INT_MSK_DMAI     (1 << 12)      /* DMA illegal                     */
#define INT_MSK_DMA0     (1 << 11)      /* Level 0-DMA                       */
#define INT_MSK_DMA1     (1 << 10)      /* Level 1-DMA                       */
#define INT_MSK_DMA2     (1 <<  9)      /* Level 2-DMA                       */
#define INT_MSK_PAD      (1 <<  8)      /* PAD                               */
#define INT_MSK_SYS      (1 <<  7)      /* System Manager                */
#define INT_MSK_SND      (1 <<  6)      /* Sound request                */
#define INT_MSK_DSP      (1 <<  5)      /* DSP termination                       */
#define INT_MSK_TIM1     (1 <<  4)      /* Timer-1                          */
#define INT_MSK_TIM0     (1 <<  3)      /* Timer - 0                          */
#define INT_MSK_HBLK_IN  (1 <<  2)      /* H-blank -in                     */
#define INT_MSK_VBLK_OUT (1 <<  1)      /* V-blank-out                    */
#define INT_MSK_VBLK_IN  (1 <<  0)      /* V-blank-in                     */

/***** Interrupt status bit value constant ****************************************/
#define INT_ST_NULL      0              /* None specified                          */
#define INT_ST_ALL      (INT_ST_ABUS      |    /* Specify All                 */\
                          INT_ST_SPR      |\
                          INT_ST_DMAI     |\
                          INT_ST_DMA0     |\
                          INT_ST_DMA1     |\
                          INT_ST_DMA2     |\
                          INT_ST_PAD      |\
                          INT_ST_SYS      |\
                          INT_ST_SND      |\
                          INT_ST_DSP      |\
                          INT_ST_TIM1     |\
                          INT_ST_TIM0     |\
                          INT_ST_HBLK_IN  |\
                          INT_ST_VBLK_OUT |\
                          INT_ST_VBLK_IN  )
#define INT_ST_ABUS     (INT_ST_ABUS01 |        /* A-Bus                   */\
                         INT_ST_ABUS02 |\
                         INT_ST_ABUS03 |\
                         INT_ST_ABUS04 |\
                         INT_ST_ABUS05 |\
                         INT_ST_ABUS06 |\
                         INT_ST_ABUS07 |\
                         INT_ST_ABUS08 |\
                         INT_ST_ABUS09 |\
                         INT_ST_ABUS10 |\
                         INT_ST_ABUS11 |\
                         INT_ST_ABUS12 |\
                         INT_ST_ABUS13 |\
                         INT_ST_ABUS14 |\
                         INT_ST_ABUS15 |\
                         INT_ST_ABUS16 )
#define INT_ST_ABUS01   (1 << 31)       /* A-Bus01                           */
#define INT_ST_ABUS02   (1 << 30)       /* A-Bus02                           */
#define INT_ST_ABUS03   (1 << 29)       /* A-Bus03                           */
#define INT_ST_ABUS04   (1 << 28)       /* A-Bus04                           */
#define INT_ST_ABUS05   (1 << 27)       /* A-Bus05                           */
#define INT_ST_ABUS06   (1 << 26)       /* A-Bus06                           */
#define INT_ST_ABUS07   (1 << 25)       /* A-Bus07                           */
#define INT_ST_ABUS08   (1 << 24)       /* A-Bus08                           */
#define INT_ST_ABUS09   (1 << 23)       /* A-Bus09                           */
#define INT_ST_ABUS10   (1 << 22)       /* A-Bus10                           */
#define INT_ST_ABUS11   (1 << 21)       /* A-Bus11                           */
#define INT_ST_ABUS12   (1 << 20)       /* A-Bus12                           */
#define INT_ST_ABUS13   (1 << 19)       /* A-Bus13                           */
#define INT_ST_ABUS14   (1 << 18)       /* A-Bus14                           */
#define INT_ST_ABUS15   (1 << 17)       /* A-Bus15                           */
#define INT_ST_ABUS16   (1 << 16)       /* A-Bus16                           */
#define INT_ST_SPR      (1 << 13)       /* Sprite drawing finished                */
#define INT_ST_DMAI     (1 << 12)       /* DMA illegal                     */
#define INT_ST_DMA0     (1 << 11)       /* Level 0-DMA                       */
#define INT_ST_DMA1     (1 << 10)       /* Level 1-DMA                       */
#define INT_ST_DMA2     (1 <<  9)       /* Level 2-DMA                       */
#define INT_ST_PAD      (1 <<  8)       /* PAD                               */
#define INT_ST_SYS      (1 <<  7)       /* System Manager                */
#define INT_ST_SND      (1 <<  6)       /* Sound request                */
#define INT_ST_DSP      (1 <<  5)       /* DSP termination                       */
#define INT_ST_TIM1     (1 <<  4)       /* Timer-1                          */
#define INT_ST_TIM0     (1 <<  3)       /* Timer - 0                          */
#define INT_ST_HBLK_IN  (1 <<  2)       /* H-blank -in                     */
#define INT_ST_VBLK_OUT (1 <<  1)       /* V-blank-out                    */
#define INT_ST_VBLK_IN  (1 <<  0)       /* V-blank-in                     */

/***** Vector number constant ********************************************************/
/********* SCU ***************************************************************/
#define INT_SCU_SPR         0x4d        /* Sprite drawing finished                */
#define INT_SCU_DMAI        0x4c        /* DMA illegal                     */
#define INT_SCU_DMA0        0x4b        /* Level 0-DMA                       */
#define INT_SCU_DMA1        0x4a        /* Level 1-DMA                       */
#define INT_SCU_DMA2        0x49        /* Level 2-DMA                       */
#define INT_SCU_PAD         0x48        /* PAD                               */
#define INT_SCU_SYS         0x47        /* System Manager                */
#define INT_SCU_SND         0x46        /* Sound request                */
#define INT_SCU_DSP         0x45        /* DSP termination                       */
#define INT_SCU_TIM1        0x44        /* Timer-1                          */
#define INT_SCU_TIM0        0x43        /* Timer - 0                          */
#define INT_SCU_HBLK_IN     0x42        /* H-blank -in                     */
#define INT_SCU_VBLK_OUT    0x41        /* V-blank-out                    */
#define INT_SCU_VBLK_IN     0x40        /* V-blank-in                     */
     
/********* CPU ***************************************************************/
#define INT_CPU_DIVU        0v6e        /* Divider                            */
#define INT_CPU_DMAC0       0x6d        /* DMAC channel 0                     */
#define INT_CPU_DMAC1       0x6c        /* DMAC channel 1                     */
#define INT_CPU_WDT         0x68        /* WDT interval                   */
#define INT_CPU_BSC         0x69        /* BSC pair match                 */
#define INT_CPU_SCI_ERI     0x60        /* SCI receive error                 */
#define INT_CPU_SCI_RXI     0x61        /* SCI receive data full                 */
#define INT_CPU_SCI_TXI     0x62        /* SCI receive data empty           */
#define INT_CPU_SCI_TEI     0x63        /* SCI sent                 */
#define INT_CPU_FRT_ICI     0x64        /* FRT input capture           */
#define INT_CPU_FRT_OCI     0x65        /* FRT output pairs           */
#define INT_CPU_FRT_OVI     0x66        /* FRT overflow                 */

/***** The acceptable value *******************************************************/
#define INT_ACK_ENA         0x1         /* Enabled                              */
#define INT_ACK_DIS         0x0         /* Hold                              */

/******************************************************************************
 *
 * NAME:    INT_GetMsk()        -   Get interrupt mask register
 *
 * PARAMETERS :
 *      None.
 *
 * DESCRIPTION:
 *
 * PRECONDITIONS:
 *      None.
 *
 * POSTCONDITIONS:
 *      (1) Uint32  Interrupt mask bit value
 *
 * CAVEATS:
 *      None.
 *
 ******************************************************************************
 */

#define INT_GetMsk(a) SYS_GETSCUIM

/******************************************************************************
 *
 * NAME:    INT_SetMsk()    -   Interrupt mask register setting
 *
 * PARAMETERS :
 *      (1) Uint32 msk_bit      <i>   Interrupt mask bit value
 *
 * DESCRIPTION:
 *
 * PRECONDITIONS:
 *      None.
 *
 * POSTCONDITIONS:
 *      None.
 *
 * CAVEATS:
 *      None.
 *
 ******************************************************************************
 */

#define INT_SetMsk(msk_bit) SYS_SETSCUIM(msk_bit)

/******************************************************************************
 *
 * NAME:   INT_ChgMsk()     -   Change interrupt mask register
 *
 * PARAMETERS :
 *      (1) Uint32 ena_msk_bit  <i>   Interrupt mask bit (enable)
 *      (2) Uint32 dis_msk_bit  <i>   Interrupt mask bit (disabled)
 *
 * DESCRIPTION:
 *
 * PRECONDITIONS:
 *      None.
 *
 * POSTCONDITIONS:
 *      None.
 *
 * CAVEATS:
 *      None.
 *
 ******************************************************************************
 */

#define INT_ChgMsk(ena_msk_bit, dis_msk_bit)\
        SYS_CHGSCUIM(~(ena_msk_bit), dis_msk_bit)

/******************************************************************************
 *
 * NAME:    INT_GetStat()   -   Get interrupt status register
 *
 * PARAMETERS :
 *      None.
 *
 * DESCRIPTION:
 *
 * PRECONDITIONS:
 *      None.
 *
 * POSTCONDITIONS:
 *      (1) Uint32  Interrupt status register value
 *
 * CAVEATS:
 *      None.
 *
 ******************************************************************************
 */

#define INT_GetStat()   (*INT_REG_IST)

/******************************************************************************
 *
 * NAME:    INT_ResStat()       -   Interrupt status register reset
 *
 * PARAMETERS :
 *      (1) Uint32 status_bit   <i>   Interrupt status bit value
 *
 * DESCRIPTION:
 *
 * PRECONDITIONS:
 *      None.
 *
 * POSTCONDITIONS:
 *      None.
 *
 * CAVEATS:
 *      None.
 *
 ******************************************************************************
 */

#define INT_ResStat(status_bit)\
    do{\
        *INT_REG_IST = status_bit;\
    }while(FALSE)

/******************************************************************************
 *
 * NAME:    INT_SetAck()            -   A-Bus interrupt acknowledge setting
 *
 * PARAMETERS :
 *      (1) Uint32 ack                  <i>   Acknowledge value
 *
 * DESCRIPTION:
 *
 * PRECONDITIONS:
 *      None.
 *
 * POSTCONDITIONS:
 *      None.
 *
 * CAVEATS:
 *      None.
 *
 ******************************************************************************
 */

#define INT_SetAck(ack)\
    do{\
        *INT_REG_IACK = ack;\
    }while(FALSE)

/******************************************************************************
 *
 * NAME:    INT_GetAck()            -   Acquire A-Bus interrupt acknowledge
 *
 * PARAMETERS :
 *      None.
 *
 * DESCRIPTION:
 *
 * PRECONDITIONS:
 *      None.
 *
 * POSTCONDITIONS:
 *      (1) Uint32  Acknowledge value
 *
 * CAVEATS:
 *      None.
 *
 ******************************************************************************
 */

#define INT_GetAck()    (*INT_REG_IACK)

/******************************************************************************
 *
 * NAME:    INT_SetFunc()       -   Register interrupt function 
 *
 * PARAMETERS :
 *      (1) Uint32 num      <i>   Vector number
 *      (2) void *hdr       <i>   Interrupt function address
 *
 * DESCRIPTION:
 *
 * PRECONDITIONS:
 *      None.
 *
 * POSTCONDITIONS:
 *
 * CAVEATS:
 *      None.
 *
 ******************************************************************************
 */

#define INT_SetFunc(num, hdr)   SYS_SETSINT(num, hdr)

/******************************************************************************
 *
 * NAME:    INT_SetScuFunc()    -   SCU function registration 
 *
 * PARAMETERS :
 *      (1) Uint32 num      <i>   Vector number (SCU vector number)
 *      (2) void *hdr       <i>   SCU function address
 *
 * DESCRIPTION:
 *
 * PRECONDITIONS:
 *      None.
 *
 * POSTCONDITIONS:
 *
 * CAVEATS:
 *      None.
 *
 ******************************************************************************
 */

#define INT_SetScuFunc(num, hdr)    SYS_SETUINT(num, hdr)

/******************************************************************************
 *
 * NAME:    INT_GetFunc()       -   Get interrupt function address
 *
 * PARAMETERS :
 *      (1) Uint32 num      <i>   Vector number
 *
 * DESCRIPTION:
 *
 * PRECONDITIONS:
 *      None.
 *
 * POSTCONDITIONS:
 *      (1) void (*)()  Interrupt function address
 *
 * CAVEATS:
 *      None.
 *
 ******************************************************************************
 */

#define INT_GetFunc(num) SYS_GETSINT(num)

/******************************************************************************
 *
 * NAME:    INT_GetScuFunc()       -   Get SCU function address
 *
 * PARAMETERS :
 *      (1) Uint32 num      <i>   Vector number (SCU vector number)
 *
 * DESCRIPTION:
 *
 * PRECONDITIONS:
 *      None.
 *
 * POSTCONDITIONS:
 *      (1) void (*)()  SCU function address
 *
 * CAVEATS:
 *      None.
 *
 ******************************************************************************
 */

#define INT_GetScuFunc(num) SYS_GETUINT(num)

/*
 * STRUCTURE DECLARATIONS
 */

/*
 * TYPEDEFS
 */

/*
 * EXTERNAL VARIABLE DECLARATIONS
 */

/*
 * EXTERNAL FUNCTION PROTOTYPE  DECLARATIONS
 */

#endif  /* ifndef SEGA_INT_H */
