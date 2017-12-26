/*---------------------------------------------------------
 * SGLライブラリCD関数用	1995.10.30
 *---------------------------------------------------------*/

/* sgl_cd.h */
#if	!defined(SGL_CD_H)
#define	SGL_CD_H
#include	"sl_def.h"
#include        <sega_stm.h>

/*--- 一般的なデータ型 -------------------------------------------------*/
/* ファイルハンドル			*/
typedef	StmGrpHn	CDHN;


/*--- ストリームデータを分類するためのキー -----------------------------*/
typedef struct {
    Sint16	cn;			/* チャネル番号			*/
    Sint16	sm;			/* サブモード			*/
    Sint16	ci;			/* コーディング情報		*/
} CDKEY;

#define	CDKEY_NONE	STM_KEY_NONE	/* ストリームキーの選択をしない	*/
#define	CDKEY_TERM	(-2)		/* ストリームキーの選択終了	*/


/*--- 読み込み領域情報 -------------------------------------------------*/
typedef struct {			/* ワークRAMへのコピー		*/
    void	*addr;			/* 転送先領域(転送しない:NULL)	*/
    Sint32	unit;			/* 転送領域のユニットサイズ	*/
    Sint32	size;			/* 転送領域のユニット数		*/
} TRANS_COPY;

typedef struct {			/* 転送関数を使用する場合	*/
    Sint32	(*func )(void *obj, Uint32 *addr, 
                        Sint32 adinc, Sint32 nsct);	/* 転送関数	*/
    void	*obj;			/* オブジェクト			*/
} TRANS_FUNC;

typedef struct {
    Sint32	type;			/* 転送方法			*/
    union {
	TRANS_COPY	copy;		/* ワークRAMへのコピー		*/
	TRANS_FUNC	func;		/* 転送関数を使用する場合	*/
    } trans;
} CDBUF;

/* 転送方法				*/
enum TRANS_TYPE {
    CDBUF_COPY	= 0,			/* ワークRAMへのコピー		*/
    CDBUF_FUNC	= 1,			/* 転送関数を使用する		*/
    CDBUF_TERM	= 2			/* 読み込み領域情報の指定終了	*/
};

/* 転送領域のユニットサイズ		*/
enum TRANS_UNIT {
    CDBUF_FORM1	= STM_UNIT_FORM1,	/* 2048byte単位			*/
    CDBUF_FORM2	= STM_UNIT_FORM2,	/* 2324byte単位			*/
    CDBUF_BYTE	= STM_UNIT_BYTE		/* バイト単位			*/
};

/* CDブロックの状態を取得するための定数	*/
#define	CDREQ_FREE	((CDHN)0xffffffff)	/* CDブロック空きセクタ	*/
#define	CDREQ_FAD	((CDHN)0xfffffffe)	/* ピックアップの位置	*/
#define	CDREQ_DRV	((CDHN)0xfffffffd)	/* ドライブの状態	*/

/*--- CDドライブ状態 ---------------------------------------------------*/
enum CDDRV {
    CDDRV_BUSY	= CDC_ST_BUSY,		/* 状態遷移中			*/
    CDDRV_PAUSE = CDC_ST_PAUSE,		/* ポーズ中			*/
    CDDRV_STDBY	= CDC_ST_STANDBY,	/* スタンバイ			*/
    CDDRV_PLAY	= CDC_ST_PLAY,		/* ＣＤ再生中			*/
    CDDRV_SEEK	= CDC_ST_SEEK,		/* シーク中			*/
    CDDRV_SCAN	= CDC_ST_SCAN,		/* スキャン再生中		*/
    CDDRV_OPEN	= CDC_ST_OPEN,		/* トレイが開いている		*/
    CDDRV_NODISC= CDC_ST_NODISC,	/* ディスクがない		*/
    CDDRV_RETRY	= CDC_ST_RETRY,		/* リードリトライ処理中		*/
    CDDRV_ERROR	= CDC_ST_ERROR,		/* リードデータエラーが発生した	*/
    CDDRV_FATAL	= CDC_ST_FATAL		/* 致命的エラーが発生した	*/
};

/*--- ステータス -------------------------------------------------------*/
enum CDSTAT {
    CDSTAT_PAUSE	= STM_EXEC_PAUSE,	/* 読み込み停止中	*/
    CDSTAT_DOING	= STM_EXEC_DOING,	/* 読み込み中		*/
    CDSTAT_WAIT		= STM_EXEC_WAIT,	/* 転送待ち状態		*/
    CDSTAT_COMPLETED	= STM_EXEC_COMPLETED	/* 読み込み完了		*/
};

/*--- エラーコード -----------------------------------------------------*/
enum CDERR {
    CDERR_OK	= STM_ERR_OK,		/* 正常終了			*/
    CDERR_RDERR	= STM_ERR_CDRD,		/* リードエラー			*/
    CDERR_NODISC= STM_ERR_CDNODISC,	/* ディスクがセットされていない	*/
    CDERR_CDROM	= GFS_ERR_CDROM,	/* ディスクがCD-ROMでない       */
    CDERR_IPARA	= GFS_ERR_DIRTBL,	/* 初期化パラメータ不正         */
    CDERR_DIR	= GFS_ERR_DIR,		/* ディレクトリ以外への移動     */
    CDERR_NEXIST= GFS_ERR_NEXIST,	/* ファイルが存在しない		*/
    CDERR_NUM	= STM_ERR_NUM,		/* バイト数などが負             */
    CDERR_PUINUSE=STM_ERR_PUINUSE,	/* ピックアップ動作中           */
    CDERR_ALIGN	= GFS_ERR_ALIGN,	/* 作業領域が4byte境界にない    */
    CDERR_TMOUT	= STM_ERR_TMOUT,	/* タイムアウト			*/
    CDERR_OPEN	= STM_ERR_CDOPEN,	/* トレイオープン		*/
    CDERR_FATAL	= STM_ERR_FATAL,	/* CDドライブが<FATAL>状態	*/
    CDERR_BUSY	= (-50)			/* 状態遷移中                   */
};

/*--- 作業領域のサイズ -------------------------------------------------*/

/* 初期化時の作業領域のサイズ		*/
#define	SLCD_MAX_OPEN	24
#define	SLCD_WORK_SIZE(nfile)	(nfile * sizeof(GfsDirName))

/*--- プロトタイプ -----------------------------------------------------*/

/* 初期化				*/
Sint32	slCdInit(Sint32 nfile, void *work);

/* ディレクトリ移動			*/
Sint32	slCdChgDir(Sint8 *pathname);

/* ファイルオープン			*/
CDHN	slCdOpen(Sint8 *pathname, CDKEY key[]);

/* ファイルの読み込み			*/
Sint32	slCdLoadFile(CDHN cdhn, CDBUF buf[]);

/* ストリームの転送			*/
Sint32	slCdTrans(CDHN cdhn, CDBUF buf[], Sint32 ndata[]);

/* 転送領域のリセット			*/
Bool	slCdResetBuf(CDHN cdhn, CDKEY *key);

/* 読み込み中断				*/
Sint32	slCdAbort(CDHN cdhn);

/* 読み込み一時停止			*/
Sint32	slCdPause(CDHN cdhn);

/* ステータス取得			*/
Sint32	slCdGetStatus(CDHN cdhn, Sint32 ndata[]);

/* ＣＤ用イベント関数			*/
void	slCdEvent(void);

#endif
