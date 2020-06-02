/*-----------------------------------------------------------------------------
 *  FILE: sega_tim.h
 *
 *      Copyright(c) 1994 SEGA.
 *
 *  PURPOSE:
 *
 *      Timer library header file.
 *
 *  DESCRIPTION:
 *
 *      Realize timer management.
 *
 *  AUTHOR(S)
 *
 *      1994-08-10  N.T Ver.1.00
 *      1995-11/14  H.O Ver.1.00a
 *
 *  MOD HISTORY:
 *      1995-11/14  Prohibition of external clock designation.
 *
 *-----------------------------------------------------------------------------
 */

/*
 * C VIRTUAL TYPES DEFINITIONS
 */
#include "SEGA_XPT.H"

/*
 * USER SUPPLIED INCLUDE FILES
 */
#include "SEGA_INT.H"
#include "SEGA_SYS.H"

#ifndef SEGA_TIM_H
#define SEGA_TIM_H

/*
 * GLOBAL DEFINES/MACROS DEFINES
 */
/**** SCU ********************************************************************/
/******** Registers ***********************************************************/
#define TIM_REG_T0C     ((volatile Uint32 *)0x25fe0090)          /* Thai Macon pair register     */
#define TIM_REG_T1S     ((volatile Uint32 *)0x25fe0094)          /* Timer 1 set resistor    */
#define TIM_REG_T1MD    ((volatile Uint32 *)0x25fe0098)          /* Timer 1 mode resistor     */
/******** Constants ***************************************************************/
/************ Bit position *****************************************************/
/**************** Timer 1 mode ***************************************************/
#define TIM_B_TENB      (0)                             /* Tie-mable resistor*/
#define TIM_B_T1MD      (8)                             /* Timer 1 mode resistor*/
/************ Value *************************************************************/
/**************** Timer 1 mode ***************************************************/
#define TIM_TENB_OFF    (0 << TIM_B_TENB)               /* Timer off            */
#define TIM_TENB_ON     (1 << TIM_B_TENB)               /* Timer on*/
#define TIM_T1MD_ALL_LINE   (0 << TIM_B_T1MD)           /* Line interrupt per line  */
#define TIM_T1MD_CST_LINE   (1 << TIM_B_T1MD)           /* Specified line interrupt*/

/**** CPU (Free running Timer) *****************************************************/
/******** Registers ***********************************************************/
#define TIM_REG_TIER    ((volatile Uint8  *)0xfffffe10)          /* Timer interrupt enable  */
#define TIM_REG_TCSR    ((volatile Uint8  *)0xfffffe11)          /* Timer control status   */
#define TIM_REG_FRC_H   ((volatile Uint8  *)0xfffffe12)          /* Free running counter H.   */
#define TIM_REG_FRC_L   ((volatile Uint8  *)0xfffffe13)          /* Free running counter L.   */
#define TIM_REG_OCRX_H  ((volatile Uint8  *)0xfffffe14)          /* Output pair a,B H. */
#define TIM_REG_OCRX_L  ((volatile Uint8  *)0xfffffe15)          /* Output pair a,B L */
#define TIM_REG_TCR     ((volatile Uint8  *)0xfffffe16)          /* Timer control   */
#define TIM_REG_TOCR    ((volatile Uint8  *)0xfffffe17)          /* Timer output pairs   */
#define TIM_REG_ICRA_H  ((volatile Uint8  *)0xfffffe18)          /* Input capture a H.   */
#define TIM_REG_ICRA_L  ((volatile Uint8  *)0xfffffe19)          /* Input capture a L.   */
/******** Constants ***************************************************************/
/************ Bit position *****************************************************/
/**************** Timer interrupt enable  *******************************************/
#define TIM_B_ICIAE     (7)                             /* ICAE              */
#define TIM_B_OCIAE     (3)                             /* OCIAE             */
#define TIM_B_OCIBE     (2)                             /* OCIBE             */
#define TIM_B_OVIE      (1)                             /* OVIE              */
/**************** Timer control stator *********************************************/
#define TIM_B_ICFA      (7)                             /* ICFA              */
#define TIM_B_OCFA      (3)                             /* OCFA              */
#define TIM_B_OCFB      (2)                             /* OCFB              */
#define TIM_B_OVI       (1)                             /* OVI               */
#define TIM_B_CCLRA     (0)                             /* CCLRA             */
/**************** Timer control **************************************************/
#define TIM_B_IEDGA     (7)                             /* IEDGA             */
#define TIM_B_BUFEA     (3)                             /* BUFEA             */
#define TIM_B_BUFEB     (2)                             /* BUFEB             */
#define TIM_B_CKS1      (1)                             /* CKS1              */
#define TIM_B_CKS0      (0)                             /* CKS0              */
/**************** Timer output pairs*********************************************/
#define TIM_B_OCRS      (4)                             /* OCRS              */
#define TIM_B_OEA       (3)                             /* OEA               */
#define TIM_B_OEB       (2)                             /* OEB               */
#define TIM_B_OLVLA     (1)                             /* LVLA             */
#define TIM_B_OLVLB     (0)                             /* OLVLB             */
/************ Mask *********************************************************/
/**************** Timer interrupt enable  *******************************************/
#define TIM_M_ICIAE     (1 << TIM_B_ICIAE)              /* ICAE              */
#define TIM_M_OCIAE     (1 << TIM_B_OCIAE)              /* OCIAE             */
#define TIM_M_OCIBE     (1 << TIM_B_OCIBE)              /* OCIBE             */
#define TIM_M_OVIE      (1 << TIM_B_OVIE )              /* OVIE              */
/**************** Timer control stator *********************************************/
#define TIM_M_ICFA      (1 << TIM_B_ICFA )              /* ICFA              */
#define TIM_M_OCFA      (1 << TIM_B_OCFA )              /* OCFA              */
#define TIM_M_OCFB      (1 << TIM_B_OCFB )              /* OCFB              */
#define TIM_M_OVI       (1 << TIM_B_OVI  )              /* OVI               */
#define TIM_M_CCLRA     (1 << TIM_B_CCLRA)              /* CCLRA             */
/**************** Timer control **************************************************/
#define TIM_M_IEDGA     (1 << TIM_B_IEDGA)              /* IEDGA             */
#define TIM_M_BUFEA     (1 << TIM_B_BUFEA)              /* BUFEA             */
#define TIM_M_BUFEB     (1 << TIM_B_BUFEB)              /* BUFEB             */
#define TIM_M_CKS       (3 << TIM_B_CKS0 )              /* CKS1,0            */
/**************** Timer output pairs*********************************************/
#define TIM_M_OCRS      (1 << TIM_B_OCRS )              /* OCRS              */
#define TIM_M_OEA       (1 << TIM_B_OEA  )              /* OEA               */
#define TIM_M_OEB       (1 << TIM_B_OEB  )              /* OEB               */
#define TIM_M_OLVLA     (1 << TIM_B_OLVLA)              /* LVLA             */
#define TIM_M_OLVLB     (1 << TIM_B_OLVLB)              /* OLVLB             */
/************ Value *************************************************************/
/**************** Timer interrupt enable ********************************************/
/******************** Timer interrupt enable ****************************************/
#define TIM_ICIAE_DIS   (0 << TIM_B_ICIAE)              /* ICAE inhibited          */
#define TIM_ICIAE_ENA   (1 << TIM_B_ICIAE)              /* ICAE allowed          */
/******************** Output pair A. ******************************************/
#define TIM_OCIAE_DIS   (0 << TIM_B_OCIAE)              /* OCFAE prohibited         */
#define TIM_OCIAE_ENA   (1 << TIM_B_OCIAE)              /* OCFAE allowed         */
/******************** Output pair B. ******************************************/
#define TIM_OCIBE_DIS   (0 << TIM_B_OCIBE)              /* OCFAE prohibited         */
#define TIM_OCIBE_ENA   (1 << TIM_B_OCIBE)              /* OCFBE authorization         */
/******************** Timer overflow interrupt enable ********************************/
#define TIM_OVIE_DIS    (0 << TIM_B_OVIE)               /* FOVI inhibited         */
#define TIM_OVIE_ENA    (1 << TIM_B_OVIE)               /* FOVI allowed         */
/**************** Timer control stator *********************************************/
/******************** ICFA ***************************************************/
#define TIM_ICFA_CLR    (0 << TIM_B_ICFA)               /* ICFA cleared       */
#define TIM_ICFA_SET    (1 << TIM_B_ICFA)               /* ICFA set       */
/******************** OCFA ***************************************************/
#define TIM_OCFA_CLR    (0 << TIM_B_OCFA)               /* OCFA cleared       */
#define TIM_OCFA_SET    (1 << TIM_B_OCFA)               /* OCFA set       */
/******************** OCFB ***************************************************/
#define TIM_OCFB_CLR    (0 << TIM_B_OCFB)               /* OCFB cleared       */
#define TIM_OCFB_SET    (1 << TIM_B_OCFB)               /* OCFB set       */
/******************** OVF ****************************************************/
#define TIM_OVF_CLR     (0 << TIM_B_OVI)                /* OVI Clear       */
#define TIM_OVF_SET     (1 << TIM_B_OVI)                /* OVI Set       */
/******************** CCLRA **************************************************/
#define TIM_CCLRA_CLR   (0 << TIM_B_CCLRA)              /* FRC clear inhibit   */
#define TIM_CCLRA_SET   (1 << TIM_B_CCLRA)              /* Clear FRC       */
/**************** Timer control **************************************************/
/******************** IEDGA **************************************************/
#define TIM_IEDGA_UP_EG (1 << TIM_B_IEDGA)              /* Up EG Capture      */
#define TIM_IEDGA_DW_EG (0 << TIM_B_IEDGA)              /* DW EG Capture      */
/******************** BUFEA **************************************************/
#define TIM_BUFEA_NO_US (0 << TIM_B_BUFEA)              /* ICRC buffered   */
#define TIM_BUFEA_US    (1 << TIM_B_BUFEA)              /* ICRC buffered   */
/******************** BUFEB **************************************************/
#define TIM_BUFEB_NO_US (0 << TIM_B_BUFEB)              /* ICRD buffered   */
#define TIM_BUFEB_US    (1 << TIM_B_BUFEB)              /* ICRD buffered   */
/******************** CKS1,0 *************************************************/
#define TIM_CKS_8       ( 0 << TIM_B_CKS0)              /* 8 counts        */
#define TIM_CKS_32      ( 1 << TIM_B_CKS0)              /* 32 counts        */
#define TIM_CKS_128     ( 2 << TIM_B_CKS0)              /* 128 counts       */
#define TIM_CKS_OUT     ( 3 << TIM_B_CKS0)              /* External count      */
/**************** Timer output pairs*********************************************/
/******************** OCRS ***************************************************/
#define TIM_OCRS_OCRA   ( 0 << TIM_B_OCRS)              /* OCRA selection        */
#define TIM_OCRS_OCRB   ( 1 << TIM_B_OCRS)              /* OCRB selection          */
/******************** OEA ****************************************************/
#define TIM_OEA_DIS     ( 0 << TIM_B_OEA)               /* OEA prohibited        */
#define TIM_OEA_ENA     ( 1 << TIM_B_OEA)               /* OEA allowed        */
/******************** OEB ****************************************************/
#define TIM_OEB_DIS     ( 0 << TIM_B_OEB)               /* OEB inhibited        */
#define TIM_OEB_ENA     ( 1 << TIM_B_OEB)               /* OEB allowed        */
/******************** LVLA **************************************************/
#define TIM_OLVLA_0     ( 0 << TIM_B_OLVLA)             /* "0"output 　        */
#define TIM_OLVLA_1     ( 1 << TIM_B_OLVLA)             /* "1"output         */
/******************** OLVLB **************************************************/
#define TIM_OLVLB_0     ( 0 << TIM_B_OLVLB)             /* "0"output 　        */
#define TIM_OLVLB_1     ( 1 << TIM_B_OLVLB)             /* "1"output         */
/**** Processing macros *************************************************************/
/**** Memory light ***********************************************************/
#define TIM_POKE_B(adr, data)   (*((volatile Uint8 *)(adr)) = ((Uint8)(data)))  /*Bytes*/
#define TIM_POKE_W(adr, data)   (*((volatile Uint16 *)(adr)) = ((Uint16)(data)))/*Word*/
#define TIM_POKE_L(adr, data)   (*((volatile Uint32 *)(adr)) = ((Uint32)(data)))/*Long*/
/**** Memory read ***********************************************************/
#define TIM_PEEK_B(adr)         (*((volatile Uint8 *)(adr)))                    /*Bytes*/
#define TIM_PEEK_W(adr)         (*((volatile Uint16 *)(adr)))                   /*Word*/
#define TIM_PEEK_L(adr)         (*((volatile Uint32 *)(adr)))                   /*Long*/

/*****************************************************************************/
/*****************************************************************************/
/**** Function format macros *********************************************************/
/*****************************************************************************/
/*****************************************************************************/

/*****************************************************************************/
/******* SCU *****************************************************************/
/*****************************************************************************/

/******************************************************************************
 *
 * NAME:    TIM_T0_ENABLE           - Timer 0 interrupt enable
 *
 * PARAMETERS :
 *      None.
 *
 * POSTCONDITIONS:
 *      None
 *
 ******************************************************************************
 */
#define TIM_T0_ENABLE()\
    do{\
        INT_ChgMsk(INT_MSK_TIM0, INT_MSK_NULL);\
    }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_T0_DISABLE          - Timer 0 interrupt disabled
 *
 * PARAMETERS :
 *      None.
 *
 * POSTCONDITIONS:
 *      None
 *
 ******************************************************************************
 */
#define TIM_T0_DISABLE()\
    do{\
        INT_ChgMsk(INT_MSK_NULL, INT_MSK_TIM0);\
    }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_T1_ENABLE           - Timer 1 interrupt enable
 *
 * PARAMETERS :
 *      None.
 *
 * POSTCONDITIONS:
 *      None
 *
 ******************************************************************************
 */
#define TIM_T1_ENABLE()\
    do{\
        INT_ChgMsk(INT_MSK_TIM1, INT_MSK_NULL);\
    }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_T1_DISABLE          - Timer 1 interrupt disabled
 *
 * PARAMETERS :
 *      None.
 *
 * POSTCONDITIONS:
 *      None
 *
 ******************************************************************************
 */
#define TIM_T1_DISABLE()\
    do{\
        INT_ChgMsk(INT_MSK_NULL, INT_MSK_TIM1);\
    }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_T0_SET_CMP          - Timer 0 compare register setting
 *
 * PARAMETERS :
 *      (1) Uint32 time_cmp         - <i>   Timer 0 compare register setting value
 *
 * POSTCONDITIONS:
 *      None
 *
 ******************************************************************************
 */
#define TIM_T0_SET_CMP(time_cmp)\
            TIM_POKE_L(TIM_REG_T0C, (time_cmp))

/******************************************************************************
 *
 * NAME:    TIM_T1_SET_DATA         - Timer 1 set data register
 *
 * PARAMETERS :
 *      (1) Uint32 time_data        - <i>   Timer 1 set data register setting value
 *
 * POSTCONDITIONS:
 *      None
 *
 ******************************************************************************
 */
#define TIM_T1_SET_DATA(time_data)\
            TIM_POKE_L(TIM_REG_T1S, (time_data))

/******************************************************************************
 *
 * NAME:    TIM_T1_SET_MODE         - Timer 1 mode register setting
 *
 * PARAMETERS :
 *      (1) Uint32 time_mode        - <i>   Timer 1 mode register setting value
 *
 * POSTCONDITIONS:
 *      None
 *
 ******************************************************************************
 */
#define TIM_T1_SET_MODE(time_mode)\
            TIM_POKE_L(TIM_REG_T1MD, (time_mode))

/******************************************************************************
 *
 * NAME:    TIM_T1_SET_MODE         - Timer 1 mode register setting
 *
 * PARAMETERS :
 *      (1) Uint32 time_mode        - <i>   Timer 1 mode register setting value
 *
 * POSTCONDITIONS:
 *      None
 *
 ******************************************************************************
 */
#define TIM_T1_SET_MODE(time_mode)\
            TIM_POKE_L(TIM_REG_T1MD, (time_mode))

/*****************************************************************************/
/******* CPU *****************************************************************/
/*****************************************************************************/
/******************************************************************************
 *
 * NAME:    TIM_FRT_INIT            - FRT initialization
 *
 * PARAMETERS :
 *      (1) Uint32 mode             - <i>   Number of divisions
 *
 * POSTCONDITIONS:
 *      None
 * NOTICE:
 *      External clock selection prohibition version.
 *
 ******************************************************************************
 */
#if 0
#define TIM_FRT_INIT(mode)\
            TIM_POKE_B(TIM_REG_TCR ,\
                       (TIM_PEEK_B(TIM_REG_TCR) & ~TIM_M_CKS) | (mode))
#else
#define TIM_FRT_INIT(mode)\
	do{\
		unsigned char _tcr = (unsigned char)(TIM_PEEK_B(TIM_REG_TCR) & ~TIM_M_CKS);\
		if( ( ( mode ) & TIM_M_CKS ) != TIM_M_CKS ){\
			_tcr |= ( ( mode ) & TIM_M_CKS );\
            TIM_POKE_B(TIM_REG_TCR , _tcr);\
		}\
	}while(0)
#endif

/******************************************************************************
 *
 * NAME:    TIM_FRT_SET_16          - Counter value setting (16 bits)
 *
 * PARAMETERS :
 *      (1) Uint16 cnt              - <i>   Counter value
 *
 * POSTCONDITIONS:
 *      None
 *
 ******************************************************************************
 */
#define TIM_FRT_SET_16(cnt)\
        do{\
            TIM_FRT_SET_FRC(cnt);\
        }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_FRT_GET_16          - Get counter value (16 bits)
 *
 * PARAMETERS :
 *      None
 *
 * POSTCONDITIONS:
 *      (1) Uint16 cnt              - <o>   Counter value
 *
 ******************************************************************************
 */
#define TIM_FRT_GET_16()\
        TIM_FRT_GET_FRC()

/******************************************************************************
 *
 * NAME:    TIM_FRT_DELAY_16        - Wait for time (16 bits)
 *
 * PARAMETERS :
 *      (1) Uint16 cnt              - <i>   Time-waiting counter value
 *
 * POSTCONDITIONS:
 *      None
 *
 ******************************************************************************
 */
#define TIM_FRT_DELAY_16(cnt)\
        do{\
            TIM_FRT_SET_16(0);\
            while((cnt) != TIM_FRT_GET_16());\
        }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_FRT_CNT_TO_MCR      -   Counter value-> microsecond conversion
 *
 * PARAMETERS :
 *      (1) Uint32 count            - <i>   Counter value
 *
 * POSTCONDITIONS:
 *      None
 *
 ******************************************************************************
 */
#define TIM_FRT_CNT_TO_MCR(count)\
    (\
    (((*(Uint16 *)0x25f80004 & 0x1) == 0x1) ?   /* PAL? */\
     ((SYS_GETSYSCK == 0) ? (Float32)0.037470726 : (Float32)0.035164835 ) :/*PAL 26, 28*/\
     ((SYS_GETSYSCK == 0) ? (Float32)0.037210548 : (Float32)0.03492059 )) /*NT 26, 28*/\
     * (count) * (8 << ((TIM_PEEK_B(TIM_REG_TCR) & TIM_M_CKS) << 1)))

/******************************************************************************
 *
 * NAME:    TIM_FRT_MCR_TO_CNT      -   マイクロ秒->カウンタ値変換
 *
 * PARAMETERS :
 *      (1) Uint32 mcr              - <i>   マイクロ秒値
 *
 * POSTCONDITIONS:
 *      None
 *
 ******************************************************************************
 */
#define TIM_FRT_MCR_TO_CNT(mcr)\
    ((mcr) /\
    (((*(volatile Uint16 *)0x25f80004 & 0x1) == 0x1) ?   /* PAL? */\
     ((SYS_GETSYSCK == 0) ? (Float32)0.037470726 : (Float32)0.035164835 ) :/*PAL 26, 28*/\
     ((SYS_GETSYSCK == 0) ? (Float32)0.037210548 : (Float32)0.03492059 )) /*NT 26, 28*/\
    / (8 << ((TIM_PEEK_B(TIM_REG_TCR) & TIM_M_CKS) << 1)))
/******************************************************************************
 *
 * NAME:    TIM_FRT_SET_TIER        - Timer interrupt enable register setting
 *
 * PARAMETERS :
 *      (1) Uint8 reg               - <i>   Setting value
 *
 * POSTCONDITIONS:
 *      None
 *
 ******************************************************************************
 */
#define TIM_FRT_SET_TIER(reg)\
        TIM_POKE_B(TIM_REG_TIER, (reg))

/******************************************************************************
 *
 * NAME:    TIM_FRT_SET_TCSR        - Timer control status register setting
 *
 * PARAMETERS :
 *      (1) Uint8 reg               - <i>   Setting value
 *
 * POSTCONDITIONS:
 *      None
 *
 ******************************************************************************
 */
#define TIM_FRT_SET_TCSR(reg)\
        TIM_POKE_B(TIM_REG_TCSR, (reg))

/******************************************************************************
 *
 * NAME:    TIM_FRT_SET_FRC         - Free running counter setting
 *
 * PARAMETERS :
 *      (1) Uint16 reg              - <i>   Setting value
 *
 * POSTCONDITIONS:
 *      None
 *
 ******************************************************************************
 */
#define TIM_FRT_SET_FRC(reg)\
        do{\
            TIM_POKE_B(TIM_REG_FRC_H, ((reg) >> 8));\
            TIM_POKE_B(TIM_REG_FRC_L, (reg));\
        }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_FRT_SET_OCRA        - Output compare register A setting
 *
 * PARAMETERS :
 *      (1) Uint16 reg              - <i>   Setting value
 *
 * POSTCONDITIONS:
 *      None
 *
 ******************************************************************************
 */
#define TIM_FRT_SET_OCRA(reg)\
        do{\
            TIM_FRT_SET_TOCR(((TIM_FRT_GET_TOCR()) & ~TIM_M_OCRS) |\
            TIM_OCRS_OCRA);\
            TIM_POKE_B(TIM_REG_OCRX_H, ((reg) >> 8));\
            TIM_POKE_B(TIM_REG_OCRX_L, (reg));\
        }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_FRT_SET_OCRB        - Output compare register B setting
 *
 * PARAMETERS :
 *      (1) Uint16 reg              - <i>   Setting value
 *
 * POSTCONDITIONS:
 *      None
 *
 ******************************************************************************
 */
#define TIM_FRT_SET_OCRB(reg)\
        do{\
            TIM_FRT_SET_TOCR(((TIM_FRT_GET_TOCR()) & ~TIM_M_OCRS) |\
            TIM_OCRS_OCRB);\
            TIM_POKE_B(TIM_REG_OCRX_H, ((reg) >> 8));\
            TIM_POKE_B(TIM_REG_OCRX_L, (reg));\
        }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_FRT_SET_TCR         - Timer control register setting
 *
 * PARAMETERS :
 *      (1) Uint8 reg               - <i>   Setting value
 *
 * POSTCONDITIONS:
 *      None
 * NOTICE:
 *      External clock selection prohibition version.
 *
 ******************************************************************************
 */
#if 0
#define TIM_FRT_SET_TCR(reg)\
            TIM_POKE_B(TIM_REG_TCR, (reg))
#else
#define TIM_FRT_SET_TCR(reg)\
	do{\
		unsigned char _tcr;\
		if(((reg) & TIM_M_CKS ) != TIM_M_CKS ){\
			_tcr = (unsigned char)( (reg) & 0x83 );\
		}\
		else{\
			_tcr = (unsigned char)( (reg) & 0x80 );\
		}\
        TIM_POKE_B(TIM_REG_TCR, _tcr);\
	}while(0)
#endif

/******************************************************************************
 *
 * NAME:    TIM_FRT_SET_TOCR        - Timer output compare control
 *                                    Register setting
 * PARAMETERS :
 *      (1) Uint8 reg               - <i>   Setting value
 *
 * POSTCONDITIONS:
 *      None
 *
 ******************************************************************************
 */
#define TIM_FRT_SET_TOCR(reg)\
            TIM_POKE_B(TIM_REG_TOCR, (reg))

/******************************************************************************
 *
 * NAME:    TIM_FRT_GET_TIER        - Get timer interrupt enable register
 *
 * PARAMETERS :
 *      None
 *
 * POSTCONDITIONS:
 *      (1) Uint8 reg               - <o>   Register value
 *
 ******************************************************************************
 */
#define TIM_FRT_GET_TIER()\
            TIM_PEEK_B(TIM_REG_TIER)

/******************************************************************************
 *
 * NAME:    TIM_FRT_GET_TCSR        - Get timer control status register
 *
 * PARAMETERS :
 *      None
 *
 * POSTCONDITIONS:
 *      (1) Uint8 reg               - <o>   Register value
 *
 ******************************************************************************
 */
#define TIM_FRT_GET_TCSR()\
            TIM_PEEK_B(TIM_REG_TCSR)

/******************************************************************************
 *
 * NAME:    TIM_FRT_GET_FRC          - Get free running count
 *
 * PARAMETERS :
 *      None
 *
 * POSTCONDITIONS:
 *      (1) Uint16 reg               - <o>   Register value
 *
 ******************************************************************************
 */
#define TIM_FRT_GET_FRC()\
            (TIM_PEEK_B(TIM_REG_FRC_H) << 8 | TIM_PEEK_B(TIM_REG_FRC_L))

/******************************************************************************
 *
 * NAME:    TIM_FRT_GET_OCRA         - Output compare register A acquisition
 *
 * PARAMETERS :
 *      (1) Uint16 reg               - <o>   Register value
 *
 * POSTCONDITIONS:
 *
 ******************************************************************************
 */
#define TIM_FRT_GET_OCRA(reg)\
        do{\
            TIM_FRT_SET_TOCR(((TIM_FRT_GET_TOCR()) & ~TIM_M_OCRS) |\
            TIM_OCRS_OCRA);\
            (reg) = (TIM_PEEK_B(TIM_REG_OCRX_H) << 8) |\
                                       TIM_PEEK_B(TIM_REG_OCRX_L);\
        }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_FRT_GET_OCRB         - Get output compare register B
 *
 * PARAMETERS :
 *      (1) Uint16 reg               - <o>   Register value
 *
 * POSTCONDITIONS:
 *
 ******************************************************************************
 */
#define TIM_FRT_GET_OCRB(reg)\
        do{\
            TIM_FRT_SET_TOCR(((TIM_FRT_GET_TOCR()) & ~TIM_M_OCRS) |\
            TIM_OCRS_OCRB);\
            (reg) = (TIM_PEEK_B(TIM_REG_OCRX_H) << 8) |\
                                       TIM_PEEK_B(TIM_REG_OCRX_L);\
        }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_FRT_GET_TCR          - Get timer control register
 *
 * PARAMETERS :
 *      None
 *
 * POSTCONDITIONS:
 *      (1) Uint8  reg               - <o>   Register value
 *
 ******************************************************************************
 */
#define TIM_FRT_GET_TCR()\
            TIM_PEEK_B(TIM_REG_TCR)
                                                              
/******************************************************************************
 *
 * NAME:    TIM_FRT_GET_TOCR         - Timer output compare control
 *                                     Register acquisition
 * PARAMETERS :
 *      None
 *
 * POSTCONDITIONS:
 *      (1) Uint8 reg                - <o>   Register value
 *
 ******************************************************************************
 */
#define TIM_FRT_GET_TOCR()\
            TIM_PEEK_B(TIM_REG_TOCR)

/******************************************************************************
 *
 * NAME:    TIM_FRT_GET_ICRA         - Get input capture register A
 *
 * PARAMETERS :
 *      None
 *
 * POSTCONDITIONS:
 *      (1) Uint16 reg               - <o>   Register value
 *
 ******************************************************************************
 */
#define TIM_FRT_GET_ICRA()\
            ((TIM_PEEK_B(TIM_REG_ICRA_H) << 8) | TIM_PEEK_B(TIM_REG_ICRA_L))

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

#endif  /* ifndef SEGA_TIM_H */
