/*-----------------------------------------------------------------------------
 *  FILE: snd_main.h
 *
 *      Copyright (c) by SEGA.
 *
 *  PURPOSE:
 *
 *      サウンドI/Fライブラリヘッダファイル。
 *
 *  DESCRIPTION:
 *
 *      サウンドドライバとのインタフェースを実現する。
 *
 *  AUTHOR(S)
 *
 *      1994-05-18  N.T Ver.0.90
 *
 *  MOD HISTORY:
 *
 *      1994-09-23  N.T Ver.1.04
 *      1994-12-01  N.T Ver.1.05
 *      1996-07-10  N.T Ver.1.08
 *
 *-----------------------------------------------------------------------------
 */

#ifndef SEGA_SND_H
#define SEGA_SND_H

/*
 * C VIRTUAL TYPES DEFINITIONS
 */
#include <machine.h>
#include "sega_xpt.h"

/*
 * USER SUPPLIED INCLUDE FILES
 */

/*
 * GLOBAL DEFINES/MACROS DEFINES
 */
/**** 定数 *******************************************************************/
/******** ｺﾏﾝﾄﾞ実行状態 ******************************************************/
#define SND_RET_SET     0                       /* 正常終了                     */
#define SND_RET_NSET    1                       /* 異常終了                     */
/******** ｻｳﾝﾄﾞｺﾝﾄﾛｰﾙ設定 ****************************************************/
#define SND_CTRL_OUTPUT_STEREO  (0 << 7)        /* Sound output mode : STEREO*/
#define SND_CTRL_OUTPUT_MONO    (1 << 7)        /* Sound output mode : MONO  */

/******** Sequence PAN ｺﾝﾄﾛｰﾙｽｲｯﾁ ********************************************/
#define SND_SEQ_PAN_OFF (0 << 7)                /* Control OFF               */
#define SND_SEQ_PAN_ON  (1 << 7)                /* Control ON                */

/******** ﾊｰﾄﾞｳｪｱﾁｪｯｸﾊﾟﾗﾒｰﾀ **************************************************/
#define SND_DRAM4           0                   /* DRAM 4Mbit r/w            */
#define SND_DRAM8           1                   /* DRAM 8Mbit r/w            */
#define SND_SCSP_MIDI       2                   /* SCSP MIDI                 */
#define SND_SOUND_SRC_LR    3                   /* 音源出力(L/R)            */
#define SND_SOUND_SRC_L     4                   /* 音源出力(L)              */
#define SND_SOUND_SRC_R     5                   /* 音源出力(R)              */
/******** ﾊｰﾄﾞｳｪｱﾁｪｯｸｽﾃｰﾀｽ ***************************************************/
#define SND_HARD_OK     0x8000                  /* 正常終了                  */
#define SND_HARD_ERR    0x7fff                  /* 異常終了                  */
/******** 割り込み許可ビット *************************************************/
#define SND_INT_PCM_ADR     (1 <<  7)           /* PCM play address 更新   */
/******** 対Host割り込み種別 *************************************************/
#define SND_FCT_PCM_ADR     0x01                /* PCM play address 更新   */
/******** PCMｱﾄﾞﾚｽ更新割り込みｽﾃｰﾀｽ ******************************************/
#define SND_PCM_PLAY0   (1 <<  0)               /* PCM再生番号0              */
#define SND_PCM_PLAY1   (1 <<  1)               /* PCM再生番号1              */
#define SND_PCM_PLAY2   (1 <<  2)               /* PCM再生番号2              */
#define SND_PCM_PLAY3   (1 <<  3)               /* PCM再生番号3              */
#define SND_PCM_PLAY4   (1 <<  4)               /* PCM再生番号4              */
#define SND_PCM_PLAY5   (1 <<  5)               /* PCM再生番号5              */
#define SND_PCM_PLAY6   (1 <<  6)               /* PCM再生番号6              */
#define SND_PCM_PLAY7   (1 <<  7)               /* PCM再生番号7              */

/******** SndSeqStat(シーケンスステータス) ***********************************/
/************* SND_SEQ_STAT_MODE(song mode) **********************************/
#define SND_MD_STOP     0x000                           /* stop              */
#define SND_MD_PLAY     0x100                           /* play              */
#define SND_MD_FADE     0x200                           /* fade              */
#define SND_MD_PLAY_PS  0x300                           /* play pause        */
#define SND_MD_FADE_PS  0x400                           /* fade pause        */
/******** SndPcmStartPrm(PCM開始パラメータ) **********************************/
/************* SND_PRM_MODE(ｽﾃﾚｵ･ﾓﾉｸﾛ+ｻﾝﾌﾟﾘﾝｸﾞﾚｰﾄ) ***************************/
#define SND_MD_MONO     (0 <<  7)                   /* ﾓﾉｸﾛ                  */
#define SND_MD_STEREO   (1 <<  7)                   /* ｽﾃﾚｵ                  */
#define SND_MD_16       (0 <<  4)                   /* 16bitPCM              */
#define SND_MD_8        (1 <<  4)                   /* 8bitPCM               */
/******** SND_MoveData(サウンドデータ転送) ***********************************/
#define SND_KD_TONE         0x0                         /* 音色              */
#define SND_KD_SEQ          0x1                         /* シーケンス        */
#define SND_KD_DSP_PRG      0x2                         /* DSPプログラム     */
#define SND_KD_DSP_RAM      0x3                         /* DSPﾜｰｸRAM         */
/**** アクセスマクロ *********************************************************/
/******** SndIniDt(システム起動) *********************************************/
#define SND_INI_PRG_ADR(sys_ini)    ((sys_ini).prg_adr) /* 68Kﾌﾟﾛｸﾞﾗﾑ格納... */
#define SND_INI_PRG_SZ(sys_ini)     ((sys_ini).prg_sz)  /* 68Kﾌﾟﾛｸﾞﾗﾑｻｲｽﾞ... */
#define SND_INI_ARA_ADR(sys_ini)    ((sys_ini).ara_adr) /* ｻｳﾝﾄﾞｴﾘｱﾏｯﾌﾟ格納. */
#define SND_INI_ARA_SZ(sys_ini)     ((sys_ini).ara_sz)  /* ｻｳﾝﾄﾞｴﾘｱﾏｯﾌﾟｻｲｽﾞ. */
/******** SndSeqStat(シーケンスステータス) ***********************************/
#define SND_SEQ_STAT_MODE(status)   ((status).mode)     /* song mode         */
#define SND_SEQ_STAT_STAT(status)   ((status).stat)     /* song status       */
/******** SndPcmStartPrm(PCM開始パラメータ) **********************************/
#define SND_PRM_MODE(prm)       ((prm).mode)        /* ｽﾃﾚｵ･ﾓﾉｸﾛ+ｻﾝﾌﾟﾘﾝｸﾞﾚｰﾄ */
#define SND_PRM_SADR(prm)       ((prm).sadr)        /* PCMｽﾄﾘｰﾑﾊﾞｯﾌｧｽﾀｰﾄｱﾄﾞﾚｽ*/
#define SND_PRM_SIZE(prm)       ((prm).size)        /* PCMｽﾄﾘｰﾑﾊﾞｯﾌｧｻｲｽﾞ   */
#define SND_PRM_OFSET(prm)      ((prm).ofset)       /* PCMｽﾄﾘｰﾑ再生開始ｵﾌｾｯﾄ */
/******** SndPcmChgPrm(PCM変更パラメータ) ************************************/
#define SND_PRM_NUM(prm)        ((prm).num)         /* PCMｽﾄﾘｰﾑ再生番号       */
#define SND_PRM_LEV(prm)        ((prm).lev)         /* ﾀﾞｲﾚｸﾄ音Level         */
#define SND_PRM_PAN(prm)        ((prm).pan)         /* ﾀﾞｲﾚｸﾄ音pan           */
#define SND_PRM_PICH(prm)       ((prm).pich)        /* PICHﾜｰﾄﾞ              */
#define SND_R_EFCT_IN(prm)      ((prm).r_efct_in)   /* Efect in select(右)   */
#define SND_R_EFCT_LEV(prm)     ((prm).r_efct_lev)  /* Efect send Level(右)  */
#define SND_L_EFCT_IN(prm)      ((prm).l_efct_in)   /* Efect in select(左)   */
#define SND_L_EFCT_LEV(prm)     ((prm).l_efct_lev)  /* Efect send Level(左)  */
#define SND_PRM_TL(prm)     	((prm).lev)  		/* Master Level			 */
/******** SndPcmPlayAdr(PCM再生ｱﾄﾞﾚｽ) ****************************************/
#define SND_PCM_RADR(prm)       ((prm).radr)        /* 右出力(ﾓﾉｸﾛ)          */
#define SND_PCM_LADR(prm)       ((prm).ladr)        /* 左出力                 */
/******** SndCdHzSrVl(周波数帯域別ｽﾃﾚｵ解析音量) ******************************/
#define SND_CD_LHIGH(hz_vl)     ((hz_vl).high.left) /* 左出力 高音域         */
#define SND_CD_LMID(hz_vl)      ((hz_vl).mid.left)  /* 左出力 中音域         */
#define SND_CD_LLOW(hz_vl)      ((hz_vl).low.left)  /* 左出力 低音域         */
#define SND_CD_RHIGH(hz_vl)     ((hz_vl).high.right)/* 右出力 高音域         */
#define SND_CD_RMID(hz_vl)      ((hz_vl).mid.right) /* 右出力 中音域         */
#define SND_CD_RLOW(hz_vl)      ((hz_vl).low.right) /* 右出力 低音域         */

#define SND_POKE_B(adr, data)   (*((volatile Uint8 *)(adr)) = ((Uint8)(data))) /* ﾊﾞｲﾄ*/
#define SND_POKE_W(adr, data)   (*((volatile Uint16 *)(adr)) = ((Uint16)(data)))
#define SND_PEEK_B(adr)         (*((volatile Uint8 *)(adr)))                   /* ﾊﾞｲﾄ*/

/**** システムインタフェースワークオフセット *********************************/
#define SND_ADR_INTR_CTRL_WORD  (0x00)          /* Interrupt control word    */
#define SND_ADR_INTR_MODE       (0x01)          /* Interrupt mode            */
#define SND_ADR_PCM_PLAY_NUM    (0x02)          /* PCM play num              */
#define SND_ADR_SND_CTRL        (0x03)          /* Sound control             */
/**** アドレス ***************************************************************/
#define SND_ADR_INTR_RESET  ((volatile Uint16 *)0x25b0042e)  /* SCSP割り込みﾘｾｯﾄﾚｼﾞｽﾀ */

/*割り込み *******************************************************************/
/******************************************************************************
 *
 * NAME:    SND_SET_ENA_INT()       - 割り込み許可設定
 *
 * PARAMETERS :
 *      (1) Uint8 ena_bit   - <i>   割り込み許可ビット
 *
 * DESCRIPTION:
 *      割り込み許可を設定します。
 *
 * PRECONDITIONS:
 *      なし。
 *
 * POSTCONDITIONS:
 *      なし
 *
 * CAVEATS:
 *      なし。
 *
 ******************************************************************************
 */

#define SND_SET_ENA_INT(ena_bit)\
    do{\
        snd_msk_work_work = get_imask();           /* ｻｳﾝﾄﾞ割り込みをDisable*/\
        set_imask(15);\
        SND_POKE_B(snd_adr_sys_int_work + SND_ADR_INTR_CTRL_WORD, (ena_bit));\
        set_imask(snd_msk_work_work);\
    }while(FALSE)

/******************************************************************************
 *
 * NAME:    SND_GET_ENA_INT()       - 割り込み許可取得
 *
 * PARAMETERS :
 *      なし。
 *
 * DESCRIPTION:
 *      割り込み許可を取得します。
 *
 * PRECONDITIONS:
 *      なし。
 *
 * POSTCONDITIONS:
 *      (1) Uint8           - <o>   割り込み許可ビット
 *
 * CAVEATS:
 *      なし。
 *
 ******************************************************************************
 */

#define SND_GET_ENA_INT()\
        (SND_PEEK_B(snd_adr_sys_int_work + SND_ADR_INTR_CTRL_WORD))

/******************************************************************************
 *
 * NAME:    SND_GET_FCT_INT()       - 割り込み要因取得
 *
 * PARAMETERS :
 *      なし。
 *
 * DESCRIPTION:
 *      割り込み要因を取得します。
 *
 * PRECONDITIONS:
 *      なし。
 *
 * POSTCONDITIONS:
 *      (1) Uint8       - <o>   割り込み要因
 *
 * CAVEATS:
 *      なし。
 *
 ******************************************************************************
 */

#define SND_GET_FCT_INT()\
        (SND_PEEK_B(snd_adr_sys_int_work + SND_ADR_INTR_MODE))

/******************************************************************************
 *
 * NAME:    SND_RESET_INT()     - 割り込みﾘｾｯﾄ
 *
 * PARAMETERS :
 *      なし。
 *
 * DESCRIPTION:
 *      割り込みをﾘｾｯﾄします。
 *
 * PRECONDITIONS:
 *      なし。
 *
 * POSTCONDITIONS:
 *      なし
 *
 * CAVEATS:
 *      なし。
 *
 ******************************************************************************
 */

#define SND_RESET_INT()\
        (SND_POKE_W(SND_ADR_INTR_RESET, (1 << 5)))

/******************************************************************************
 *
 * NAME:    SND_GET_INT_STAT()  - PCM実行アドレス更新割り込みステータス取得
 *
 * PARAMETERS :
 *      なし。
 *
 * DESCRIPTION:
 *      PCM実行アドレス更新割り込みステータスを取得します。
 *
 * PRECONDITIONS:
 *      なし。
 *
 * POSTCONDITIONS:
 *      (1) SndPcmIntStat *     - <o>   PCM実行アドレス更新割り込みステータス
 *
 * CAVEATS:
 *      なし。
 *
 ******************************************************************************
 */
 
#define SND_GET_INT_STAT()\
    (SND_PEEK_B(snd_adr_sys_int_work + SND_ADR_PCM_PLAY_NUM))

/*その他 *********************************************************************/
/******************************************************************************
 *
 * NAME:    SND_SET_SND_CTRL        - サウンドコントロール情報設定
 *
 * PARAMETERS :
 *      (1) Uint8   ctrl_bit   - <i>   割り込み許可ビット
 *
 * DESCRIPTION:
 *      サウンドコントロール情報を設定します。
 *
 * PRECONDITIONS:
 *      なし。
 *
 * POSTCONDITIONS:
 *      なし
 *
 * CAVEATS:
 *      なし。
 *
 ******************************************************************************
 */

#define SND_SET_SND_CTRL(ctrl_bit)\
    do{\
        SND_POKE_B(snd_adr_sys_int_work + SND_ADR_SND_CTRL, (ctrl_bit));\
    }while(FALSE)

/******************************************************************************
 *
 * NAME:    SND_GET_SND_CTRL        - サウンドコントロール情報取得
 *
 * PARAMETERS :
 *      なし。
 *
 * DESCRIPTION:
 *      サウンドコントロール情報を取得します。
 *
 * PRECONDITIONS:
 *      なし。
 *
 * POSTCONDITIONS:
 *      なし
 *
 * CAVEATS:
 *      なし。
 *
 ******************************************************************************
 */

#define SND_GET_SND_CTRL()\
        (SND_PEEK_B(snd_adr_sys_int_work + SND_ADR_SND_CTRL))

/*
 * STRUCTURE DECLARATIONS
 */

/*
 * TYPEDEFS
 */

/**** 共通 *******************************************************************/
typedef struct{
    Uint16 *prg_adr;                        /* 68Kﾌﾟﾛｸﾞﾗﾑ格納先頭ｱﾄﾞﾚｽ       */
    Uint16 prg_sz;                          /* 68Kﾌﾟﾛｸﾞﾗﾑｻｲｽﾞｻｲｽﾞ           */
    Uint16 *ara_adr;                        /* ｻｳﾝﾄﾞｴﾘｱﾏｯﾌﾟ格納先頭ｱﾄﾞﾚｽ     */
    Uint16 ara_sz;                          /* ｻｳﾝﾄﾞｴﾘｱﾏｯﾌﾟｻｲｽﾞ(ﾜｰﾄﾞ単位)    */
}SndIniDt;                                  /* システム起動データ型          */

typedef Uint8 SndAreaMap;                   /* ｻｳﾝﾄﾞｴﾘｱﾏｯﾌﾟデータ型          */
typedef Uint8 SndTlVl;                      /* 全体音量データ型              */
typedef Uint8 SndEfctBnkNum;                /* Effct bank numberデータ型     */
typedef Uint8 SndToneBnkNum;                /* 音色 bank numberデータ型      */
typedef Uint8 SndMixBnkNum;                 /* Mixer numberデータ型          */
typedef Uint8 SndEfctOut;                   /* Effect out selectデータ型     */
typedef Uint8 SndLev;                       /* Levelデータ型                 */
typedef Sint8 SndPan;                       /* PANデータ型                   */
typedef Uint8 SndRet;                       /* ｺﾏﾝﾄﾞ実行状態データ型         */
typedef Uint8 SndHardPrm;                   /* ﾊｰﾄﾞｳｪｱﾁｪｯｸﾊﾟﾗﾒｰﾀデータ型     */
typedef Uint16 SndHardStat;                 /* ﾊｰﾄﾞｳｪｱﾁｪｯｸｽﾃｰﾀｽデータ型      */

/**** シーケンス *************************************************************/
typedef Uint8 SndSeqNum;                    /* 発音管理番号                  */
typedef Uint8 SndSeqBnkNum;                 /* Sequence bank number          */
typedef Uint8 SndSeqSongNum;                /* Sequence song number          */
typedef Uint8 SndSeqPri;                    /* Priorty level                 */
typedef Uint8 SndSeqVl;                     /* Sequence Volume               */
typedef Uint8 SndFade;                      /* fade Rate                     */
typedef Sint16 SndTempo;                    /* Tempo                         */
typedef struct{
    Uint16 mode;                            /* song mode                     */
    Uint8 stat;                             /* song status                   */
}SndSeqStat;                                /* シーケンスステータス          */
typedef Uint16 SndSeqPlayPos;               /* 発音管理番号再生位置          */

/**** PCM ********************************************************************/
typedef struct{
    Uint8 mode;                             /* ｽﾃﾚｵ･ﾓﾉｸﾛ+ｻﾝﾌﾟﾘﾝｸﾞﾚｰﾄ         */
    Uint16 sadr;                            /* PCMｽﾄﾘｰﾑﾊﾞｯﾌｧｽﾀｰﾄｱﾄﾞﾚｽ        */
    Uint16 size;                            /* PCMｽﾄﾘｰﾑﾊﾞｯﾌｧｻｲｽﾞ             */
}SndPcmStartPrm;                            /* PCM開始パラメータ          */

typedef Uint8 SndPcmNum;                    /* PCMｽﾄﾘｰﾑ再生番号               */
typedef Uint8 SndEfctIn;                    /* Efect in select               */
typedef struct{
    SndPcmNum num;                          /* PCMｽﾄﾘｰﾑ再生番号               */
    SndLev lev;                             /* ﾀﾞｲﾚｸﾄ音Level                 */
    SndPan pan;                             /* ﾀﾞｲﾚｸﾄ音pan                   */
    Uint16 pich;                            /* PICHﾜｰﾄﾞ                      */
    SndEfctIn r_efct_in;                    /* Efect in select(右出力)       */
    SndLev r_efct_lev;                      /* Efect send Level(右出力)      */
    SndEfctIn l_efct_in;                    /* Efect in select(左出力)       */
    SndLev l_efct_lev;                      /* Efect send Level(左出力)      */
}SndPcmChgPrm;                              /* PCM変更パラメータ           */

typedef struct{
    Uint8 radr;                             /* 右出力(ﾓﾉｸﾛ)                  */
    Uint8 ladr;                             /* 左出力                         */
}SndPcmPlayAdr;                             /* PCM再生ｱﾄﾞﾚｽ               */

typedef Uint8 SndPcmIntStat;                /* PCMｱﾄﾞﾚｽ更新割り込みｽﾃｰﾀｽ     */

/**** CD *********************************************************************/
typedef Uint16 SndCdVlAnl;                  /* 解析音量                      */

typedef struct{
    SndCdVlAnl left;                        /* 左出力                        */
    SndCdVlAnl right;                       /* 右出力                        */
}SndCdHzVl;                                 /* 周波数帯域別ｽﾃﾚｵ解析音量      */

typedef struct{
    SndCdHzVl high;                         /* 高音域                        */
    SndCdHzVl mid;                          /* 中音域                        */
    SndCdHzVl low;                          /* 低音域                        */
}SndCdHzSrVl;                               /* 周波数帯域別解析音量          */

/*
 * EXTERNAL VARIABLE DECLARATIONS
 */
extern Uint8 *snd_adr_sys_int_work;             /*ｼｽﾃﾑｲﾝﾀﾌｪｰｽﾜｰｸ先頭ｱﾄﾞﾚｽ格納*/
extern Uint32 snd_msk_work_work;                /* sound priority msk        */

/*
 * EXTERNAL FUNCTION PROTOTYPE  DECLARATIONS
 */
void SND_Init(SndIniDt *);
void SND_MoveData(Uint16 *, Uint32, Uint16, Uint16);
SndRet SND_ChgMap(SndAreaMap);
SndRet SND_SetTlVl(SndTlVl);
SndRet SND_ChgEfct(SndEfctBnkNum);
SndRet SND_ChgMix(SndToneBnkNum, SndMixBnkNum);
SndRet SND_ChgMixPrm(SndEfctOut, SndLev, SndPan);
SndRet SND_ChkHard(SndHardStat *, SndHardPrm);
SndRet SND_StopDsp(void);
SndRet SND_OffAllSound(void);
SndRet SND_StartSeq(SndSeqNum, SndSeqBnkNum, SndSeqSongNum, SndSeqPri);
SndRet SND_StopSeq(SndSeqNum);
SndRet SND_PauseSeq(SndSeqNum);
SndRet SND_ContSeq(SndSeqNum);
SndRet SND_SetSeqVl(SndSeqNum, SndSeqVl, SndFade);
SndRet SND_ChgTempo(SndSeqNum, SndTempo);
SndRet SND_CtrlDirMidi(SndSeqNum, SndSeqPri, Uint8, Uint8, Uint8, Uint8);
SndRet SND_SetSeqPan(SndSeqNum, Uint8, Uint8 );
SndRet SND_StartPcm(SndPcmStartPrm *, SndPcmChgPrm *);
SndRet SND_StopPcm(SndPcmNum);
SndRet SND_ChgPcm(SndPcmChgPrm *);
SndRet SND_StartVlAnl(void);
SndRet SND_StopVlAnl(void);
SndRet SND_SetCdDaLev(SndLev, SndLev);
SndRet SND_SetCdDaPan(SndPan, SndPan);
void SND_GetSeqStat(SndSeqStat *, SndSeqNum);
void SND_GetSeqPlayPos(SndSeqPlayPos *, SndSeqNum);
void SND_GetPcmPlayAdr(SndPcmPlayAdr *, SndPcmNum);
void SND_GetAnlTlVl(SndCdVlAnl *, SndCdVlAnl *);
void SND_GetAnlHzVl(SndCdHzSrVl *);

#endif  /* ifndef SEGA_SND_H */
