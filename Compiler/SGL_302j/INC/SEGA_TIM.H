/*-----------------------------------------------------------------------------
 *  FILE: sega_tim.h
 *
 *      Copyright(c) 1994 SEGA.
 *
 *  PURPOSE:
 *
 *      タイマライブラリヘッダファイル。
 *
 *  DESCRIPTION:
 *
 *      タイマ管理を実現する。
 *
 *  AUTHOR(S)
 *
 *      1994-08-10  N.T Ver.1.00
 *      1995-11/14  H.O Ver.1.00a
 *
 *  MOD HISTORY:
 *      1995-11/14  外部クロック指定の禁止。
 *
 *-----------------------------------------------------------------------------
 */

/*
 * C VIRTUAL TYPES DEFINITIONS
 */
#include "sega_xpt.h"

/*
 * USER SUPPLIED INCLUDE FILES
 */
#include "sega_int.h"
#include "sega_sys.h"

#ifndef SEGA_TIM_H
#define SEGA_TIM_H

/*
 * GLOBAL DEFINES/MACROS DEFINES
 */
/**** SCU ********************************************************************/
/******** レジスタ ***********************************************************/
#define TIM_REG_T0C     ((volatile Uint32 *)0x25fe0090)          /* ﾀｲﾏｺﾝﾍﾟｱﾚｼﾞｽﾀ     */
#define TIM_REG_T1S     ((volatile Uint32 *)0x25fe0094)          /* ﾀｲﾏ1ｾｯﾄﾚｼﾞｽﾀ　    */
#define TIM_REG_T1MD    ((volatile Uint32 *)0x25fe0098)          /* ﾀｲﾏ1ﾓｰﾄﾞﾚｼﾞｽﾀ     */
/******** 定数 ***************************************************************/
/************ ビット位置 *****************************************************/
/**************** ﾀｲﾏ1ﾓｰﾄﾞ ***************************************************/
#define TIM_B_TENB      (0)                             /* ﾀｲﾏｲﾈｰﾌﾞﾙﾚｼﾞｽﾀ　　*/
#define TIM_B_T1MD      (8)                             /* ﾀｲﾏ1ﾓｰﾄﾞﾚｼﾞｽﾀ 　　*/
/************ 値 *************************************************************/
/**************** ﾀｲﾏ1ﾓｰﾄﾞ ***************************************************/
#define TIM_TENB_OFF    (0 << TIM_B_TENB)               /* ﾀｲﾏoff            */
#define TIM_TENB_ON     (1 << TIM_B_TENB)               /* ﾀｲﾏon         　　*/
#define TIM_T1MD_ALL_LINE   (0 << TIM_B_T1MD)           /* 毎ライン割り込み  */
#define TIM_T1MD_CST_LINE   (1 << TIM_B_T1MD)           /* 指定ライン割り込み*/

/**** CPU (ﾌﾘｰﾗﾝﾆﾝｸﾞﾀｲﾏ) *****************************************************/
/******** レジスタ ***********************************************************/
#define TIM_REG_TIER    ((volatile Uint8  *)0xfffffe10)          /* ﾀｲﾏｲﾝﾀﾗﾌﾟﾄｲﾈｰﾌﾞﾙ  */
#define TIM_REG_TCSR    ((volatile Uint8  *)0xfffffe11)          /* ﾀｲﾏｺﾝﾄﾛｰﾙ･ｽﾃｰﾀｽ   */
#define TIM_REG_FRC_H   ((volatile Uint8  *)0xfffffe12)          /* ﾌﾘｰﾗﾝﾆﾝｸﾞｶｳﾝﾀ H   */
#define TIM_REG_FRC_L   ((volatile Uint8  *)0xfffffe13)          /* ﾌﾘｰﾗﾝﾆﾝｸﾞｶｳﾝﾀ L   */
#define TIM_REG_OCRX_H  ((volatile Uint8  *)0xfffffe14)          /* ｱｳﾄﾌﾟｯﾄｺﾝﾍﾟｱA,B H */
#define TIM_REG_OCRX_L  ((volatile Uint8  *)0xfffffe15)          /* ｱｳﾄﾌﾟｯﾄｺﾝﾍﾟｱA,B L */
#define TIM_REG_TCR     ((volatile Uint8  *)0xfffffe16)          /* ﾀｲﾏｺﾝﾄﾛｰﾙ　　　   */
#define TIM_REG_TOCR    ((volatile Uint8  *)0xfffffe17)          /* ﾀｲﾏｱｳﾄﾌﾟｯﾄｺﾝﾍﾟｱ   */
#define TIM_REG_ICRA_H  ((volatile Uint8  *)0xfffffe18)          /* ｲﾝﾌﾟｯﾄｷｬﾌﾟﾁｬA H   */
#define TIM_REG_ICRA_L  ((volatile Uint8  *)0xfffffe19)          /* ｲﾝﾌﾟｯﾄｷｬﾌﾟﾁｬA L   */
/******** 定数 ***************************************************************/
/************ ビット位置 *****************************************************/
/**************** ﾀｲﾏｲﾝﾀﾗﾌﾟﾄｲﾈｰﾌﾞ  *******************************************/
#define TIM_B_ICIAE     (7)                             /* ICAE              */
#define TIM_B_OCIAE     (3)                             /* OCIAE             */
#define TIM_B_OCIBE     (2)                             /* OCIBE             */
#define TIM_B_OVIE      (1)                             /* OVIE              */
/**************** ﾀｲﾏｺﾝﾄﾛｰﾙ･ｽﾃｰﾀ *********************************************/
#define TIM_B_ICFA      (7)                             /* ICFA              */
#define TIM_B_OCFA      (3)                             /* OCFA              */
#define TIM_B_OCFB      (2)                             /* OCFB              */
#define TIM_B_OVI       (1)                             /* OVI               */
#define TIM_B_CCLRA     (0)                             /* CCLRA             */
/**************** ﾀｲﾏｺﾝﾄﾛｰﾙ **************************************************/
#define TIM_B_IEDGA     (7)                             /* IEDGA             */
#define TIM_B_BUFEA     (3)                             /* BUFEA             */
#define TIM_B_BUFEB     (2)                             /* BUFEB             */
#define TIM_B_CKS1      (1)                             /* CKS1              */
#define TIM_B_CKS0      (0)                             /* CKS0              */
/**************** ﾀｲﾏｱｳﾄﾌﾟｯﾄｺﾝﾍﾟｱ*********************************************/
#define TIM_B_OCRS      (4)                             /* OCRS              */
#define TIM_B_OEA       (3)                             /* OEA               */
#define TIM_B_OEB       (2)                             /* OEB               */
#define TIM_B_OLVLA     (1)                             /* OLVLA             */
#define TIM_B_OLVLB     (0)                             /* OLVLB             */
/************ マスク *********************************************************/
/**************** ﾀｲﾏｲﾝﾀﾗﾌﾟﾄｲﾈｰﾌﾞ  *******************************************/
#define TIM_M_ICIAE     (1 << TIM_B_ICIAE)              /* ICAE              */
#define TIM_M_OCIAE     (1 << TIM_B_OCIAE)              /* OCIAE             */
#define TIM_M_OCIBE     (1 << TIM_B_OCIBE)              /* OCIBE             */
#define TIM_M_OVIE      (1 << TIM_B_OVIE )              /* OVIE              */
/**************** ﾀｲﾏｺﾝﾄﾛｰﾙ･ｽﾃｰﾀ *********************************************/
#define TIM_M_ICFA      (1 << TIM_B_ICFA )              /* ICFA              */
#define TIM_M_OCFA      (1 << TIM_B_OCFA )              /* OCFA              */
#define TIM_M_OCFB      (1 << TIM_B_OCFB )              /* OCFB              */
#define TIM_M_OVI       (1 << TIM_B_OVI  )              /* OVI               */
#define TIM_M_CCLRA     (1 << TIM_B_CCLRA)              /* CCLRA             */
/**************** ﾀｲﾏｺﾝﾄﾛｰﾙ **************************************************/
#define TIM_M_IEDGA     (1 << TIM_B_IEDGA)              /* IEDGA             */
#define TIM_M_BUFEA     (1 << TIM_B_BUFEA)              /* BUFEA             */
#define TIM_M_BUFEB     (1 << TIM_B_BUFEB)              /* BUFEB             */
#define TIM_M_CKS       (3 << TIM_B_CKS0 )              /* CKS1,0            */
/**************** ﾀｲﾏｱｳﾄﾌﾟｯﾄｺﾝﾍﾟｱ*********************************************/
#define TIM_M_OCRS      (1 << TIM_B_OCRS )              /* OCRS              */
#define TIM_M_OEA       (1 << TIM_B_OEA  )              /* OEA               */
#define TIM_M_OEB       (1 << TIM_B_OEB  )              /* OEB               */
#define TIM_M_OLVLA     (1 << TIM_B_OLVLA)              /* OLVLA             */
#define TIM_M_OLVLB     (1 << TIM_B_OLVLB)              /* OLVLB             */
/************ 値 *************************************************************/
/**************** ﾀｲﾏｲﾝﾀﾗﾌﾟﾄｲﾈｰﾌﾞ ********************************************/
/******************** ﾀｲﾏｲﾝﾀﾗﾌﾟﾄｲﾈｰﾌﾞ ****************************************/
#define TIM_ICIAE_DIS   (0 << TIM_B_ICIAE)              /* ICAE禁止          */
#define TIM_ICIAE_ENA   (1 << TIM_B_ICIAE)              /* ICAE許可          */
/******************** ｱｳﾄﾌﾟｯﾄｺﾝﾍﾟｱA ******************************************/
#define TIM_OCIAE_DIS   (0 << TIM_B_OCIAE)              /* OCFAE禁止         */
#define TIM_OCIAE_ENA   (1 << TIM_B_OCIAE)              /* OCFAE許可         */
/******************** ｱｳﾄﾌﾟｯﾄｺﾝﾍﾟｱB ******************************************/
#define TIM_OCIBE_DIS   (0 << TIM_B_OCIBE)              /* OCFAE禁止         */
#define TIM_OCIBE_ENA   (1 << TIM_B_OCIBE)              /* OCFBE許可         */
/******************** ﾀｲﾏｵｰﾊﾞﾌﾛｰｲﾝﾀﾗﾌﾟﾄｲﾈｰﾌﾞﾙ ********************************/
#define TIM_OVIE_DIS    (0 << TIM_B_OVIE)               /* FOVI 禁止         */
#define TIM_OVIE_ENA    (1 << TIM_B_OVIE)               /* FOVI 許可         */
/**************** ﾀｲﾏｺﾝﾄﾛｰﾙ･ｽﾃｰﾀ *********************************************/
/******************** ICFA ***************************************************/
#define TIM_ICFA_CLR    (0 << TIM_B_ICFA)               /* ICFA クリア       */
#define TIM_ICFA_SET    (1 << TIM_B_ICFA)               /* ICFA セット       */
/******************** OCFA ***************************************************/
#define TIM_OCFA_CLR    (0 << TIM_B_OCFA)               /* OCFA クリア       */
#define TIM_OCFA_SET    (1 << TIM_B_OCFA)               /* OCFA セット       */
/******************** OCFB ***************************************************/
#define TIM_OCFB_CLR    (0 << TIM_B_OCFB)               /* OCFB クリア       */
#define TIM_OCFB_SET    (1 << TIM_B_OCFB)               /* OCFB セット       */
/******************** OVF ****************************************************/
#define TIM_OVF_CLR     (0 << TIM_B_OVI)                /* OVI  クリア       */
#define TIM_OVF_SET     (1 << TIM_B_OVI)                /* OVI  セット       */
/******************** CCLRA **************************************************/
#define TIM_CCLRA_CLR   (0 << TIM_B_CCLRA)              /* FRC  クリア禁止   */
#define TIM_CCLRA_SET   (1 << TIM_B_CCLRA)              /* FRCをクリア       */
/**************** ﾀｲﾏｺﾝﾄﾛｰﾙ **************************************************/
/******************** IEDGA **************************************************/
#define TIM_IEDGA_UP_EG (1 << TIM_B_IEDGA)              /* UP EG ｷｬﾌﾟﾁｬ      */
#define TIM_IEDGA_DW_EG (0 << TIM_B_IEDGA)              /* DW EG ｷｬﾌﾟﾁｬ      */
/******************** BUFEA **************************************************/
#define TIM_BUFEA_NO_US (0 << TIM_B_BUFEA)              /* ICRCをﾊﾞｯﾌｧ使用   */
#define TIM_BUFEA_US    (1 << TIM_B_BUFEA)              /* ICRCをﾊﾞｯﾌｧ使用   */
/******************** BUFEB **************************************************/
#define TIM_BUFEB_NO_US (0 << TIM_B_BUFEB)              /* ICRDをﾊﾞｯﾌｧ使用   */
#define TIM_BUFEB_US    (1 << TIM_B_BUFEB)              /* ICRDをﾊﾞｯﾌｧ使用   */
/******************** CKS1,0 *************************************************/
#define TIM_CKS_8       ( 0 << TIM_B_CKS0)              /* 8 カウント        */
#define TIM_CKS_32      ( 1 << TIM_B_CKS0)              /* 32カウント        */
#define TIM_CKS_128     ( 2 << TIM_B_CKS0)              /* 128カウント       */
#define TIM_CKS_OUT     ( 3 << TIM_B_CKS0)              /* 外部カウント      */
/**************** ﾀｲﾏｱｳﾄﾌﾟｯﾄｺﾝﾍﾟｱ*********************************************/
/******************** OCRS ***************************************************/
#define TIM_OCRS_OCRA   ( 0 << TIM_B_OCRS)              /* OCRA選択　        */
#define TIM_OCRS_OCRB   ( 1 << TIM_B_OCRS)              /* OCRB選択          */
/******************** OEA ****************************************************/
#define TIM_OEA_DIS     ( 0 << TIM_B_OEA)               /* OEA禁止 　        */
#define TIM_OEA_ENA     ( 1 << TIM_B_OEA)               /* OEA許可 　        */
/******************** OEB ****************************************************/
#define TIM_OEB_DIS     ( 0 << TIM_B_OEB)               /* OEB禁止 　        */
#define TIM_OEB_ENA     ( 1 << TIM_B_OEB)               /* OEB許可 　        */
/******************** OLVLA **************************************************/
#define TIM_OLVLA_0     ( 0 << TIM_B_OLVLA)             /* "0"出力 　        */
#define TIM_OLVLA_1     ( 1 << TIM_B_OLVLA)             /* "1"出力　         */
/******************** OLVLB **************************************************/
#define TIM_OLVLB_0     ( 0 << TIM_B_OLVLB)             /* "0"出力 　        */
#define TIM_OLVLB_1     ( 1 << TIM_B_OLVLB)             /* "1"出力　         */
/**** 処理マクロ *************************************************************/
/**** メモリライト ***********************************************************/
#define TIM_POKE_B(adr, data)   (*((volatile Uint8 *)(adr)) = ((Uint8)(data)))  /*ﾊﾞｲﾄ*/
#define TIM_POKE_W(adr, data)   (*((volatile Uint16 *)(adr)) = ((Uint16)(data)))/*ﾜｰﾄﾞ*/
#define TIM_POKE_L(adr, data)   (*((volatile Uint32 *)(adr)) = ((Uint32)(data)))/*ﾛﾝｸﾞ*/
/**** メモリリード ***********************************************************/
#define TIM_PEEK_B(adr)         (*((volatile Uint8 *)(adr)))                    /*ﾊﾞｲﾄ*/
#define TIM_PEEK_W(adr)         (*((volatile Uint16 *)(adr)))                   /*ﾜｰﾄﾞ*/
#define TIM_PEEK_L(adr)         (*((volatile Uint32 *)(adr)))                   /*ﾛﾝｸﾞ*/

/*****************************************************************************/
/*****************************************************************************/
/**** 関数形式マクロ *********************************************************/
/*****************************************************************************/
/*****************************************************************************/

/*****************************************************************************/
/******* SCU *****************************************************************/
/*****************************************************************************/

/******************************************************************************
 *
 * NAME:    TIM_T0_ENABLE           - タイマ0割り込みイネーブル
 *
 * PARAMETERS :
 *      なし。
 *
 * POSTCONDITIONS:
 *      なし
 *
 ******************************************************************************
 */
#define TIM_T0_ENABLE()\
    do{\
        INT_ChgMsk(INT_MSK_TIM0, INT_MSK_NULL);\
    }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_T0_DISABLE          - タイマ0割り込みディセーブル
 *
 * PARAMETERS :
 *      なし。
 *
 * POSTCONDITIONS:
 *      なし
 *
 ******************************************************************************
 */
#define TIM_T0_DISABLE()\
    do{\
        INT_ChgMsk(INT_MSK_NULL, INT_MSK_TIM0);\
    }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_T1_ENABLE           - タイマ1割り込みイネーブル
 *
 * PARAMETERS :
 *      なし。
 *
 * POSTCONDITIONS:
 *      なし
 *
 ******************************************************************************
 */
#define TIM_T1_ENABLE()\
    do{\
        INT_ChgMsk(INT_MSK_TIM1, INT_MSK_NULL);\
    }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_T1_DISABLE          - タイマ1割り込みディセーブル
 *
 * PARAMETERS :
 *      なし。
 *
 * POSTCONDITIONS:
 *      なし
 *
 ******************************************************************************
 */
#define TIM_T1_DISABLE()\
    do{\
        INT_ChgMsk(INT_MSK_NULL, INT_MSK_TIM1);\
    }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_T0_SET_CMP          - タイマ0コンペアレジスタ設定
 *
 * PARAMETERS :
 *      (1) Uint32 time_cmp         - <i>   タイマ0コンペアレジスタ設定値
 *
 * POSTCONDITIONS:
 *      なし
 *
 ******************************************************************************
 */
#define TIM_T0_SET_CMP(time_cmp)\
            TIM_POKE_L(TIM_REG_T0C, (time_cmp))

/******************************************************************************
 *
 * NAME:    TIM_T1_SET_DATA         - タイマ1セットデータレジスタ
 *
 * PARAMETERS :
 *      (1) Uint32 time_data        - <i>   タイマ1セットデータレジスタ設定値
 *
 * POSTCONDITIONS:
 *      なし
 *
 ******************************************************************************
 */
#define TIM_T1_SET_DATA(time_data)\
            TIM_POKE_L(TIM_REG_T1S, (time_data))

/******************************************************************************
 *
 * NAME:    TIM_T1_SET_MODE         - タイマ1モードレジスタ設定
 *
 * PARAMETERS :
 *      (1) Uint32 time_mode        - <i>   タイマ1モードレジスタ設定値
 *
 * POSTCONDITIONS:
 *      なし
 *
 ******************************************************************************
 */
#define TIM_T1_SET_MODE(time_mode)\
            TIM_POKE_L(TIM_REG_T1MD, (time_mode))

/******************************************************************************
 *
 * NAME:    TIM_T1_SET_MODE         - タイマ1モードレジスタ設定
 *
 * PARAMETERS :
 *      (1) Uint32 time_mode        - <i>   タイマ1モードレジスタ設定値
 *
 * POSTCONDITIONS:
 *      なし
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
 * NAME:    TIM_FRT_INIT            - FRT初期化
 *
 * PARAMETERS :
 *      (1) Uint32 mode             - <i>   分周数
 *
 * POSTCONDITIONS:
 *      なし
 * NOTICE:
 *      外部クロック選択の禁止対策版。
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
 * NAME:    TIM_FRT_SET_16          - カウンタ値設定(16ビット)
 *
 * PARAMETERS :
 *      (1) Uint16 cnt              - <i>   カウンタ値
 *
 * POSTCONDITIONS:
 *      なし
 *
 ******************************************************************************
 */
#define TIM_FRT_SET_16(cnt)\
        do{\
            TIM_FRT_SET_FRC(cnt);\
        }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_FRT_GET_16          - カウンタ値取得(16ビット)
 *
 * PARAMETERS :
 *      なし
 *
 * POSTCONDITIONS:
 *      (1) Uint16 cnt              - <o>   カウンタ値
 *
 ******************************************************************************
 */
#define TIM_FRT_GET_16()\
        TIM_FRT_GET_FRC()

/******************************************************************************
 *
 * NAME:    TIM_FRT_DELAY_16        - 時間待ち(16ビット)
 *
 * PARAMETERS :
 *      (1) Uint16 cnt              - <i>   時間待ちカウンタ値
 *
 * POSTCONDITIONS:
 *      なし
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
 * NAME:    TIM_FRT_CNT_TO_MCR      -   カウンタ値->マイクロ秒変換
 *
 * PARAMETERS :
 *      (1) Uint32 count            - <i>   カウンタ値
 *
 * POSTCONDITIONS:
 *      なし
 *
 ******************************************************************************
 */
#define TIM_FRT_CNT_TO_MCR(count)\
    (\
    (((*(Uint16 *)0x25f80004 & 0x1) == 0x1) ?   /* PAL か? */\
     ((SYS_GETSYSCK == 0) ? (Float32)0.037470726 : (Float32)0.035164835 ) :/*PAL 26,28*/\
     ((SYS_GETSYSCK == 0) ? (Float32)0.037210548 : (Float32)0.03492059 )) /*NT 26,28*/\
     * (count) * (8 << ((TIM_PEEK_B(TIM_REG_TCR) & TIM_M_CKS) << 1)))

/******************************************************************************
 *
 * NAME:    TIM_FRT_MCR_TO_CNT      -   マイクロ秒->カウンタ値変換
 *
 * PARAMETERS :
 *      (1) Uint32 mcr              - <i>   マイクロ秒値
 *
 * POSTCONDITIONS:
 *      なし
 *
 ******************************************************************************
 */
#define TIM_FRT_MCR_TO_CNT(mcr)\
    ((mcr) /\
    (((*(volatile Uint16 *)0x25f80004 & 0x1) == 0x1) ?   /* PAL か? */\
     ((SYS_GETSYSCK == 0) ? (Float32)0.037470726 : (Float32)0.035164835 ) :/*PAL 26,28*/\
     ((SYS_GETSYSCK == 0) ? (Float32)0.037210548 : (Float32)0.03492059 )) /*NT 26,28*/\
    / (8 << ((TIM_PEEK_B(TIM_REG_TCR) & TIM_M_CKS) << 1)))
/******************************************************************************
 *
 * NAME:    TIM_FRT_SET_TIER        - タイマインタラプトイネーブルレジスタ設定
 *
 * PARAMETERS :
 *      (1) Uint8 reg               - <i>   設定値
 *
 * POSTCONDITIONS:
 *      なし
 *
 ******************************************************************************
 */
#define TIM_FRT_SET_TIER(reg)\
        TIM_POKE_B(TIM_REG_TIER, (reg))

/******************************************************************************
 *
 * NAME:    TIM_FRT_SET_TCSR        - タイマコントロールステータスレジスタ設定
 *
 * PARAMETERS :
 *      (1) Uint8 reg               - <i>   設定値
 *
 * POSTCONDITIONS:
 *      なし
 *
 ******************************************************************************
 */
#define TIM_FRT_SET_TCSR(reg)\
        TIM_POKE_B(TIM_REG_TCSR, (reg))

/******************************************************************************
 *
 * NAME:    TIM_FRT_SET_FRC         - フリーランニングカウンタ設定
 *
 * PARAMETERS :
 *      (1) Uint16 reg              - <i>   設定値
 *
 * POSTCONDITIONS:
 *      なし
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
 * NAME:    TIM_FRT_SET_OCRA        - アウトプットコンペアレジスタA設定
 *
 * PARAMETERS :
 *      (1) Uint16 reg              - <i>   設定値
 *
 * POSTCONDITIONS:
 *      なし
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
 * NAME:    TIM_FRT_SET_OCRB        - アウトプットコンペアレジスタB設定
 *
 * PARAMETERS :
 *      (1) Uint16 reg              - <i>   設定値
 *
 * POSTCONDITIONS:
 *      なし
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
 * NAME:    TIM_FRT_SET_TCR         - タイマコントロールレジスタ設定
 *
 * PARAMETERS :
 *      (1) Uint8 reg               - <i>   設定値
 *
 * POSTCONDITIONS:
 *      なし
 * NOTICE:
 *      外部クロック選択の禁止対策版。
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
 * NAME:    TIM_FRT_SET_TOCR        - タイマアウトプットコンペアコントロール
 *                                    レジスタ設定
 * PARAMETERS :
 *      (1) Uint8 reg               - <i>   設定値
 *
 * POSTCONDITIONS:
 *      なし
 *
 ******************************************************************************
 */
#define TIM_FRT_SET_TOCR(reg)\
            TIM_POKE_B(TIM_REG_TOCR, (reg))

/******************************************************************************
 *
 * NAME:    TIM_FRT_GET_TIER        - タイマインタラプトイネーブルレジスタ取得
 *
 * PARAMETERS :
 *      なし
 *
 * POSTCONDITIONS:
 *      (1) Uint8 reg               - <o>   レジスタ値
 *
 ******************************************************************************
 */
#define TIM_FRT_GET_TIER()\
            TIM_PEEK_B(TIM_REG_TIER)

/******************************************************************************
 *
 * NAME:    TIM_FRT_GET_TCSR        - タイマコントロールステータスレジスタ取得
 *
 * PARAMETERS :
 *      なし
 *
 * POSTCONDITIONS:
 *      (1) Uint8 reg               - <o>   レジスタ値
 *
 ******************************************************************************
 */
#define TIM_FRT_GET_TCSR()\
            TIM_PEEK_B(TIM_REG_TCSR)

/******************************************************************************
 *
 * NAME:    TIM_FRT_GET_FRC          - フリーランニングカウント取得
 *
 * PARAMETERS :
 *      なし
 *
 * POSTCONDITIONS:
 *      (1) Uint16 reg               - <o>   レジスタ値
 *
 ******************************************************************************
 */
#define TIM_FRT_GET_FRC()\
            (TIM_PEEK_B(TIM_REG_FRC_H) << 8 | TIM_PEEK_B(TIM_REG_FRC_L))

/******************************************************************************
 *
 * NAME:    TIM_FRT_GET_OCRA         - アウトプットコンペアレジスタA取得
 *
 * PARAMETERS :
 *      (1) Uint16 reg               - <o>   レジスタ値
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
 * NAME:    TIM_FRT_GET_OCRB         - アウトプットコンペアレジスタB取得
 *
 * PARAMETERS :
 *      (1) Uint16 reg               - <o>   レジスタ値
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
 * NAME:    TIM_FRT_GET_TCR          - タイマコントロールレジスタ取得
 *
 * PARAMETERS :
 *      なし
 *
 * POSTCONDITIONS:
 *      (1) Uint8  reg               - <o>   レジスタ値
 *
 ******************************************************************************
 */
#define TIM_FRT_GET_TCR()\
            TIM_PEEK_B(TIM_REG_TCR)
                                                              
/******************************************************************************
 *
 * NAME:    TIM_FRT_GET_TOCR         - タイマアウトプットコンペアコントロール
 *                                     レジスタ取得
 * PARAMETERS :
 *      なし
 *
 * POSTCONDITIONS:
 *      (1) Uint8 reg                - <o>   レジスタ値
 *
 ******************************************************************************
 */
#define TIM_FRT_GET_TOCR()\
            TIM_PEEK_B(TIM_REG_TOCR)

/******************************************************************************
 *
 * NAME:    TIM_FRT_GET_ICRA         - インプットキャプチャレジスタA取得
 *
 * PARAMETERS :
 *      なし
 *
 * POSTCONDITIONS:
 *      (1) Uint16 reg               - <o>   レジスタ値
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
