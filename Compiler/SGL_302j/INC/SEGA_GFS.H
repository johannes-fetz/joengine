/******************************************************************************
 *      ソフトウェアライブラリ
 *
 *      Copyright (c) 1994,1995,1996 SEGA
 *
 * Library:ファイルシステム
 * Module :ライブラリヘッダ
 * File   :sega_gfs.h
 * Date   :1996-02-18
 * Version:2.11
 * Author :H.T.
 *
 *************************************************************************** */

#if !defined(SEGA_GFS_H)
#define SEGA_GFS_H

/*****************************************************************************
 *      インクルードファイル
 *****************************************************************************/
#include "sega_cdc.h"

/* バージョン   */
                            /*             1         2         3         4  */
                            /*    1234567890123456789012345678901234567890  */
                            /*    ++++++++++++++++++++++++++++++++          */
#if defined(USE_SGL)
    #define GFS_VERSION_STR     ("GFS_SGL Version 2.11 1996-02-18")
#else
    #define GFS_VERSION_STR     ("GFS_SBL Version 2.11 1996-02-18")
#endif
#define GFS_VERSION_LEN 32

/*****************************************************************************
 *      定数マクロ
 *****************************************************************************/
#define GFS_FNAME_LEN   12              /* ファイル名の長さ                 */

/* ファイルの属性 */
#define GFS_ATR_DIR     0x80            /* ディレクトリ                     */
#define GFS_ATR_CDDA    0x40            /* CDDAファイル                     */
#define GFS_ATR_INTLV   0x20            /* インターリーブセクタを含む       */
#define GFS_ATR_FORM2   0x10            /* Form2セクタを含む                */
#define GFS_ATR_FORM1   0x08            /* Form1セクタを含む                */
#define GFS_ATR_END_TBL 0x01            /* ディレクトリテーブルの最後       */

#define GFS_BUFSIZ_INF  -1              /* 読み込み領域サイズに制限はない   */
#define GFS_RPARA_DFL   0x7fffffff      /* 読み込みパラメータの初期値       */

/* デモデモ用   */
#define GFS_DDS_ADDR    0x6000ccc       /* デモデモ情報参照領域のアドレス   */
#define GFS_DDS_EXEC    0x01            /* DDS実行中を示す                  */
#define GFS_DDS_ID()    (*(Uint8 *)(GFS_DDS_ADDR+0))  /* DDS識別子の値      */
#define GFS_DDS_TNO()   (*(Uint8 *)(GFS_DDS_ADDR+1))  /* 先頭CDDAトラックNo */
#define GFS_DDS_FID()   (*(Uint16 *)(GFS_DDS_ADDR+2)) /* ディレクトリ識別子 */
#define GFS_IS_DDS()    (GFS_DDS_ID()==GFS_DDS_EXEC)  /* DDS実行中の判断    */

/* エラーコード */
#define GFS_ERR_OK              0       /* 正常終了                         */
#define GFS_ERR_CDRD            (-1)    /* CDリードエラー                   */
#define GFS_ERR_CDNODISC        (-2)    /* CDがセットされていない           */
#define GFS_ERR_CDROM           (-3)    /* ディスクがCD-ROMでない           */
#define GFS_ERR_DIRTBL          (-4)    /* ディレクトリ管理テーブルが不正   */
#define GFS_ERR_OPENMAX         (-5)    /* 最大オープン数の値が不正         */
#define GFS_ERR_DIR             (-6)    /* 指定ファイルはディレクトリでない */
#define GFS_ERR_CDBFS           (-7)    /* CDブロックファイルシステムの     */
                                        /* 使用宣言していない               */
#define GFS_ERR_NONAME          (-8)    /* ファイル名を扱えない             */
#define GFS_ERR_NEXIST          (-9)    /* 指定されたファイルは存在しない   */
#define GFS_ERR_FID             (-10)   /* 不正なファイル識別子             */
#define GFS_ERR_HNDL            (-11)   /* ファイルハンドルが不正           */
#define GFS_ERR_SEEK            (-12)   /* シーク位置が不正                 */
#define GFS_ERR_ORG             (-13)   /* 基準位値が不正                   */
#define GFS_ERR_NUM             (-14)   /* バイト数が負                     */
#define GFS_ERR_OFS             (-15)   /* オフセットが不正                 */
#define GFS_ERR_FBUSY           (-16)   /* 指定ファイルの処理が残っている   */
#define GFS_ERR_PARA            (-17)   /* パラメータが不正                 */
#define GFS_ERR_BUSY            (-18)   /* ライブラリ関数実行中             */
#define GFS_ERR_NOHNDL          (-19)   /* ファイルハンドルに空きがない     */
#define GFS_ERR_PUINUSE         (-20)   /* ピックアップ動作中               */
#define GFS_ERR_ALIGN           (-21)   /* 作業領域が4byte境界にない        */
#define GFS_ERR_TMOUT           (-22)   /* タイムアウト                     */
#define GFS_ERR_CDOPEN          (-23)   /* トレイが開いている               */
#define GFS_ERR_BFUL            (-24)   /* バッファフルで読み込み中止       */
#define GFS_ERR_FATAL           (-25)   /* CDがFATAL状態                    */

/*****************************************************************************
 *      列挙定数
 *****************************************************************************/
/* アクセスモード(GfsFile:amode) */
enum GfsAccessMode {
    GFS_NWSTAT_NOACT = 0,               /* 何もしていない              */
    GFS_NWSTAT_FREAD,                   /* 読み込み中                  */
    GFS_NWSTAT_CDREAD                   /* ＣＤリード中                */
};


/* シークモード(GFS_Seek)               */
enum GfsSeekMode {
    GFS_SEEK_SET = 0,                   /* 先頭からシーク               */
    GFS_SEEK_CUR,                       /* 現在の位置からシーク         */
    GFS_SEEK_END                        /* 終端からシーク               */
};


/* 取り出しモード(GFS_SetGmode:gmode) */
enum GfsGetMode {
    GFS_GMODE_ERASE = 0,                /* 転送後、バッファから削除     */
    GFS_GMODE_RESIDENT                  /* ＣＤバッファに常駐           */
};


/* 転送モード(GFS_SetTmode) */
enum GfsTransMode {
    GFS_TMODE_SCU = 0,                  /* ＳＣＵによりＤＭＡ転送    */
    GFS_TMODE_SDMA0,                    /* ＤＭＡサイクルスチール転送   cn=0 */
    GFS_TMODE_SDMA1,                    /*                              cn=1 */
    GFS_TMODE_CPU,                      /* ソフトウェア転送 */
    GFS_TMODE_STM,                      /* ストリーム用転送 */
    GFS_TMODE_END
};

/* アクセスサーバ状態(GFS_NwExecServer) */
enum GfsServerStatus {
    GFS_SVR_COMPLETED = 0,              /* 全ファイルのアクセス終了     */
    GFS_SVR_BUSY,                       /* ファイルアクセス中           */
    GFS_SVR_CDPAUSE,                    /* ＣＤバッファフルの為一時停止 */
    GFS_SVR_ERROR                       /* エラー発生 */
};


/* ディレクトリ種別(GFS_LoadDir, GFS_SetDir)     */
enum GfsDirType {
    GFS_DIR_ID          = 0,            /* ファイル名によるアクセス不可 */
    GFS_DIR_NAME                        /* ファイル名によるアクセス可   */
};


/*****************************************************************************
 *      処理マクロ
 *****************************************************************************/
#define GFS_WORK_SIZE(open_max) \
            (sizeof(GfsMng) + ((open_max) - 1) * sizeof(GfsFile))

/*****************************************************************************
 *      構造体アクセス処理マクロ
 *****************************************************************************/

/* ディレクトリレコード構造体  */
#define GFS_DIR_REC(rec)        ((rec)->dirrec)
#define GFS_DIR_FAD(rec)        CDC_FILE_FAD(&GFS_DIR_REC(rec))
#define GFS_DIR_SIZE(rec)       CDC_FILE_SIZE(&GFS_DIR_REC(rec))
#define GFS_DIR_UNIT(rec)       CDC_FILE_UNIT(&GFS_DIR_REC(rec))
#define GFS_DIR_GAP(rec)        CDC_FILE_GAP(&GFS_DIR_REC(rec))
#define GFS_DIR_FN(rec)         CDC_FILE_FN(&GFS_DIR_REC(rec))
#define GFS_DIR_ATR(rec)        CDC_FILE_ATR(&GFS_DIR_REC(rec))
#define GFS_DIR_FNAME(rec)      ((rec)->fname)


/* ディレクトリ情報管理構造体 */
#define GFS_DIRTBL_TYPE(dirtbl)         ((dirtbl)->type)
#define GFS_DIRTBL_NDIR(dirtbl)         ((dirtbl)->ndir)
#define GFS_DIRTBL_DIRID(dirtbl)        ((dirtbl)->dir.dir_i)
#define GFS_DIRTBL_DIRNAME(dirtbl)      ((dirtbl)->dir.dir_n)


/* エラー状態 */
#define GFS_ERR_FUNC(err)       ((err)->func)
#define GFS_ERR_OBJ(err)        ((err)->obj)
#define GFS_ERR_CODE(err)       ((err)->code)


/*****************************************************************************
 *      データ型の定義
 *****************************************************************************/

typedef Sint32 (*GfsTransFunc)(void *obj, Sint32 nsct);

     /* ファイル情報 */
typedef struct {
    Sint32      fid;                    /* ファイル識別子 */
    CdcFile     finfo;
    Sint32      sctsz;                  /* セクタ長 */
    Sint32      nsct;                   /* セクタ数 */
    Sint32      lstrm;                  /* 最終セクタ中の無効データ数   */
} GfsFinfo;


/* CDバッファの資源管理 */
typedef struct {
    Sint32      bufno;                  /* バッファ区画番号 */
    Sint32      sctsz;                  /* ＣＤバッファのセクタサイズ   */
    Sint32      flt;                    /* 絞り番号 */
    CdcSubh     subh;                   /* サブヘッダ */
    Sint32      fmode;                  /* 絞りモード */
    Sint32      puid;                   /* 取得ピックアップのユーザID */
    Sint32      filepos;                /* 先頭データのファイル上の位置 */
    Sint32      sctpos;                 /* 転送位置 */
    /* 再生範囲 */
    Sint32      sfad;                   /* 開始FAD */
    Sint32      efad;                   /* 終了FAD */
} GfsCdRsrc;


/* SCSIファイルの仮想CDバッファ管理 */
typedef struct {
    Sint32      fid;                    /* SCSIファイル識別子   */
    Sint32      filepos;                /* 先頭データのファイル上の位置 */
    Sint32      sctpos;                 /* バッファ上のセクタ位置 */
    Sint32      sctnum;                 /* バッファ内のセクタ数 */
} GfsScsiRsrc;


/* メモリファイルの仮想CDバッファ管理 */
typedef struct {
    void        *data;                  /* データ領域           */
    Sint32      filepos;                /* 先頭データのファイル上の位置 */
    Sint32      sctpos;                 /* バッファ上のセクタ位置 */
    Sint32      sctnum;                 /* バッファ内のセクタ数 */
} GfsMemRsrc;


/* 読み込み元管理 */
typedef struct {
    /* 読み込み元管理 */
    Sint32      ftype;                  /* ファイル種別         */
    union {
        GfsCdRsrc       cd;             /* CDの資源             */
        GfsScsiRsrc     scsi;           /* SCSIファイルの資源   */
        GfsMemRsrc      mem;            /* メモリファイルの資源 */
    } rsrc;
} GfsDtsrc;


/* CDからの読み込みを管理する */
typedef struct {
    GfsFinfo    finfo;                  /* ファイル情報         */
    GfsDtsrc    dtsrc;                  /* 読み込み元           */
    Sint32      gmode;                  /* 取り出しモード       */
    Sint32      stat;                   /* 実行状態             */
    /* flow in管理 */
    Sint32      sct;                    /* 読み込みセクタ数     */
    Sint32      sctcnt;                 /* 読み込みカウンタ     */
    Sint32      sctmax;                 /* 読み込み最大セクタ数 */
} GfsFlow;


/* データパック構造体 */
typedef struct {
    void        *data;                  /* データ領域           */
    Sint32      adlt;                   /* アドレスの増加分     */
    Sint32      len;                    /* 転送バイト数         */
    Sint32      nsct;                   /* 転送セクタ数         */
    Bool        use;                    /* 使用フラグ           */
} GfsDataPack;


typedef GfsDataPack *GfdpHn;

/* 転送管理 */
/* CDバッファからの転送を管理する */
typedef struct {
    void        *buf;                   /* 転送先アドレス */
    Sint32      size;                   /* データ領域のサイズ */
    Sint32      wp;                     /* 書き込みオフセット */
    /* 転送管理 */
    GfdpHn      dps;                    /* 転送元のデータパック */
    GfdpHn      dpd;                    /* 転送先からのデータパック */
    Sint32      tsctmax;                /* １回の転送の最大セクタ数 */
    Sint32      tsct;                   /* 転送目標セクタ数 */
    Sint32      tsctcnt;                /* 現在まで転送セクタ数 */
    Sint32      tbytcnt;                /* セクタ内の転送バイト数 */
    /* 転送関数 */
    void        *obj;                   /* 転送関数への第１引数 */
    GfsTransFunc tfunc;                 /* 転送関数 */
    Sint32      unit;                   /* 転送単位 */
    Bool        active;                 /* 動作中フラグ */
    Sint32      stat;                   /* 状態 */
    Sint32      mode;                   /* 転送モード */
} GfsTrans;


typedef GfsTrans *GftrHn;


/* オープンしたファイルを管理する */
typedef struct {
    Bool        used;                   /* この構造体が使用中かどうか */
    Sint32      amode;                  /* アクセスモード */
    Sint32      astat;                  /* アクセス状態 */
    GfsFlow     flow;
    GfsTrans    trans;
} GfsFile;


/* ファイルハンドル */
typedef GfsFile *GfsHn;


/* ユーザ登録転送関数 */
typedef Sint32 (*GfsTrFunc)(GfsHn gfs, Sint32 nsct);
     

/* ディレクトリレコード構造体(ファイル名なし)   */
typedef struct {
    CdcFile     dirrec;
} GfsDirId;


/* ディレクトリレコード構造体(ファイル名あり)   */
typedef struct {
    CdcFile     dirrec;
    Sint8       fname[GFS_FNAME_LEN];   /* ファイル名 */
} GfsDirName;


/* ディレクトリ情報管理構造体 */
typedef struct {
    Sint32      type;                   /* 種別 */
    Sint32      ndir;                   /* 最大要素数 */
    union {
        GfsDirId *dir_i;                /* ファイル名なしディレクトリ情報 */
        GfsDirName *dir_n;              /* ファイル名つきディレクトリ情報 */
    } dir;
} GfsDirTbl;


/* エラー処理関数 */
typedef void (*GfsErrFunc)(void *obj, Sint32 ec);

/* エラー状態 */
typedef struct {
    GfsErrFunc  func;                   /* エラー発生時の呼び出し関数   */
    void        *obj;                   /* 呼び出し関数に渡す第一引数   */
    Sint32      code;                   /* エラーコード                 */
} GfsErrStat;


/*****************************************************************************
 *      関数の宣言
 *****************************************************************************/

/* ファイルシステムの初期化 */
Sint32 GFS_Init(Sint32 open_max, void *work, GfsDirTbl *dirtbl);

/* ディレクトリレコードの読み込み */
Sint32 GFS_LoadDir(Sint32 fid, GfsDirTbl *dirtbl);

/* カレントディレクトリレコードの設定 */
Sint32 GFS_SetDir(GfsDirTbl *dirtbl);

/* 名前からファイル識別子への変換 */
Sint32 GFS_NameToId(Sint8 *fname);

/* ファイル識別子からファイル名への変換 */
Sint8 *GFS_IdToName(Sint32 fid);

/* ファイル識別子からディレクトリレコード取得 */
Sint32 GFS_GetDirInfo(Sint32 fid, GfsDirId *dirrec);

/*  ファイルシステムのリセット  */
void    GFS_Reset(void);

/* ファイルハンドルの割当て */
GfsHn GFS_Open(Sint32 fid);

/* ファイルハンドルの割当て解除 */
void GFS_Close(GfsHn gfs);

/* アクセスポインタの移動 */
Sint32 GFS_Seek(GfsHn gfs, Sint32 ofs, Sint32 org);

/* アクセスポインタの取得 */
Sint32 GFS_Tell(GfsHn gfs);

/* ファイル終了のチェック */
Bool GFS_IsEof(GfsHn gfs);

/* バイトサイズからセクタサイズへの変換 */
Sint32 GFS_ByteToSct(GfsHn gfs, Sint32 nbyte);

/* ファイルサイズに関する情報の取得 */
void GFS_GetFileSize(GfsHn gfs, Sint32 *sctsz, Sint32 *nsct, Sint32 *lstsz);

/* ファイル情報の取得 */
void GFS_GetFileInfo(GfsHn gfs, Sint32 *fid, Sint32 *fn, Sint32 *fsize,
                     Sint32 *atr);

/*  CDバッファ区画のセクタ数の取得      */
Sint32 GFS_GetNumCdbuf(GfsHn gfs);

/* 一括読み込み */
Sint32 GFS_Load(Sint32 fid, Sint32 ofs, void *buf, Sint32 bsize);

/* 完了復帰型読み込み */
Sint32 GFS_Fread(GfsHn gfs, Sint32 nsct, void *buf, Sint32 bsize);

/* 即時復帰型読み込み */
Sint32 GFS_NwFread(GfsHn gfs, Sint32 nsct, void *buf, Sint32 bsize);

/* 即時復帰型ＣＤバッファ読み込み */
Sint32 GFS_NwCdRead(GfsHn gfs, Sint32 nsct);

/* アクセス動作完了チェック */
Bool GFS_NwIsComplete(GfsHn gfs);

/* アクセス動作の中止 */
Sint32 GFS_NwStop(GfsHn gfs);

/* アクセス状態の取得 */
void GFS_NwGetStat(GfsHn gfs, Sint32 *amode, Sint32 *ndata);

/* ファイル単位でのアクセス動作の実行 */
Sint32 GFS_NwExecOne(GfsHn gfs);

/* サーバアクセス動作の実行 */
Sint32 GFS_NwExecServer(GfsHn *now_gfs);

/* ＣＤバッファからの取り出しモードの設定 */
Sint32 GFS_SetGmode(GfsHn gfs, Sint32 gmode);

/* 転送モードの設定 */
Sint32 GFS_SetTmode(GfsHn gfs, Sint32 tmode);

/* ソースバッファへの読み込みセクタ数の設定 */
Sint32 GFS_SetReadPara(GfsHn gfs, Sint32 nsct);

/* ソースバッファからホストへの転送セクタ数の設定 */
Sint32 GFS_SetTransPara(GfsHn gfs, Sint32 tsize);

/* 転送関数の登録       */
void    GFS_SetTrFunc(GfsHn gfs, GfsTrFunc func);

/* 転送関数における転送開始     */
Uint32  *GFS_StartTrans(GfsHn gfs, Sint32 *dadr);

/* ピックアップをアクセスポインタの位置に移動する */
Sint32 GFS_CdMovePickup(GfsHn gfs);

/* エラー処理関数の登録 */
void GFS_SetErrFunc(GfsErrFunc func, void *obj);

/* エラー状態の取得 */
void GFS_GetErrStat(GfsErrStat *stat);

/*  トラック番号の変換      */
Sint32  GFS_ConvTno(Sint32 logtno);

/* fidとオフセットからフレームアドレスを取得する */
Sint32 GFS_GetFad(Sint32 fid, Sint32 ofs);


/*****************************************************************************
 *      システム用情報
 *****************************************************************************/

/*---定数--------------------------------------------------------------------*/
#define GFS_OPEN_MAX    24              /* 最大オープンファイル数 */
#define GFS_TBUF_NR     2               /* 転送バッファの数 */
#define GFS_FTYPE_NR    3               /* ファイル種別数 */
#define GFS_SCTBUF_SIZ  2048            /* セクタバッファの大きさ */
#define GFS_CDBBUF_NR   24              /* CDブロックの区画数 */
#define GFS_SELQ_MAX    24              /* 絞り設定コマンドキューの最大長 */
#define GFS_FCONQ_MAX   24              /* 接続コマンドキューの最大長 */

/*---データ型----------------------------------------------------------------*/

/* ファイル操作用関数(管理テーブル, GfsMngのメンバ) */
typedef struct {
    Sint32 (*flowin)(GfsFlow *);        /* ソースバッファ読み込み関数   */
    void (*stopin)(GfsFlow *, Bool);    /* 読み込み中止関数             */
    Sint32 (*seek)(GfsFlow *, Sint32);  /* アクセスポインタ設定関数     */
    Sint32 (*tell)(GfsFlow *);          /* アクセスポインタ取得関数     */
} GfsFileFunc;

/* アクセスサーバ管理構造体(管理テーブル, GfsMngのメンバ) */
typedef struct {
    GfsHn   access_file[GFS_OPEN_MAX];  /* アクセスモード指定済ファイル */
    Sint32  nfile;                      /* 登録ファイル数               */
} GfsSvr;


/* ディレクトリ管理構造体(管理テーブル, GfsMngのメンバ) */
typedef struct {
    GfsDirTbl   dirtbl;                 /* ディレクトリ情報管理 */
    Sint32      nfile;                  /* ファイル数 */
} GfsDirMng;


/* 絞り設定コマンドキューの要素(CDブロック管理構造体, GfsCdbMngのメンバ) */
typedef struct {
    Uint8 flt;                          /* 設定先絞り番号 */
    Uint8 fmode;                        /* 絞りモード */
    CdcSubh subh;                       /* サブヘッダ */
    Sint32 fad;                         /* 開始FAD */
    Sint32 snum;                        /* FADセクタ数 */
} GfcdSelQu;

/* 接続コマンドキューの要素(CDブロック管理構造体, GfsCdbMngのメンバ) */
typedef struct {
    Sint32 flt;                         /* 設定絞り */
    Sint32 buf;                         /* 真出力接続バッファ */
    Sint32 flnout;                      /* 偽出力接続絞り(負の値を使用) */
} GfcdFconQu;    

/* CDブロック管理構造体(管理テーブル, GfsMngのメンバ) */
typedef struct {
    /* 資源管理 */
    Sint8 use_buf[GFS_CDBBUF_NR];       /* バッファ区画使用状態 */
    Sint8 use_filt[GFS_CDBBUF_NR];      /* 絞り使用状態 */
    Bool use_pu;                        /* ピックアップ使用状態 */
    Sint32 tr_bufno;                    /* データ転送中のバッファ区画   */
    Sint32 puid;                        /* ピックアップの所有者ID */
    Sint32 timer;                       /* タイムアウトカウンタ */
    CdcStat stat;                       /* CDブロックの状態 */
    void (*func)(void *);               /* CSCTハンドラ */
    void *obj;                          /* CSCTハンドラの引数 */
    /* GFCD_SetFiltタスク */
    struct {
        Sint32 len;                     /* 絞り設定コマンドキューの長さ */
        Sint32 stat;                    /* コマンドキュー先頭の状態 */
        GfcdSelQu selq[GFS_SELQ_MAX];   /* 絞り設定コマンドキュー */
    } tsk_setflt;
    /* GFCD_SetFiltConタスク */
    struct {
        Sint32 len;                     /* 接続コマンドキューの長さ */
        Sint32 stat;                    /* タスク終了フラグ */
        GfcdFconQu fconq[GFS_FCONQ_MAX]; /* 接続コマンドキュー */
    } tsk_fltcon;
    /* GFCD_SetConタスク */
    struct {
        Sint32 stat;                    /* タスク終了フラグ */
        Sint32 flt;                     /* CD接続先絞り */
    } tsk_setcon;
    /* GFCD_GetLenDataタスク */
    struct {
        Sint32 stat;                    /* タスク終了フラグ */
        Sint32 bufno;                   /* 対象区画 */
        Sint32 spos;                    /* 先頭セクタ位置 */
        Sint32 usct;                    /* 要求セクタ数 */
        Sint32 cnt;                     /* タスクの実行回数 */
        Sint32 *nsct;                   /* 区画内セクタ数格納先 */
        Sint32 *nbyte;                  /* 区画内バイト数格納先 */
    }tsk_getlen;
    /* GFCD_ReqDataタスク */
    struct {
        Sint32 stat;                    /* タスク終了フラグ */
        Sint32 bufno;                   /* 対象区画 */
        Sint32 sctpos;                  /* セクタ位置 */
        Sint32 nsct;                    /* 要求セクタ数 */
    } tsk_reqdat;
    /* GFCD_DelSctDataタスク */
    struct {
        Sint32 stat;                    /* タスク終了フラグ */
        Sint32 bufno;                   /* 対象区画 */
        Sint32 sctpos;                  /* 削除開始セクタ位置 */
        Sint32 nsct;                    /* 削除セクタ数 */
    } tsk_delsct;
    /* GFCD_MoveSctDataタスク */
    struct {
        Sint32 stat;                    /* タスク終了フラグ */
        Sint32 dst;                     /* 移動先区画 */
        Sint32 src;                     /* 移動元区画 */
        Sint32 spos;                    /* 移動開始セクタ位置 */
        Sint32 snum;                    /* 移動セクタ数 */
        Sint32 fmode;                   /* 移動先セレクタの絞りモード保存 */
    } tsk_movsct;
    /* GFCD_ChgDirタスク */
    struct {
        Sint32 stat;                    /* タスク終了フラグ */
        Sint16 fid;                     /* 設定ファイル識別子 */
        Sint16 work;                    /* 作業バッファ */
        Sint32 *ndir;                   /* 保持ディレクトリ情報個数 */
    } tsk_chgdir;
} GfsCdbMng;

/* 管理テーブル */
typedef struct {
    Sint32      openmax;                /* オープンファイルの最大数 */
    GfsFileFunc functbl[GFS_FTYPE_NR];  /* ファイル操作用関数 */
    GfsSvr      svr;                    /* アクセスサーバ */
    GfsDirMng   curdir;                 /* カレントディレクトリ */
    GfsHn       pickup;                 /* ピックアップ制御中ファイル */
    Sint32      sfad;                   /* 再生範囲:開始FAD */
    Sint32      efad;                   /* 再生範囲:終了FAD+1 */
    GfsHn       trans;                  /* データ転送中ファイル */
    GfsErrStat  error;                  /* エラー状態 */
    Uint32      flags;                  /* デバイス接続情報など */
    Sint32      timer;                  /* タイムアウトカウンタ */
    GfsCdbMng   cdb;                    /* CDブロック管理構造体 */
    GfsDataPack srcpk;                  /* 転送元データパック */
    GfsDataPack dstpk;                  /* 転送先データパック */
    Uint8       sect_buf[GFS_SCTBUF_SIZ]; /* １セクタの読み込み用バッファ */
    GfsFile     file[1];                /* 全ハンドルの配列（可変長） */
} GfsMng;

#endif      /* !defined(SEGA_GFS_H) */

/* end of file */
