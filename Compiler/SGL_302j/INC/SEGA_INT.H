/*-----------------------------------------------------------------------------
 *  FILE: sega_int.h
 *
 *      Copyright(c) 1994 SEGA.
 *
 *  PURPOSE:
 *
 *      INTライブラリヘッダファイル。
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
#include "sega_sys.h"

/*
 * GLOBAL DEFINES/MACROS DEFINES
 */

/* 定数 */
/**** 割り込みアドレスレジスタ ***********************************************/
#define INT_REG_IST     ((Uint32 *)0x25fe00a4)  /* 割り込みステータスレジスタ*/
#define INT_REG_IACK    ((Uint32 *)0x25fe00bc)  /* A-Bus割り込みアクノリッジ */
/***** 割り込みマスクビット値定数 ********************************************/
#define INT_MSK_NULL      0             /* 指定なし                          */
#define INT_MSK_ALL      (INT_MSK_ABUS     |    /* 全指定　                 */\
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
#define INT_MSK_SPR      (1 << 13)      /* スプライト描画終了                */
#define INT_MSK_DMAI     (1 << 12)      /* DMAイリーガル                     */
#define INT_MSK_DMA0     (1 << 11)      /* レベル0-DMA                       */
#define INT_MSK_DMA1     (1 << 10)      /* レベル1-DMA                       */
#define INT_MSK_DMA2     (1 <<  9)      /* レベル2-DMA                       */
#define INT_MSK_PAD      (1 <<  8)      /* PAD                               */
#define INT_MSK_SYS      (1 <<  7)      /* システムマネージャ                */
#define INT_MSK_SND      (1 <<  6)      /* サウンドリクエスト                */
#define INT_MSK_DSP      (1 <<  5)      /* DSP終了　　                       */
#define INT_MSK_TIM1     (1 <<  4)      /* タイマ-1                          */
#define INT_MSK_TIM0     (1 <<  3)      /* タイマ-0                          */
#define INT_MSK_HBLK_IN  (1 <<  2)      /* H-ブランク-IN                     */
#define INT_MSK_VBLK_OUT (1 <<  1)      /* V-ブランク-OUT                    */
#define INT_MSK_VBLK_IN  (1 <<  0)      /* V-ブランク-IN                     */

/***** 割り込みステータスビット値定数 ****************************************/
#define INT_ST_NULL      0              /* 指定なし                          */
#define INT_ST_ALL      (INT_ST_ABUS      |    /* 全指定　                 */\
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
#define INT_ST_SPR      (1 << 13)       /* スプライト描画終了                */
#define INT_ST_DMAI     (1 << 12)       /* DMAイリーガル                     */
#define INT_ST_DMA0     (1 << 11)       /* レベル0-DMA                       */
#define INT_ST_DMA1     (1 << 10)       /* レベル1-DMA                       */
#define INT_ST_DMA2     (1 <<  9)       /* レベル2-DMA                       */
#define INT_ST_PAD      (1 <<  8)       /* PAD                               */
#define INT_ST_SYS      (1 <<  7)       /* システムマネージャ                */
#define INT_ST_SND      (1 <<  6)       /* サウンドリクエスト                */
#define INT_ST_DSP      (1 <<  5)       /* DSP終了　　                       */
#define INT_ST_TIM1     (1 <<  4)       /* タイマ-1                          */
#define INT_ST_TIM0     (1 <<  3)       /* タイマ-0                          */
#define INT_ST_HBLK_IN  (1 <<  2)       /* H-ブランク-IN                     */
#define INT_ST_VBLK_OUT (1 <<  1)       /* V-ブランク-OUT                    */
#define INT_ST_VBLK_IN  (1 <<  0)       /* V-ブランク-IN                     */

/***** ベクタ番号定数 ********************************************************/
/********* SCU ***************************************************************/
#define INT_SCU_SPR         0x4d        /* スプライト描画終了                */
#define INT_SCU_DMAI        0x4c        /* DMAイリーガル                     */
#define INT_SCU_DMA0        0x4b        /* レベル0-DMA                       */
#define INT_SCU_DMA1        0x4a        /* レベル1-DMA                       */
#define INT_SCU_DMA2        0x49        /* レベル2-DMA                       */
#define INT_SCU_PAD         0x48        /* PAD                               */
#define INT_SCU_SYS         0x47        /* システムマネージャ                */
#define INT_SCU_SND         0x46        /* サウンドリクエスト                */
#define INT_SCU_DSP         0x45        /* DSP終了　　                       */
#define INT_SCU_TIM1        0x44        /* タイマ-1                          */
#define INT_SCU_TIM0        0x43        /* タイマ-0                          */
#define INT_SCU_HBLK_IN     0x42        /* H-ブランク-IN                     */
#define INT_SCU_VBLK_OUT    0x41        /* V-ブランク-OUT                    */
#define INT_SCU_VBLK_IN     0x40        /* V-ブランク-IN                     */
     
/********* CPU ***************************************************************/
#define INT_CPU_DIVU        0v6e        /* 除算器                            */
#define INT_CPU_DMAC0       0x6d        /* DMACチャネル0                     */
#define INT_CPU_DMAC1       0x6c        /* DMACチャネル1                     */
#define INT_CPU_WDT         0x68        /* WDTインターバル                   */
#define INT_CPU_BSC         0x69        /* BSCコンペアマッチ                 */
#define INT_CPU_SCI_ERI     0x60        /* SCI受信エラー　　                 */
#define INT_CPU_SCI_RXI     0x61        /* SCI受信データフル                 */
#define INT_CPU_SCI_TXI     0x62        /* SCI受信データエンプティ           */
#define INT_CPU_SCI_TEI     0x63        /* SCI送信終了  　　                 */
#define INT_CPU_FRT_ICI     0x64        /* FRTインプットキャプチャ           */
#define INT_CPU_FRT_OCI     0x65        /* FRTアウトプットコンペア           */
#define INT_CPU_FRT_OVI     0x66        /* FRTオーバフロー　                 */

/***** アクノリッジ値 *******************************************************/
#define INT_ACK_ENA         0x1         /* 有効                              */
#define INT_ACK_DIS         0x0         /* 保持                              */

/******************************************************************************
 *
 * NAME:    INT_GetMsk()        -   割り込みマスクレジスタ取得
 *
 * PARAMETERS :
 *      なし。
 *
 * DESCRIPTION:
 *
 * PRECONDITIONS:
 *      なし。
 *
 * POSTCONDITIONS:
 *      (1) Uint32  割り込みマスクビット値
 *
 * CAVEATS:
 *      なし。
 *
 ******************************************************************************
 */

#define INT_GetMsk(a) SYS_GETSCUIM

/******************************************************************************
 *
 * NAME:    INT_SetMsk()    -   割り込みマスクレジスタ設定
 *
 * PARAMETERS :
 *      (1) Uint32 msk_bit      <i>   割り込みマスクビット値
 *
 * DESCRIPTION:
 *
 * PRECONDITIONS:
 *      なし。
 *
 * POSTCONDITIONS:
 *      なし。
 *
 * CAVEATS:
 *      なし。
 *
 ******************************************************************************
 */

#define INT_SetMsk(msk_bit) SYS_SETSCUIM(msk_bit)

/******************************************************************************
 *
 * NAME:   INT_ChgMsk()     -   割り込みマスクレジスタ変更
 *
 * PARAMETERS :
 *      (1) Uint32 ena_msk_bit  <i>   割り込みマスクビット（イネーブル)
 *      (2) Uint32 dis_msk_bit  <i>   割り込みマスクビット（ディセーブル)
 *
 * DESCRIPTION:
 *
 * PRECONDITIONS:
 *      なし。
 *
 * POSTCONDITIONS:
 *      なし。
 *
 * CAVEATS:
 *      なし。
 *
 ******************************************************************************
 */

#define INT_ChgMsk(ena_msk_bit, dis_msk_bit)\
        SYS_CHGSCUIM(~(ena_msk_bit), dis_msk_bit)

/******************************************************************************
 *
 * NAME:    INT_GetStat()   -   割り込みステータスレジスタ取得
 *
 * PARAMETERS :
 *      なし。
 *
 * DESCRIPTION:
 *
 * PRECONDITIONS:
 *      なし。
 *
 * POSTCONDITIONS:
 *      (1) Uint32  割り込みステータスレジスタ値
 *
 * CAVEATS:
 *      なし。
 *
 ******************************************************************************
 */

#define INT_GetStat()   (*INT_REG_IST)

/******************************************************************************
 *
 * NAME:    INT_ResStat()       -   割り込みステータスレジスタリセット
 *
 * PARAMETERS :
 *      (1) Uint32 status_bit   <i>   割り込みステータスビット値
 *
 * DESCRIPTION:
 *
 * PRECONDITIONS:
 *      なし。
 *
 * POSTCONDITIONS:
 *      なし。
 *
 * CAVEATS:
 *      なし。
 *
 ******************************************************************************
 */

#define INT_ResStat(status_bit)\
    do{\
        *INT_REG_IST = status_bit;\
    }while(FALSE)

/******************************************************************************
 *
 * NAME:    INT_SetAck()            -   A-Bus割り込みアクノリッジ設定
 *
 * PARAMETERS :
 *      (1) Uint32 ack                  <i>   アクノリッジ値
 *
 * DESCRIPTION:
 *
 * PRECONDITIONS:
 *      なし。
 *
 * POSTCONDITIONS:
 *      なし。
 *
 * CAVEATS:
 *      なし。
 *
 ******************************************************************************
 */

#define INT_SetAck(ack)\
    do{\
        *INT_REG_IACK = ack;\
    }while(FALSE)

/******************************************************************************
 *
 * NAME:    INT_GetAck()            -   A-Bus割り込みアクノリッジ取得
 *
 * PARAMETERS :
 *      なし。
 *
 * DESCRIPTION:
 *
 * PRECONDITIONS:
 *      なし。
 *
 * POSTCONDITIONS:
 *      (1) Uint32  アクノリッジ値
 *
 * CAVEATS:
 *      なし。
 *
 ******************************************************************************
 */

#define INT_GetAck()    (*INT_REG_IACK)

/******************************************************************************
 *
 * NAME:    INT_SetFunc()       -   割り込み関数登録 
 *
 * PARAMETERS :
 *      (1) Uint32 num      <i>   ベクタ番号
 *      (2) void *hdr       <i>   割り込み関数アドレス
 *
 * DESCRIPTION:
 *
 * PRECONDITIONS:
 *      なし。
 *
 * POSTCONDITIONS:
 *
 * CAVEATS:
 *      なし。
 *
 ******************************************************************************
 */

#define INT_SetFunc(num, hdr)   SYS_SETSINT(num, hdr)

/******************************************************************************
 *
 * NAME:    INT_SetScuFunc()    -   SCU関数登録 
 *
 * PARAMETERS :
 *      (1) Uint32 num      <i>   ベクタ番号(SCUベクタ番号)
 *      (2) void *hdr       <i>   SCU関数アドレス
 *
 * DESCRIPTION:
 *
 * PRECONDITIONS:
 *      なし。
 *
 * POSTCONDITIONS:
 *
 * CAVEATS:
 *      なし。
 *
 ******************************************************************************
 */

#define INT_SetScuFunc(num, hdr)    SYS_SETUINT(num, hdr)

/******************************************************************************
 *
 * NAME:    INT_GetFunc()       -   割り込み関数アドレス取得
 *
 * PARAMETERS :
 *      (1) Uint32 num      <i>   ベクタ番号
 *
 * DESCRIPTION:
 *
 * PRECONDITIONS:
 *      なし。
 *
 * POSTCONDITIONS:
 *      (1) void (*)()  割り込み関数アドレス
 *
 * CAVEATS:
 *      なし。
 *
 ******************************************************************************
 */

#define INT_GetFunc(num) SYS_GETSINT(num)

/******************************************************************************
 *
 * NAME:    INT_GetScuFunc()       -   SCU関数アドレス取得
 *
 * PARAMETERS :
 *      (1) Uint32 num      <i>   ベクタ番号(SCUベクタ番号)
 *
 * DESCRIPTION:
 *
 * PRECONDITIONS:
 *      なし。
 *
 * POSTCONDITIONS:
 *      (1) void (*)()  SCU関数アドレス
 *
 * CAVEATS:
 *      なし。
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
