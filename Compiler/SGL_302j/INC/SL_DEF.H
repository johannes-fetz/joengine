/*                                       */
/*      SEGA Library defines Ver3.00     */
/*                                       */

#ifndef SEGA_XPT_H
#define SEGA_XPT_H

#if 0
#  define ST_V				/* or ST_VE (for Enhanced ST-V) */
#  define TITAN
#endif

#define		shc		1
#define		sh_gcc		2
#define		Compiler	sh_gcc		/* fot Hitachi then "shc" */

#if Compiler == shc
#  define  STCNS  static const
#else
#  define  STCNS  static
#endif

/*****************************************************************************/
/****************************** 基本データ型宣言 *****************************/
/*****************************************************************************/

typedef unsigned char	Uint8 ;		/* 符号なし１バイト整数 */
typedef signed   char	Sint8 ;		/* 符号つき１バイト整数 */
typedef unsigned short	Uint16 ;	/* 符号なし２バイト整数 */
typedef signed   short	Sint16 ;	/* 符号つき２バイト整数 */
typedef unsigned long	Uint32 ;	/* 符号なし４バイト整数 */
typedef signed   long	Sint32 ;	/* 符号つき４バイト整数 */
typedef float		Float32 ;	/* ４バイト実数 */
typedef double		Float64 ;	/* ８バイト実数 */

typedef int		Int ;		/* INT型（ツール用）		*/

typedef int		Bool ;		/* 論理型（論理定数を値にとる）	*/

/*****************************************************************************/
/********************************* 定数マクロ ********************************/
/*****************************************************************************/

#ifndef NULL
#define NULL		((void *)0)	/* NULL */
#endif  /* NULL */

/*****************************************************************************/
/********************************** 列挙定数 *********************************/
/*****************************************************************************/

enum BooleanLogic {			/* 論理定数１（偽、真） */
    FALSE = 0 ,
    TRUE  = 1
};

enum BooleanSwitch {			/* 論理定数２（スイッチ） */
    OFF = 0 ,
    ON  = 1
};

enum Judgement {			/* 結果判定の定数（成功、失敗） */
    OK = 0 ,				/* 成功 */
    NG = -1				/* 失敗 */
};

/*****************************************************************************/
/********************************* 処理マクロ ********************************/
/*****************************************************************************/

#define MAX(x, y)	((x) > (y) ? (x) : (y))	/* 最大値 */
#define MIN(x, y)	((x) < (y) ? (x) : (y))	/* 最小値 */
#define ABS(x)		((x) < 0 ? -(x) : (x))	/* 絶対値 */

/*------------------------------------------------------------------------------------*/

#define	    DI		15		/* 割り込みの禁止 */
#define	    EI		 0		/* 割り込みの許可 */

#if 0	/* Ver2.1 より削除 */
#define	    MAX_EVENTS	64		/* 使用できるイベントの数 */
#define	    MAX_WORKS	256		/* 使用できるワークの数 */
#endif

#define	    MAX_NEST	20		/* マトリクスのネストできる数 */

#define	    WORK_SIZE	0x40		/* ワークの大きさ */
#define	    EVENT_SIZE	0x80		/* イベントの大きさ */

typedef	    Sint16	ANGLE ;		/* 一周３６０度を １６ビットで表す  */
typedef	    Sint32	FIXED ;		/* 上位１６ビットで整数部、下位１６ビットで小数部を表す固定小数点型 */

enum ps {X , Y , Z , XYZ , XYZS , XYZSS , XY = Z , S = XYZ , Sh = S , Sv = XYZS} ;

typedef struct work {
    struct work *next ;
    Uint8	user[WORK_SIZE - sizeof(struct work *)] ;
} WORK ;

typedef struct evnt {
    WORK		*work ;
    struct evnt		*next ;
    struct evnt		*before ;
    void		(*exad)() ;
    Uint8		 user[EVENT_SIZE - (sizeof(WORK *) + sizeof(struct evnt *) * 2 + sizeof(void (*)()))] ;
} EVENT ;

typedef     FIXED	MATRIX[4][3] ;		/* マトリクスデータ */
typedef     FIXED	VECTOR[XYZ] ;		/* ベクトルデータ */
typedef     FIXED	POINT[XYZ] ;		/* 頂点の位置データ */

#define	    CURRENT		0		/* Current Matrix */
#define	    ORIGINAL		(0.99999)	/* Original Scale */

enum mtrx {M00 , M01 , M02 ,		/* マトリクスの要素番号 */
	   M10 , M11 , M12 ,
	   M20 , M21 , M22 ,
	   M30 , M31 , M32 ,
	   MTRX
	   } ;

enum tvsz {
	TV_320x224 , TV_320x240 , TV_320x256 , TV_dummy1 ,
	TV_352x224 , TV_352x240 , TV_352x256 , TV_dummy2 ,
	TV_640x224 , TV_640x240 , TV_640x256 , TV_dummy3 ,
	TV_704x224 , TV_704x240 , TV_704x256 , TV_dummy4 ,

	TV_320x448 , TV_320x480 , TV_320x512 , TV_dummy5 ,
	TV_352x448 , TV_352x480 , TV_352x512 , TV_dummy6 ,
	TV_640x448 , TV_640x480 , TV_640x512 , TV_dummy7 ,
	TV_704x448 , TV_704x480 , TV_704x512 , TV_dummy8
    } ;

#define		_SysPause		64
#define		_SprPause		 2
#define		_ScrPause		 1

/*----------------------------------------------------------------------------------*/

#define     M_PI                3.1415926535897932
#define	    toFIXED(a)		((FIXED)(65536.0 * (a)))
#define	    POStoFIXED(x,y,z)	{toFIXED(x),toFIXED(y),toFIXED(z)}
#define	    ATTRIBUTE(f,s,t,c,g,a,d,o)	{f,(s)|(((d)>>16)&0x1c)|(o),t,(a)|(((d)>>24)&0xc0),c,g,(d)&0x3f}
#define	    SPR_ATTRIBUTE(t,c,g,a,d)	{t,(a)|(((d)>>24)&0xc0),c,g,(d)&0x0f3f}
#define	    DEGtoANG(d)		((ANGLE)((65536.0 * (d)) / 360.0))
#define     RADtoANG(d)         ((ANGLE)((65536.0 * (d)) / (2*M_PI)))
#define	    RGB(r,g,b)		(0x8000|((b)<<10)|((g)<<5)|(r))
#define	    DGTtoRGB(c)		(0x8000|(((c)&0x0f00)<<3)|(((c)&0x4000)>>4)|(((c)&0x00f0)<<2)|(((c)&0x2000)>>8)|(((c)&0x000f)<<1)|(((c)&0x1000)>>12))
#define	    SLocate(x,y,p)	((p) * Page + (y) * Line + (x))

/*  Sprite control function */

#define	    FUNC_Sprite		1
#define	    FUNC_Texture	2
#define	    FUNC_Polygon	4
#define	    FUNC_PolyLine	5
#define	    FUNC_Line		6
#define	    FUNC_SystemClip	9
#define	    FUNC_UserClip	8
#define	    FUNC_BasePosition	10
#define	    FUNC_End		-1

#define	    _ZmLT		(0x05 << 8)	/* Zoom base Left Top */
#define	    _ZmLC		(0x09 << 8)	/* Zoom base Left Center */
#define	    _ZmLB		(0x0d << 8)	/* Zoom base Left Bottom */
#define	    _ZmCT		(0x06 << 8)	/* Zoom base Center Top */
#define	    _ZmCC		(0x0a << 8)	/* Zoom base Center Center */
#define	    _ZmCB		(0x0e << 8)	/* Zoom base Center Bottom */
#define	    _ZmRT		(0x07 << 8)	/* Zoom base Right Top */
#define	    _ZmRC		(0x0b << 8)	/* Zoom base Right Center */
#define	    _ZmRB		(0x0f << 8)	/* Zoom base Right Bottom */

#define	    MSBon		(1 << 15)	/* フレームバッファに書き込むＭＳＢを１にする */
#define	    HSSon		(1 << 12)	/* ハイスピードシュリンク有効 */
#define	    HSSoff		(0 << 12)	/* ハイスピードシュリンク無効(default) */
#define	    No_Window		(0 << 9)	/* ウィンドウの制限を受けない(default)*/
#define	    Window_In		(2 << 9)	/* ウィンドウの内側に表示 */
#define	    Window_Out		(3 << 9)	/* ウィンドウの外側に表示 */
#define	    MESHoff		(0 << 8)	/* 通常表示(default) */
#define	    MESHon		(1 << 8)	/* メッシュで表示 */
#define	    ECdis		(1 << 7)	/* エンドコードをパレットのひとつとして使用 */
#define	    ECenb		(0 << 7)	/* エンドコード有効 */
#define	    SPdis		(1 << 6)	/* スペースコードをパレットのひとつとして使用 */
#define	    SPenb		(0 << 6)	/* スペースは表示しない(default) */
#define	    CL16Bnk		(0 << 3)	/* カラーバンク１６色モード (default) */
#define	    CL16Look		(1 << 3)	/* カラールックアップ１６色モード */
#define	    CL64Bnk		(2 << 3)	/* カラーバンク６４色モード */
#define	    CL128Bnk		(3 << 3)	/* カラーバンク１２８色モード */
#define	    CL256Bnk		(4 << 3)	/* カラーバンク２５６色モード */
#define	    CL32KRGB		(5 << 3)	/* ＲＧＢ３２Ｋ色モード */
#define	    CL_Replace		0		/* 重ね書き(上書き)モード */
#define	    CL_Shadow		1		/* 影モード */
#define	    CL_Half		2		/* 半輝度モード */
#define	    CL_Trans		3		/* 半透明モード */
#define	    CL_Gouraud		4		/* グーローシェーディングモード */

#define	    UseTexture		(1 << 2)	/* テクスチャを貼るポリゴン */
#define	    UseLight		(1 << 3)	/* 光源の影響を受けるポリゴン */
#define	    UsePalette		(1 << 5)	/* ポリゴンのカラー */

/* SGL Ver3.0 Add */
#define	    UseNearClip		(1 << 6)	/* ニア・クリッピングをする */
#define	    UseGouraud		(1 << 7)	/* リアルグーロー */
#define	    UseDepth		(1 << 4)	/* デプスキュー */
/* */

#if 0 /* SGL Ver3.0 Delete */
#define	    UseClip		(1 << 4)		/* 画面外に頂点が出たら表示しない */
#else
#define	    UseClip		UseNearClip		/* ニア・クリッピングをする */
#endif

#define	    sprHflip		((1 << 4) | FUNC_Texture | (UseTexture << PDATA16))
#define	    sprVflip		((1 << 5) | FUNC_Texture | (UseTexture << 16))
#define	    sprHVflip		((3 << 4) | FUNC_Texture | (UseTexture << 16))
#define	    sprNoflip		((0) | FUNC_Texture | (UseTexture << 16))
#define	    sprPolygon		(FUNC_Polygon | ((ECdis | SPdis) << 24))
#define	    sprPolyLine		(FUNC_PolyLine | ((ECdis | SPdis) << 24))
#define	    sprLine		(FUNC_Line | ((ECdis | SPdis) << 24))

#define	    No_Texture		0		/* テクスチャを使用しない時 */
#define	    No_Option		0		/* オプションを使用しない時 */
#define	    No_Gouraud		0		/* グーローシェーディングを使用しない時 */
#define	    No_Palet	        0		/* カラーパレットの指定がいらない時 */
#define	    GouraudRAM		(0x00080000 - (32*8))	/* 光源の影響用のグーローバッファ */

typedef struct {
    VECTOR	 norm ;			/* 法線ベクトル */
    Uint16	 Vertices[4] ;		/* ポリゴンを構成する頂点番号 */
} POLYGON ;

typedef struct {
    Uint8	 flag ;			/* 片面か両面かのフラグ */
    Uint8	 sort ;			/* ソートの基準位置とオプション設定 */
    Uint16	 texno ;		/* テクスチャ番号 */
    Uint16	 atrb ;			/* アトリビュートデータ(表示モード) */
    Uint16	 colno ;		/* カラーナンバー */
    Uint16	 gstb ;			/* グーローシェーディングテーブル */
    Uint16	 dir ;			/* テクスチャの反転とファンクションナンバー */
} ATTR ;

typedef struct {
    Uint16	 texno ;		/* テクスチャ番号 */
    Uint16	 atrb ;			/* アトリビュートデータ(表示モード) */
    Uint16	 colno ;		/* カラーナンバー */
    Uint16	 gstb ;			/* グーローシェーディングテーブル */
    Uint16	 dir ;			/* テクスチャの反転とファンクションナンバー */
} SPR_ATTR ;

typedef struct {
    POINT	*pntbl ;		/* 頂点の位置データテーブル */
    Uint32	 nbPoint ;		/* 頂点の数 */
    POLYGON	*pltbl ;		/* ポリゴン定義テーブル */
    Uint32	 nbPolygon ;		/* ポリゴンの数 */
    ATTR	*attbl ;		/* ポリゴンのアトリビュートテーブル */
} PDATA ;

/* SGL Ver3.0 Add */
typedef struct {
    POINT	*pntbl ;		/* 頂点の位置データテーブル */
    Uint32	 nbPoint ;		/* 頂点の数 */
    POLYGON	*pltbl ;		/* ポリゴン定義テーブル */
    Uint32	 nbPolygon ;		/* ポリゴンの数 */
    ATTR	*attbl ;		/* ポリゴンのアトリビュートテーブル */
    VECTOR	*vntbl;
} XPDATA ;
/* */

#ifdef ST_VE

typedef struct {
    Uint16	 pmod ;			/* スプライト表示モード */
    Uint16	 colno ;		/* カラーナンバー */
    Uint16	 CGadr ;		/* スプライトＣＧアドレス / ８ */
    Uint16	 HVsize ;		/* Ｈサイズ/８、Ｖサイズ(ハード用) */
} gxATTR_VDP1 ;

typedef struct {
    Uint32	 texcel ;		/* テクスチャセルアドレス */
    Uint16	 dspmode ;		/* ディスプレイモードフラグ */
    Uint16	 colno ;		/* カラーデータ */
    Uint8	 TXA , TYA ;		/* テクスチャポイント */
    Uint8	 TXB , TYB ;
    Uint8	 TXC , TYC ;
    Uint8	 TXD , TYD ;
} gxATTR_ENHC ;

typedef struct{
    Uint16	 texno ;		/* テクスチャ番号 */
    Uint16	 atrb ;			/* アトリビュートデータ(表示モード) */
    Uint16	 colno ;		/* カラーナンバー */
    Uint16	 gstb ;			/* グーローシェーディングテーブル */
    Uint16	 dir ;			/* テクスチャの反転とファンクションナンバー */
} gxSPR_ATTR ;

typedef struct {
    Uint16	texno ;			/* テクスチャオフセットアドレス */
    Uint8	attr ;			/* 質感コード、ソーティングモード等 */
    Uint8	nvface ;		/* 面法線ベクトルナンバー */
    Uint8	nvnum[4] ;		/* 頂点法線ベクトルナンバー */
    Uint8	Vertices[4] ;		/* ポリゴンを構成する頂点番号 */
} gxPOLYGON ;

typedef struct {
    POINT	*pntbl ;		/* 頂点の位置データテーブル */
    Uint16	 nbPoint ;		/* 頂点の数 */
    Uint16	 nbNVector ;		/* 法線ベクトルの数 */
    VECTOR	*nvtbl ;		/* 法線ベクトルデータテーブル */
    gxPOLYGON	*pltbl ;		/* ポリゴン定義テーブル */
    Uint32	 nbPolygon ;		/* ポリゴンの数 */
    gxATTR_VDP1	*attbl ;		/* ポリゴンのアトリビュートテーブル */
} gxPDATA ;

typedef struct {
    Uint32	 texadr ;		/* テクスチャデータテーブル */
    Uint8	 BD ;			/* 各頂点の輝度 */
    Uint8	 BC ;
    Uint8	 BB ;
    Uint8	 BA ;
    Sint16	 XA ;			/* 各頂点の位置 */
    Sint16	 YA ;
    Sint16	 XB ;
    Sint16	 YB ;
    Sint16	 XC ;
    Sint16	 YC ;
    Sint16	 XD ;
    Sint16	 YD ;
    Uint16	 ZA ;			/* 各頂点のＺ値 */
    Uint16	 ZB ;
    Uint16	 ZC ;
    Uint16	 ZD ;
    Uint16	 winno ;		/* ウィンドウナンバー */
    Uint16	 ZF ;			/* 代表Ｚ値 */
} gxDIRECT ;

#define		winEXn		0	/* エンハンサ側 手前(near) */
#define		winEXf		1	/* エンハンサ側 奥(far) */
#define		winSTn		2	/* ＳＴ-Ｖ側 手前(near) */
#define		winSTf		3	/* ＳＴ-Ｖ側 奥(far) */

#define	    gxPOLYTBL(f,s,t,m,n)	{((f)<<16)|((t)<<20)|((s)<<12)|((m)<<8)|(n)
#define	    gxNormal(a,b,c,d)		(a,b,c,d)
#define	    gxVertices(a,b,c,d)		(a,b,c,d)}
#define	    gxATTRVDP1(p,c,a,s)		{p,c,a,s}
#define	    gxATTRENHC(cx,cy,sx,sy,cm,cl,an,hf,ms,sp,te,fm)	{\
					((cx)<<4)|((cy)<<14)|(sx)|((sy)<<10)|(cm),\
					((ms)>>5)|(((sp)^SPdis)>>4)|((te)<<1)|\
					((an)<<8)|((hf)<<6)|(fm),(cl)
#endif

typedef struct obj{
    PDATA	*pat ;			/* ポリゴンモデルデータテーブル */
    FIXED	 pos[XYZ] ;		/* 相対位置データ */
    ANGLE	 ang[XYZ] ;		/* 回転角データ */
    FIXED	 scl[XYZ] ;		/* スケーリングデータ */
    struct obj	*child ;		/* 子オブジェクトデータ */
    struct obj	*sibling ;		/* 兄弟オブジェクトデータ */
} OBJECT ;

typedef struct {
    Uint16	 Hsize ;		/* スプライトのＨサイズ */
    Uint16	 Vsize ;		/* スプライトのＶサイズ */
    Uint16	 CGadr ;		/* スプライトＣＧアドレス / ８ */
    Uint16	 HVsize ;		/* Ｈサイズ/８、Ｖサイズ(ハード用) */
} TEXTURE ;

typedef struct {
    Uint16	 texno ;		/* テクスチャ番号 */
    Uint16	 cmode ;		/* カラーモード */
    void	*pcsrc ;		/* 元データの所在 */
} PICTURE ;

#define	    COL_16	(2+1)		/* カラーバンク１６色モード */
#define	    COL_64	(2+0)		/* カラーバンク６４色モード */
#define	    COL_128	(2+0)		/* カラーバンク１２８色モード */
#define	    COL_256	(2+0)		/* カラーバンク２５６色モード */
#define	    COL_32K	(2-1)		/* ＲＧＢ３２Ｋ色モード */

    enum base {
	SORT_BFR ,			/* 直前に表示したポリゴンの位置を使う */
	SORT_MIN ,			/* ４点の内、一番手前の点を使う */
	SORT_MAX ,			/* ４点の内一番遠い点を使う */
	SORT_CEN			/* ４点の平均位置を使う */
    } ;

    enum pln {
	Single_Plane ,			/* 片面ポリゴン */
	Dual_Plane			/* 両面ポリゴン(表裏判定の結果を見ない) */
    } ;

typedef struct {
    Uint16	CTRL ;			/* コントロールファンクション */
    Uint16	LINK ;			/* リンクアドレス */
    Uint16	PMOD ;			/* プットモード */
    Uint16	COLR ;			/* カラーデータ */
    Uint16	SRCA ;			/* ＣＧアドレス */
    Uint16	SIZE ;			/* キャラクタサイズ */
    Sint16	  XA ;			/* 表示位置 Ａ */
    Sint16	  YA ;
    Sint16	  XB ;			/* 表示位置 Ｂ */
    Sint16	  YB ;
    Sint16	  XC ;			/* 表示位置 Ｃ */
    Sint16	  YC ;
    Sint16	  XD ;			/* 表示位置 Ｄ */
    Sint16	  YD ;
    Uint16	GRDA ;			/* グーローシェーディングテーブル */
    Uint16	DMMY ;			/* サイズ合わせ用 ダミーデータ */
} SPRITE ;

#define	    SpriteVRAM		0x25c00000

/*-----------------------------------------------------------------------*/

/* ＲＧＢモードカラーサンプルデータ */

#define	    RGB_Flag		0x8000

#define	    CD_Black		( 0 << 10) | ( 0 << 5) | (0)  | RGB_Flag
#define	    CD_DarkRed		( 0 << 10) | ( 0 << 5) | (8)  | RGB_Flag
#define	    CD_DarkGreen	( 0 << 10) | ( 8 << 5) | (0)  | RGB_Flag
#define	    CD_DarkYellow	( 0 << 10) | ( 8 << 5) | (8)  | RGB_Flag
#define	    CD_DarkBlue		( 8 << 10) | ( 0 << 5) | (0)  | RGB_Flag
#define	    CD_DarkPurple	( 8 << 10) | ( 0 << 5) | (8)  | RGB_Flag
#define	    CD_DarkMagenta	( 8 << 10) | ( 8 << 5) | (0)  | RGB_Flag
#define	    CD_DarkWhite	( 8 << 10) | ( 8 << 5) | (8)  | RGB_Flag
#define	    CD_MediumRed	( 0 << 10) | ( 0 << 5) | (16) | RGB_Flag
#define	    CD_MediumGreen	( 0 << 10) | (16 << 5) | (0)  | RGB_Flag
#define	    CD_MediumYellow	( 0 << 10) | (16 << 5) | (16) | RGB_Flag
#define	    CD_MediumBlue	(16 << 10) | ( 0 << 5) | (0)  | RGB_Flag
#define	    CD_MediumPurple	(16 << 10) | ( 0 << 5) | (16) | RGB_Flag
#define	    CD_MediumMagenta	(16 << 10) | (16 << 5) | (0)  | RGB_Flag
#define	    CD_MediumWhite	(16 << 10) | (16 << 5) | (16) | RGB_Flag
#define	    CD_SemiRed		( 0 << 10) | ( 0 << 5) | (24) | RGB_Flag
#define	    CD_SemiGreen	( 0 << 10) | (24 << 5) | (0)  | RGB_Flag
#define	    CD_SemiYellow	( 0 << 10) | (24 << 5) | (24) | RGB_Flag
#define	    CD_SemiBlue		(24 << 10) | ( 0 << 5) | (0)  | RGB_Flag
#define	    CD_SemiPurple	(24 << 10) | ( 0 << 5) | (24) | RGB_Flag
#define	    CD_SemiMagenta	(24 << 10) | (24 << 5) | (0)  | RGB_Flag
#define	    CD_SemiWhite	(24 << 10) | (24 << 5) | (24) | RGB_Flag
#define	    CD_Red		( 0 << 10) | ( 0 << 5) | (31) | RGB_Flag
#define	    CD_Green		( 0 << 10) | (31 << 5) | (0)  | RGB_Flag
#define	    CD_Yellow		( 0 << 10) | (31 << 5) | (31) | RGB_Flag
#define	    CD_Blue		(31 << 10) | ( 0 << 5) | (0)  | RGB_Flag
#define	    CD_Purple		(31 << 10) | ( 0 << 5) | (31) | RGB_Flag
#define	    CD_Magenta		(31 << 10) | (31 << 5) | (0)  | RGB_Flag
#define	    CD_White		(31 << 10) | (31 << 5) | (31) | RGB_Flag

/*------[ Scroll control ]------------------------------------------------*/

/*	Scroll V-RAM map address	*/

#define	    VDP2_VRAM_A0	0x25e00000
#define	    VDP2_VRAM_A1	0x25e20000
#define	    VDP2_VRAM_B0	0x25e40000
#define	    VDP2_VRAM_B1	0x25e60000

#define	    CGN01_RAM		VDP2_VRAM_B1		/* スクロール０、１用ＣＧデータ */
#define	    NBG0_MAP		(CGN01_RAM+0x16000)	/* スクロール０のＶＲＡＭアドレス */
#define	    NBG1_MAP		(CGN01_RAM+0x18000)	/* スクロール１のＶＲＡＭアドレス */

#define	    KTBL0_RAM		VDP2_VRAM_A1		/* 回転スクロール用係数テーブル */
#define	    BACK_CRAM		(KTBL0_RAM + 0x1fffe)	/* Back color */
#define     RBG_PARA_ADR	(KTBL0_RAM + 0x1ff00)
#define     RBG_PARB_ADR	(RBG_PARA_ADR+ 0x80)

#define	    CGR0_RAM		VDP2_VRAM_A0		/* 回転スクロール用ＣＧデータ*/
#define	    RBG0_MAP		VDP2_VRAM_B0		/* 回転スクロール０のＶＲＡＭアドレス */
#define	    VDP2_COLRAM    		0x25f00000

#define	    CG_Size		(8*8)		/* １セルの大きさ(CG) */
#define     CG16Size            (8*8/2)         /* １セルの大きさ(CG 16Colors) */
#define     CG256Size           (8*8)           /* １セルの大きさ(CG 256Colors) */
#define     CG2048Size          (8*8*2)         /* １セルの大きさ(CG 2048Colors) */
#define     CG32KSize           (8*8*2)         /* １セルの大きさ(CG 32KColors) */

typedef     Uint16		CELL ;		/* １セルの大きさ(Pattern name) */
#define	    Line		64		/* １ラインの大きさ */
#define	    Page		(64*Line)	/* １ページの大きさ */

typedef struct rdat{
    FIXED	XST ;		/* スクリーン画面スタート座標 Ｘst */
    FIXED	YST ;		/* スクリーン画面スタート座標 Ｙst */
    FIXED	ZST ;		/* スクリーン画面スタート座標 Ｚst */
    FIXED	DXST ;		/* スクリーン画面垂直方向座標増分 ｄＸst */
    FIXED	DYST ;		/* スクリーン画面垂直方向座標増分 ｄＹst */
    FIXED	DX ;		/* スクリーン画面水平方向座標増分 ｄＸ */
    FIXED	DY ;		/* スクリーン画面水平方向座標増分 ｄＹ */
    FIXED	MATA ;		/* 回転マトリクスパラメータＡ */
    FIXED	MATB ;		/* 回転マトリクスパラメータＢ */
    FIXED	MATC ;		/* 回転マトリクスパラメータＣ */
    FIXED	MATD ;		/* 回転マトリクスパラメータＤ */
    FIXED	MATE ;		/* 回転マトリクスパラメータＥ */
    FIXED	MATF ;		/* 回転マトリクスパラメータＦ */

    Sint16	PX ;		/* 視点座標 Ｐx */
    Sint16	PY ;		/* 視点座標 Ｐy */
    Sint16	PZ ;		/* 視点座標 Ｐz */
    Sint16	dummy0 ;
    Sint16	CX ;		/* 中心座標 Ｃx */
    Sint16	CY ;		/* 中心座標 Ｃy */
    Sint16	CZ ;		/* 中心座標 Ｃz */
    Sint16	dummy1 ;

    FIXED	MX ;		/* 平行移動量 Ｍx */
    FIXED	MY ;		/* 平行移動量 Ｍy */
    FIXED	KX ;		/* 拡大縮小係数 kx */
    FIXED	KY ;		/* 拡大縮小係数 ky */

    Uint32	KAST ;		/* 係数テーブルスタートアドレス ＫＡst */
    Sint32	DKAST ;		/* 係数テーブル垂直方向アドレス増分 ＫＡst */
    Sint32	DKA ;		/* 係数テーブル水平方向アドレス増分 ＫＡ */
} ROTSCROLL ;

/* slExtSignal */
#define		EXLTEN			0x0200		/* External latch enable */
#define		EXSYEN			0x0100		/* External synch enable */
#define		DASEL			0x0002		/* Display area select */
#define		EXBGEN			0x0001		/* External background screen enable */

/* slColRAMMode */
#define		CRM16_1024		0
#define		CRM16_2048		1
#define		CRM32_1024		2
/* slChar */
#define		COL_TYPE_16		0x00
#define		COL_TYPE_256		0x10
#define		COL_TYPE_2048		0x20
#define		COL_TYPE_32768		0x30
#define		COL_TYPE_1M		0x40
#define		CHAR_SIZE_1x1		0
#define		CHAR_SIZE_2x2		1
/* slPage */
#define		PNB_2WORD		0
#define		PNB_1WORD		0x8000
#define		CN_10BIT       		0
#define		CN_12BIT		0x4000
/* slPlane */
#define		PL_SIZE_1x1		0
#define		PL_SIZE_2x1		1
#define		PL_SIZE_2x2		3
/* slScrDisp */
#define		NBG0ON			(1<<0)
#define		NBG1ON			(1<<1)
#define		NBG2ON			(1<<2)
#define		NBG3ON			(1<<3)
#define		RBG0ON			(1<<4)
#define		LNCLON			(1<<5)
#define		SPRON			(1<<6)
#define		BACKON			(1<<5)
#define		NBG0OFF			(1<<16)
#define		NBG1OFF			(1<<17)
#define		NBG2OFF			(1<<18)
#define		NBG3OFF			(1<<19)
#define		RBG0OFF			(1<<20)
/* slZoomMode */
#define		ZOOM_1			0
#define		ZOOM_HALF	       	1
#define		ZOOM_QUARTER	       	2
/* slKtable */
#define		K_OFF		       	0
#define		K_ON		       	1
#define		K_2WORD		       	0
#define		K_1WORD		       	2
#define		K_MODE0		       	0
#define		K_MODE1			4
#define		K_MODE2			8
#define		K_MODE3			12
#define		K_LINECOL		0x10
#define		K_DOT			0x20
#define		K_LINE			0
#define		K_FIX			0x40
/* slCurRpara */
/* slRparaMode */
#define		RA			0
#define		RB			1
#define		K_CHANGE       		2
#define		W_CHANGE       		3

#define		LnRd_Xst		0x01
#define		LnRd_Yst		0x02
#define		LnRd_KAst		0x04

/* slColCalc */
#define		CC_RATE		       	0
#define		CC_ADD		       	0x100
#define		CC_TOP		       	0
#define		CC_2ND		       	0x200
#define		CC_EXT		       	0x400

/* Gradation control */
#define		grdNBG0			0xa000
#define		grdNBG1			0xc000
#define		grdNBG2			0xd000
#define		grdNBG3			0xe000
#define		grdRBG0			0x9000
#define		grdSPR			0x8000

/* color */
#define		COL_R			(1<<0)
#define		COL_G			(1<<5)
#define		COL_B			(1<<10)
/* SpriteColorCalc */
#define		CC_pr_CN		0	/* Priority <= CCalcNum */
#define		CC_PR_CN		1	/* Priority == CCalcNum */
#define		CC_PR_cn		2	/* Priority >= CCalcNum */
#define		CC_MSB			3	/* Use MSB */

#define		SPR_PAL			0	/* Palette code only */
#define		SPR_PAL_RGB		1	/* Use Palette and RGB */

    enum color_rate{
	CLRate31_1 ,		/* Top 31 : Second 1 */
	CLRate30_2 ,
	CLRate29_3 ,
	CLRate28_4 ,
	CLRate27_5 ,
	CLRate26_6 ,
	CLRate25_7 ,
	CLRate24_8 ,
	CLRate23_9 ,
	CLRate22_10 ,
	CLRate21_11 ,
	CLRate20_12 ,
	CLRate19_13 ,
	CLRate18_14 ,
	CLRate17_15 ,
	CLRate16_16 ,
	CLRate15_17 ,
	CLRate14_18 ,
	CLRate13_19 ,
	CLRate12_20 ,
	CLRate11_21 ,
	CLRate10_22 ,
	CLRate9_23 ,
	CLRate8_24 ,
	CLRate7_25 ,
	CLRate6_26 ,
	CLRate5_27 ,
	CLRate4_28 ,
	CLRate3_29 ,
	CLRate2_30 ,
	CLRate1_31 ,
	CLRate0_32		/* Top 0 : Second 32 */
    } ;

/* Screen select */
#define		scnNBG0			1
#define		scnNBG1			0
#define		scnNBG2			3
#define		scnNBG3			2
#define		scnRBG0			5
#define		scnRBGA			5
#define		scnRBGB			4
#define		scnSPR			4
#define		scnROT			7
#define		scnCCAL			6

#define		scnLNCL			7
#define		scnBACK			6

#define		scnSPR0			-7
#define		scnSPR1			-8
#define		scnSPR2			-5
#define		scnSPR3			-6
#define		scnSPR4			-3
#define		scnSPR5			-4
#define		scnSPR6			-1
#define		scnSPR7			-2

#define		bmNBG0			1
#define		bmNBG1			0
#define		bmRBG0			3

/* Window control */
#define		win_OR			0x80
#define		win_AND			0x00

#define		win0_IN			0x03
#define		win0_OUT		0x02

#define		win1_IN			0x0c
#define		win1_OUT		0x08

#define		spw_IN			0x30
#define		spw_OUT			0x20

/* Line Scroll control */
#define		lineSZ1			0x00
#define		lineSZ2	 		0x10
#define		lineSZ4			0x20
#define		lineSZ8			0x30

#define		lineZoom		0x08
#define		lineVScroll		0x04
#define		lineHScroll		0x02
#define		VCellScroll		0x01

/* BitMap control */
#define		BM_512x256		0x02
#define		BM_512x512		0x06
#define		BM_1024x256		0x0a
#define		BM_1024x512		0x0e

/* Special Function Control */
#define		spPRI_Scrn		0
#define		spPRI_Char		1
#define		spPRI_Dot		2

#define		spCCAL_Scrn		0
#define		spCCAL_Char		1
#define		spCCAL_Dot		2
#define		spCCAL_MSB		3

#define		sfCOL_01		0x01
#define		sfCOL_23		0x02
#define		sfCOL_45		0x04
#define		sfCOL_67		0x08
#define		sfCOL_89		0x10
#define		sfCOL_ab		0x20
#define		sfCOL_cd		0x40
#define		sfCOL_ef		0x80

/* V-RAM Division */
#define		Div_A			1
#define		Div_B			2

/*------[ D.M.A. control ]------------------------------------------------*/
#define		Sinc_Dinc_Byte		0
#define		Sdec_Dinc_Byte		1
#define		Sinc_Ddec_Byte		2
#define		Sfix_Dinc_Byte		3
#define		Sfix_Ddec_Byte		4
#define		Sinc_Dfix_Byte		5
#define		Sdec_Dfix_Byte		6

#define		Sinc_Dinc_Word		7
#define		Sdec_Dinc_Word		8
#define		Sinc_Ddec_Word		9
#define		Sfix_Dinc_Word		10
#define		Sfix_Ddec_Word		11
#define		Sinc_Dfix_Word		12
#define		Sdec_Dfix_Word		13

#define		Sinc_Dinc_Long		14
#define		Sdec_Dinc_Long		15
#define		Sinc_Ddec_Long		16
#define		Sfix_Dinc_Long		17
#define		Sfix_Ddec_Long		18
#define		Sinc_Dfix_Long		19
#define		Sdec_Dfix_Long		20

/*------[ Sound control ]-------------------------------------------------*/

typedef struct{
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

#define		_Stereo		0x80
#define		_Mono		0x00

#define		_PCM16Bit	0x00
#define		_PCM8Bit	0x10

#define		SND_SEQ_START		0x01	/* Sequence Start */
#define		SND_SEQ_STOP		0x02	/* Sequence Stop */
#define		SND_SEQ_PAUSE		0x03	/* Sequence Pause */
#define		SND_SEQ_CONT		0x04	/* Sequence Continue */
#define		SND_SEQ_VOL		0x05	/* Sequence Volume */
#define		SND_TEMPO_CHG		0x07	/* Tempo Change */
#define		SND_MAP_CHG		0x08	/* map Change */
#define		SND_MIDI_CTRL_DIR	0x09	/* MIDI direct control */
#define		SND_VOL_ANL_START	0x0a	/* Volume analize start */
#define		SND_VOL_ANL_STOP	0x0b	/* Volume analize stop */
#define		SND_DSP_STOP		0x0c	/* DSP stop */
#define		SND_ALL_OFF		0x0d	/* Sound all OFF */
#define		SND_SEQ_PAN		0x0e	/* Sequence PAN */
#define		SND_CDDA_LEV		0x80	/* CD-DA Level */
#define		SND_CDDA_PAN		0x81	/* CD-DA pan */
#define		SND_TTL_VOL		0x82	/* Total Volume */
#define		SND_EFCT_CHG		0x83	/* Effect Change */
#define		SND_PCM_START		0x85	/* PCM start */
#define		SND_PCM_STOP		0x86	/* PCM stop */
#define		SND_MIX_CHG		0x87	/* Mixer change */
#define		SND_MIX_PRM_CHG		0x88	/* Mixer parameter change */
#define		SND_HARD_CHK		0x89	/* Hard check */
#define		SND_PCM_PRM_CHG		0x8a	/* PCM parameter change */

#define		SoundRAM		0x25a00000

/*------[ System Work Assignments ]---------------------------------------*/

    extern const void*   MasterStack ;	/* Stack area End */
    extern const Uint16  MaxVertices ;	/* Calculatable vertice numbers */
    extern const Uint16  MaxPolygons ;	/* Calculatable polygon numbers */
    extern const Uint16  EventSize ;	/* sizeof(EVENT) */
    extern const Uint16  WorkSize ;	/* sizeof(WORK) */
    extern const Uint16  MaxEvents ;	/* Useful Event count */
    extern const Uint16  MaxWorks ;	/* Useful Work count */
    extern const void*   SortList ;	/* SCU D.M.A. Table */
    extern const Uint32  SortListSize ;	/* Sprite transfer request table size */
    extern const void*   TransList ;	/* SCU D.M.A. Table */
    extern const void*   Zbuffer ;	/* (128 + 128 + 256) * 4 Bytes fix */
    extern const void*   SpriteBuf ;	/* Sprite control data buffer */
    extern const Uint32  SpriteBufSize ; /* Sprite control data buffer size */
    extern const void*   Pbuffer ;	/* Point calculate buffer */
    extern const void*   CLOfstBuf ;	/* 32 * 32 Bytes fix */
    extern const void*   CommandBuf ;	/* Command for slave CPU buffer */
    extern const void*   PCM_Work ;	/* PCM data buffer */
    extern const Uint32  PCM_WkSize ;	/* PCM data buffer size */

    extern EVENT  EventBuf[] ;		/* buffer for Event use */
    extern WORK   WorkBuf[] ;		/* buffer for Work use */
    extern EVENT* RemainEvent[] ;	/* Remain Event address buffer */
    extern WORK*  RemainWork[] ;	/* Remain Work address buffer */

/*------[ Prottype ]------------------------------------------------------*/

/*  Math library	*/

    extern  FIXED  slSin(ANGLE) ;
    extern  FIXED  slCos(ANGLE) ;
    extern  FIXED  slTan(ANGLE) ;
    extern  ANGLE  slAtan(FIXED , FIXED) ;
    extern  Uint32 slSquart(Uint32) ;
    extern  FIXED  slSquartFX(FIXED) ;
    extern  FIXED  slMulFX(FIXED , FIXED) ;
    extern  FIXED  slDivFX(FIXED , FIXED) ;
    extern  FIXED  slInnerProduct(VECTOR vct1 , VECTOR vct2) ;
    extern  FIXED  slCalcPoint(FIXED , FIXED , FIXED , FIXED ans[XYZ]) ;
    extern  FIXED  slCalcVector(VECTOR vct , VECTOR ans) ;
    extern  FIXED  slRandom(void) ;
    extern  void   slNormalVector(VECTOR , VECTOR , VECTOR , VECTOR) ;
    extern  void   slRevNormalVector(VECTOR , VECTOR , VECTOR , VECTOR) ;

    extern  void   slInitMatrix(void) ;
    extern  void   slUnitMatrix(MATRIX) ;
    extern  Bool   slPushMatrix(void) ;
    extern  Bool   slPopMatrix(void) ;
    extern  Bool   slPushUnitMatrix(void) ;
    extern  Bool   slIncMatrixPtr(void) ;
    extern  Bool   slDecMatrixPtr(void) ;
    extern  void   slLoadMatrix(MATRIX) ;
    extern  void   slGetMatrix(MATRIX) ;
    extern  Bool   slCopyMatrix() ;
    extern  void   slGetTranslate(FIXED*) ;
    extern  void   slLoadTranslate(FIXED , FIXED , FIXED) ;
    extern  void   slRegistMatrix(MATRIX) ;
    extern  void   slInversMatrix(void) ;
    extern  void   slTransposeMatrix(void) ;
    extern  void   slUnitAngle(MATRIX) ;
    extern  void   slUnitTranslate(MATRIX) ;

    extern  void   slTranslate(FIXED , FIXED , FIXED) ;
    extern  void   slRegistTranslate(FIXED , FIXED , FIXED) ;
    extern  void   slMultiMatrix(MATRIX) ;
    extern  void   slScale(FIXED , FIXED , FIXED) ;
    extern  void   slRotX(ANGLE) ;
    extern  void   slRotY(ANGLE) ;
    extern  void   slRotZ(ANGLE) ;
    extern  void   slRotXSC(FIXED , FIXED) ;
    extern  void   slRotYSC(FIXED , FIXED) ;
    extern  void   slRotZSC(FIXED , FIXED) ;
    extern  void   slRotAX(FIXED , FIXED , FIXED , ANGLE) ;
    extern  void   slLookAt(FIXED *camera , FIXED *target , ANGLE) ;
    extern  FIXED  slBallCollision(FIXED *pos1 , FIXED size1 , FIXED *pos2 , FIXED size2) ;
    extern  FIXED  slCheckOnScreen(FIXED *pos , FIXED size) ;
    extern  FIXED  slCheckOnScreen0(FIXED size) ;
    extern  void   slBezier(VECTOR p1,VECTOR p2,VECTOR p3,VECTOR p4,FIXED time,VECTOR ans) ;
    extern  FIXED  slConvert3Dto2D( FIXED pos[XYZ], Sint32 ans[XY]) ;
    extern  FIXED  slConvert3Dto2DFX( FIXED pos[XYZ], FIXED ans[XY]) ;

/*  Text display library	*/

    extern void   *slLocate(Uint16 , Uint16) ;
    extern void    slCurScrn(Uint16) ;
    extern void    slCurColor(Uint16) ;
    extern void    slDispHex(Uint32 , void *) ;
    extern void    slPrintHex(Uint32 , void *) ;
    extern void    slPrintMatrix(MATRIX , void *) ;
    extern void    slPrintFX(FIXED , void *) ;
    extern Uint32  slHex2Dec(Uint32) ;
    extern Uint32  slDec2Hex(Uint32) ;
    extern Uint16  slAng2Hex(ANGLE) ;
    extern Uint16  slAng2Dec(ANGLE) ;
    extern FIXED   slAng2FX(ANGLE) ;
    extern void    slPrint(char * , void *) ;
    extern void    set_vbar(Uint16) ;
    extern void    reset_vbar(Uint16) ;
    extern void    slExtendScroll(Uint8 * , void *) ;
    extern void   *slLocateNbg(Uint16 , Uint16 , Uint16) ;
#define		slLocateNbg0(x,y)	slLocateNbg(x,y,scnNBG0)
#define		slLocateNbg1(x,y)	slLocateNbg(x,y,scnNBG1)
#define		slLocateNbg2(x,y)	slLocateNbg(x,y,scnNBG2)
#define		slLocateNbg3(x,y)	slLocateNbg(x,y,scnNBG3)
    extern void   *slLocatePage(Uint16 , Uint16 , Uint16) ;

/*  Sprite library	*/

    extern  Bool   slPutPolygon(PDATA *) ;
    extern  Bool   slPutPolygonS(PDATA *) ;
    extern  Bool   slDispPolygon(PDATA * , Uint16) ;
    extern  Bool   slPutSprite(FIXED * , SPR_ATTR *attr , ANGLE) ;
    extern  Bool   slDispSprite(FIXED * , SPR_ATTR *attr , ANGLE) ;
    extern  Bool   slDispSpriteHV(FIXED * , SPR_ATTR *attr , ANGLE) ;
    extern  Bool   slDispSpriteSZ(FIXED * , SPR_ATTR *attr , ANGLE) ;
    extern  Bool   slDispSprite4P(FIXED * , FIXED , SPR_ATTR *attr) ;
    extern  void   slPerspective(ANGLE) ;
    extern  void   slSetScreenDist(FIXED) ;
    extern  Bool   slWindow(Sint16 , Sint16 , Sint16 , Sint16 , Sint16 , Sint16 , Sint16) ;
    extern  void   slLight(VECTOR) ;
    extern  Bool   slSetSprite(SPRITE * , FIXED) ;
    extern  void   slZdspLevel(Uint16) ;
    extern  Bool   slPutObject(OBJECT *) ;
    extern  void   slCurWindow(Uint8) ;
    enum {
	winFar ,		/* Far side Window */
	winNear			/* Near side Window */
    } ;
    extern  Uint8  slSprWinNum() ;
    extern  void   slFrameClipSize(Sint16 , Sint16 );
    extern  void   slSetScreenDist(FIXED);
    extern  void   slGetFrameData(void * , Uint16 , Uint16 );

/*  Scroll library	*/

    extern  void    slTVOn(void) ;
    extern  void    slTVOff(void) ;
    extern  Uint16  slGetHCount(void) ;
    extern  Uint16  slGetVCount(void) ;
    extern  void    slExtSignal(Uint16 , Uint16) ;
    extern  void    slColRAMMode(Uint16) ;
    extern  void    slVRAMMode(Uint16) ;
    extern  void    slScrCycleSet(Uint32 , Uint32 , Uint32 , Uint32) ;
    extern  void    slScrDisp(Uint32) ;
    extern  Bool    slScrAutoDisp(Uint32) ;
    extern  void    slScrTransparent(Uint16) ;
    extern  void    slZrotR(ANGLE) ;
    extern  void    slScrMatSet(void) ;
    extern  void    slBackColTable(void *) ;
    extern  void    slBack1ColSet(void * , Uint16) ;
    extern  void    slLineColTable(void *) ;
    extern  void    slLine1ColSet(void * , Uint16) ;
    extern  void    slLineColDisp(Uint16) ;
    extern  void    slColorCalc(Uint16) ;
    extern  void    slColorCalcMode(Uint16) ;
    extern  void    slColorCalcOn(Uint16) ;
    extern  void    slColRate(Sint16 , Uint16) ;
#define		slColRateNbg0(rate)	slColRate(scnNBG0 , rate)
#define		slColRateNbg1(rate)	slColRate(scnNBG1 , rate)
#define		slColRateNbg2(rate)	slColRate(scnNBG2 , rate)
#define		slColRateNbg3(rate)	slColRate(scnNBG3 , rate)
#define		slColRateRbg0(rate)	slColRate(scnRBG0 , rate)
#define		slColRateLNCL(rate)	slColRate(scnLNCL , rate)
#define		slColRateBACK(rate)	slColRate(scnBACK , rate)
#define		slColRateSpr0(rate)	slColRate(scnSPR0 , rate)
#define		slColRateSpr1(rate)	slColRate(scnSPR1 , rate)
#define		slColRateSpr2(rate)	slColRate(scnSPR2 , rate)
#define		slColRateSpr3(rate)	slColRate(scnSPR3 , rate)
#define		slColRateSpr4(rate)	slColRate(scnSPR4 , rate)
#define		slColRateSpr5(rate)	slColRate(scnSPR5 , rate)
#define		slColRateSpr6(rate)	slColRate(scnSPR6 , rate)
#define		slColRateSpr7(rate)	slColRate(scnSPR7 , rate)
    extern  void    slColOffsetOn(Uint16) ;
    extern  void    slColOffsetOff(Uint16) ;
    extern  void    slColOffsetAUse(Uint16) ;
    extern  void    slColOffsetBUse(Uint16) ;
    extern  void    slColOffsetScrn(Uint16 , Uint16) ;
    extern  void    slColOffsetA(Sint16 r,Sint16 g,Sint16 b) ;
    extern  void    slColOffsetB(Sint16 r,Sint16 g,Sint16 b) ;
    extern  void    slCharNbg0(Uint16 type,Uint16 size) ;
    extern  void    slCharNbg1(Uint16 type,Uint16 size) ;
    extern  void    slCharNbg2(Uint16 type,Uint16 size) ;
    extern  void    slCharNbg3(Uint16 type,Uint16 size) ;
    extern  void    slCharRbg0(Uint16 type,Uint16 size) ;
    extern  void    slPageNbg0(void *,void *,Uint16) ;
    extern  void    slPageNbg1(void *,void *,Uint16) ;
    extern  void    slPageNbg2(void *,void *,Uint16) ;
    extern  void    slPageNbg3(void *,void *,Uint16) ;
    extern  void    slPageRbg0(void *,void *,Uint16) ;
    extern  void    slPlaneNbg0(Uint16) ;
    extern  void    slPlaneNbg1(Uint16) ;
    extern  void    slPlaneNbg2(Uint16) ;
    extern  void    slPlaneNbg3(Uint16) ;
    extern  void    slPlaneRA(Uint16) ;
    extern  void    slPlaneRB(Uint16) ;
    extern  void    slOverRA(Uint16) ;
    extern  void    slOverRB(Uint16) ;
    extern  void    slMapNbg0(void *,void *,void *,void *) ;
    extern  void    slMapNbg1(void *,void *,void *,void *) ;
    extern  void    slMapNbg2(void *,void *,void *,void *) ;
    extern  void    slMapNbg3(void *,void *,void *,void *) ;
    extern  void    sl1MapRA(void *) ;
    extern  void    sl1MapRB(void *) ;
    extern  void    sl16MapRA(Uint8 *) ;
    extern  void    sl16MapRB(Uint8 *) ;
    extern  void    slRparaInitSet(ROTSCROLL *) ;
    extern  void    slCurRpara(Uint16) ;
    extern  void    slRparaMode(Uint16) ;
    extern  void    slMakeKtable(void *) ;
    extern  void    slKtableRA(void *,Uint16) ;
    extern  void    slKtableRB(void *,Uint16) ;
    extern  void    slPriority(Sint16 , Uint16) ;
#define		slPriorityNbg0(num)	slPriority(scnNBG0 , num)
#define		slPriorityNbg1(num)	slPriority(scnNBG1 , num)
#define		slPriorityNbg2(num)	slPriority(scnNBG2 , num)
#define		slPriorityNbg3(num)	slPriority(scnNBG3 , num)
#define		slPriorityRbg0(num)	slPriority(scnRBG0 , num)
#define		slPrioritySpr0(num)	slPriority(scnSPR0 , num)
#define		slPrioritySpr1(num)	slPriority(scnSPR1 , num)
#define		slPrioritySpr2(num)	slPriority(scnSPR2 , num)
#define		slPrioritySpr3(num)	slPriority(scnSPR3 , num)
#define		slPrioritySpr4(num)	slPriority(scnSPR4 , num)
#define		slPrioritySpr5(num)	slPriority(scnSPR5 , num)
#define		slPrioritySpr6(num)	slPriority(scnSPR6 , num)
#define		slPrioritySpr7(num)	slPriority(scnSPR7 , num)
    extern  void    slScrPosNbg0(FIXED x,FIXED y) ;
    extern  void    slScrPosNbg1(FIXED x,FIXED y) ;
    extern  void    slScrPosNbg2(FIXED x,FIXED y) ;
    extern  void    slScrPosNbg3(FIXED x,FIXED y) ;
    extern  void    slScrMoveNbg0(FIXED x,FIXED y) ;
    extern  void    slScrMoveNbg1(FIXED x,FIXED y) ;
    extern  void    slScrMoveNbg2(FIXED x,FIXED y) ;
    extern  void    slScrMoveNbg3(FIXED x,FIXED y) ;
    extern  void    slLookR(FIXED x,FIXED y) ;
    extern  void    slDispCenterR(FIXED x,FIXED y) ;
    extern  void    slZoomNbg0(FIXED x,FIXED y) ;
    extern  void    slZoomNbg1(FIXED x,FIXED y) ;
    extern  void    slZoomR(FIXED x,FIXED y) ;
    extern  void    slScrScaleNbg0(FIXED x,FIXED y) ;
    extern  void    slScrScaleNbg1(FIXED x,FIXED y) ;
    extern  void    slScrScaleR(FIXED x,FIXED y) ;
    extern  void    slZoomMode(Uint16 , Uint16) ;
#define		slZoomModeNbg0(mode)	slZoomMode(scnNBG0 , mode)
#define		slZoomModeNbg1(mode)	slZoomMode(scnNBG1 , mode)
    extern  void    slScrAscSet(void *) ;
    extern  void    slScrMatConv(void) ;
    extern  void    slSpriteCCalcCond(Uint16) ;
    extern  void    slSpriteCCalcNum(Uint16) ;
    extern  void    slSpriteColMode(Uint16) ;
    extern  void    slSpriteWinMode(Uint16) ;
    extern  void    slSpriteType(Uint16) ;
    extern  void    slScrMosaicOn(Uint16) ;
    extern  void    slScrMosSize(Uint16 , Uint16) ;
    extern  void    slScrWindow0(Uint16 , Uint16 , Uint16 , Uint16) ;
    extern  void    slScrWindow1(Uint16 , Uint16 , Uint16 , Uint16) ;
    extern  void    slScrLineWindow0(void *) ;
    extern  void    slScrLineWindow1(void *) ;
    extern  void    slScrWindowMode(Uint16 , Uint16) ;
#define		slScrWindowModeNbg0(mode)	slScrWindowMode(scnNBG0 , mode)
#define		slScrWindowModeNbg1(mode)	slScrWindowMode(scnNBG1 , mode)
#define		slScrWindowModeNbg2(mode)	slScrWindowMode(scnNBG2 , mode)
#define		slScrWindowModeNbg3(mode)	slScrWindowMode(scnNBG3 , mode)
#define		slScrWindowModeRbg0(mode)	slScrWindowMode(scnRBG0 , mode)
#define		slScrWindowModeSPR(mode)	slScrWindowMode(scnSPR  , mode)
#define		slScrWindowModeROT(mode)	slScrWindowMode(scnROT  , mode)
#define		slScrWindowModeCCAL(mode)	slScrWindowMode(scnCCAL , mode)
    extern  void    slLineScrollMode(Uint16 , Uint16) ;
#define		slLineScrollModeNbg0(mode)	slLineScrollMode(scnNBG0 , mode)
#define		slLineScrollModeNbg1(mode)	slLineScrollMode(scnNBG1 , mode)
    extern  void    slLineScrollTable0(void *) ;
    extern  void    slLineScrollTable1(void *) ;
    extern  void    slVCellTable(void *) ;
    extern  void    slGradationOn(Uint16) ;
#define		slGradationOff()	slGradationOn(OFF)
    extern  void    slShadowOn(Uint16) ;
    extern  void    slTpShadowMode(Uint16) ;
    extern  void    slOverPatRA(Uint16 pat) ;
    extern  void    slOverPatRB(Uint16 pat) ;
    extern  void    slColRAMOffsetNbg0(Uint16 offset) ;
    extern  void    slColRAMOffsetNbg1(Uint16 offset) ;
    extern  void    slColRAMOffsetNbg2(Uint16 offset) ;
    extern  void    slColRAMOffsetNbg3(Uint16 offset) ;
    extern  void    slColRAMOffsetRbg0(Uint16 offset) ;
    extern  void    slColRAMOffsetSpr(Uint16 offset) ;
    extern  void    slBitMapNbg0(Uint16 , Uint16 , void *) ;
    extern  void    slBitMapNbg1(Uint16 , Uint16 , void *) ;
    extern  void    slBitMapRbg0(Uint16 , Uint16 , void *) ;
    extern  void    slBMPalette(Uint16 , Uint16) ;
#define		slBMPaletteNbg0(pal)	slBMPalette(bmNBG0 , pal)
#define		slBMPaletteNbg1(pal)	slBMPalette(bmNBG1 , pal)
#define		slBMPaletteRbg0(pal)	slBMPalette(bmRBG0 , pal)
    extern  void    slSpecialPrioModeNbg0(Uint16) ;
    extern  void    slSpecialPrioModeNbg1(Uint16) ;
    extern  void    slSpecialPrioModeNbg2(Uint16) ;
    extern  void    slSpecialPrioModeNbg3(Uint16) ;
    extern  void    slSpecialPrioModeRbg0(Uint16) ;
    extern  void    slSpecialCCalcModeNbg0(Uint16) ;
    extern  void    slSpecialCCalcModeNbg1(Uint16) ;
    extern  void    slSpecialCCalcModeNbg2(Uint16) ;
    extern  void    slSpecialCCalcModeNbg3(Uint16) ;
    extern  void    slSpecialCCalcModeRbg0(Uint16) ;
    extern  void    slSpecialFuncCodeA(Uint16) ;
    extern  void    slSpecialFuncCodeB(Uint16) ;
    extern  void    slSpecialFuncSelectB(Uint16) ;
    extern  void    slRparaReadCtrlRA(Uint16) ;
    extern  void    slRparaReadCtrlRB(Uint16) ;
    extern  void    slSpecialPrioBitN01(Uint16 , Uint16) ;
    extern  void    slSpecialPrioBitScr(Uint16 , Uint16) ;
    extern  void    slSpecialPrioBitRbg0(Uint16) ;
#define		slSpecialPrioBitNbg0(flag)	slSpecialPrioBitN01(scnNBG0 , flag)
#define		slSpecialPrioBitNbg1(flag)	slSpecialPrioBitN01(scnNBG1 , flag)
#define		slSpecialPrioBitNbg2(flag)	slSpecialPrioBitScr(scnNBG2 , flag)
#define		slSpecialPrioBitNbg3(flag)	slSpecialPrioBitScr(scnNBG3 , flag)
#define		slSpecialPrioBitBMN0(flag)	slSpecialPrioBitN01(scnNBG0 , flag)
#define		slSpecialPrioBitBMN1(flag)	slSpecialPrioBitN01(scnNBG1 , flag)
#define		slSpecialPrioBitBMR0(flag)	slSpecialPrioBitRbg0(flag)
    extern  void    slSpecialCCalcBitN01(Uint16 , Uint16) ;
    extern  void    slSpecialCCalcBitScr(Uint16 , Uint16) ;
    extern  void    slSpecialCCalcBitRbg0(Uint16) ;
#define		slSpecialCCalcBitNbg0(flag)	slSpecialCCalcBitN01(scnNBG0 , flag)
#define		slSpecialCCalcBitNbg1(flag)	slSpecialCCalcBitN01(scnNBG1 , flag)
#define		slSpecialCCalcBitNbg2(flag)	slSpecialCCalcBitScr(scnNBG2 , flag)
#define		slSpecialCCalcBitNbg3(flag)	slSpecialCCalcBitScr(scnNBG3 , flag)
#define		slSpecialCCalcBitBMN0(flag)	slSpecialCCalcBitN01(scnNBG0 , flag)
#define		slSpecialCCalcBitBMN1(flag)	slSpecialCCalcBitN01(scnNBG1 , flag)
#define		slSpecialCCalcBitBMR0(flag)	slSpecialCCalcBitRbg0(flag)
    extern  void    setASC_1to8(Uint8 * , void * , Uint32 , Uint8) ;
    extern  void    setASC_4to8(Uint8 * , void * , Uint32 , Uint8) ;
    extern  void    setASC_1to4(Uint8 * , void * , Uint32 , Uint8 , Sint32) ;
    extern  Uint8   SGL_ASCII_CG[] ;

/*  BitMap graphics library */

    extern  Bool    slInitBitMap(Uint16 , Uint16 , void*) ;
#define         slInitBitMapNbg0(sz,adr)        slInitBitMap(bmNBG0,sz,adr)
#define         slInitBitMapNbg1(sz,adr)        slInitBitMap(bmNBG1,sz,adr)
#define         slInitBitMapRbg0(sz,adr)        slInitBitMap(bmRBG0,sz,adr)
    extern  void    slBitMapBase(Sint16 , Sint16) ;
    extern  Sint8  *slLocateBitMap(Sint16 , Sint16) ;
    extern  void    slClearBitMap(Sint8) ;
    extern  Bool    slBMPset(Sint16 , Sint16 , Sint8) ;
    extern  Sint8   slBMPoint(Sint16 , Sint16) ;
    extern  void    slBMCircle(Sint16 , Sint16 , Sint16 , Sint8) ;
    extern  Bool    slBMLine(Sint16 , Sint16 , Sint16 , Sint16 , Sint8) ;
    extern  Bool    slBMBox(Sint16 , Sint16 , Sint16 , Sint16 , Sint8) ;
    extern  Bool    slBMBoxFill(Sint16 , Sint16 , Sint16 , Sint16 , Sint8) ;
    extern  Bool    slBMGet(Sint16 , Sint16 , Sint16 , Sint16 , Sint8*) ;
    extern  Bool    slBMPut(Sint16 , Sint16 , Sint16 , Sint16 , Sint8*) ;
    extern  Bool    slBMSprPut(Sint16 , Sint16 , Sint16 , Sint16 , Sint8*) ;

#ifdef ST_VE
/*  Graphics enhancer control */

    extern  Bool gxInitEnhancer(Uint16 , Uint16 , Uint16) ;
#define	    EGX_IMMPAL		0	/* パレット直接 */
#define	    EGX_PAL15		1	/* パレット間接 １５ビットカラー */
#define	    EGX_PAL24		2	/* パレット間接 ２４ビットカラー */
#define	    EGX_PAL_RGB15	3	/* パレット、ＲＧＢ混在 １５ビットカラー */
#define	    EGX_PAL_RGB24	4	/* パレット、ＲＧＢ混在 ２４ビットカラー */

#define	    USE_EXVDP1		0
#define	    USE_DSPDMA		1

    extern  void gxFocus(FIXED , FIXED) ;
    extern  void gxCalcMode(Uint16) ;
#define	    CALCNVECTOR		1	/* 法線再計算 */
#define	    USENVECTOR		0	/* 法線使用 */

#define	    NOSHADE		2	/* シェーディングなし */
#define	    FLAT		4	/* フラットシェーディング */
#define	    GOURAUD		8	/* グーローシェーディング */

    extern  void gxSetMaterial(Uint16 , Uint16 , Uint16 , Uint16 , Uint16) ;

#define	    EFUNC_LoadTrans	0x0c0c0c0c
#define	    EFUNC_FlashMatrix	0x10101010
#define	    EFUNC_BaseMatrix	0x11111111
#define	    EFUNC_PushMatrix	0x12121212
#define	    EFUNC_PopMatrix	0x13131313
#define	    EFUNE_Translate	0x14141414
#define	    EFUNE_Scale		0x15151515
#define	    EFUNE_RotX		0x16161616
#define	    EFUNE_RotY		0x17171717
#define	    EFUNE_RotZ		0x18181818
#define	    EFUNE_MultiMatrix	0x19191919
#define	    EFUNC_RotLight	0x1a1a1a1a

    extern  void gxFuncOut(Uint32) ;
    extern  void gxFuncOut2(Uint32 , Uint32) ;
    extern  void gxFuncOut3(Uint32 , Uint32 , Uint32 , Uint32) ;
#define	    gxFlashMatrix()	gxFuncOut(EFUNC_FlashMatrix)
#define	    gxBaseMatrix()	gxFuncOut(EFUNC_BaseMatrix)
#define	    gxPushMatrix()	gxFuncOut(EFUNC_PushMatrix)
#define	    gxPopMatrix()	gxFuncOut(EFUNC_PopMatrix)
#define	    gxRotLight()	gxFuncOut(EFUNC_RotLight)

#define	    gxLoadTrans(x,y,z)	gxFuncOut3(EFUNC_LoadTrans , x , y , z)
#define	    gxTranslate(x,y,z)	gxFuncOut3(EFUNC_Translate , x , y , z)
#define	    gxScale(x,y,z)	gxFuncOut3(EFUNC_Scale , x , y , z)
#define	    gxRotX(ax)		gxFuncOut2(EFUNC_RotX , ax)
#define	    gxRotY(ay)		gxFuncOut2(EFUNC_RotY , ay)
#define	    gxRotZ(az)		gxFuncOut2(EFUNC_RotZ , az)

    extern  void gxMatrixOut(Uint32 , MATRIX) ;
#define	    gxLoadMatrix(mtrx)	gxMatrixOut(EFUNC_LoadMatrix , mtrx)
#define	    gxMultiMatrix(mtrx)	gxMatrixOut(EFUNC_MultiMatrix , mtrx)

    extern  void gxPutPolygon(gxPDATA*) ;
#endif

/*  Peripheral control */

#define	SMPC_MASK_STE		0x80	/* ＳＥＴＴＩＭＥ実行済マスク		*/
#define	SMPC_MASK_RESD		0x40	/* リセットイネーブルマスク		*/
#define	SMPC_MASK_WEEK		0xf0	/* ＲＴＣ曜日マスク			*/
#define	SMPC_MASK_MONTH		0x0f	/* ＲＴＣ月マスク			*/
#define	SMPC_MASK_CTG1		0x02	/* カートリッジコード１			*/
#define	SMPC_MASK_CTG0		0x01	/* カートリッジコード０			*/
#define	SMPC_MASK_DOTSEL	0x4000	/* ＤＯＴＳＥＬ信号			*/
#define	SMPC_MASK_MSHNMI	0x0800	/* ＭＳＨＮＭＩ信号			*/
#define	SMPC_MASK_SYSRES	0x0200	/* ＳＹＳＲＥＳ信号			*/
#define	SMPC_MASK_SNDRES	0x0100	/* ＳＮＤＲＥＳ信号			*/
#define	SMPC_MASK_CDRES		0x0040	/* ＣＤＲＥＳ信号			*/

#define	SMPC_SUN	(0 << 4)	/* ＲＴＣ曜日番号（日曜日）		*/
#define	SMPC_MON	(1 << 4)	/* ＲＴＣ曜日番号（月曜日）		*/
#define	SMPC_TUE	(2 << 4)	/* ＲＴＣ曜日番号（火曜日）		*/
#define	SMPC_WED	(3 << 4)	/* ＲＴＣ曜日番号（水曜日）		*/
#define	SMPC_THU	(4 << 4)	/* ＲＴＣ曜日番号（木曜日）		*/
#define	SMPC_FRI	(5 << 4)	/* ＲＴＣ曜日番号（金曜日）		*/
#define	SMPC_SAT	(6 << 4)	/* ＲＴＣ曜日番号（土曜日）		*/

#define	PER_RESULT_MAX		30	/* 最大ペリフェラル数			*/

#define	PER_ID_StnPad		0x02	/* サターン標準パッド			*/
#define	PER_ID_StnLacing	0x13	/* サターンレーシングコントローラー	*/
#define	PER_ID_StnAnalog	0x15	/* サターンアナログジョイスティック	*/
#define	PER_ID_StnMouse		0x23	/* サターンマウス			*/
#define	PER_ID_StnShooting	0x25	/* サターンシューティングコントローラー */
#define	PER_ID_StnKeyBoard	0x34	/* サターンキーボード			*/
#define	PER_ID_MD3BtnPad	0xe1	/* メガドライブ３ボタンパッド		*/
#define	PER_ID_MD6BtnPad	0xe2	/* メガドライブ６ボタンパッド		*/
#define	PER_ID_ShuttleMouse	0xe3	/* シャトルマウス			*/
#define	PER_ID_ExtDigital	0x00	/* 拡張サイズデジタルデバイス		*/
#define	PER_ID_ExtAnalog	0x10	/* 拡張サイズアナログデバイス		*/
#define	PER_ID_ExtPointing	0x20	/* 拡張サイズポインティングデバイス	*/
#define	PER_ID_ExtKeyBoard	0x30	/* 拡張サイズキーボードデバイス		*/
#define	PER_ID_ExtMegaDrive	0xe0	/* 拡張サイズメガドライブデバイス	*/
#define	PER_ID_NotConnect	0xff	/* 未接続				*/

					/* デジタルデバイス			*/
#define	PER_DGT_KR	(1 << 15)	/* Right Key				*/
#define	PER_DGT_KL	(1 << 14)	/*  Left Key				*/
#define	PER_DGT_KD	(1 << 13)	/*  Down Key				*/
#define	PER_DGT_KU	(1 << 12)	/*    Up Key				*/
#define	PER_DGT_ST	(1 << 11)	/* Start				*/
#define	PER_DGT_TA	(1 << 10)	/* A Trigger				*/
#define	PER_DGT_TC	(1 <<  9)	/* C Trigger				*/
#define	PER_DGT_TB	(1 <<  8)	/* B Trigger				*/
#define	PER_DGT_TR	(1 <<  7)	/* R Trigger				*/
#define	PER_DGT_TX	(1 <<  6)	/* X Trigger				*/
#define	PER_DGT_TY	(1 <<  5)	/* Y Trigger				*/
#define	PER_DGT_TZ	(1 <<  4)	/* Z Trigger				*/
#define	PER_DGT_TL	(1 <<  3)	/* L Trigger				*/

					/* キーボードデバイス			*/
#define	PER_KBD_CL	(1 << 6)	/* Caps Lock				*/
#define	PER_KBD_NL	(1 << 5)	/* Num Lock				*/
#define	PER_KBD_SL	(1 << 4)	/* Scroll Lock				*/
#define	PER_KBD_MK	(1 << 3)	/* Make					*/
#define	PER_KBD_BR	(1 << 0)	/* Break				*/

enum SmpcSelect {			/* ＳＭＰＣ入出力セレクトモード		*/
	SMPC_SH2_DIRECT,		/* (00) ＳＨ２ダイレクト		*/
	SMPC_CONTROL			/* (01) ＳＭＰＣコントロール		*/
};

enum SmpcExtLatch {			/* ＳＭＰＣ外部ラッチ入力モード		*/
	SMPC_EXL_ENA,			/* (00) 外部ラッチ入力許可		*/
	SMPC_EXL_DIS			/* (01) 外部ラッチ入力禁止		*/
};

enum SmpcResetButton {			/* ＳＭＰＣリセットボタン状態		*/
	SMPC_RES_ON,			/* (00) リセットボタンＯＮ		*/
	SMPC_RES_OFF			/* (01) リセットボタンＯＦＦ		*/
};

enum SmpcAreaCode {			/* エリアコード				*/
	SMPC_AREA_RSV_0,		/* (00) 使用禁止			*/
	SMPC_AREA_JP,			/* (01) 日本地域			*/
	SMPC_AREA_AJ_NTSC,		/* (02) アジアＮＴＳＣ地域		*/
	SMPC_AREA_RSV_3,		/* (03) 使用禁止			*/
	SMPC_AREA_NA,			/* (04) 北米地域			*/
	SMPC_AREA_SA_NTSC,		/* (05) 中南米ＮＴＳＣ地域		*/
	SMPC_AREA_KO,			/* (06) 韓国地域			*/
	SMPC_AREA_RSV_7,		/* (07) 使用禁止			*/
	SMPC_AREA_RSV_8,		/* (08) 使用禁止			*/
	SMPC_AREA_RSV_9,		/* (09) 使用禁止			*/
	SMPC_AREA_AJ_PAL,		/* (0a) アジアＰＡＬ地域		*/
	SMPC_AREA_RSV_B,		/* (0b) 使用禁止			*/
	SMPC_AREA_EU_PAL,		/* (0c) ヨーロッパＰＡＬ地域		*/
	SMPC_AREA_SA_PAL,		/* (0d) 中南米ＰＡＬ地域		*/
	SMPC_AREA_RSV_E,		/* (0e) 使用禁止			*/
	SMPC_AREA_RSV_F			/* (0f) 使用禁止			*/
};

enum SmpcOptimize {			/* 取得時間最適化モード			*/
	SMPC_OPT_DIS,			/* (00) 取得時間最適化禁止		*/
	SMPC_OPT_ENA			/* (01) 取得時間最適化許可		*/
};

enum SmpcPortMode {			/* ＳＭＰＣポートモード			*/
	SMPC_PORT_15,			/* (00) １５バイトモード		*/
	SMPC_PORT_255,			/* (01) ２５５バイトモード		*/
	SMPC_PORT_RSV,			/* (02) 設定禁止			*/
	SMPC_PORT_ZERO			/* (03) ０バイトモード			*/
};

enum SmpcLanguage {			/* ＳＭＰＣメモリ言語番号		*/
	SMPC_ENGLISH,			/* (00) 英語				*/
	SMPC_DEUTSCH,			/* (01) ドイツ語			*/
	SMPC_FRANCAIS,			/* (02) フランス語			*/
	SMPC_ESPANOL,			/* (03) スペイン語			*/
	SMPC_ITALIANO,			/* (04) イタリア語			*/
	SMPC_JAPAN			/* (05) 日本語				*/
};

enum SmpcSoundEffect {			/* ＳＭＰＣメモリ効果音モード		*/
	SMPC_EFFECT_OFF,		/* (00) 効果音ＯＦＦ			*/
	SMPC_EFFECT_ON			/* (01) 効果音ＯＮ			*/
};

enum SmpcSoundOutput {			/* ＳＭＰＣメモリ音声出力モード		*/
	SMPC_SOUND_MONO,		/* (00) モノラル			*/
	SMPC_SOUND_STEREO		/* (01) ステレオ			*/
};

enum SmpcHelpWindow {			/* ＳＭＰＣメモリヘルプウィンドウモード	*/
	SMPC_HELP_DIS,			/* (00) 表示しない			*/
	SMPC_HELP_ENA			/* (01) 表示する			*/
};

enum SmpcMonth {			/* ＲＴＣ月番号				*/
	SMPC_DMY,			/* (00) ダミー				*/
	SMPC_JAN,			/* (01) 　１月				*/
	SMPC_FEB,			/* (02) 　２月				*/
	SMPC_MAR,			/* (03) 　３月				*/
	SMPC_APR,			/* (04) 　４月				*/
	SMPC_MAY,			/* (05) 　５月				*/
	SMPC_JUN,			/* (06) 　６月				*/
	SMPC_JUL,			/* (07) 　７月				*/
	SMPC_AUG,			/* (08) 　８月				*/
	SMPC_SEP,			/* (09) 　９月				*/
	SMPC_OCT,			/* (0a) １０月				*/
	SMPC_NOV,			/* (0b) １１月				*/
	SMPC_DEC			/* (0c) １２月				*/
};

enum SmpcCommand {			/* ＳＭＰＣコマンド番号			*/
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

enum SmpcWaitMode {			/* ＳＭＰＣコマンド実行モード		*/
	SMPC_NO_WAIT,			/* (00) コマンド実行終了待ち無し	*/
	SMPC_WAIT			/* (01) コマンド実行終了待ち有り	*/
};

typedef struct {			/* ＲＴＣ時刻				*/
	Uint16	year;			/* 西暦年号				*/
	Uint8	month;			/* 曜日・月				*/
	Uint8	date;			/* 日					*/
	Uint8	hour;			/* 時					*/
	Uint8	minute;			/* 分					*/
	Uint8	second;			/* 秒					*/
	Uint8	dummy;			/* ダミー				*/
} SmpcDateTime;

typedef struct {			/* ＳＭＰＣステータス			*/
	Uint8	cond;			/* 状態ステータス			*/
	Uint8	dummy1;			/* ダミー１				*/
	Uint16	dummy2;			/* ダミー２				*/
	SmpcDateTime	rtc;		/* ＲＴＣ時刻				*/
	Uint8	ctg;			/* カートリッジコード			*/
	Uint8	area;			/* エリアコード				*/
	Uint16	system;			/* システムステータス			*/
	Uint32	smem;			/* ＳＭＰＣメモリ保持データ		*/
} SmpcStatus;

typedef struct {			/* デジタルデバイス			*/
	Uint8	id;			/* ペリフェラルＩＤ			*/
	Uint8	ext;			/* 拡張データサイズ			*/
	Uint16	data;			/* ボタン現在データ			*/
	Uint16	push;			/* ボタン押下データ			*/
	Uint16	pull;			/* ボタン引上データ			*/
	Uint32	dummy2[4];		/* ダミー２				*/
} PerDigital;

typedef struct {			/* アナログデバイス			*/
	Uint8	id;			/* ペリフェラルＩＤ			*/
	Uint8	ext;			/* 拡張データサイズ			*/
	Uint16	data;			/* ボタン現在データ			*/
	Uint16	push;			/* ボタン押下データ			*/
	Uint16	pull;			/* ボタン引上データ			*/
	Uint8	x;			/* Ｘ軸データ絶対値			*/
	Uint8	y;			/* Ｙ軸データ絶対値			*/
	Uint8	z;			/* Ｚ軸データ絶対値			*/
	Uint8	dummy1;			/* ダミー１				*/
	Uint32	dummy2[3];		/* ダミー２				*/
} PerAnalog;

typedef struct {			/* ポインティングデバイス		*/
	Uint8	id;			/* ペリフェラルＩＤ			*/
	Uint8	ext;			/* 拡張データサイズ			*/
	Uint16	data;			/* ボタン現在データ			*/
	Uint16	push;			/* ボタン押下データ			*/
	Uint16	pull;			/* ボタン引上データ			*/
	Uint16	x;			/* Ｘ座標				*/
	Uint16	y;			/* Ｙ座標				*/
	Uint32	dummy2[3];		/* ダミー２				*/
} PerPoint;

typedef struct {			/* キーボードデバイス			*/
	Uint8	id;			/* ペリフェラルＩＤ			*/
	Uint8	ext;			/* 拡張データサイズ			*/
	Uint16	data;			/* ボタン現在データ			*/
	Uint16	push;			/* ボタン押下データ			*/
	Uint16	pull;			/* ボタン引上データ			*/
	Uint8	cond;			/* 状態データ				*/
	Uint8	code;			/* キーコード				*/
	Uint16	dummy1;			/* ダミー１				*/
	Uint32	dummy2[3];		/* ダミー２				*/
} PerKeyBoard;

extern	Uint8	slGetPortDir1(void);
extern	Uint8	slGetPortDir2(void);
extern	Bool	slSetPortDir1(Uint8);
extern	Bool	slSetPortDir2(Uint8);

extern	Uint8	slGetPortData1(void);
extern	Uint8	slGetPortData2(void);
extern	void	slSetPortData1(Uint8);
extern	void	slSetPortData2(Uint8);

extern	Uint8	slGetPortSelect1(void);
extern	Uint8	slGetPortSelect2(void);
extern	Bool	slSetPortSelect1(Uint8);
extern	Bool	slSetPortSelect2(Uint8);

extern	Uint8	slGetPortExt1(void);
extern	Uint8	slGetPortExt2(void);
extern	Bool	slSetPortExt1(Uint8);
extern	Bool	slSetPortExt2(Uint8);

extern	Uint8	slGetLanguage(void);
extern	void	slSetLanguage(Uint8);

extern	Uint8	slGetSoundEffect(void);
extern	void	slSetSoundEffect(Uint8);

extern	Uint8	slGetSoundOutput(void);
extern	void	slSetSoundOutput(Uint8);

extern	Uint8	slGetHelpWindow(void);
extern	void	slSetHelpWindow(Uint8);

extern	void	slInitPeripheral(void);
extern	Bool	slSetResultBuffer(Uint8*, PerDigital*, Uint16);

extern	Bool	slIntBackCancel(void);
extern	Uint8	slCheckReset(void);
extern	Bool	slClearReset(void);

extern	Uint8	slGetOptimize(void);
extern	Bool	slSetOptimize(Uint8);

extern	Uint8	slGetPortMode1(void);
extern	Uint8	slGetPortMode2(void);
extern	Bool	slSetPortMode1(Uint8);
extern	Bool	slSetPortMode2(Uint8);

extern	Bool	slRequestCommand(Uint8, Uint8);

extern	SmpcStatus*	Smpc_Status;
extern	PerDigital*	Smpc_Peripheral;
extern	Uint8		Per_Connect1, Per_Connect2;

#ifdef  TITAN
typedef struct {                        /* System Switch */
        Uint8   dummy ;
        Uint8   data ;
        Uint8   push ;
        Uint8   pull ;
} SysSwitch ;

    extern  PerDigital  PerDigitalBuf[4] ;
    extern  SysSwitch   SystemSwitchBuf ;

#define     _ServiceSw          0x80    /* 基盤上のスイッチとＪＡＭＭＡ入力の ＯＲしたもの */
#define     _TestSw             0x40
#define     _ServiceSwBD        0x20    /* 基盤上のタクトスイッチ */
#define     _TestSwBD           0x10
#define     _ServiceSwJM        0x08    /* ＪＡＭＭＡコネクタからのスイッチ入力 */
#define     _TestSwJM           0x04
#define     _Coin2              0x02
#define     _Coin1              0x01
#endif

#ifdef  ST_V
    extern  PerDigital	PerDigitalBuf[4] ;
#endif
#ifdef  ST_VE
    extern  PerDigital	PerDigitalBuf[4] ;
#endif

#define	slMasterOn()			slRequestCommand(SMPC_MSHON , SMPC_NO_WAIT)
#define	slSlaveOn()			slRequestCommand(SMPC_SSHON , SMPC_NO_WAIT)
#define	slSlaveOff()			slRequestCommand(SMPC_SSHOFF, SMPC_NO_WAIT)

#ifdef TITAN
    extern void slSoundOn() ;
    extern void slSoundOff() ;
#else
#define	slSoundOn()		slRequestCommand(SMPC_SNDON , SMPC_NO_WAIT)
#define	slSoundOff()		slRequestCommand(SMPC_SNDOFF, SMPC_NO_WAIT)
#endif

#define	slCDOn()			slRequestCommand(SMPC_CDON  , SMPC_NO_WAIT)
#define	slCDOff()			slRequestCommand(SMPC_CDOFF , SMPC_NO_WAIT)
#define	slSystemReset()			slRequestCommand(SMPC_SYSRES, SMPC_NO_WAIT)
#define	slClockChange352()		slRequestCommand(SMPC_CKC352, SMPC_NO_WAIT)
#define	slClockChange320()		slRequestCommand(SMPC_CKC320, SMPC_NO_WAIT)
#define	slNMIRequest()			slRequestCommand(SMPC_NMIREQ, SMPC_NO_WAIT)
#define	slResetEnable()			slRequestCommand(SMPC_RESENA, SMPC_NO_WAIT)
#define	slResetDisable()		slRequestCommand(SMPC_RESDIS, SMPC_NO_WAIT)
#define	slGetStatus()			slRequestCommand(SMPC_GETSTS, SMPC_NO_WAIT)
#define	slGetPeripheral()		slRequestCommand(SMPC_GETPER, SMPC_NO_WAIT)
#define	slSetSmpcMemory()		slRequestCommand(SMPC_SETMEM, SMPC_NO_WAIT)
#define	slSetDateTime()			slRequestCommand(SMPC_SETTIM, SMPC_NO_WAIT)

#define	slMasterOnWait()		slRequestCommand(SMPC_MSHON , SMPC_WAIT)
#define	slSlaveOnWait()			slRequestCommand(SMPC_SSHON , SMPC_WAIT)
#define	slSlaveOffWait()		slRequestCommand(SMPC_SSHOFF, SMPC_WAIT)
#define	slSoundOnWait()			slRequestCommand(SMPC_SNDON , SMPC_WAIT)
#define	slSoundOffWait()		slRequestCommand(SMPC_SNDOFF, SMPC_WAIT)
#define	slCDOnWait()			slRequestCommand(SMPC_CDON  , SMPC_WAIT)
#define	slCDOffWait()			slRequestCommand(SMPC_CDOFF , SMPC_WAIT)
#define	slNMIRequestWait()		slRequestCommand(SMPC_NMIREQ, SMPC_WAIT)
#define	slResetEnableWait()		slRequestCommand(SMPC_RESENA, SMPC_WAIT)
#define	slResetDisableWait()		slRequestCommand(SMPC_RESDIS, SMPC_WAIT)
#define	slSetSmpcMemoryWait()		slRequestCommand(SMPC_SETMEM, SMPC_WAIT)
#define	slSetDateTimeWait()		slRequestCommand(SMPC_SETTIM, SMPC_WAIT)

/*  Event library	*/

    extern void    slInitEvent(void) ;
    extern EVENT  *slGetEvent(void) ;
    extern void    slReturnEvent(EVENT *) ;
    extern EVENT  *slSetEvent(void (*func)()) ;
    extern void    slCloseEvent(EVENT *) ;
    extern EVENT  *slSetEventNext(EVENT * , void (*func)()) ;
    extern void    slExecuteEvent(void) ;
    extern WORK   *slGetWork(void) ;
    extern void    slReturnWork(WORK *) ;

/*  DMA control */

    extern void    slDMACopy(void * , void * , Uint32) ;
    extern void    slDMAXCopy(void * , void * , Uint32 , Uint16) ;
    extern void    slDMAWait() ;
    extern Bool    slDMAStatus() ;

/* DMA control with SBL */

/*
 * GLOBAL DEFINES/MACROS DEFINES
 */

/* 定数(CPU-DMA) */
/**** DMAレジスタビット位置 **************************************************/
/******** チャネルコントロールレジスタ ***************************************/
#define DMA_CHCR_DE     0       /* DMAイネーブルビット                       */
#define DMA_CHCR_TE     1       /* トランスファエンドフラグビット            */
#define DMA_CHCR_IE     2       /* インタラプトイネーブルビット              */
#define DMA_CHCR_TA     3       /* トランスファアドレスモードビット          */
#define DMA_CHCR_TB     4       /* トランスファバスモードビット              */
#define DMA_CHCR_DL     5       /* DREQレベルビット　　                      */
#define DMA_CHCR_DS     6       /* DREQセレクトビット                        */
#define DMA_CHCR_AL     7       /* アクノリッジレベルビット                  */
#define DMA_CHCR_AM     8       /* アクノリッジ/トランスファモードビット     */
#define DMA_CHCR_AR     9       /* オートリクエストビット                    */
#define DMA_CHCR_TS     10      /* トランスファサイズビット                  */
#define DMA_CHCR_SM     12      /* ソースアドレスモードビット                */
#define DMA_CHCR_DM     14      /* ディスティネーションアドレスモードビット  */
/******** DMAオペレーションレジスタ ******************************************/
#define DMA_DMAOR_DME   0               /* DMAマスタイネーブルビット         */
#define DMA_DMAOR_NMIF  1               /* NMIフラグビット                   */
#define DMA_DMAOR_AE    2               /* アドレスエラーフラグビット        */
#define DMA_DMAOR_PR    3               /* プライオリティモードビット        */

/*****************************************************************************/
/**** DMAチャネル ************************************************************/
/*****************************************************************************/
#define DMA_CPU_CH0 0                           /* チャネル0                 */
#define DMA_CPU_CH1 1                           /* チャネル1                 */

/**** (DmaCpuComPrm)共通転送パラメータ ***************************************/
/******** (pr)プライオリティモード  ******************************************/
#define DMA_CPU_FIX (0 << DMA_DMAOR_PR)         /* 優先順位は固定            */
#define DMA_CPU_ROR (1 << DMA_DMAOR_PR)         /* 優先順位はラウンドロビン- */
                                                /* による                    */
/******** (dme)DMAマスタイネーブル *******************************************/
#define DMA_CPU_DIS     (0 << DMA_DMAOR_DME)    /* 全チャネルのDMA転送を禁止 */
#define DMA_CPU_ENA     (1 << DMA_DMAOR_DME)    /* 全チャネルのDMA転送を許可 */
/******** (msk)マスクビット **************************************************/
#define DMA_CPU_M_PR    (1 << 0)                /* プライオリティモード　　  */
#define DMA_CPU_M_AE    (1 << 1)                /* アドレスエラーフラグ　　  */
#define DMA_CPU_M_NMIF  (1 << 2)                /* NMIフラグ           　　  */
#define DMA_CPU_M_DME   (1 << 3)                /* DMAマスタイネーブル 　　  */

/**** (DmaCpuPrm)転送パラメータ **********************************************/
/******** (dm)ディスティネーションアドレスモードビット ***********************/
/******** (sm)ソースアドレスモードビット *************************************/
#define DMA_CPU_AM_NOM  0x0                    /* 固定                      */
#define DMA_CPU_AM_ADD  0x1                    /* 増加                      */
#define DMA_CPU_AM_SUB  0x2                    /* 減少                      */
/******** (ts)トランスファサイズ *********************************************/
#define DMA_CPU_1   (0 << DMA_CHCR_TS)          /* バイト単位                */
#define DMA_CPU_2   (1 << DMA_CHCR_TS)          /* ワード(2バイト)単位       */
#define DMA_CPU_4   (2 << DMA_CHCR_TS)          /* ロングワード(4バイト)単位 */
#define DMA_CPU_16  (3 << DMA_CHCR_TS)          /* 16バイト転送              */
/******** (ar)オートリクエストモード *****************************************/
#define DMA_CPU_MOD     (0 << DMA_CHCR_AR)      /* モジュールリクエスト      */
#define DMA_CPU_AUTO    (1 << DMA_CHCR_AR)      /* オートリクエスト          */
/******** (ie)インタラプトイネーブル *****************************************/
#define DMA_CPU_INT_ENA (1 << DMA_CHCR_IE)      /* 割り込み要求を許可        */
#define DMA_CPU_INT_DIS (0 << DMA_CHCR_IE)      /* 割り込み要求を禁止(初期値)*/
/******** (drcr)DMA要求／応答選択制御 ****************************************/
#if 0
#define DMA_CPU_DREQ    0x0                     /* DREQ(外部リクエスト)      */
/*
 *  DMA要求に外部リクエストを指定するのは禁止('95-11/14 H.O)
 */
#endif
#define DMA_CPU_RXI     0x1                     /* RXI(内蔵SCIの受信データ-  */
                                                /* フル割り込み転送要求)     */
#define DMA_CPU_TXI     0x2                     /* TXI(内蔵SCIの送信データ-  */
                                                /* エンプティ割り込み転送要求*/
/******** (msk)マスクビット **************************************************/
#define DMA_CPU_M_SAR   (1 << 0)                /* DMAソースアドレス         */
#define DMA_CPU_M_DAR   (1 << 1)                /* DMAディスティネーション-  */
                                                /* アドレス                  */
#define DMA_CPU_M_TCR   (1 << 2)                /* DMAトランスファカウント   */
#define DMA_CPU_M_DM    (1 << 3)                /* DMAディスティネーション-  */
                                                /* アドレスモードビット      */
#define DMA_CPU_M_SM    (1 << 4)                /* DMAソースアドレスモード-  */
                                                /* ビット                    */
#define DMA_CPU_M_TS    (1 << 5)                /* トランスファサイズ        */
#define DMA_CPU_M_AR    (1 << 6)                /* オートリクエストモード    */
#define DMA_CPU_M_IE    (1 << 7)                /* インタラプトイネーブル    */
#define DMA_CPU_M_DRCR  (1 << 8)                /* DMA要求／応答選択制御     */
#define DMA_CPU_M_TE    (1 << 9)                /* トランスファエンドフラグ  */

/**** (DmaCpuComStatus)共通ステータス ****************************************/
/******** (ae)アドレスエラーフラグ *******************************************/
#define DMA_CPU_ADR_ERR     (1 << DMA_DMAOR_AE) /* DMACによるアドレスエラー- */
                                                /* 発生                      */
#define DMA_CPU_ADR_NO_ERR  (0 << DMA_DMAOR_AE) /* DMACによるアドレスエラー- */
                                                /* なし                      */

/******** (nmif)NMIフラグ ****************************************************/
#define DMA_CPU_NMI_ON  (1 << DMA_DMAOR_NMIF)   /* NMI割り込み発生           */
#define DMA_CPU_NMI_OFF (0 << DMA_DMAOR_NMIF)   /* NMI割り込みなし           */

/**** (DmaCpuComStatus)共通ステータス ****************************************/
#define DMA_CPU_TE_MV   (0 << DMA_CHCR_TE)      /* DMA転送中またはDMA転送中断*/
#define DMA_CPU_TE_SP   (1 << DMA_CHCR_TE)      /* DMA転送正常終了           */


typedef struct {
    Uint32 pr;                                  /* プライオリティモード      */
    Uint32 dme;                                 /* DMAマスタイネーブル       */
    Uint32 msk;                                 /* マスクビット              */
}DmaCpuComPrm;                                  /* 共通転送パラメータ        */

typedef struct {
    Uint32 sar;                                 /* DMAソースアドレス         */
    Uint32 dar;                                 /* DMAディスティネーション-  */
                                                /* アドレス                  */
    Uint32 tcr;                                 /* DMAトランスファカウント   */
    Uint32 dm;                                  /* ディスティネーションアド- */
                                                /* レスモードビット          */
    Uint32 sm;                                  /* ソースアドレスモードビット*/
    Uint32 ts;                                  /* トランスファサイズ        */
    Uint32 ar;                                  /* オートリクエストモード    */
    Uint32 ie;                                  /* インタラプトイネーブル    */
    Uint32 drcr;                                /* DMA要求／応答選択制御     */
    Uint32 msk;                                 /* マスクビット              */
}DmaCpuPrm;                                     /* 転送パラメータ            */

typedef struct {
    Uint32 ae;                                  /* アドレスエラーフラグ      */
    Uint32 nmif;                                /* NMIフラグ                 */
}DmaCpuComStatus;                               /* 共通ステータス            */

typedef Uint32 DmaCpuStatus;                    /* ステータス                */

/* 定数(SCU-DMA) */
/*****************************************************************************/
/**** DMAチャネル ************************************************************/
/*****************************************************************************/
#define DMA_SCU_CH0 0x0                         /* チャネル0                 */
#define DMA_SCU_CH1 0x1                         /* チャネル1                 */
#define DMA_SCU_CH2 0x2                         /* チャネル2                 */

/**** DMAレジスタビット位置 **************************************************/
/******** 加算値レジスタ *****************************************************/
#define DMA_DXAD_DXRAD  8                       /* 読み込みアドレス加算値    */
#define DMA_DXAD_DXWAD  0                       /* 書き込みアドレス加算値    */
/******** DMA許可レジスタ ****************************************************/
#define DMA_DXEN_DXGO   0                       /* DMA起動ビット             */
#define DMA_DXEN_DXEN   8                       /* DMA許可ビット             */
/******** DMAモード、アドレス更新、起動要因レジスタ **************************/
#define DMA_DXMD_DXMOD  24                      /* モードビット              */
#define DMA_DXMD_DXRUP  16                      /* 読み込みアドレス更新ビット*/
#define DMA_DXMD_DXWUP   8                      /* 書き出しアドレス更新ビット*/
#define DMA_DXMD_DXFT    0                      /* 起動要因選択ビット        */


/*****************************************************************************/
/**** (DmaScuPrm)転送パラメータ **********************************************/
/*****************************************************************************/
/******** (dxad_r)読み込みアドレス加算値 *************************************/
#define DMA_SCU_R0  (0x0 << DMA_DXAD_DXRAD)     /* 加算しない                */
#define DMA_SCU_R4  (0x1 << DMA_DXAD_DXRAD)     /* 4バイト加算する           */
/******** (dxad_w)書き込みアドレス加算値 *************************************/
#define DMA_SCU_W0     (0x0 << DMA_DXAD_DXWAD)  /* 加算しない                */
#define DMA_SCU_W2     (0x1 << DMA_DXAD_DXWAD)  /* 2バイト加算する           */
#define DMA_SCU_W4     (0x2 << DMA_DXAD_DXWAD)  /* 4バイト加算する           */
#define DMA_SCU_W8     (0x3 << DMA_DXAD_DXWAD)  /* 8バイト加算する           */
#define DMA_SCU_W16    (0x4 << DMA_DXAD_DXWAD)  /* 16バイト加算する          */
#define DMA_SCU_W32    (0x5 << DMA_DXAD_DXWAD)  /* 32バイト加算する          */
#define DMA_SCU_W64    (0x6 << DMA_DXAD_DXWAD)  /* 64バイト加算する          */
#define DMA_SCU_W128   (0x7 << DMA_DXAD_DXWAD)  /* 128バイト加算する         */
/******** (dxmod)モードビット ************************************************/
#define DMA_SCU_DIR    (0x0 << DMA_DXMD_DXMOD)  /* 直接モード                */
#define DMA_SCU_IN_DIR (0x1 << DMA_DXMD_DXMOD)  /* 間接モード                */
/******** (dxrup)読み込みアドレス更新ビット **********************************/
/******** (dxwup)書き出しアドレス更新ビット **********************************/
#define DMA_SCU_KEEP    0x0                     /* 保持する                  */
#define DMA_SCU_REN     0x1                     /* 更新する                  */

/******** (dxft)起動要因選択ビット *******************************************/
#define DMA_SCU_F_VBLK_IN  (0x0 << DMA_DXMD_DXFT)/* V-ブランク-IN            */
#define DMA_SCU_F_VBLK_OUT (0x1 << DMA_DXMD_DXFT)/* V-ブランク-OUT           */
#define DMA_SCU_F_HBLK_IN  (0x2 << DMA_DXMD_DXFT)/* H-ブランク-IN            */
#define DMA_SCU_F_TIM0     (0x3 << DMA_DXMD_DXFT)/* タイマ0                  */
#define DMA_SCU_F_TIM1     (0x4 << DMA_DXMD_DXFT)/* タイマ1                  */
#define DMA_SCU_F_SND      (0x5 << DMA_DXMD_DXFT)/* サウンド-Req             */
#define DMA_SCU_F_SPR      (0x6 << DMA_DXMD_DXFT)/* スプライト描画           */
#define DMA_SCU_F_DMA      (0x7 << DMA_DXMD_DXFT)/* DMA起動要因ビットのセット*/
/******** (msk)マスクビット *************************************************/
#define DMA_SCU_M_DXR      (1 << 0)             /* 読み込みアドレス          */
#define DMA_SCU_M_DXW      (1 << 1)             /* 書き出しアドレス          */

/*****************************************************************************/
/**** (DmaScuStatus)ステータス ***********************************************/
/*****************************************************************************/
/******** (dxbkr)DMA動作中フラグ *********************************************/
/******** (ddmv)DSP側DMA動作中フラグ *****************************************/
#define DMA_SCU_MV      0x1                     /* 動作中                    */
#define DMA_SCU_NO_MV   0x0                     /* 動作中でない              */

typedef struct {
    Uint32 dxr;                                 /* 読み込みアドレス          */
    Uint32 dxw;                                 /* 書き出しアドレス          */
    Uint32 dxc;                                 /* 転送バイト数              */
    Uint32 dxad_r;                              /* 読み込みアドレス加算値    */
    Uint32 dxad_w;                              /* 書き込みアドレス加算値    */
    Uint32 dxmod;                               /* モードビット              */
    Uint32 dxrup;                               /* 読み込みアドレス更新ビット*/
    Uint32 dxwup;                               /* 書き込みアドレス更新ビット*/
    Uint32 dxft;                                /* 起動要因選択ビット        */
    Uint32 msk;                                 /* マスクビット              */
}DmaScuPrm;                                     /* 転送パラメータ            */

typedef struct {
    Uint32 dxmv;                            /* DMA動作中フラグ               */
}DmaScuStatus;                              /* ステータス                    */

    extern void    DMA_CpuSetComPrm(DmaCpuComPrm *) ;
    extern void    DMA_CpuSetPrm(DmaCpuPrm *, Uint32) ;
    extern void    DMA_CpuStart(Uint32) ;
    extern void    DMA_CpuStop(Uint32) ;
    extern void    DMA_CpuAllStop() ;
    extern void    DMA_CpuGetComStatus(DmaCpuComStatus *) ;
    extern DmaCpuStatus DMA_CpuGetStatus(Uint32) ;
    extern void    DMA_ScuSetPrm(DmaScuPrm *, Uint32) ;
    extern void    DMA_ScuStart(Uint32) ;
    extern void    DMA_ScuGetStatus(DmaScuStatus *, Uint32) ;

    extern void    DMA_ScuMemCopy(void *, void *, Uint32) ;
    extern Uint32  DMA_ScuResult() ;

    extern void    DMA_CpuMemCopy1(void *, void *, Uint32) ;
    extern void    DMA_CpuMemCopy2(void *, void *, Uint32) ;
    extern void    DMA_CpuMemCopy4(void *, void *, Uint32) ;
    extern void    DMA_CpuMemCopy16(void *, void *, Uint32) ;
    extern Uint32  DMA_CpuResult() ;

/*  Slave CPU control */
    extern void    slSlaveFunc(void (*func)() , void *) ;

/*  System library */
    extern  void   slInitSystem(Uint16 , TEXTURE * , Sint8) ;
    extern  void   slSetTVMode(Uint16) ;
    extern  void   slSetScrTVMode(Uint16) ;
    extern  void   slSetSprTVMode(Uint16) ;
    extern  Bool   slDynamicFrame(Uint16) ;
    extern  void   slSynch(void) ;
    extern  void   slInitSynch(void) ;
    extern  Bool   slTransferEntry(void * , void * , Uint16) ;
    extern  void   slIntFunction(void (*func)()) ;
    extern  void   slSynchFunction(void (*func)()) ;
    extern  void   SetCDFunc(void (*func)()) ;
    extern  void   slCashPurge() ;

/*  Sound library */
    extern  void   slInitSound(Uint8 * , Uint32 , Uint8 * , Uint32) ;
    extern  Bool   slBGMOn(Uint16 , Uint8 , Uint8 , Uint8) ;
    extern  Bool   slBGMPause() ;
    extern  Bool   slBGMCont() ;
    extern  Bool   slBGMOff() ;
    extern  Bool   slBGMFade(Uint8 , Uint8) ;
    extern  Bool   slBGMTempo(Sint16) ;
#define     slBGMStat()    slSequenceStat(0)
    extern  Bool   slSoundAllOff() ;
    extern  Bool   slSoundAllPause() ;
    extern  Bool   slSoundAllCont() ;
    extern  Bool   slDSPOff() ;
    extern  Bool   slSndVolume(Uint8) ;
    extern  Uint8  slSequenceOn(Uint16 , Uint8 , Uint8 , Sint8) ;
    extern  Bool   slSequenceOff(Uint8) ;
    extern  Bool   slSequenceFade(Uint8 , Uint8 , Uint8) ;
    extern  Bool   slSequenceTempo(Uint8 , Sint16) ;
    extern  Bool   slSequencePause(Uint8) ;
    extern  Bool   slSequenceCont(Uint8) ;
    extern  Bool   slSequencePan(Uint8 , Sint8) ;
    extern  Bool   slSequenceStat(Uint8) ;
    extern  Bool   slSequenceReset(Uint8) ;
    extern  void  *slSndMapChange(Uint8) ;
    extern  void   slWaitSound(void *) ;
    extern  Bool   slCDDAOn(Uint8 , Uint8 , Sint8 , Sint8) ;
    extern  Bool   slCDDAOff() ;
    extern  Sint8  slPCMOn(PCM * , void * , Uint32) ;
    extern  Bool   slPCMOff(PCM *) ;
    extern  Bool   slPCMParmChange(PCM *) ;
    extern  Bool   slPCMStat(PCM *) ;
    extern  Sint8  slSndPCMNum() ;
    extern  Uint8  slSndSeqNum() ;
    extern  Bool   slSndEffect(Uint8) ;
    extern  Bool   slSndMixChange(Uint8 , Uint8) ;
    extern  Bool   slSndMixParmChange(Uint8 , Uint8 , Sint8) ;
    extern  Sint8  slSoundRequest(const char*,...) ;
    extern  void   slSndFlush() ;

/*  Gouraud library */

    typedef Uint16 GOURAUDTBL[4];

    extern  void slInitGouraud( GOURAUDTBL *, Uint32, Uint32, Uint8 *);
    extern  void slPutPolygonX( XPDATA *, FIXED *);
    extern  void slGouraudTblCopy(void);
    extern  void slSetGouraudTbl( Uint16 *);
    extern  void slSetGouraudColor( Uint16 );
    extern  void slSetAmbient( Uint16);

    extern  void slSetFlatColor( Uint16);

    extern  void slSetDepthLimit( Uint32, Uint16, Uint16 );
    extern  void slSetDepthTbl( Uint16 *, Uint16, Uint16 );

    extern  void slNearClipFlag( Uint32 );
    extern  void slDispPlaneLevel( Sint32 );
    extern  void slWindowClipLevel( Sint32 );

/*------------------------------------------------------------------------*/

#endif
