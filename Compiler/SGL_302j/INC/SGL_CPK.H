/*******************************************************************
*
*                       Cinepak for SATURN Player 
*                                by SOJ
*
*                      Copyright(c) 1994,1995 SEGA
*
*   Comment: 利用者用ヘッダ
*   File   : SEGA_CPK.H
*   Date   : 1995-10-05
*   Version: 1.20
*   Author : Y.T
*
*******************************************************************/
#ifndef SEGA_CPK_H
#define SEGA_CPK_H


/*-------------------------- 《処理選択》 --------------------------*/

/* コードブックアドレステーブルを使用する
 * 　これを有効にする場合は、cpk_ra15.src cpk_er15.src も同時に１と
 *   宣言すること
 */
/* #define CPK_USE_CBTBL */


/*-------------------- 《インクルードファイル》 --------------------*/
#include <sega_xpt.h>
#if 0
#include <sega_gfs.h>
#include <sega_stm.h>
#else
#include <sgl_cd.h>
#endif


/*------------------------- 《マクロ定数》 -------------------------*/

/* バージョン */
#define CPK_VERSION				("CPK Version 1.20 1995-10-05")
							/*	           1         2         3         4	*/
							/*	  1234567890123456789012345678901234567890	*/
							/*	  ++++++++++++++++++++++++++++++++			*/
#define CPK_VERSION_LEN			(32)

/* ハンドル総数 */
#define CPK_HN_MAX				(32)

/* エラーコード	*/
typedef enum {
	CPK_ERR_OK					= 0x00000000,/* エラーは発生していない		*/

	/* CPK_Create のエラー */
	CPK_ERR_OUT_OF_HANDLE		= 0x00000101,/* ハンドル売り切れ 			*/
	CPK_ERR_TOO_SMALL_BUF		= 0x00000102,/* ﾘﾝｸﾞﾊﾞｯﾌｧｻｲｽﾞが小さすぎる	*/
	CPK_ERR_NO_INIT				= 0x00000103,/* 初期化関数がｺｰﾙされてない 	*/

	/* CPK_Destroy のエラー */
	CPK_ERR_INVALID_HN			= 0x00000201,/* 不正なハンドルで消去		*/
	CPK_ERR_ILL_CREATE_MODE		= 0x00000202,/* Create 時のモードと違う		*/

	/* cpk_HeaderProcess のエラー */
	CPK_ERR_TOO_SMALL_FLOP		= 0x00000301,/* FLOP初期値が小さすぎる		*/
	CPK_ERR_TOO_LARGE_HEADER	= 0x00000302,/* ヘッダがき大すぎる			*/
											/* (ﾘﾝｸﾞﾊﾞｯﾌｧｻｲｽﾞが小さすぎる)	*/
	CPK_ERR_HEADER_DATA			= 0x00000303,/* ヘッダデータエラー 			*/
	CPK_ERR_TOO_SMALL_PCMBUF	= 0x00000304,/* PCMﾊﾞｯﾌｧｻｲｽﾞが小さすぎる	*/
	CPK_ERR_ILL_SIZE_PCMBUF		= 0x00000305,/* PCMﾊﾞｯﾌｧｻｲｽﾞ不正 4096*2～16	*/

	/* cpk_VideoSample のエラー */
	CPK_ERR_SAMPLE_ID			= 0x00000401,/* 識別子エラー。00h,01h 以外だ｡*/
	CPK_ERR_SAMPLE_DATA			= 0x00000402,/* 不正データ。				*/

	/*  */
	CPK_ERR_ILLEGAL_PARA		= 0x00000503,/* 引き数の指定が間違っている 	*/
	CPK_ERR_ILLEGAL_HANDLE		= 0x00000504,/* ハンドルが異常 				*/

	/* 連続再生に関するエラー */
	CPK_ERR_NEXT_MOVI_STATUS	= 0x00000601,/* 連続再生ﾑｰﾋﾞｰの状態異常 	*/
	CPK_ERR_NEXT_MOVI_AUDIO		= 0x00000602,/* オーディオの条件が一致しない */
	CPK_ERR_NEXT_MOVI_SCALE		= 0x00000603,/* タイムスケールが一致しない 	*/
	CPK_ERR_CHANGE_NO_DATA		= 0x00000604,/* データ供給不足状態でチェンジ */
	CPK_ERR_CHANGE_NO_ENTRY		= 0x00000605,/* エントリがない状態でチェンジ */

	/* ポーズに関するエラー */
	CPK_ERR_PAUSE_STATUS		= 0x00000701,/* CPK_STAT_PLAY_TIME, または、
											 * CPK_STAT_PLAY_PAUSE 以外の
											 * 状態でポーズ関数をコールした。
											 */

	/* DMA転送に関するエラー */
	CPK_ERR_DMA_MODE			= 0x00000801,/* 未対応の転送方式 			*/
	CPK_ERR_DMA_CPU_PCM			= 0x00000802,/* CPUDMAが異常終了 			*/

	/* ファイル読み込みエラー */
	CPK_ERR_GFS_READ			= 0x00000901,/* ＧＦＳの読み込みで失敗 		*/
	CPK_ERR_STM_FAD				= 0x00000902,/* STM転送関数で,fadが矛盾 	*/
	CPK_ERR_STM_HN				= 0x00000903,/* STM転送関数で,STMﾊﾝﾄﾞﾙ異常 	*/

	/* CPK_DecodeFrameに関するエラー */
	CPK_ERR_DF_NO_DATA			= 0x00000A01,/* ヘッダ分のデータ供給がない 	*/
	CPK_ERR_DF_IS_NOT_VIDEO		= 0x00000A02,/* ビデオサンプルでない 		*/


	CPK_ERR_END
} CpkErrCode;

/* 再生ステータス */
typedef enum {
	CPK_STAT_PLAY_ERR_STOP		= -1,		/* 異常停止					*/
	CPK_STAT_PLAY_STOP			= 0,		/* 停止(クリエイト) 		*/
	CPK_STAT_PLAY_PAUSE			= 1,		/* 一時停止		 			*/
	CPK_STAT_PLAY_START			= 2,		/* 開始						*/
	CPK_STAT_PLAY_HEADER		= 3,		/* ヘッダ処理　				*/
	CPK_STAT_PLAY_TIME			= 4,		/* 再生中(タイマスタート)	*/
	CPK_STAT_PLAY_END			= 5			/* 再生完了 				*/
} CpkPlayStatus;

/* 一時停止の制御コマンド */
typedef enum {
	CPK_PAUSE_ON_AT_ONCE,					/* 即時一時停止 			*/
	CPK_PAUSE_ON_KEYFRAME,					/* キーフレーム待ち一時停止	*/
	CPK_PAUSE_OFF							/* 一時停止解除 			*/
} CpkPauseCmd;

/* 表示色数[bit] */
typedef enum {
	CPK_COLOR_24BIT				= 24,		/* 1600万色 				*/
	CPK_COLOR_15BIT				= 15		/* 32000色 					*/
} CpkColorType;

/* 強制切り替え可能性チェック値 */
typedef enum {
	CPK_CHANGE_OK_AT_ONCE		= 0,	/* 即時切り替え可能。			*/
	CPK_CHANGE_NO_DATA			= 1,	/* データ供給不足。				*/
	CPK_CHANGE_NO_ENTRY			= 2,	/* エントリが無い。				*/
	CPK_CHANGE_TOO_LARGE_HEADER	= 3		/* ヘッダがき大すぎる。			*/
										/* (バッファサイズが小さすぎる)	*/
} CpkChangeStatus;

/* 画像伸張タイミングの設定値 */
typedef enum {
	CPK_DEC_TIMING_DECODE_IN_HASTE = 0,	/* 取りあえず伸張しておく		*/
	CPK_DEC_TIMING_WAIT_AND_DECODE = 1	/* 表示時刻を待って伸張する		*/
} CpkDecTiming;

/* データの転送方式 */
typedef enum {
	CPK_TRMODE_CPU = 0,					/* ソフトウェア転送 			*/
	CPK_TRMODE_SDMA,					/* ＤＭＡサイクルスチール 		*/
	CPK_TRMODE_SCU,						/* ＳＣＵのＤＭＡ				*/
	CPK_TRMODE_SCUDSPDMA				/* ＳＣＵ－ＤＳＰのＤＭＡ		*/
} CpkTrMode;

/* [注意1] 転送方式の設定には次の表に示す制限があります。
 *　　　　　　　　　　　データ転送方式の設定の制限
 *  ====================+=================================================
 *     処理　＼　方式   |  ｿﾌﾄｳｪｱ転送  CPUのDMA  SCUのDMA  SCU-DSPのDMA
 *  ====================+=================================================
 *    CPK_SetTrModeCd   |      ○         ○        ○          ×
 *    CPK_SetTrModePcm  |      ○         ○        ×          ○
 *  ====================+=================================================
 * [注意2] 転送方式に CPK_TRMODE_SCUDSPDMA が指定されていると、初期状態として、
 *  SCU-DSP の使用権は Cinepakﾗｲﾌﾞﾗﾘ にあります。SCU-DSP を Cinepakﾗｲﾌﾞﾗﾘ と 
 *  ｱﾌﾟﾘｹｰｼｮﾝ の両方で利用する場合には CPK_AllocScuDsp, CPK_FreeScuDsp を使い
 *  ます。
 */

/* ４ＢＹＴＥ文字コードを Uint32 型で表現する。
 *   ('cvid'などと書くとﾜｰﾆﾝｸﾞが出てしまうので、これを回避するために)
 */
#define CPK_4BYTE_CHAR(a,b,c,d)					\
	(	((Uint32)((a) & 0x000000FF) << 24) | 	\
		((Uint32)((b) & 0x000000FF) << 16) | 	\
		((Uint32)((c) & 0x000000FF) <<  8) | 	\
		 (Uint32)((d) & 0x000000FF)				\
	)

/* 圧縮方式 */
typedef enum {
	CPK_CTYPE_NOVIDEO 	= 0,								/* 画像なし 	*/
	CPK_CTYPE_NONE 		= CPK_4BYTE_CHAR('r','a','w',' '),	/* 非圧縮 		*/
	CPK_CTYPE_CVID 		= CPK_4BYTE_CHAR('c','v','i','d'),	/* Cinepak圧縮 	*/
	CPK_CTYPE_END
} CpkCType;

/* ＣＰＵ使用条件 */
typedef enum {
	CPK_CPU_MAIN 		= 1, 		/* main cpu only (default)			*/
	CPK_CPU_SLAVE 		= 2, 		/* slave cpu only (Not supported) 	*/
	CPK_CPU_DUAL 		= 3, 		/* dual cpu 						*/
	CPK_CPU_END
} CpkCpu;
/* [注意] 
 *  ＣＰＵ使用条件に CPK_CPU_DUAL が指定されていると、slave cpu は Cinepak
 *  ﾗｲﾌﾞﾗﾘ が専有します。ｱﾌﾟﾘｹｰｼｮﾝは slave cpu を利用することはできません。
 */


/*---------------------- 《各種デフォルト値》 ----------------------*/

/* クロマキーカットオフのデフォルト値 */
#define CPK_DEFAULT_CKEY_CUTOFF_15		(3 * 7)
#define CPK_DEFAULT_CKEY_CUTOFF_24		(CPK_DEFAULT_CKEY_CUTOFF_15 * 8)

/* 無音率のデフォルト値 [1/s] */
#define CPK_DEFAULT_SILENCE_RATE	(8)

/* 再生速度のデフォルト値(通常の再生速度) */
#define CPK_SPEED_ORIGINAL		(0)

/* 再生速度の設定単位 */
#define CPK_SPEED_UNIT			(1024)

/* デバッグ情報の出力開始アドレス（デフォルト値） */
#define CPKD_START_ADDR			(0x04000000)

/* デバッグ情報の出力終了アドレス（デフォルト値） */
#define CPKD_END_ADDR			(0x047FFFF0)

/* デバッグ情報の出力開始位置のマージン[dword] */
#define CPKD_HEAD_MARGIN		(4)

/* CPKDシステム構造体の識別文字列長 [dword], [byte] */
#define CPKD_STRING_LEN			(4)
#define CPKD_STRING_LEN_BYTE	(4 * CPKD_STRING_LEN)


/*------------------------- 《処理マクロ》 -------------------------*/
/* 作成パラメータへのアクセスマクロ */
#define	CPK_PARA_WORK_ADDR(para)	((para)->work_addr)
#define	CPK_PARA_WORK_SIZE(para)	((para)->work_size)
#define	CPK_PARA_BUF_ADDR(para)		((para)->buf_addr)
#define	CPK_PARA_BUF_SIZE(para)		((para)->buf_size)
#define	CPK_PARA_PCM_ADDR(para)		((para)->pcm_addr)
#define	CPK_PARA_PCM_SIZE(para)		((para)->pcm_size)

/* ハンドルアクセス */
#define	CPK_HN_DONT_DEC_VIDEO(hn) ((*(CpkWork **)(hn))->status.dont_dec_video)

/* Code1Bookのサイズ [byte],[dbyte] */
#define CPK_BSIZE_CODE1BOOK				(0x800)
#define CPK_BSIZE_CODE1BOOK_FUL			(2 * CPK_BSIZE_CODE1BOOK)
#define CPK_DSIZE_CODE1BOOK				(CPK_BSIZE_CODE1BOOK     / 4)
#define CPK_DSIZE_CODE1BOOK_FUL			(CPK_BSIZE_CODE1BOOK_FUL / 4)

/* ２４ｂｉｔフルカラー用のワークバッファサイズ */
#define	CPK_24WORK_BSIZE	sizeof(CpkWork)
#define	CPK_24WORK_DSIZE	((CPK_24WORK_BSIZE + 3) / 4)

/* １５ｂｉｔカラー用のワークバッファサイズ */
#define	CPK_15WORK_BSIZE	(sizeof(CpkWork) - (4 * CPK_BSIZE_CODE1BOOK))
#define	CPK_15WORK_DSIZE	((CPK_15WORK_BSIZE + 3) / 4)

/* CPKDシステム構造体アクセス */
#define CPKD_SYS_STRING			(cpkd_sys.string)
#define CPKD_SYS_MODE			(cpkd_sys.mode)
#define CPKD_SYS_START_ADDR		(cpkd_sys.start_addr)
#define CPKD_SYS_END_ADDR		(cpkd_sys.end_addr)
#define CPKD_SYS_PRINT_ADDR		(cpkd_sys.print_addr)
#define CPKD_SYS_RETURN_CNT		(cpkd_sys.return_cnt)


/*-------------------------- 《データ型》 --------------------------*/
/* ムービハンドル */
typedef void *CpkHn;

/* 作成パラメータ */
typedef struct {
	Uint32		*work_addr;		/* ワークの先頭アドレス */
								/* 本ライブラリがムービを再生する時の作業領域*/
	Uint32		work_size;		/* ワークのバイト数 */
								/* CPK_BSIZE_WORK を指定する */
	Uint32		*buf_addr;		/* ﾑｰﾋﾞﾌｧｲﾙを格納するバッファの先頭アドレス */
								/* メモリのムービファイルの時はそのアドレス */
								/* CDのムービファイルの時はﾘﾝｸﾞﾊﾞｯﾌｧのｱﾄﾞﾚｽ */
	Uint32		buf_size;		/* バッファサイズ */
								/* メモリのムービファイルの時はファイルサイズ*/
								/* CDのムービファイルの時はﾘﾝｸﾞﾊﾞｯﾌｧのサイズ */
	Uint16		*pcm_addr;		/* サウンドメモリのPCMバッファアドレス 		*/
	Uint32		pcm_size;		/* サウンドメモリのPCMバッファサイズ		*/
								/* (1ch分のサンプル数(4096*1..4096*16))		*/
} CpkCreatePara;


/* フィルムサンプルテーブル項目 */
typedef struct {
	Uint32		offset;				/* メディア開始位置からのオフセット */
	Sint32		size;				/* サンプルサイズ 					*/
	Sint32		time;				/* フレーム表示時刻、-1:ｵｰﾃﾞｨｵｻﾝﾌﾟﾙ */
	Sint32		duration;			/* フレーム表示持続時間 			*/
} CpkFilmSample;


/* フィルムヘッダ */
typedef struct {
	/* FilmHeader */
	Uint32		film;				/* "FILM" 								*/
	Uint32		size_header;		/* ヘッダサイズ(メディア開始へのｵﾌｾｯﾄ)	*/
	Uint32		version;			/* バージョン 							*/
	Uint32		reserved;			/* 予約 								*/
	/* FrameDescription */
	Uint32		fdsc;				/* "FDSC" 								*/
	Uint32		size_fdsc;			/* FDSCサイズ 0x0000001C = 7*4[byte]	*/
	CpkCType	c_type;				/* 圧縮方式 "cvid" 						*/
	Uint32		height;				/* たて幅[pixel]						*/
	Uint32		width;				/* よこ幅[pixel] 						*/
	Uint8		color;				/* 表示色数[bit] 						*/
	Uint8		sound_channel;		/* チャネル数 							*/
	Uint8		sound_smpling_bit;	/* サンプリングビット数[bit] 			*/
	Uint8		sound_compress;		/* 圧縮 00h:無し 01h:ADPCM 				*/
	Uint32		sound_smpling_rate;	/* ｻﾝﾌﾟﾘﾝｸﾞﾚｰﾄ[Hz] 下位２byteは小数 	*/
	Uint32		ckey;				/* クロマキー情報	 					*/
	/* FilmSampleTable */
	Uint32		stab;				/* "STAB" 								*/
	Uint32		size_stab;			/* STABサイズ (総ｻﾝﾌﾟﾙ数+1)*16[byte] 	*/
	Uint32		time_scale_film;	/* タイムスケール 						*/
	Uint32		sample_total;		/* 総サンプル数 						*/
	CpkFilmSample sample[1];		/* サンプルテーブル 任意サイズ 			*/
} CpkHeader;

/* エラー登録関数 */
typedef void (*CpkErrFunc)(void *obj, Sint32 err_code);


/* パラメータ */
typedef struct {
	Uint32		AddrBuf;		/* バッファ開始位置。シムの場合 0x4000000 	*/
	Uint32		SizeBuf;		/* バッファサイズ。　シムの場合 0x1000000 	*/
	Uint32		size_trigger;	/* 再生開始トリガサイズ						*/
	Uint32		AddrVram;		/* 画像出力アドレス 						*/
	Uint32		SizeVramLine;	/* 画像出力バッファ行サイズ[byte]			*/
	/* サウンドドライバ関連 */
	Uint32		play_pcm;		/* PCM再生フラグ 0:音の処理なし 1:再生		*/
	Uint32		command_blk_no;	/* サウンドドライバコマンドブロック番号 	*/
	Uint32		pcm_stream_no;	/* PCMストリーム再生番号 					*/
	Uint16		*addr_pcmbuf;	/* ｻｳﾝﾄﾞﾒﾓﾘのPCMバッファアドレス 			*/
	Uint32		size_pcmbuf;	/* ｻｳﾝﾄﾞﾒﾓﾘのPCMバッファサイズ[sample/1ch] 	*/
	Uint32		pcm_pan;		/* 0..31	*/
	Uint32		pcm_level;		/* 0..7		*/
	Uint32		frequency_vbl;	/* CPK_VblIn()をコールする周波数[256*Hz] 	*/
} CpkPara;

/* DMA転送情報 */
typedef struct {
	Uint32		dst;				/* 転送先アドレス 						*/
	Uint32		src;				/* 転送元アドレス 						*/
	Uint32		cnt;				/* 転送回数 							*/
} CpkDmaStock;

/* 各種状態 */
typedef struct {
	/* 全体制御情報 */
	Sint32		stat_start;			/* ステータスの開始						*/
	Sint32		play;				/* 再生状態 							*/
	Uint32		RequestDispFrame;	/* フレーム表示要求フラグ 				*/
	Uint32		CntSample;			/* サンプルカウンタ 					*/
	Uint32		CntFrame;			/* フレームカウンタ 					*/
	Uint32		offset_media;		/* メディア開始位置へのオフセット		*/
	Uint32		display_keyframe;	/* 表示画像キーフレームフラグ 			*/
	Uint32		standby_keyframe;	/* 待機画像キーフレームフラグ 			*/
	Uint32		standby_video_flag;	/* 表示待機画像ありフラグ 				*/
	Uint32		standby_time_display;/* 待機画像の表示時刻					*/
	Uint32		standby_time_duration;/* 待機画像の表示持続時間				*/
	Uint32		only_audio;			/* flag that only audio sample 			*/
	Uint32		only_video;			/* flag that only video sample 			*/
	Uint32		color;				/* 出力色数[bit]。15 or 24				*/
	Uint32		wait_keyframe_to_pause;/* 一時停止処理用ｷｰﾌﾚｰﾑ待ちフラグ 	*/
	Uint32		wait_and_decode;	/* 1:表示時刻を待ってから伸張する		*/
	Sint32		flag_prohibit_komaotoshi;/* 1:コマ落としkinnshiフラグ		*/
	Sint32		ckey_cutoff;		/* chroma key cutoff level 				*/
	Sint32		final_audio_sample;	/* 最後のオーディオサンプル 			*/
	Sint32		silence_rate;		/* 無音率 [1/s]							*/
	
	/* リングバッファ制御情報 */
	Sint32		ring_start;			/* の開始								*/
	Uint32		OffsetWrite;		/* バッファ読み・書き位置[byte]			*/
	Uint32		OffsetRead;			/* (ファイル先頭からのオフセット)		*/
	Uint32		AddrWrite;			/* バッファ読み・書き位置(アドレス)		*/
	Uint32		AddrRead;			/* 										*/
	Uint32		AddrRing;			/* リングバッファ開始アドレス 			*/
	Uint32		AddrRingEnd;		/* リングバッファ終了アドレス 			*/
	Sint32		SizeHead;			/* ヘッダバッファサイズ [byte] ｾｸﾀ整数倍 */
	Uint32		SizeRing;			/* リングバッファサイズ [byte] ｾｸﾀ整数倍 */
	Uint32		SizeFlop;			/* ＦＬＯＰバッファサイズ [byte] 		*/
	
	/* PCM再生情報 */
	Sint32		audio_start;		/* の開始								*/
	Uint32		play_pcm;			/* PCM再生フラグ 0:音の処理なし 1:再生	*/
	Uint32		bsize_pcmbuf;		/* ｻｳﾝﾄﾞﾒﾓﾘのPCMバッファサイズ[byte/1ch]*/
	Uint16		*addr_pcmbuf_end;	/* PCMバッファ終了アドレス 				*/
	Uint16		*addr_pcm_write;	/* PCMバッファ書き込みアドレス 			*/
	Uint32		smpling_rate;		/* ｻﾝﾌﾟﾘﾝｸﾞﾚｰﾄ[Hz] 下位１byteは小数 	*/
	Uint32		cnt_4ksample;		/* PCM play address 4kｻﾝﾌﾟﾙ単位更新ｶｳﾝﾀ */
	Sint32		flag_fill_silence;	/* 無音設定済みフラグ					*/
	Sint32		sample_write;		/* 供給サンプル数[sample]				*/
									/* (先頭ﾌｨﾙﾑからの現在までの総供給量)	*/
	Sint32		sample_film_start;	/* フィルム開始時サンプル数[sample]		*/
									/* (先頭ﾌｨﾙﾑから現在ﾌｨﾙﾑ開始時点までの)	*/
	Sint32		sample_pause;		/* ポーズ時サンプル数[sample]			*/
									/* (先頭ﾌｨﾙﾑから最近のポーズ時点までの)	*/
	Sint32		count_start;		/* ＣＰＵクロックタイマ開始時刻 		*/
									/* 						[CPUｸﾛｯｸ>>15] 	*/
	Sint32		clock_scale;		/* ＣＰＵクロックタイマのスケール 		*/
	Sint32		dont_stop_pcm;		/* 終了時にPCMストップしないフラグ 		*/

	/* 次ムービー管理情報 */
	Sint32		next_start;			/* の開始								*/
/*	CpkHn		hn_next;*/			/* 次ハンドル(未登録，開始後は NULL) 	*/
	CpkHn		hn_next_start;		/* 次ハンドル(開始後) 					*/
	Sint32		dont_dec_video;		/* 終了時にPCMストップしないフラグ 		*/
	
	
	/* PCMデータのＤＭＡ転送に関する情報 */
	Sint32		cppcm_start;		/* の開始								*/
	CpkTrMode	copy_mode_pcm;		/* データの転送方式 					*/
	Uint32		flag_dma_hold;		/* 1:このムービーがDMA資源を保持している*/
	Uint32		pcm_renew_size;		/* DMA終了時pcmﾊﾞｯﾌｧ書き込みﾎﾟｲﾝﾀ更新ｻｲｽﾞ*/
	Uint32		buf_renew_size;		/* DMA終了時ﾊﾞｯﾌｧ読み取りﾎﾟｲﾝﾀ更新ｻｲｽﾞ 	*/
	Uint32		idx_dma_pcm_tbl;	/* dma_pcm_tbl のインデックス 			*/
	CpkDmaStock	dma_pcm_tbl[4];		/* DMA転送情報エントリテーブル 			*/
	
	/* 時間管理情報(ビデオのみムービー用) */
	Sint32		time_start;			/* の開始								*/
									/* 95-08-17 Y.T : CPK_SetSpeed 対応 	*/
									/*   Vblｶｳﾝﾀをﾊﾝﾄﾞﾙ毎に持つ				*/
	Sint32		cnt_vbl_in;			/* Vlb-In 割り込みカウンタ[vbl]			*/
	Sint32		vblcnt_regulator;	/* Vlb-In カウンタ再生速度調節器		*/
	Sint32		vbl_film_start;		/* フィルム開始時刻[vbl] 				*/
	Sint32		vbl_pause;			/* ポーズ開始時刻[vbl] 					*/
	Sint32		vbl_pcm_start;		/* PCM開始時刻[vbl]
									 * 最初の頃に cnt_4ksample を誤ってｶｳﾝﾄ
									 * ｱｯﾌﾟしないように。
									 */
	Sint32		time_frame;			/* 表示したフレームの表示時刻＋持続時間	*/

	/* 再生速度管理情報 */
	Sint32		speed;				/* 再生速度[対等速比×1024] 			*/
	Bool		audio_sw;			/* 再生速度変更時のｵｰﾃﾞｨｵ出力ｽｲｯﾁ		*/

	/* エラー情報 */
	Uint32		err_start;			/* エラー情報の開始                     */
	Uint32		CntLossFrame;		/* コマ落ちカウンタ 					*/
	Uint32		CntPcmLoadMiss;		/* PCMロードミスカウンタ 				*/
	Uint32		CntCallTask;		/* タスクコールカウンタ 				*/
	Uint32		CntOverCallTask;	/* 過剰タスクコールカウンタ 			*/
	Uint32		cnt_buf_empty;		/* バッファ空タスクカウンタ 			*/
	Uint32		max_late_time;		/* 最大遅れ時間[フィルム時間単位] 		*/
	Uint32		max_late_frame;		/* 最大遅れフレーム番号					*/
} CpkStatus;


/* ファイルアクセス関係 */
typedef struct {
	GfsHn		gfs;			/* ファイルハンドル */
	Sint32		load_sect;		/* 転送最大セクタ数 */
	Bool		called_cdread;	/* GFS_NwCdRead をコールしたら TRUE */
	Bool		exec_one_state;	/* ExecOne 実行中なら TRUE */
	Sint32		exec_load_size;	/* ExecOne で読み込むバイト数 */
	Sint32		now_load_size;	/* ExecOne で読み込んでいるバイト数 */
	Sint32		load_total_sect;/* 読み込み総セクタ数 */
	Sint32		file_sect;		/* ファイルのセクタ数 */
	Sint32		tr_mode;		/* 転送モード */
} CpkFileGfsPara;

typedef struct {
	StmHn		stm;			/* ストリームハンドル */
	Sint32		load_sect;		/* 転送最大セクタ数 */
	Sint32		old_cd_buf_num;	/* 前回のＣＤバッファの数 */
	Bool		dma_state;		/* ＤＭＡ転送しているなら TRUE */
	Sint32		dma_sect;		/* ＤＭＡ転送するセクタ数 */
	Uint32		*write_addr;	/* 書き込むアドレス */
	Sint32		buf_bsize;		/* 空きバッファのバイト数 */
	Sint32		write_bsize;	/* 転送したバイト数 */
	Sint32		load_total_sect;/* 読み込み総セクタ数 */
	StmTrFunc	load_func;		/* 転送関数 */
} CpkFileStmPara;

typedef struct {
	Sint32		faccess_type;		/* ファイルアクセスタイプ */
	/* ファイルローカルデータ */
	union	{
		CpkFileGfsPara	gfs;
		CpkFileStmPara	stm;
	} data;
} CpkFilePara;

/* 実行関数 */
typedef struct {
	void	(*start)(CpkHn cpk);
	void	(*task)(CpkHn cpk);
	Sint32	(*preload_movie)(CpkHn cpk, Sint32 size);
	void	(*set_load_num)(CpkHn cpk, Sint32 load_sct);
	void	(*set_trmode)(CpkHn cpk, CpkTrMode mode);
	void	(*get_stock_size)(CpkHn cpk, Sint32 *ring_bsize, Sint32 *all_bsiz);
	void	(*get_stock_time)(CpkHn cpk, Sint32 *ring_time, Sint32 *all_time);
} CpkExecFunc;


typedef struct {
	Sint32		work_start;				/* ワークの開始					*/
	CpkPara 	para;					/* Ｃｉｎｅｐａｋ起動パラメータ */
	CpkStatus 	status;					/* 各種状態（公開部分） 		*/
	CpkHeader 	*header;				/* フィルムファイルヘッダ 		*/
	CpkFilePara	filepara;				/* ファイルアクセスパラメータ	*/
	CpkExecFunc	execfunc;				/* 実行関数						*/
#ifdef CPK_USE_CBTBL
	Uint32		*cbtblp[4];				/* ｺｰﾄﾞﾌﾞｯｸｱﾄﾞﾚｽﾃｰﾌﾞﾙ中央ﾎﾟｲﾝﾀ 	*/
	Uint32		cbtbl1a[256];			/* ｺｰﾄﾞﾌﾞｯｸｱﾄﾞﾚｽﾃｰﾌﾞﾙ 			*/
	Uint32		cbtbl2a[256];			/* 								*/
	Uint32		cbtbl1b[256];			/* 								*/
	Uint32		cbtbl2b[256];			/* 								*/
#endif
	Uint32 		code_book[4 * CPK_DSIZE_CODE1BOOK_FUL];
										/* 上段 code1book, code2book	*/
										/* 下段 code1book, code2book	*/
} CpkWork;


/* CPKDシステム構造体 */
typedef struct {
	Sint32 string[CPKD_STRING_LEN];	/* 識別文字列 */
	Sint32 mode;					/* 出力モード ０：出力しない(デフォルト) */
	Sint32 *start_addr;				/* 出力アドレスの先頭 */
	Sint32 *end_addr;				/* 出力アドレスの末尾 */
	Sint32 *print_addr;				/* 現在の出力アドレス */
	Sint32 return_cnt;				/* 折り返しカウンタ */
} CpkdSys;


/*------------------------ 《デバッグ用宣言》 ------------------------*/

/* CPKDシステム構造体 */
extern CpkdSys cpkd_sys;

/* デバッグ出力モードの設定　0N:出力する, OFF:出力しない(ﾃﾞﾌｫﾙﾄ) */
void CPKD_SetMode(Bool mode);

/* デバッグ出力アドレスの設定 */
void CPKD_SetPrintAddr(Sint32 *start, Sint32 *end);

/* デバッグ文字列の出力(文字列長は４の整数倍に規格化される) */
void CPKD_PrintStr(char *str);

/* デバッグデータの出力(４バイト) */
void CPKD_PrintData(Sint32 data);


/*-------------------------- 《関数宣言》 --------------------------*/

/* ライブラリの初期化 */
Bool CPK_Init(void);

/* ライブラリの終了処理 */
void CPK_Finish(void);

/* ハンドルの作成（メモリ） */
CpkHn CPK_CreateMemMovie(CpkCreatePara *para);

/* ハンドルの消去（メモリ） */
void CPK_DestroyMemMovie(CpkHn cpk);

/* ハンドルの作成（ファイルシステム） */
CpkHn CPK_CreateGfsMovie(CpkCreatePara *para, GfsHn gfs);

/* ハンドルの消去（ファイルシステム） */
void CPK_DestroyGfsMovie(CpkHn cpk);

/* ハンドルの作成（ストリームシステム） */
CpkHn CPK_CreateStmMovie(CpkCreatePara *para, StmHn stm);

/* ハンドルの消去（ストリームシステム） */
void CPK_DestroyStmMovie(CpkHn cpk);

/* 再生タスク */
void CPK_Task(CpkHn cpk);

/* 表示タイミングの判定 */
Bool CPK_IsDispTime(CpkHn cpk);

/* 表示フレーム種別の判定 */
Bool CPK_IsKeyframe(CpkHn cpk);

/* 表示完了の通知 */
void CPK_CompleteDisp(CpkHn cpk);

/* ＶブランクＩＮ処理関数 */
void CPK_VblIn(void);

/* 再生開始 */
void CPK_Start(CpkHn cpk);

/* 再生停止 */
void CPK_Stop(CpkHn cpk);

/* 一時停止 */
Bool CPK_Pause(CpkHn cpk, CpkPauseCmd cmd);

/* 次に再生するムービの登録 */
void CPK_EntryNext(CpkHn cpk);

/* ムービの強制切り替え */
void CPK_Change(void);

/* ムービの切り替え状態の取得 */
CpkChangeStatus CPK_CheckChange(void);

/* 指定フレームの展開 */
void CPK_DecodeFrame(CpkHn cpk, Sint32 sample_no);

/* 指定フレームの展開(差分フレーム保証) */
void CPK_DecodeFrameSafety(CpkHn cpk, Sint32 sample_no);

/* 展開先アドレスの設定 */
void CPK_SetDecodeAddr(CpkHn cpk, void *dec_addr, Sint32 dec_line_size);

/* 表示色数の設定 */
void CPK_SetColor(CpkHn cpk, CpkColorType color);

/* 最大転送セクタ数の設定 */
void CPK_SetLoadNum(CpkHn cpk, Sint32 read_sct);

/* 再生ＰＡＮの設定 */
void CPK_SetPan(CpkHn cpk, Sint32 pan);

/* 再生ボリュームの設定 */
void CPK_SetVolume(CpkHn cpk, Sint32 volume);

/* 再生速度の設定 */
void CPK_SetSpeed(CpkHn cpk, Sint32 speed, Bool audio_sw);

/* ＰＣＭコマンドブロック番号の設定 */
/* void CPK_SetPcmCmdBlockNo(CpkHn cpk, Sint32 blk_no); */

/* ＰＣＭストリーム番号の設定 */
void CPK_SetPcmStreamNo(CpkHn cpk, Sint32 stream_no);

/* キーアウト（透明にする色）範囲の設定 */
void CPK_SetKeyOutRange(CpkHn cpk, Sint32 range);

/* 無音率の設定 */
void CPK_SetSilenceRate(CpkHn cpk, Sint32 silence_rate);

/* データの転送方式の設定（ＣＤブロック→リングバッファ） */
void CPK_SetTrModeCd(CpkHn cpk, CpkTrMode mode);

/* データの転送方式の設定（リングバッファ→ＰＣＭバッファ） */
void CPK_SetTrModePcm(CpkHn cpk, CpkTrMode mode);

/* 再生開始トリガサイズの設定 [byte] */
void CPK_SetStartTrgSize(CpkHn cpk, Sint32 bsize);

/* タイムスケールの取得 */
Sint32 CPK_GetTimeScale(CpkHn cpk);

/* 再生時刻の取得 [timescale] */
Sint32 CPK_GetTime(CpkHn cpk);

/* 再生ステータスの取得 */
CpkPlayStatus CPK_GetPlayStatus(CpkHn cpk);

/* フィルムヘッダのアドレスの取得 */
CpkHeader *CPK_GetHeader(CpkHn cpk);

/* 書き込みバッファアドレスの取得 */
Uint32 *CPK_GetWriteBuf(CpkHn cpk, Sint32 *free_size, Sint32 *free_total);

/* バッファへの書き込みサイズの通知 */
void CPK_NotifyWriteSize(CpkHn cpk, Sint32 write_size);

/* メモリにムービを読み込む */
Sint32 CPK_PreloadMovie(CpkHn cpk, Sint32 size);

/* メモリにフィルムヘッダを読み込む */
Sint32 CPK_PreloadHeader(CpkHn cpk);

/* エラー関数の登録 */
void CPK_SetErrFunc(CpkErrFunc func, void *obj);

/* エラー情報の取得 */
CpkErrCode CPK_GetErr(void);

/* SCU-DSP資源の確保 */
Sint32 CPK_AllocScuDsp(void);

/* SCU-DSP資源の解放 */
void CPK_FreeScuDsp(void);

/* ＣＰＵ使用条件の設定 */
void CPK_SetCpu(CpkCpu cpu);

/* ストリームサーバ関数のコールの設定 */
void CPK_SetStmServerCall(Bool mode);

/* データ備蓄量の取得 [byte] */
/* ＳＧＬのＧＦＳのバージョンがｖｅｒ１．２１以上の場合のみ使用可能 */
void CPK_GetStockSize(CpkHn cpk, Sint32 *ring_bsize, Sint32 *all_bsize);

/* データ備蓄量の取得 [timescale] */
/* ＳＧＬのＧＦＳのバージョンがｖｅｒ１．２１以上の場合のみ使用可能 */
void CPK_GetStockTime(CpkHn cpk, Sint32 *ring_time, Sint32 *all_time);

#endif /* SEGA_CPK_H */
