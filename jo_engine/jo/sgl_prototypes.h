/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2017, Johannes Fetz (johannesfetz@gmail.com)
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the Johannes Fetz nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL Johannes Fetz BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/** @file sgl_prototypes.h
 *  @author Johannes Fetz
 *
 *  @brief SGL prototypes
 */

#ifndef __JO_SGL_PROTOTYPES_H__
# define __JO_SGL_PROTOTYPES_H__

#if !JO_COMPILE_USING_SGL

//HACK
#define SEGA_XPT_H
#define SEGA_GFS_H
#define SEGA_CDC_H
#define SEGA_BUP_H

/*
#include <sgl.h>
*/
#define	    VDP2_VRAM_A0	0x25e00000
#define	    VDP2_VRAM_A1	0x25e20000
#define	    VDP2_VRAM_B0	0x25e40000
#define	    VDP2_VRAM_B1	0x25e60000

#define	    VDP2_COLRAM    		0x25f00000
#define		CHAR_SIZE_1x1		0

#define		COL_TYPE_256		0x10

#define	    KTBL0_RAM		VDP2_VRAM_A1		/* 回転スクロール用係数テーブル */
#define	    BACK_CRAM		(KTBL0_RAM + 0x1fffe)	/* Back color */

#define	    CL_Half		2		/* 半輝度モード */
#define	    CL_Trans		3		/* 半透明モード */
#define	    CL_Gouraud		4		/* グーローシェーディングモード */

#define	PER_ID_NotConnect	0xff	/* 未接続				*/
#define	PER_ID_StnPad		0x02	/* サターン標準パッド			*/
#define	PER_ID_StnMouse		0x23	/* サターンマウス			*/
#define	PER_ID_ShuttleMouse	0xe3	/* シャトルマウス			*/

#define	    toFIXED(a)		((FIXED)(65536.0 * (a)))
#define	    POStoFIXED(x,y,z)	{toFIXED(x),toFIXED(y),toFIXED(z)}
#define	    DEGtoANG(d)		((ANGLE)((65536.0 * (d)) / 360.0))
#define	NORMAL(x,y,z)		{POStoFIXED(x,y,z)
#define	VERTICES(v0,v1,v2,v3)	{v0 , v1 , v2 , v3}}
#define	C_RGB(r,g,b)		(((b)&0x1f)<<10|((g)&0x1f)<<5|((r)&0x1f)|0x8000)

#define	    ECdis		(1 << 7)	/* エンドコードをパレットのひとつとして使用 */

#define	    SPdis		(1 << 6)	/* スペースコードをパレットのひとつとして使用 */

#define	    sprPolygon		(FUNC_Polygon | ((ECdis | SPdis) << 24))

#define	    FUNC_Texture	2
#define	    FUNC_Polygon	4
#define	    FUNC_PolyLine	5

#define	    MESHon		(1 << 8)	/* メッシュで表示 */
#define	    CL32KRGB		(5 << 3)	/* ＲＧＢ３２Ｋ色モード */

#define	    UseTexture		(1 << 2)	/* テクスチャを貼るポリゴン */
#define	    UseLight		(1 << 3)	/* 光源の影響を受けるポリゴン */
#define	    UsePalette		(1 << 5)	/* ポリゴンのカラー */

#define	    No_Texture		0		/* テクスチャを使用しない時 */
#define	    No_Option		0		/* オプションを使用しない時 */
#define	    No_Gouraud		0		/* グーローシェーディングを使用しない時 */
#define	    No_Palet	        0		/* カラーパレットの指定がいらない時 */

#define	    sprNoflip		((0) | FUNC_Texture | (UseTexture << 16))

typedef unsigned char	Uint8 ;		/* 符号なし１バイト整数 */
typedef signed   char	Sint8 ;		/* 符号つき１バイト整数 */
typedef unsigned short	Uint16 ;	/* 符号なし２バイト整数 */
typedef signed   short	Sint16 ;	/* 符号つき２バイト整数 */
typedef unsigned long	Uint32 ;	/* 符号なし４バイト整数 */
typedef signed   long	Sint32 ;	/* 符号つき４バイト整数 */
typedef int		Bool ;		/* 論理型（論理定数を値にとる）	*/
typedef float		Float32 ;	/* ４バイト実数 */

enum ps {X, Y, Z, XYZ, XYZS, XYZSS, XY = Z, S = XYZ, Sh = S, Sv = XYZS} ;
enum base
{
    SORT_BFR,			/* 直前に表示したポリゴンの位置を使う */
    SORT_MIN,			/* ４点の内、一番手前の点を使う */
    SORT_MAX,			/* ４点の内一番遠い点を使う */
    SORT_CEN			/* ４点の平均位置を使う */
} ;

enum pln
{
    Single_Plane,			/* 片面ポリゴン */
    Dual_Plane			/* 両面ポリゴン(表裏判定の結果を見ない) */
} ;

typedef	    Sint16	ANGLE ;
typedef	    Sint32	FIXED ;
typedef     FIXED	MATRIX[4][3] ;		/* マトリクスデータ */
typedef     FIXED	VECTOR[XYZ] ;		/* ベクトルデータ */
typedef     FIXED	POINT[XYZ] ;		/* 頂点の位置データ */

#define		_Stereo		0x80
#define		_Mono		0x00

#define		_PCM16Bit	0x00
#define		_PCM8Bit	0x10

typedef struct
{
    Uint8	mode ;		/* Mode */
    Uint8	channel ;	/* PCM Channel Number */
    Uint8	level ;		/* 0 ~ 127 */
    Sint8	pan ;		/* -128 ~ +127 */
    Uint16	pitch ;
    Uint8	eflevelR ;	/* Effect level for Right(mono) 0 ~ 7 */
    Uint8	efselectR ;	/* Effect select for Right(mono) 0 ~ 15 */
    Uint8	eflevelL ;	/* Effect level for Left 0 ~ 7 */
    Uint8	efselectL ;	/* Effect select for Left 0 ~ 15 */
} PCM ;

enum SmpcCommand  			/* ＳＭＰＣコマンド番号			*/
{
    SMPC_MSHON,			/* (00) マスタ　ＳＨ２　ＯＮ		*/
    SMPC_SSHON,			/* (01) スレーブ　ＳＨ２　ＯＮ		*/
    SMPC_SSHOFF,			/* (02) スレーブ　ＳＨ２　ＯＦＦ	*/
    SMPC_SNDON,			/* (03) サウンド　ＯＮ			*/
    SMPC_SNDOFF,			/* (04) サウンド　ＯＦＦ		*/
    SMPC_CDON,			/* (05) ＣＤ　ＯＮ			*/
    SMPC_CDOFF,			/* (06) ＣＤ　ＯＦＦ			*/
    SMPC_SYSRES,			/* (07) システム全体リセット		*/
    SMPC_CKC352,			/* (08) クロックチェンジ３５２モード	*/
    SMPC_CKC320,			/* (09) クロックチェンジ３２０モード	*/
    SMPC_NMIREQ,			/* (0a) ＮＭＩリクエスト		*/
    SMPC_RESENA,			/* (0b) リセットイネーブル		*/
    SMPC_RESDIS,			/* (0c) リセットディスエーブル		*/
    SMPC_GETSTS,			/* (0d) ステータス取得			*/
    SMPC_GETPER,			/* (0e) ペリフェラル取得		*/
    SMPC_SETMEM,			/* (0f) ＳＭＰＣメモリ設定		*/
    SMPC_SETTIM,			/* (10) 時刻設定			*/
    SMPC_CMDMAX			/* (11) コマンド番号最大値		*/
};

enum SmpcWaitMode  			/* ＳＭＰＣコマンド実行モード		*/
{
    SMPC_NO_WAIT,			/* (00) コマンド実行終了待ち無し	*/
    SMPC_WAIT			/* (01) コマンド実行終了待ち有り	*/
};


enum BooleanSwitch  			/* 論理定数２（スイッチ） */
{
    OFF = 0,
    ON  = 1
};

typedef struct
{
    Uint16	 texno ;		/* テクスチャ番号 */
    Uint16	 atrb ;			/* アトリビュートデータ(表示モード) */
    Uint16	 colno ;		/* カラーナンバー */
    Uint16	 gstb ;			/* グーローシェーディングテーブル */
    Uint16	 dir ;			/* テクスチャの反転とファンクションナンバー */
} SPR_ATTR ;

typedef struct
{
    Uint8	 flag ;			/* 片面か両面かのフラグ */
    Uint8	 sort ;			/* ソートの基準位置とオプション設定 */
    Uint16	 texno ;		/* テクスチャ番号 */
    Uint16	 atrb ;			/* アトリビュートデータ(表示モード) */
    Uint16	 colno ;		/* カラーナンバー */
    Uint16	 gstb ;			/* グーローシェーディングテーブル */
    Uint16	 dir ;			/* テクスチャの反転とファンクションナンバー */
} ATTR ;

typedef struct
{
    VECTOR	 norm ;			/* 法線ベクトル */
    Uint16	 Vertices[4] ;		/* ポリゴンを構成する頂点番号 */
} POLYGON ;

typedef struct
{
    POINT	*pntbl ;		/* 頂点の位置データテーブル */
    Uint32	 nbPoint ;		/* 頂点の数 */
    POLYGON	*pltbl ;		/* ポリゴン定義テーブル */
    Uint32	 nbPolygon ;		/* ポリゴンの数 */
    ATTR	*attbl ;		/* ポリゴンのアトリビュートテーブル */
} PDATA ;

typedef struct
{
    Uint16	 Hsize ;		/* スプライトのＨサイズ */
    Uint16	 Vsize ;		/* スプライトのＶサイズ */
    Uint16	 CGadr ;		/* スプライトＣＧアドレス / ８ */
    Uint16	 HVsize ;		/* Ｈサイズ/８、Ｖサイズ(ハード用) */
} TEXTURE ;

typedef struct
{
    Uint16	 texno ;		/* テクスチャ番号 */
    Uint16	 cmode ;		/* カラーモード */
    void	*pcsrc ;		/* 元データの所在 */
} PICTURE ;

typedef struct  			/* デジタルデバイス			*/
{
    Uint8	id;			/* ペリフェラルＩＤ			*/
    Uint8	ext;			/* 拡張データサイズ			*/
    Uint16	data;			/* ボタン現在データ			*/
    Uint16	push;			/* ボタン押下データ			*/
    Uint16	pull;			/* ボタン引上データ			*/
    Uint32	dummy2[4];		/* ダミー２				*/
} PerDigital;

enum BooleanLogic  			/* 論理定数１（偽、真） */
{
    FALSE = 0,
    TRUE  = 1
};

enum tvsz
{
    TV_320x224, TV_320x240, TV_320x256, TV_dummy1,
    TV_352x224, TV_352x240, TV_352x256, TV_dummy2,
    TV_640x224, TV_640x240, TV_640x256, TV_dummy3,
    TV_704x224, TV_704x240, TV_704x256, TV_dummy4,

    TV_320x448, TV_320x480, TV_320x512, TV_dummy5,
    TV_352x448, TV_352x480, TV_352x512, TV_dummy6,
    TV_640x448, TV_640x480, TV_640x512, TV_dummy7,
    TV_704x448, TV_704x480, TV_704x512, TV_dummy8
} ;

#define		PNB_2WORD		0
#define		PNB_1WORD		0x8000
#define		CN_10BIT       		0
#define		CN_12BIT		0x4000
#define		COL_TYPE_32768		0x30

#define		PL_SIZE_1x1		0
#define		PL_SIZE_2x1		1
#define		PL_SIZE_2x2		3

#define		NBG0ON			(1<<0)
#define		NBG1ON			(1<<1)

#define		BM_512x256		0x02
#define		BM_512x512		0x06
#define		BM_1024x256		0x0a
#define		BM_1024x512		0x0e

extern void    slCurColor(Uint16) ;
extern	PerDigital*	Smpc_Peripheral;
extern	Uint8		Per_Connect1, Per_Connect2;
extern  void    slPlaneNbg1(Uint16) ;
extern  void   slUnitMatrix(MATRIX) ;
extern  Bool   slDispSprite(FIXED *, SPR_ATTR *attr, ANGLE) ;
extern  void    slScrPosNbg1(FIXED x,FIXED y) ;
extern  void    slZoomNbg1(FIXED x,FIXED y) ;
extern void    slDMACopy(void *, void *, Uint32) ;
extern  void    slBack1ColSet(void *, Uint16) ;
extern  void    slCharNbg0(Uint16 type,Uint16 size) ;
extern  void    slCharNbg1(Uint16 type,Uint16 size) ;
extern  void    slTVOn(void) ;
extern  void    slPageNbg1(void *,void *,Uint16) ;
extern  void    slMapNbg1(void *,void *,void *,void *) ;
extern void    slPrint(char *, void *) ;
extern  void    slBitMapNbg1(Uint16, Uint16, void *) ;
extern  void    slTVOff(void) ;
extern  Bool    slScrAutoDisp(Uint32) ;
extern  void   slInitSystem(Uint16, TEXTURE *, Sint8) ;
extern  void   slSynch(void) ;
extern  void    slPriority(Sint16, Uint16) ;
extern void   *slLocate(Uint16, Uint16) ;
#define		scnNBG0			1
#define		scnNBG1			0

extern  void   slLookAt(FIXED *camera, FIXED *target, ANGLE) ;
extern  void   slLight(VECTOR) ;
extern  Bool   slPushMatrix(void) ;
extern  Bool   slPopMatrix(void) ;
extern  void   slRotX(ANGLE) ;
extern  void   slRotY(ANGLE) ;
extern  void   slRotZ(ANGLE) ;
extern  void   slScale(FIXED, FIXED, FIXED) ;
extern  Bool   slPutPolygon(PDATA *) ;
extern  Bool   slWindow(Sint16, Sint16, Sint16, Sint16, Sint16, Sint16, Sint16) ;
extern  void   slPerspective(ANGLE) ;
extern  void   slTranslate(FIXED, FIXED, FIXED) ;
extern  void   slZdspLevel(Uint16) ;

extern	Bool	slRequestCommand(Uint8, Uint8);

#define CDC_POS_PTYPE(pos)          ((pos)->ptype)
#define CDC_POS_FAD(pos)            ((pos)->pbody.fad)
#define CDC_POS_TNO(pos)            ((pos)->pbody.trkidx.tno)
#define CDC_POS_IDX(pos)            ((pos)->pbody.trkidx.idx)

#define CDC_PLY_START(ply)          ((ply)->start)
#define CDC_PLY_END(ply)            ((ply)->end)
#define CDC_PLY_PMODE(ply)          ((ply)->pmode)

#define CDC_PLY_STYPE(ply)          CDC_POS_PTYPE(&CDC_PLY_START(ply))
#define CDC_PLY_SFAD(ply)           CDC_POS_FAD(&CDC_PLY_START(ply))
#define CDC_PLY_STNO(ply)           CDC_POS_TNO(&CDC_PLY_START(ply))
#define CDC_PLY_SIDX(ply)           CDC_POS_IDX(&CDC_PLY_START(ply))

#define CDC_PLY_ETYPE(ply)          CDC_POS_PTYPE(&CDC_PLY_END(ply))
#define CDC_PLY_EFAS(ply)           CDC_POS_FAD(&CDC_PLY_END(ply))
#define CDC_PLY_ETNO(ply)           CDC_POS_TNO(&CDC_PLY_END(ply))
#define CDC_PLY_EIDX(ply)           CDC_POS_IDX(&CDC_PLY_END(ply))

#define CDC_SM_EOR      0x01    /* レコード最後のセクタ */
#define CDC_SM_VIDEO    0x02    /* ビデオセクタ */
#define CDC_SM_AUDIO    0x04    /* オーディオセクタ */
#define CDC_SM_DATA     0x08    /* データセクタ */
#define CDC_SM_TRIG     0x10    /* トリガＯＮ */
#define CDC_SM_FORM     0x20    /* フォームビット（1:Form2,  0:Form1）*/
#define CDC_SM_RT       0x40    /* リアルタイムセクタ */
#define CDC_SM_EOF      0x80    /* ファイル最後のセクタ */

#define CDC_PM_DFL          0x00    /* 再生モードの省略値 */

/* エラーコード */
#define GFS_ERR_OK              0       /* 正常終了                         */
#define GFS_ERR_CDRD            (-1)    /* CDリードエラー                   */
#define GFS_ERR_CDNODISC        (-2)    /* CDがセットされていない           */
#define GFS_ERR_CDROM           (-3)    /* ディスクがCD-ROMでない           */
#define GFS_ERR_DIRTBL          (-4)    /* ディレクトリ管理テーブルが不正   */
#define GFS_ERR_OPENMAX         (-5)    /* 最大オープン数の値が不正         */
#define GFS_ERR_DIR             (-6)    /* 指定ファイルはディレクトリでない */
#define GFS_ERR_CDBFS           (-7)    /* CDブロックファイルシステムの     */

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


/* ステータス */
enum CdcStatus
{
    /* ステータスコード（ＣＤドライブ状態） */
    CDC_ST_BUSY     = 0x00,     /* 状態遷移中 */
    CDC_ST_PAUSE    = 0x01,     /* ポーズ中（一時停止） */
    CDC_ST_STANDBY  = 0x02,     /* スタンバイ（ドライブ停止状態） */
    CDC_ST_PLAY     = 0x03,     /* ＣＤ再生中 */
    CDC_ST_SEEK     = 0x04,     /* シーク中 */
    CDC_ST_SCAN     = 0x05,     /* スキャン再生中 */
    CDC_ST_OPEN     = 0x06,     /* トレイが開いている */
    CDC_ST_NODISC   = 0x07,     /* ディスクがない */
    CDC_ST_RETRY    = 0x08,     /* リードリトライ処理中 */
    CDC_ST_ERROR    = 0x09,     /* リードデータエラーが発生した */
    CDC_ST_FATAL    = 0x0a,     /* 致命的エラーが発生した */

    /* その他 */
    CDC_ST_PERI     = 0x20,     /* 定期レスポンス */
    CDC_ST_TRNS     = 0x40,     /* データ転送要求あり */
    CDC_ST_WAIT     = 0x80,     /* WAIT */
    CDC_ST_REJECT   = 0xff      /* REJECT */
};

/* 転送モード(GFS_SetTmode) */
enum GfsTransMode
{
    GFS_TMODE_SCU = 0,                  /* ＳＣＵによりＤＭＡ転送    */
    GFS_TMODE_SDMA0,                    /* ＤＭＡサイクルスチール転送   cn=0 */
    GFS_TMODE_SDMA1,                    /*                              cn=1 */
    GFS_TMODE_CPU,                      /* ソフトウェア転送 */
    GFS_TMODE_STM,                      /* ストリーム用転送 */
    GFS_TMODE_END
};


enum CdcPosType
{
    CDC_PTYPE_DFL,          /* 省略値の指定 */
    CDC_PTYPE_FAD,          /* フレームアドレス指定 */
    CDC_PTYPE_TNO,          /* トラック／インデックス指定 */
    CDC_PTYPE_NOCHG,        /* 未変更の指定（設定値を変更しない） */

    CDC_PTYPE_END
};

/* ＣＤ位置パラメータ */
typedef struct
{
    Sint32 ptype;           /* 位置タイプ（位置パラメータの種類の指定）*/
    union
    {
        Sint32 fad;         /* フレームアドレス、セクタ数 */
        struct
        {
            Uint8 tno;      /* トラック番号 */
            Uint8 idx;      /* インデックス番号 */
        } trkidx;
    } pbody;
} CdcPos;


/* ＣＤ再生パラメータ */
typedef struct
{
    CdcPos  start;          /* 開始位置 */
    CdcPos  end;            /* 終了位置 */
    Uint8   pmode;          /* 再生モード（ﾋﾟｯｸｱｯﾌﾟ移動、繰り返し回数）*/
} CdcPly;


extern  void   slInitSound(Uint8 *, Uint32, Uint8 *, Uint32) ;
extern  Bool   slCDDAOn(Uint8, Uint8, Sint8, Sint8) ;
extern  Sint8  slPCMOn(PCM *, void *, Uint32) ;
extern  Bool   slPCMOff(PCM *) ;
extern  Sint8  slSndPCMNum() ;
extern  void   slSndFlush() ;
extern  Bool   slPCMStat(PCM *) ;
extern  Bool   slSndVolume(Uint8) ;

Sint32  CDC_CdPlay(CdcPly *ply);
Sint32  CDC_CdSeek(CdcPos *pos);
Sint32  CDC_CdInit(Sint32 iflag, Sint32 stnby, Sint32 ecc, Sint32 retry);

/* ファイル情報 */
typedef struct
{
    Sint32  fad;            /* ファイル先頭フレームアドレス */
    Sint32  size;           /* ファイルサイズ（バイト数） */
    Uint8   unit;           /* ファイルユニットサイズ */
    Uint8   gap;            /* ギャップサイズ */
    Uint8   fn;             /* ファイル番号 */
    Uint8   atr;            /* ファイルアトリビュート */
} CdcFile;

/* サブヘッダ条件 */
typedef struct
{
    Uint8   fn;             /* ファイル番号 */
    Uint8   cn;             /* チャネル番号 */
    Uint8   smmsk;          /* サブモードのマスクパターン */
    Uint8   smval;          /* サブモードの比較値 */
    Uint8   cimsk;          /* コーディング情報のマスクパターン */
    Uint8   cival;          /* コーディング情報の比較値 */
} CdcSubh;


enum GfsSeekMode {
    GFS_SEEK_SET = 0,                   /* 先頭からシーク               */
    GFS_SEEK_CUR,                       /* 現在の位置からシーク         */
    GFS_SEEK_END                        /* 終端からシーク               */
};

typedef Sint32 (*GfsTransFunc)(void *obj, Sint32 nsct);



/* ファイル情報 */
typedef struct
{
    Sint32      fid;                    /* ファイル識別子 */
    CdcFile     finfo;
    Sint32      sctsz;                  /* セクタ長 */
    Sint32      nsct;                   /* セクタ数 */
    Sint32      lstrm;                  /* 最終セクタ中の無効データ数   */
} GfsFinfo;


/* CDバッファの資源管理 */
typedef struct
{
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
typedef struct
{
    Sint32      fid;                    /* SCSIファイル識別子   */
    Sint32      filepos;                /* 先頭データのファイル上の位置 */
    Sint32      sctpos;                 /* バッファ上のセクタ位置 */
    Sint32      sctnum;                 /* バッファ内のセクタ数 */
} GfsScsiRsrc;


/* メモリファイルの仮想CDバッファ管理 */
typedef struct
{
    void        *data;                  /* データ領域           */
    Sint32      filepos;                /* 先頭データのファイル上の位置 */
    Sint32      sctpos;                 /* バッファ上のセクタ位置 */
    Sint32      sctnum;                 /* バッファ内のセクタ数 */
} GfsMemRsrc;


/* 読み込み元管理 */
typedef struct
{
    /* 読み込み元管理 */
    Sint32      ftype;                  /* ファイル種別         */
    union
    {
        GfsCdRsrc       cd;             /* CDの資源             */
        GfsScsiRsrc     scsi;           /* SCSIファイルの資源   */
        GfsMemRsrc      mem;            /* メモリファイルの資源 */
    } rsrc;
} GfsDtsrc;


/* CDからの読み込みを管理する */
typedef struct
{
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
typedef struct
{
    void        *data;                  /* データ領域           */
    Sint32      adlt;                   /* アドレスの増加分     */
    Sint32      len;                    /* 転送バイト数         */
    Sint32      nsct;                   /* 転送セクタ数         */
    Bool        use;                    /* 使用フラグ           */
} GfsDataPack;


typedef GfsDataPack *GfdpHn;

/* 転送管理 */
/* CDバッファからの転送を管理する */
typedef struct
{
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
typedef struct
{
    Bool        used;                   /* この構造体が使用中かどうか */
    Sint32      amode;                  /* アクセスモード */
    Sint32      astat;                  /* アクセス状態 */
    GfsFlow     flow;
    GfsTrans    trans;
} GfsFile;

#define GFS_FNAME_LEN   12              /* ファイル名の長さ                 */



/* ディレクトリレコード構造体(ファイル名なし)   */
typedef struct
{
    CdcFile     dirrec;
} GfsDirId;


/* ディレクトリレコード構造体(ファイル名あり)   */
typedef struct
{
    CdcFile     dirrec;
    Sint8       fname[GFS_FNAME_LEN];   /* ファイル名 */
} GfsDirName;


/* ディレクトリ情報管理構造体 */
typedef struct
{
    Sint32      type;                   /* 種別 */
    Sint32      ndir;                   /* 最大要素数 */
    union
    {
        GfsDirId *dir_i;                /* ファイル名なしディレクトリ情報 */
        GfsDirName *dir_n;              /* ファイル名つきディレクトリ情報 */
    } dir;
} GfsDirTbl;


/* ファイルハンドル */
typedef GfsFile *GfsHn;

#define GFS_FTYPE_NR    3               /* ファイル種別数 */
#define GFS_SCTBUF_SIZ  2048            /* セクタバッファの大きさ */
#define GFS_OPEN_MAX    24              /* 最大オープンファイル数 */
#define GFS_FCONQ_MAX   24              /* 接続コマンドキューの最大長 */

/* エラー処理関数 */
typedef void (*GfsErrFunc)(void *obj, Sint32 ec);

/* エラー状態 */
typedef struct
{
    GfsErrFunc  func;                   /* エラー発生時の呼び出し関数   */
    void        *obj;                   /* 呼び出し関数に渡す第一引数   */
    Sint32      code;                   /* エラーコード                 */
} GfsErrStat;

#define GFS_CDBBUF_NR   24              /* CDブロックの区画数 */
#define GFS_SELQ_MAX    24              /* 絞り設定コマンドキューの最大長 */

/* ディレクトリ種別(GFS_LoadDir, GFS_SetDir)     */
enum GfsDirType
{
    GFS_DIR_ID          = 0,            /* ファイル名によるアクセス不可 */
    GFS_DIR_NAME                        /* ファイル名によるアクセス可   */
};

enum GfsServerStatus
{
    GFS_SVR_COMPLETED = 0,              /* 全ファイルのアクセス終了     */
    GFS_SVR_BUSY,                       /* ファイルアクセス中           */
    GFS_SVR_CDPAUSE,                    /* ＣＤバッファフルの為一時停止 */
    GFS_SVR_ERROR                       /* エラー発生 */
};



/* 接続コマンドキューの要素(CDブロック管理構造体, GfsCdbMngのメンバ) */
typedef struct
{
    Sint32 flt;                         /* 設定絞り */
    Sint32 buf;                         /* 真出力接続バッファ */
    Sint32 flnout;                      /* 偽出力接続絞り(負の値を使用) */
} GfcdFconQu;


/* ＣＤステータス情報（ステータス＋ＣＤレポート） */
typedef struct
{
    Uint8   status;         /* ステータス */
    struct                  /* ＣＤレポート */
    {
        Uint8   flgrep;     /* ＣＤフラグとリピート回数 */
        Uint8   ctladr;     /* CONTROL/ADRバイト */
        Uint8   tno;        /* トラック番号 */
        Uint8   idx;        /* インデックス番号 */
        Sint32  fad;        /* フレームアドレス */
    } report;
} CdcStat;

/* 絞り設定コマンドキューの要素(CDブロック管理構造体, GfsCdbMngのメンバ) */
typedef struct
{
    Uint8 flt;                          /* 設定先絞り番号 */
    Uint8 fmode;                        /* 絞りモード */
    CdcSubh subh;                       /* サブヘッダ */
    Sint32 fad;                         /* 開始FAD */
    Sint32 snum;                        /* FADセクタ数 */
} GfcdSelQu;


/* CDブロック管理構造体(管理テーブル, GfsMngのメンバ) */
typedef struct
{
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
    struct
    {
        Sint32 len;                     /* 絞り設定コマンドキューの長さ */
        Sint32 stat;                    /* コマンドキュー先頭の状態 */
        GfcdSelQu selq[GFS_SELQ_MAX];   /* 絞り設定コマンドキュー */
    } tsk_setflt;
    /* GFCD_SetFiltConタスク */
    struct
    {
        Sint32 len;                     /* 接続コマンドキューの長さ */
        Sint32 stat;                    /* タスク終了フラグ */
        GfcdFconQu fconq[GFS_FCONQ_MAX]; /* 接続コマンドキュー */
    } tsk_fltcon;
    /* GFCD_SetConタスク */
    struct
    {
        Sint32 stat;                    /* タスク終了フラグ */
        Sint32 flt;                     /* CD接続先絞り */
    } tsk_setcon;
    /* GFCD_GetLenDataタスク */
    struct
    {
        Sint32 stat;                    /* タスク終了フラグ */
        Sint32 bufno;                   /* 対象区画 */
        Sint32 spos;                    /* 先頭セクタ位置 */
        Sint32 usct;                    /* 要求セクタ数 */
        Sint32 cnt;                     /* タスクの実行回数 */
        Sint32 *nsct;                   /* 区画内セクタ数格納先 */
        Sint32 *nbyte;                  /* 区画内バイト数格納先 */
    } tsk_getlen;
    /* GFCD_ReqDataタスク */
    struct
    {
        Sint32 stat;                    /* タスク終了フラグ */
        Sint32 bufno;                   /* 対象区画 */
        Sint32 sctpos;                  /* セクタ位置 */
        Sint32 nsct;                    /* 要求セクタ数 */
    } tsk_reqdat;
    /* GFCD_DelSctDataタスク */
    struct
    {
        Sint32 stat;                    /* タスク終了フラグ */
        Sint32 bufno;                   /* 対象区画 */
        Sint32 sctpos;                  /* 削除開始セクタ位置 */
        Sint32 nsct;                    /* 削除セクタ数 */
    } tsk_delsct;
    /* GFCD_MoveSctDataタスク */
    struct
    {
        Sint32 stat;                    /* タスク終了フラグ */
        Sint32 dst;                     /* 移動先区画 */
        Sint32 src;                     /* 移動元区画 */
        Sint32 spos;                    /* 移動開始セクタ位置 */
        Sint32 snum;                    /* 移動セクタ数 */
        Sint32 fmode;                   /* 移動先セレクタの絞りモード保存 */
    } tsk_movsct;
    /* GFCD_ChgDirタスク */
    struct
    {
        Sint32 stat;                    /* タスク終了フラグ */
        Sint16 fid;                     /* 設定ファイル識別子 */
        Sint16 work;                    /* 作業バッファ */
        Sint32 *ndir;                   /* 保持ディレクトリ情報個数 */
    } tsk_chgdir;
} GfsCdbMng;


/* アクセスサーバ管理構造体(管理テーブル, GfsMngのメンバ) */
typedef struct
{
    GfsHn   access_file[GFS_OPEN_MAX];  /* アクセスモード指定済ファイル */
    Sint32  nfile;                      /* 登録ファイル数               */
} GfsSvr;

/* ディレクトリ管理構造体(管理テーブル, GfsMngのメンバ) */
typedef struct
{
    GfsDirTbl   dirtbl;                 /* ディレクトリ情報管理 */
    Sint32      nfile;                  /* ファイル数 */
} GfsDirMng;

/* ファイル操作用関数(管理テーブル, GfsMngのメンバ) */
typedef struct
{
    Sint32 (*flowin)(GfsFlow *);        /* ソースバッファ読み込み関数   */
    void (*stopin)(GfsFlow *, Bool);    /* 読み込み中止関数             */
    Sint32 (*seek)(GfsFlow *, Sint32);  /* アクセスポインタ設定関数     */
    Sint32 (*tell)(GfsFlow *);          /* アクセスポインタ取得関数     */
} GfsFileFunc;


/* 管理テーブル */
typedef struct
{
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

#define GFS_WORK_SIZE(open_max) \
            (sizeof(GfsMng) + ((open_max) - 1) * sizeof(GfsFile))
#define GFS_DIRTBL_TYPE(dirtbl)         ((dirtbl)->type)
#define GFS_DIRTBL_DIRNAME(dirtbl)      ((dirtbl)->dir.dir_n)
#define GFS_DIRTBL_NDIR(dirtbl)         ((dirtbl)->ndir)

/* ファイルシステムの初期化 */
Sint32 GFS_Init(Sint32 open_max, void *work, GfsDirTbl *dirtbl);
/* 即時復帰型読み込み */
Sint32 GFS_NwFread(GfsHn gfs, Sint32 nsct, void *buf, Sint32 bsize);
/* ファイル単位でのアクセス動作の実行 */
Sint32 GFS_NwExecOne(GfsHn gfs);
/* 完了復帰型読み込み */
Sint32 GFS_Fread(GfsHn gfs, Sint32 nsct, void *buf, Sint32 bsize);
/* アクセス状態の取得 */
void GFS_NwGetStat(GfsHn gfs, Sint32 *amode, Sint32 *ndata);
/* ファイルハンドルの割当て */
GfsHn GFS_Open(Sint32 fid);
/* ファイルハンドルの割当て解除 */
void GFS_Close(GfsHn gfs);
void GFS_GetFileSize(GfsHn gfs, Sint32 *sctsz, Sint32 *nsct, Sint32 *lstsz);
Sint32 GFS_NwCdRead(GfsHn gfs, Sint32 nsct);
Sint32 GFS_SetTransPara(GfsHn gfs, Sint32 tsize);

Sint32 GFS_LoadDir(Sint32 fid, GfsDirTbl *dirtbl);
Sint32 GFS_SetDir(GfsDirTbl *dirtbl);
Sint32 GFS_NameToId(Sint8 *fname);
Sint32 GFS_Load(Sint32 fid, Sint32 ofs, void *buf, Sint32 bsize);
Sint32 GFS_Seek(GfsHn gfs, Sint32 ofs, Sint32 org);

typedef struct  			/* ＲＴＣ時刻				*/
{
    Uint16	year;			/* 西暦年号				*/
    Uint8	month;			/* 曜日・月				*/
    Uint8	date;			/* 日					*/
    Uint8	hour;			/* 時					*/
    Uint8	minute;			/* 分					*/
    Uint8	second;			/* 秒					*/
    Uint8	dummy;			/* ダミー				*/
} SmpcDateTime;


typedef struct  			/* ＳＭＰＣステータス			*/
{
    Uint8	cond;			/* 状態ステータス			*/
    Uint8	dummy1;			/* ダミー１				*/
    Uint16	dummy2;			/* ダミー２				*/
    SmpcDateTime	rtc;		/* ＲＴＣ時刻				*/
    Uint8	ctg;			/* カートリッジコード			*/
    Uint8	area;			/* エリアコード				*/
    Uint16	system;			/* システムステータス			*/
    Uint32	smem;			/* ＳＭＰＣメモリ保持データ		*/
} SmpcStatus;

enum SmpcLanguage  			/* ＳＭＰＣメモリ言語番号		*/
{
    SMPC_ENGLISH,			/* (00) 英語				*/
    SMPC_DEUTSCH,			/* (01) ドイツ語			*/
    SMPC_FRANCAIS,			/* (02) フランス語			*/
    SMPC_ESPANOL,			/* (03) スペイン語			*/
    SMPC_ITALIANO,			/* (04) イタリア語			*/
    SMPC_JAPAN			/* (05) 日本語				*/
};

extern  void   slIntFunction(void (*func)()) ;
extern	Uint8	slGetLanguage(void);

extern	SmpcStatus*	Smpc_Status;
#define	slGetStatus()			slRequestCommand(SMPC_GETSTS, SMPC_NO_WAIT)


#define	    COL_256	(2+0)		/* カラーバンク２５６色モード */
#define	    COL_32K	(2-1)		/* ＲＧＢ３２Ｋ色モード */

#define	    SPR_ATTRIBUTE(t,c,g,a,d)	{t,(a)|(((d)>>24)&0xc0),c,g,(d)&0x0f3f}
#define	    FUNC_Sprite		1
#define	    CL256Bnk		(4 << 3)	/* カラーバンク２５６色モード */

#define	    SpriteVRAM		0x25c00000

#define	CGADDRESS	0x10000
#define	AdjCG(cga,hs,vs,col)		((cga) + (((((hs)*(vs)*4)>>(col))+0x1f) &0x7ffe0))
#define	TEXTBL(hs,vs,cga)		{hs , vs , (cga)>>3 , ((hs)&0x1f8)<<5|(vs)}
#define	PICTBL(texno,cmode,pcsrc)	{(Uint16)(texno),(Uint16)(cmode),(void *)(pcsrc)}

/*
#include <SGL_CPK.H>
*/

#else /* JO_COMPILE_USING_SGL */

#include <SGL.H>
#include <SL_DEF.H>
#include <SEGA_CDC.H>
#include <SEGA_GFS.H>

#endif

/*
#include <SEGA_SYS.H>
*/
void SYS_Exit(Sint32 code);

#endif /* !__JO_SGL_PROTOTYPES_H__ */

/*
** END OF FILE
*/
