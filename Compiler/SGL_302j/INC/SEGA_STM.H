/*****************************************************************************
 *      ソフトウェアライブラリ
 *
 *      Copyright (c) 1994,1995,1996 SEGA
 *
 * Library: ストリームシステム
 * Module : 公開用ヘッダファイル
 * File   : sega_stm.h
 * Date   : 1996-03-21
 * Version: 2.11
 * Auther : H.T
 *
 *****************************************************************************/
#if !defined(SEGA_STM_H)
#define     SEGA_STM_H

/*****************************************************************************
 *      インクルードファイル
 *****************************************************************************/
#include        "sega_gfs.h"

/* バージョン   */
                            /*             1         2         3         4  */
                            /*    1234567890123456789012345678901234567890  */
                            /*    ++++++++++++++++++++++++++++++++          */
#if defined(USE_SGL)
    #define STM_VERSION_STR     ("STM_SGL Version 2.11 1996-03-21")
#else
    #define STM_VERSION_STR     ("STM_SBL Version 2.11 1996-03-21")
#endif
#define STM_VERSION_LEN 32

/*****************************************************************************
 *      定数マクロ
 *****************************************************************************/

#define STM_LONG_MAX    0x7fffffff

/* ループ再生時の読み込み指定           */
enum StmLoopRead {
    STM_LOOP_READ       = TRUE,         /* ループ再生時読み込みをする   */
    STM_LOOP_NOREAD     = FALSE,        /* ループ再生時読み込みをしない */
    STM_LOOP_END
};

/* ループストリームを先頭ストリームに設定する   */
#define STM_LOOP_DFL    ((StmHn)(0xffffffff))

/* 無限回ループの指定                   */
#define STM_LOOP_ENDLESS        STM_LONG_MAX

/* ストリームアクセス状態(acstat)       */
enum StmAcStat {
    STM_EXEC_COMPLETED  = 0x100,        /* アクセス終了                 */
    STM_EXEC_PAUSE      = 0x101,        /* アクセス一時停止中           */
    STM_EXEC_DOING      = 0x102,        /* アクセス中                   */
    STM_EXEC_WAIT       = 0x103,        /* 転送ゲートの開き待ち状態     */
    STM_EXEC_TSKEND     = 0x104,        /* 即時復帰処理完了             */
    STM_EXEC_END
};


/* 転送ユニットサイズ           */
enum StmUnitSize {
    STM_UNIT_FORM1      = 2048,         /* MODE1,MODE2Form1セクタサイズ */
    STM_UNIT_FORM2      = 2324,         /* MODE2Form2セクタサイズ       */
    STM_UNIT_WORD       = 2,            /* ワード                       */
    STM_UNIT_BYTE       = 1,            /* バイト                       */
    STM_UNIT_END
};


/* ストリーム情報                       */
#define STM_KEY_NONE    (-1)            /* 各ストリームキー選択を無視   */

/* 読み込みセクタ範囲                   */
enum StmFad {
    STM_FAD_CDTOP       = 150,          /* ディスク先頭FAD              */
    STM_FAD_CDEND       = STM_LONG_MAX  /* ディスク終了迄読む場合のセクタ数*/
};


/* CDバッファセクタ位置                 */
enum StmSctPos {
    STM_CDBUF_TOP       = 0,            /* バッファ区画先頭             */
    STM_CDBUF_END       = 0xffff        /* 全セクタ数                   */
};

/* オープン時読み込みザブモード設定用   */
enum StmSctType {
    STM_SM_EOR          = CDC_SM_EOR,           /* レコード最後のセクタ */
    STM_SM_VIDEO        = CDC_SM_VIDEO,         /* ビデオセクタ         */
    STM_SM_AUDIO        = CDC_SM_AUDIO,         /* オーディオセクタ     */
    STM_SM_DATA         = CDC_SM_DATA,          /* データセクタ         */
    STM_SM_TRIG         = CDC_SM_TRIG,          /* トリガービット       */
    STM_SM_FORM         = CDC_SM_FORM,          /* フォームビット       */
    STM_SM_RT           = CDC_SM_RT,            /* リアルタイムセクタ   */
    STM_SM_EOF          = CDC_SM_EOF,           /* ファイル最後のセクタ */
    STM_SM_END
};


/* 最大転送セクタ数                     */
#define STM_TR_ALL      STM_LONG_MAX


/* 転送ゲート状態                       */
enum StmGate {
    STM_GATE_OPEN       = 0x300,                /* 開き                 */
    STM_GATE_CLOSE      = 0x301,                /* 閉じ                 */
    STM_GATE_END
};


/* 転送モード                   */
enum StmTrMode {
    STM_TR_SCU          = GFS_TMODE_SCU,        /* SCUによるDMA         */
    STM_TR_SDMA0        = GFS_TMODE_SDMA0,      /* CPUサイクルスチール  */
    STM_TR_SDMA1        = GFS_TMODE_SDMA1,      /* CPUサイクルスチール  */
    STM_TR_CPU          = GFS_TMODE_CPU,        /* ソフトウエア転送     */
    STM_TR_END
};


/* 関数ID                       */
enum StmFuncId {
    STM_INIT            = 0x300,        /* ストリームシステム初期化     */
    STM_OPENGRP         = 0x301,        /* ストリームグループのオープン */
    STM_CLOSEGRP        = 0x302,        /* ストリームグループのクローズ */
    STM_GETSTMNUM       = 0x303,        /* ストリーム数の取得           */
    STM_GETSTMHNDL      = 0x304,        /* ストリームハンドルの取得     */
    STM_SETCDBUFFUNC    = 0x305,        /* バッファサイズ監視関数の登録 */
    STM_OPENFID         = 0x306,        /* 識別子によるオープン         */
    STM_OPENFRANGE      = 0x307,        /* セクタ位置によるオープン     */
    STM_OPENRESI        = 0x308,        /* 常駐ストリームのオープン     */
    STM_CLOSE           = 0x309,        /* ストリームのクローズ         */
    STM_SETKEY          = 0x310,        /* ストリームキーの設定         */
    STM_GETINFO         = 0x311,        /* ストリーム情報の取得         */
    STM_SCTTOWORD       = 0x312,        /* セクタ数からワード数への変換 */
    STM_WORDTOSCT       = 0x313,        /* ワード数からセクタ数への変換 */
    STM_SETALIAS        = 0x314,        /* エイリアスの設定             */
    STM_UNSETALIAS      = 0x315,        /* エイリアスの解除             */
    STM_SETTRBUF        = 0x316,        /* 転送領域の設定               */
    STM_SETTRFUNC       = 0x317,        /* 転送関数の設定               */
    STM_STARTTRANS      = 0x318,        /* 転送関数における転送開始     */
    STM_SETTRGATE       = 0x319,        /* 転送ゲートの開閉             */
    STM_SETTRPARA       = 0x320,        /* 最大転送セクタ数の設定       */
    STM_SETTRFAD        = 0x321,        /* 転送開始FADの設定            */
    STM_SETTRMODE       = 0x322,        /* 転送モードの設定             */
    STM_RESETTRBUF      = 0x323,        /* 転送領域のリセット           */
    STM_GETNUMCDBUF     = 0x324,        /* CDバッファ有効セクタ数の取得 */
    STM_GETSCTINFO      = 0x325,        /* 読み込みセクタ情報の取得     */
    STM_GETLENTRBUF     = 0x326,        /* 転送領域のデータ数の取得     */
    STM_ISTRBUFFULL     = 0x327,        /* 転送領域フルのチェック       */
    STM_ISTRANS         = 0x328,        /* 転送中のチェック             */
    STM_SETEXECGRP      = 0x329,        /* サーバ実行グループの指定     */
    STM_EXECSERVER      = 0x330,        /* サーバの実行                 */
    STM_MOVEPICKUP      = 0x331,        /* ピックアップの移動           */
    STM_SETLOOP         = 0x332,        /* ループストリームの指定       */
    STM_GETLOOPCNT      = 0x333,        /* ループカウンタの取得         */
    STM_GETEXECSTAT     = 0x334,        /* 実行状態の取得               */
    STM_ISCOMPLETE      = 0x335,        /* 実行終了のチェック           */
    STM_EXECTRANS       = 0x336,        /* ストリームデータの転送       */
    STM_CONNECTCDBUF    = 0x337,        /* 絞りとCDバッファ区画の接続   */
    STM_MOVECDBUF       = 0x338,        /* CDバッファデータの移動       */
    STM_ERASECDBUF      = 0x339,        /* CDバッファデータの消去       */
    STM_SETERRFUNC      = 0x340,        /* エラー関数の登録             */
    STM_GETERRSTAT      = 0x341,        /* エラー状態の取得             */
    STM_FUNC_END
};


/* エラーコード         */
enum StmErrCode {
    STM_ERR_OK          = GFS_ERR_OK,           /* 正常終了             */
    STM_ERR_CDRD        = GFS_ERR_CDRD,         /* CDリードエラー       */
    STM_ERR_CDNODISC    = GFS_ERR_CDNODISC,     /* CDをセットしていない */
    STM_ERR_FID         = GFS_ERR_FID,          /* ファイル識別子不正   */
    STM_ERR_HNDL        = GFS_ERR_HNDL,         /* ハンドルが不正       */
    STM_ERR_NUM         = GFS_ERR_NUM,          /* バイト数などが負     */
    STM_ERR_PARA        = GFS_ERR_PARA,         /* パラメータが不正     */
    STM_ERR_NOHNDL      = GFS_ERR_NOHNDL,       /* ハンドルに空きがない */
    STM_ERR_PUINUSE     = GFS_ERR_PUINUSE,      /* ピックアップ動作中   */
    STM_ERR_TMOUT       = GFS_ERR_TMOUT,        /* タイムアウト         */
    STM_ERR_CDOPEN      = GFS_ERR_CDOPEN,       /* トレイが開いている   */
    STM_ERR_FATAL       = GFS_ERR_FATAL,        /* CDがFATAL状態        */
    STM_ERR_END
};


/*****************************************************************************
 *      データ型の定義
 *****************************************************************************/

/* ストリームハンドル           */
typedef struct stmobj {
    void            *grp;           /* 所属するストリームグループ           */
    struct stmobj   *alias;         /* 実際に操作するストリーム             */
    struct stmobj   *sfadlst;       /* 開始FAD昇順リスト                    */
    struct stmobj   *fltlst;        /* 絞りの接続順リスト                   */
    GfsHn           gfs;            /* ファイルハンドル                     */
    GfsFlow         *flow;          /* 読み込み管理                         */
    Sint32          (*trfunc)(void *obj, struct stmobj *stm, Sint32 nsct);
                                    /* 転送関数(StmTrFunc)                  */
    void            *obj;           /* 転送関数の第一引数   　　　　        */
    Uint32          *trsrc;         /* 転送元アドレス                       */
    Sint32          adlt;           /* 転送元アドレス変化分                 */
    Sint32          trfad;          /* 転送開始FAD                          */
    Uint8           atr;            /* アトリビュート                       */
    Uint8           svrstat;
} StmObj;

typedef StmObj      *StmHn;


/* 転送関数     */
typedef Sint32  (*StmTrFunc)(void *obj, StmHn stm, Sint32 nsct);
#define STM_TR_NULLFUNC         ((StmTrFunc)0)

/* エラー関数   */
typedef void    (*StmErrFunc)(void *obj, Sint32 ec);
#define STM_ERR_NULLFUNC        ((StmErrFunc)0)

/* バッファ監視関数  */
typedef void    (*StmCdbufFunc)(void *obj);
#define STM_CDBUF_NULLFUNC      ((StmCdbufFunc)0)
#define STM_CDBUF_REST          NULL

/* ストリームグループ           */
typedef struct {
    StmHn       sfadlst;        /* 読み込み開始FADによるリスト          */
    StmHn       fltlst;         /* 絞り接続順リスト                     */
    StmHn       loopstm;        /* ループストリーム                     */
    StmHn       lststm;         /* 最終ストリーム                       */
    StmHn       nexttrns;       /* 次の転送ストリーム           */
    Sint32      fad;            /* 読み込み実行時、一時停止時のFAD      */
    Sint32      acstat;         /* アクセス状態                         */
                                /* (COMPLETED, PAUSE, DOING, WAIT)      */
    StmHn       chkstm;         /* バッファサイズ監視ストリーム         */
    Sint32      chksctnum;      /* バッファサイズ監視関数呼びだしセクタ数 */
    StmCdbufFunc  chkfunc;      /* バッファサイズ監視関数               */
    void        *chkobj;        /* バッファサイズ監視オブジェクト       */
    Sint32      loopmax;        /* ループ回数                           */
    Sint32      loopcnt;        /* ループカウンタ                       */
    Sint32      puid;           /* ピックアップユーザID                 */
    Uint16      atr;            /* アトリビュート                       */
    Uint8       svrstat;
} StmGrp;

typedef StmGrp  *StmGrpHn;

/* ストリームキー               */
typedef struct {
    Sint16      fn;             /* ファイル番号                         */
    Sint16      cn;             /* チャネル番号                         */
    Sint16      smmsk;          /* サブモードマスクパターン             */
    Sint16      smval;          /* サブモード比較値                     */
    Sint16      cimsk;          /* コーディング情報マスクパターン       */
    Sint16      cival;          /* コーディング情報比較値               */
} StmKey;

#define STM_KEY_FN(stmkey)      ((stmkey)->fn)
#define STM_KEY_CN(stmkey)      ((stmkey)->cn)
#define STM_KEY_SMMSK(stmkey)   ((stmkey)->smmsk)
#define STM_KEY_SMVAL(stmkey)   ((stmkey)->smval)
#define STM_KEY_CIMSK(stmkey)   ((stmkey)->cimsk)
#define STM_KEY_CIVAL(stmkey)   ((stmkey)->cival)


/* ストリーム再生範囲           */
typedef struct {
    Sint32      sfad;           /* 再生開始FAD                          */
    Sint32      fasnum;         /* 再生セクタ数                         */
} StmFrange;

#define STM_FRANGE_SFAD(frange)         ((frange)->sfad)
#define STM_FRANGE_FASNUM(frange)       ((frange)->fasnum)


/* セクタ情報                   */
typedef struct {
    Sint32      fad;            /* フレームアドレス                     */
    Sint32      fn;             /* ファイル番号                         */
    Sint32      cn;             /* チャネル番号                         */
    Uint8       sm;             /* サブモード                           */
    Uint8       ci;             /* コーディング情報                     */
} StmSct;

#define STM_SCT_FAD(sct)        ((sct)->fad)
#define STM_SCT_FN(sct)         ((sct)->fn)
#define STM_SCT_CN(sct)         ((sct)->cn)
#define STM_SCT_SM(sct)         ((sct)->sm)
#define STM_SCT_CI(sct)         ((sct)->ci)


/* エラー管理                   */
typedef struct {
    Sint32      code;           /* エラーコード                         */
    Sint32      where;          /* エラー発生場所                       */
    StmErrFunc  func;           /* エラー発生時の呼びだし関数           */
    void        *obj;           /* 呼びだし関数の第一引数               */
} StmErrStat;

#define STM_ERR_CODE(err)       ((err)->code)
#define STM_ERR_WHERE(err)      ((err)->where)
#define STM_ERR_FUNC(err)       ((err)->func)
#define STM_ERR_OBJ(err)        ((err)->obj)


/* ストリームシステム管理       */
typedef struct {
    StmGrp      *stmgrptbl;     /* ストリームグループ割当て領域         */
    Sint32      stmgrpmax;      /* ストリームグループ最大数             */
    StmObj      *stmtbl;        /* ストリーム割当て領域                 */
    Sint32      stmmax;         /* ストリーム最大数                     */
    StmGrpHn    curgrp;         /* 現在処理中ストリームグループ         */
    Sint32      sfad;           /* 再生開始FAD                          */
    Sint32      efad;           /* 再生終了FAD                          */
    StmErrStat  err;            /* エラー管理                           */
    Bool        svrexec;        /* STM_ExecServer処理中                 */
} StmMng;


/*****************************************************************************
 *      処理マクロ
 *****************************************************************************/

/* 作業領域サイズの取得                 */
#define STM_WORK_SIZE(grpmax, stmmax)   \
    (sizeof(StmMng) + sizeof(StmGrp) * (grpmax) + sizeof(StmObj) * (stmmax))

/*****************************************************************************
 *      関数の宣言
 *****************************************************************************/

/* ストリームシステム初期化             */
Bool    STM_Init(Sint32 grpmax, Sint32 stmmax, void *work);

/* ストリームシステムのリセット         */
void    STM_Reset(void);

/* ストリームグループのオープン         */
StmGrpHn        STM_OpenGrp(void);

/* ストリームグループのクローズ         */
void    STM_CloseGrp(StmGrpHn grp);

/* ストリーム数の取得                   */
Sint32  STM_GetStmNum(StmGrpHn grp);

/* ストリームハンドルの取得             */
StmHn   STM_GetStmHndl(StmGrpHn grp, Sint32 nstm);

/* バッファサイズ監視関数の登録         */
void    STM_SetCdbufFunc(StmGrpHn grp, StmHn stm, Sint32 nsct, 
                                       StmCdbufFunc func, void *obj);

/* 識別子によるファイルのオープン       */
StmHn   STM_OpenFid(StmGrpHn grp, Sint32 fid, StmKey *key, Sint32 lpread);

/* セクタ位置によるファイルのオープン   */
StmHn   STM_OpenFrange(StmGrpHn grp, StmFrange *frange, StmKey *key,
                                                           Sint32 lpread);

/* 常駐ストリームのオープン             */
StmHn   STM_OpenResi(StmGrpHn grp, Sint32 fid, StmKey *key, Sint32 fad);

/* ストリームのクローズ                 */
void    STM_Close(StmHn stm);

/* ストリームキーの設定                 */
void    STM_SetKey(StmHn stm, StmKey *stmkey);

/* ストリーム情報の取得                 */
StmGrpHn        STM_GetInfo(StmHn stm, Sint32 *fid, StmFrange *range, 
                                                Sint32 *bn, StmKey *stmkey);

/* セクタ数からワード数への変換         */
Sint32  STM_SctToWord(StmHn stm, Sint32 nsct);

/* ワード数からセクタ数への変換         */
Sint32  STM_WordToSct(StmHn stm, Sint32 nword);

/* ストリームハンドルのエイリアス設定   */
Bool    STM_SetAlias(StmHn alias, StmHn stm);

/* ストリームハンドルのエイリアス解除   */
void    STM_UnsetAlias(StmHn alias);

/* 転送領域の設定                       */
void    STM_SetTrBuf(StmHn stm, void *buffer, Sint32 nunit, Sint32 unitsize);

/* 転送関数の設定                       */
void    STM_SetTrFunc(StmHn stm, StmTrFunc func, void *obj);

/* 転送関数における転送開始             */
Uint32  *STM_StartTrans(StmHn stm, Sint32 *dadr);

/* 転送ゲートの開閉                     */
void    STM_SetTrGate(StmHn stm, Sint32 gate);

/* 最大転送セクタ数の設定               */
void    STM_SetTrPara(StmHn stm, Sint32 tsct);

/* 転送開始FADの設定                    */
void    STM_SetTrFad(StmHn stm, Sint32 fad);

/* 転送モードの設定                     */
void    STM_SetTrMode(StmHn stm, Sint32 tmode);

/* 転送領域のリセット                   */
void    STM_ResetTrBuf(StmHn stm);

/* ＣＤバッファ有効セクタ数の取得       */
Sint32  STM_GetNumCdbuf(StmHn stm);

/* 読み込みセクタ情報の取得             */
Bool    STM_GetSctInfo(StmHn stm, Sint32 spos, StmSct *sinfo);

/* 転送領域のデータ数(ワード)の取得     */
Sint32  STM_GetLenTrBuf(StmHn stm);

/* 転送領域フルのチェック               */
Bool    STM_IsTrBufFull(StmHn stm);

/* ストリームデータ転送中のチェック     */
Bool    STM_IsTrans(StmHn stm);

/* サーバ実行グループの指定(即時復帰)   */
Bool    STM_NwSetExecGrp(StmGrpHn grp);

/* サーバ実行グループの指定(完了復帰)   */
Bool    STM_SetExecGrp(StmGrpHn grp);

/* サーバの実行                         */
Sint32  STM_ExecServer(void);

/* ピックアップの移動                   */
void    STM_MovePickup(StmHn stm, Sint32 off);

/* リードエラー発生時のリカバリ         */
Sint32  STM_Recover(void);

/* ループストリームの指定               */
Bool    STM_SetLoop(StmGrpHn grp, StmHn loopstm, Sint32 loopmax);

/* ループカウンタの取得                 */
Sint32  STM_GetLoopCnt(StmGrpHn grp);

/* 実行状態の取得                       */
Sint32  STM_GetExecStat(StmGrpHn grp, Sint32 *fad);

/* 実行終了のチェック                   */
Bool    STM_IsComplete(StmHn stm);

/* ストリームデータの転送               */
Bool    STM_ExecTrans(StmHn stm);

/* ストリームの接続                     */
void    STM_ConnectCdbuf(StmHn keystm, StmHn bufstm);

/* CDバッファデータの移動               */
void    STM_MoveCdbuf(StmHn src, Sint32 spos, Sint32 snum, StmHn dst);

/* CDバッファデータの消去               */
void    STM_EraseCdbuf(StmHn stm, Sint32 spos, Sint32 snum);

/* エラー発生時の呼びだし関数の登録     */
void    STM_SetErrFunc(StmErrFunc func, void *obj);

/* エラー状態の取得                     */
void    STM_GetErrStat(StmErrStat *stat);

#endif  /* SEGA_STM_H   */

/* end of sega_stm.h            */
