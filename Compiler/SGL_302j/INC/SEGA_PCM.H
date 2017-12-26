/******************************************************************************
 *	ソフトウェアライブラリ
 *
 *	Copyright (c) 1994,1995 SEGA
 *
 * Library	:ＰＣＭ・ＡＤＰＣＭ再生ライブラリ
 * Module 	:ライブラリヘッダ
 * File		:sega_pcm.h
 * Date		:1995-03-31
 * Version	:1.16
 * Auther	:Y.T
 *
 ****************************************************************************/
#if !defined(SEGA_PCM_H)
#define SEGA_PCM_H

/* from Ver1.10, adpcm function support */
#define ADPCM_FUNCTION

/*-------------------- 《インクルードファイル》 --------------------*/
#include "sega_xpt.h"
#include "sega_gfs.h"
#include "sega_stm.h"

/*------------------------- 《マクロ定数》 -------------------------*/

/* バージョン */
#define PCM_VERSION				("PCM Version 1.16 1995-03-31")
							/*	           1         2         3         4	*/
							/*	  1234567890123456789012345678901234567890	*/
							/*	  ++++++++++++++++++++++++++++++++			*/
#define PCM_VERSION_LEN			(32)

/* ハンドル総数 */
#define PCM_HN_MAX				(32)

/* 1024, 2048, 4096,... */
#define PCM_SIZE_1K				(1024)
#define PCM_SIZE_2K				(2 * PCM_SIZE_1K)
#define PCM_SIZE_3K				(3 * PCM_SIZE_1K)
#define PCM_SIZE_4K				(4 * PCM_SIZE_1K)
#define PCM_SIZE_8K				(8 * PCM_SIZE_1K)

/* フラグ */
typedef enum {
    PCM_OFF = 0,
    PCM_ON  = 1
} PcmFlag;

/* エラーコード	*/
typedef enum {
	PCM_ERR_OK					= 0x00000000,/* エラーは発生していない		*/

	/* PCM_Create のエラー */
	PCM_ERR_OUT_OF_HANDLE		= 0x00000101,/* ハンドル売り切れ 			*/
	PCM_ERR_NO_INIT				= 0x00000103,/* 初期化関数がｺｰﾙされてない 	*/

	/* PCM_Destroy のエラー */
	PCM_ERR_INVALID_HN			= 0x00000201,/* 不正なハンドルで消去		*/
	PCM_ERR_ILL_CREATE_MODE		= 0x00000202,/* Create 時のモードと違う		*/

	/* pcm_HeaderProcess のエラー */
	PCM_ERR_TOO_LARGE_HEADER	= 0x00000302,/* ヘッダがき大すぎる			*/
											/* (バッファサイズが小さすぎる)	*/
	PCM_ERR_HEADER_DATA			= 0x00000303,/* ヘッダデータエラー 			*/
	PCM_ERR_TOO_SMALL_PCMBUF	= 0x00000304,/* PCMﾊﾞｯﾌｧｻｲｽﾞが小さすぎる	*/
	PCM_ERR_ILL_SIZE_PCMBUF		= 0x00000305,/* PCMﾊﾞｯﾌｧｻｲｽﾞ不正 4096*2～16	*/
	PCM_ERR_AFI_NO_COMMON		= 0x00000306,/* CommonChunk が無い 			*/
	PCM_ERR_AFI_COMMPRESS		= 0x00000307,/* 未対応圧縮タイプ			*/
	PCM_ERR_NOT_DECLARE_ADPCM	= 0x00000308,/* ADPCM使用宣言がない			*/

	/*  */
	PCM_ERR_ILLEGAL_PARA		= 0x00000503,/* 引き数の指定が間違っている 	*/
	PCM_ERR_ILLEGAL_HANDLE		= 0x00000504,/* ハンドルが異常 				*/

	/* 連続再生に関するエラー */
	PCM_ERR_NEXT_HN_STATUS		= 0x00000601,/* 連続再生ハンドルの状態異常 	*/
	PCM_ERR_NEXT_HN_AUDIO		= 0x00000602,/* オーディオの条件が一致しない */
	PCM_ERR_CHANGE_NO_ENTRY		= 0x00000605,/* エントリがない状態でチェンジ */

	/* ポーズに関するエラー */
	PCM_ERR_PAUSE_STATUS		= 0x00000701,/* PCM_STAT_PLAY_TIME, または、
											 * PCM_STAT_PLAY_PAUSE 以外の
											 * 状態でポーズ関数をコールした。
											 */
	PCM_ERR_PAUSE_WORK_NULL		= 0x00000702,/* ポーズ処理用ワーク異常		*/
	PCM_ERR_PAUSE_WORK_SIZE		= 0x00000703,/* ポーズ処理用ワーク異常		*/
	PCM_ERR_PAUSE_WORK_SET		= 0x00000704,/* ポーズ処理用ワーク指定が不正*/

	/* DMA転送に関するエラー */
	PCM_ERR_DMA_MODE			= 0x00000801,/* 未対応の転送方式 			*/
	PCM_ERR_DMA_CPU_PCM			= 0x00000802,/* ＤＭＡが異常終了 			*/

	/* ファイル読み込みエラー */
	PCM_ERR_GFS_READ			= 0x00000901,/* ＧＦＳの読み込みで失敗 		*/

	/* リングバッファへのデータ供給に関するエラー */
	PCM_ERR_RING_SUPPLY			= 0x00000a01,/* 再生完了後に供給があった    */
											 /* （供給が間に合わす中断した）*/

	PCM_ERR_END
} PcmErrCode;

/* 再生ステータス */
typedef enum {
	PCM_STAT_PLAY_ERR_STOP		= -1,		/* 異常停止					*/
	PCM_STAT_PLAY_CREATE		= 0,		/* 初期状態					*/
	PCM_STAT_PLAY_PAUSE			= 1,		/* 一時停止		 			*/
	PCM_STAT_PLAY_START			= 2,		/* 開始						*/
	PCM_STAT_PLAY_HEADER		= 3,		/* ヘッダ設定　				*/
	PCM_STAT_PLAY_TIME			= 4,		/* 再生(タイマスタート)		*/
	PCM_STAT_PLAY_END			= 5			/* 再生完了 				*/
} PcmPlayStatus;

/* ファイルタイプ */
typedef enum {
	PCM_FILE_TYPE_NO_HEADER		= 1,	/* ヘッダなし。
										 * (PCMﾗｲﾌﾞﾗﾘが未対応のﾌｧｲﾙ形式
										 *  をﾒﾓﾘ再生させる場合も含む)
										 */
	PCM_FILE_TYPE_AIFF			= 2,	/* AIFF							*/
	PCM_FILE_TYPE_AIFC			= 3 	/* AIFC							*/
} PcmFileType;
/* [注意] AIFCは使用できません。*/

/* データタイプ */
typedef enum {
	PCM_DATA_TYPE_LRLRLR		= 1,	/* １サンプルづつ左右左右････	*/
	PCM_DATA_TYPE_RLRLRL		= 2,	/* １サンプルづつ右左右左････	*/
	PCM_DATA_TYPE_LLLRRR		= 3,	/* ブロック単位で左左右右････	*/
	PCM_DATA_TYPE_RRRLLL		= 4, 	/* ブロック単位で右右左左････	*/
	PCM_DATA_TYPE_ADPCM_SG		= 5, 	/* ADPCM,ｻｳﾝﾄﾞｸﾞﾙｰﾌﾟの連続データ*/
	PCM_DATA_TYPE_ADPCM_SCT		= 6 	/* ADPCM,1ｾｸﾀ18ｻｳﾝﾄﾞｸﾞﾙｰﾌﾟ		*/
} PcmDataType;

/* 一時停止の制御コマンド */
typedef enum {
	PCM_PAUSE_ON_AT_ONCE,				/* 即時一時停止 			*/
	PCM_PAUSE_OFF						/* 一時停止解除 			*/
} PcmPauseCmd;

/* 強制切り替え可能性チェック値 */
typedef enum {
	PCM_CHANGE_OK_AT_ONCE		= 0,	/* 即時切り替え可能。			*/
	PCM_CHANGE_NO_DATA			= 1,	/* データ供給不足。				*/
	PCM_CHANGE_NO_ENTRY			= 2,	/* エントリが無い。				*/
	PCM_CHANGE_TOO_LARGE_HEADER	= 3		/* ヘッダがき大すぎる。			*/
										/* (バッファサイズが小さすぎる)	*/
} PcmChangeStatus;

/* データの転送方式 */
typedef enum {
	PCM_TRMODE_CPU = 0,					/* ソフトウェア転送 			*/
	PCM_TRMODE_SDMA,					/* ＤＭＡサイクルスチール 		*/
	PCM_TRMODE_SCU						/* ＳＣＵのＤＭＡ				*/
} PcmTrMode;

/* [注意] 
	リングバッファ→ＰＣＭバッファの転送は、ソフトウェア転送に固定です。
*/

/*------------------------- 《デフォルト値》 -------------------------*/

/* １回のタスクで処理する量の上限 [sample/1ch] */
#define PCM_DEFAULT_SAMPLE_1TASK		(PCM_SIZE_1K)

/* 再生開始トリガサイズ[byte] */
#define PCM_DEFAULT_SIZE_START_TRG		(0)

/* 再生開始トリガサンプル[sample/1ch] */
#define PCM_DEFAULT_SAMPLE_START_TRG	(PCM_SIZE_4K)

/* 再生停止トリガサンプル[sample/1ch] */
#define PCM_DEFAULT_SAMPLE_STOP_TRG		(PCM_SIZE_2K)


/*------------------------- 《処理マクロ》 -------------------------*/

/* ４ＢＹＴＥ文字コードを Uint32 型で表現する。
 *   ('cvid'などと書くとﾜｰﾆﾝｸﾞが出てしまうので、これを回避するために)
 */
#define PCM_4BYTE_CHAR(a,b,c,d)					\
	(	((Uint32)((a) & 0x000000FF) << 24) | 	\
		((Uint32)((b) & 0x000000FF) << 16) | 	\
		((Uint32)((c) & 0x000000FF) <<  8) | 	\
		 (Uint32)((d) & 0x000000FF)				\
	)

/* SEGA_PCM conpatible */
#define PCM_PARA_WORK_ADDR(para)	PCM_PARA_WORK(para)
#define PCM_PARA_BUF_ADDR(para)		PCM_PARA_RING_ADDR(para)
#define PCM_PARA_BUF_SIZE(para)		PCM_PARA_RING_SIZE(para)
#define PCM_CreateMemMovie			PCM_CreateMemHandle
#define PCM_DestroyMemMovie			PCM_DestroyMemHandle

/*-------------------- 《構造体アクセスマクロ》 --------------------*/

/* 作成パラメータへのアクセスマクロ */
#define PCM_PARA_WORK(para)			((para)->work)
#define PCM_PARA_RING_ADDR(para)	((para)->ring_addr)
#define PCM_PARA_RING_SIZE(para)	((para)->ring_size)
#define PCM_PARA_PCM_ADDR(para)		((para)->pcm_addr)
#define PCM_PARA_PCM_SIZE(para)		((para)->pcm_size)

/* 再生情報の設定パラメータへのアクセスマクロ */
#define PCM_INFO_FILE_TYPE(info)		((info)->file_type)
#define PCM_INFO_DATA_TYPE(info)		((info)->data_type)
#define PCM_INFO_FILE_SIZE(info)		((info)->file_size)
#define PCM_INFO_CHANNEL(info)			((info)->channel)
#define PCM_INFO_SAMPLING_BIT(info)		((info)->sampling_bit)
#define PCM_INFO_SAMPLING_RATE(info)	((info)->sampling_rate)
#define PCM_INFO_SAMPLE_FILE(info)		((info)->sample_file)
#define PCM_INFO_COMPRESSION_TYPE(info)	((info)->compression_type)

/* ハンドル(ワーク)のアクセス */
#define PCM_HN_START_TRG_SIZE(hn)	(((*(PcmWork **)hn))->para.start_trg_size)
#define PCM_HN_START_TRG_SAMPLE(hn)	(((*(PcmWork **)hn))->para.start_trg_sample)
#define PCM_HN_STOP_TRG_SAMPLE(hn)	(((*(PcmWork **)hn))->para.stop_trg_sample)
#define PCM_HN_CNT_LOOP(hn)			(((*(PcmWork **)hn))->status.cnt_loop)


/*-------------------------- 《データ型》 --------------------------*/
/* ハンドル */
typedef void *PcmHn;

/* エラー登録関数 */
typedef void (*PcmErrFunc)(void *obj, Sint32 err_code);

typedef struct {
	Sint8		command;
	Sint8		resered;
	Uint8		pad[14];				/* P1-14	*/
} PcmCmdBlk;

typedef struct {
	Sint8		command;
	Sint8		resered;
	Sint8		channel_bit_stm_no;		/* P1 		*/
	Sint8		level_pan;				/* P2 		*/
	Sint16		buf_addr;				/* P3-4 	*/
	Sint16		buf_size;				/* P5-6 	*/
	Sint16		pitch_word;				/* P7-8 	*/
	Uint8		effect_r_mono;			/* P9 		*/
	Uint8		effect_l;				/* P10 		*/
	Uint8		pad[4];					/* P11-14 	*/
} PcmStartPcm;

typedef struct {
	Sint8		command;
	Sint8		resered;
	Sint8		stm_no;					/* P1 		*/
	Uint8		pad[13];				/* P2-14	*/
} PcmStopPcm;

typedef struct {
	Sint8		command;
	Sint8		resered;
	Sint8		stm_no;					/* P1 		*/
	Sint8		level_pan;				/* P2 		*/
	Sint16		pitch_word;				/* P3-4 	*/
	Uint8		effect_r_mono;			/* P5 		*/
	Uint8		effect_l;				/* P6 		*/
	Uint8		pad[8];					/* P7-14 	*/
} PcmChangePcmPara;

typedef struct {
	Sint8		command;
	Sint8		resered;
	Sint8		area_no;				/* P1 		*/
	Uint8		pad[13];				/* P2-14 	*/
} PcmChangeMap;

typedef struct {
	Sint8		command;
	Sint8		resered;
	Sint8		total_volume;			/* P1 		*/
	Uint8		pad[13];				/* P2-14 	*/
} PcmTotalVolume;

typedef struct {
	Sint8		command;
	Sint8		resered;
	Sint8		effect_bank_no;			/* P1 		*/
	Uint8		pad[13];				/* P2-14 	*/
} PcmChangeEffect;

typedef struct {
	Sint8		command;
	Sint8		resered;
	Sint8		neiro_bank_no;			/* P1 		*/
	Sint8		mixer_no;				/* P2 		*/
	Uint8		pad[12];				/* P3-14 	*/
} PcmChangeMixer;

typedef struct {
	Sint8		command;
	Sint8		resered;
	Sint8		effect_out_select;		/* P1 		*/
	Sint8		effect_level_pan;		/* P2 		*/
	Uint8		pad[12];				/* P3-14 	*/
} PcmChangeMixerPara;

/* 最後に発行した各コマンドの内容 */
typedef struct {
	PcmStartPcm			start_pcm;
	PcmStopPcm			stop_pcm;
	PcmChangePcmPara	change_pcm_para;

/*	PcmChangeMap		change_map;
	PcmTotalVolume		total_volume;
	PcmChangeEffect		change_effect;
	PcmChangeMixer		change_mixer;
	PcmChangeMixerPara	change_mixer_para;
*/
} PcmCommand;

/* 再生情報 */
typedef struct {
	PcmFileType		file_type;			/* ファイルタイプ 					*/
	PcmDataType		data_type;			/* データタイプ 					*/
	Sint32			file_size;			/* ファイルサイズ[byte]
										 * これ以上のﾘﾝｸﾞﾊﾞｯﾌｧへの供給を許す
										 * が、その部分は処理しない。
										 */
	Sint32			channel;			/* チャネル数 						*/
	Sint32			sampling_bit;		/* サンプリングビット数 			*/
	Sint32			sampling_rate;		/* サンプリングレート[Hz] 			*/
	Sint32			sample_file;		/* ファイルのサンプル数[sample/1ch]	*/
	Sint32			compression_type;	/* 圧縮タイプ 						*/
} PcmInfo;

/* PCM転送情報 */
typedef struct {
	Sint8		*dst1;				/* 転送先アドレス1 						*/
	Sint8		*dst2;				/* 転送先アドレス2 						*/
	Sint8		*src;				/* 転送元アドレス 						*/
	Sint32		size;				/* 転送バイト数 						*/
} PcmCopyTbl;

/* 各種制御情報 */
typedef struct {
	/* 全体制御情報 */
	Sint32			stat_start;		/* 'STAT' ステータスの開始 				*/
	PcmPlayStatus	play;			/* 再生状態 							*/
	PcmFlag			ignore_header;	/* 1:ヘッダ情報無視 					*/
	PcmFlag			need_ci;		/* 1:サブヘッダからciを読む 			*/
	Sint32			cnt_loop;		/* ループ回数(カウントダウン) 			*/

	/* 再生情報 */
	Sint32		info_start;			/* 'INFO' 再生情報の開始 				*/
	PcmInfo		info;				/* 再生情報 							*/

	/* リングバッファ制御情報 */
	Sint32		ring_start;			/* 'RING' リングバッファ制御情報の開始 	*/
	Sint32		ring_write_offset;	/* リングバッファ書き込み位置[byte] 	*/
									/*	(ﾌｧｲﾙ先頭からのオフセット)			*/
	Sint32		ring_read_offset;	/* リングバッファ読み込みオフセット 	*/
									/*	(ﾌｧｲﾙ先頭からのオフセット)			*/
	Sint8		*ring_end_addr;		/* リングバッファ終了アドレス 			*/
	Sint8		*ring_write_addr;	/* リングバッファ書き込みアドレス 		*/
	Sint8		*ring_read_addr;	/* リングバッファ読み取りアドレス 		*/
	Sint32		media_offset;		/* ファイル先頭からデータまでのｵﾌｾｯﾄ 	*/

	/* PCM再生情報 */
	Sint32		pcm_start;			/* 'PCM ' PCM再生情報の開始 			*/
	Sint8		*pcm2_addr;			/* PCM第２ﾁｬﾝﾈﾙバッファ開始アドレス 	*/
	Sint32		pcm_bsize;			/* ｻｳﾝﾄﾞﾒﾓﾘのPCMバッファサイズ[byte/1ch]*/
	Sint32		pcm_write_offset;	/* PCMバッファ書き込み位置[byte]		*/
									/*  (PCMバッファの先頭からのオフセット)	*/
	Sint32		cnt_4ksample;		/* PCM play address 4kｻﾝﾌﾟﾙ単位更新ｶｳﾝﾀ */
	Sint32		sample_write_file;	/* 当ファイルの供給サンプル数[sample/1ch]*/
									/* (当ファイルの現在までの総供給量)		*/
	Sint32		sample_write;		/* 供給サンプル数[sample/1ch]			*/
									/* (先頭ﾌｨﾙﾑからの現在までの総供給量)	*/
	Sint32		sample_film_start;	/* フィルム開始時サンプル数[sample/1ch]	*/
									/* (先頭ﾌｨﾙﾑから現在ﾌｨﾙﾑ開始時点までの)	*/
	Sint32		sample_pause;		/* ポーズ時サンプル数[sample/1ch]		*/
									/* (先頭ﾌｨﾙﾑから最近のポーズ時点までの)	*/
	Sint32		count_start;		/* ＣＰＵクロックタイマ開始時刻 		*/
									/* 						[CPUｸﾛｯｸ>>15] 	*/
	Sint32		clock_scale;		/* ＣＰＵクロックタイマのスケール 		*/
	PcmFlag		fill_silence;		/* 無音設定済みフラグ					*/
	Sint32		*pwork_addr;		/* ポーズ処理用ワーク					*/
	Sint32		pwork_size;			/* ポーズ処理用ワークサイズ[byte] 		*/
									/*  (ポーズ機能を使用する場合に必要)	*/
	Sint32		cnt_sg;				/* サウンドグループカウンタ				*/
	Sint32		onetask_sample;		/* １回のタスクで処理する量[sample/1ch]	*/
	Sint32		onetask_size;		/* １回のタスクで処理する量[byte/1ch]	*/
	Sint32		onetask_sg;			/* １回のタスクで処理する量[sg/1ch]		*/

	/* サウンドドライバ関連 */
	Sint32		sddrv_start;		/* 'SDRV' サウンドドライバ関連の開始 	*/
	PcmCommand	command;			/* 最後に発行した各コマンドの内容 		*/

	/* PCMデータ転送に関する情報 */
	Sint32		copy_start;			/* 'COPY' PCMデータ転送関連の開始 		*/
	PcmTrMode	copy_mode_pcm;		/* データの転送方式 					*/
	PcmFlag		dma_hold;			/* 1:このハンドルがDMA資源を保持している*/
	Uint32		pcm_renew_size;		/* DMA終了時pcmﾊﾞｯﾌｧ書き込みﾎﾟｲﾝﾀ更新ｻｲｽﾞ*/
	Uint32		buf_renew_size;		/* DMA終了時ﾊﾞｯﾌｧ読み取りﾎﾟｲﾝﾀ更新ｻｲｽﾞ 	*/
	Sint32		copy_idx;			/* PCM転送情報テーブルのインデックス 	*/
	PcmCopyTbl 	copy_tbl[4];		/* PCM転送情報テーブル 					*/

	/* 時間管理情報 */
	Sint32		vbl_start;			/* 'VBL ' 時間管理情報の開始 			*/
	Sint32		vbl_film_start;		/* フィルム開始時刻[vbl] 				*/
	Sint32		vbl_pause;			/* ポーズ開始時刻[vbl] 					*/
	Sint32		vbl_pcm_start;		/* PCM開始時刻[vbl]
									 * 最初の頃に cnt_4ksample を誤ってｶｳﾝﾄ
									 * ｱｯﾌﾟしないように。
									 */

	/* ADPCM切り分け用関数ポインタ */
	void (*audio_process_fp)(PcmHn hn);
									/* １オーディオサンプル処理
									 * pcm_AudioProcess      または、
									 * pcm_AudioProcessAdpcm が登録
									 * される
									 */

	/* エラー情報 */
	Uint32		err_start;			/* 'ERR ' エラー情報の開始 				*/
	Uint32		cnt_load_miss;		/* PCMロードミスカウンタ 				*/
	Uint32		cnt_task_call;		/* タスクコールカウンタ 				*/
	Uint32		cnt_over_task_call;	/* 過剰タスクコールカウンタ 			*/
	Uint32		cnt_buf_empty;		/* バッファ空タスクカウンタ 			*/
	Uint32		max_late_time;		/* 最大遅れ時刻[sample] 				*/
	Uint32		max_late_sample;	/* 最大遅れ時間[sample] 				*/
	Sint32		cnt_fill_silence;	/* 無音設定カウンタ						*/
} PcmStatus;

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
	PcmTrMode	tr_mode;		/* 転送モード */
} PcmFileGfsPara;

typedef struct {
	StmHn		stm;			/* ストリームハンドル */
	Sint32		load_sect;		/* 転送最大セクタ数 */
	Sint32		old_cd_buf_num;	/* 前回のＣＤバッファの数 */
	Bool		dma_state;		/* ＤＭＡ転送しているなら TRUE */
	Sint32		dma_sect;		/* ＤＭＡ転送するセクタ数 */
	Uint32		*write_addr;	/* 書き込むアドレス */
	Sint32		buf_bsize;		/* 空きバッファのバイト数 */
	Sint32		write_bsize;	/* 転送したバイト数 */
	Sint32		sect_bsize;		/* １セクタのバイト数 */
	Sint32		audio_1st_sect;	/* 先頭オーディオセクタ */
	StmTrFunc	load_func;		/* 転送関数 */
} PcmFileStmPara;

typedef struct {
	Sint32		faccess_type;		/* ファイルアクセスタイプ */
	/* ファイルローカルデータ */
	union	{
		PcmFileGfsPara	gfs;
		PcmFileStmPara	stm;
	} data;
} PcmFilePara;

/* 実行関数 */
typedef struct {
	void	(*start)(PcmHn pcm);
	void	(*task)(PcmHn pcm);
	Sint32	(*preload_file)(PcmHn pcm, Sint32 size);
	void	(*set_load_num)(PcmHn pcm, Sint32 load_sct);
	void	(*set_trmode)(PcmHn pcm, PcmTrMode mode);
} PcmExecFunc;

/* 相互参照のための空宣言 */
struct PcmWork;

/* 作成パラメータ */
typedef struct {
	struct PcmWork *work;		/* ワークのアドレス 						*/
								/*  (ライブラリが使用するﾊﾝﾄﾞﾙ毎の作業領域) */
	Sint8		*ring_addr;		/* ファイルを格納するバッファの先頭アドレス */
								/*  (メモリのファイルの時はそのアドレス) 	*/
								/*  (CDのファイルの時はﾘﾝｸﾞﾊﾞｯﾌｧのｱﾄﾞﾚｽ) 	*/
	Sint32		ring_size;		/* バッファサイズ[byte] 					*/
								/*  (メモリのファイルの時はファイルサイズ) 	*/
								/*  (CDのファイルの時はﾘﾝｸﾞﾊﾞｯﾌｧのサイズ 	*/
	Sint8		*pcm_addr;		/* サウンドメモリのPCMバッファアドレス 		*/
	Sint32		pcm_size;		/* サウンドメモリのPCMバッファサイズ[sample]*/
								/*  (1ch分のサンプル数 4096*1..4096*16) 	*/
} PcmCreatePara;

/* 作成パラメータ */
typedef struct {
	Sint8		*ring_addr;		/* ファイルを格納するバッファの先頭アドレス */
								/*  (メモリのファイルの時はそのアドレス) 	*/
								/*  (CDのファイルの時はﾘﾝｸﾞﾊﾞｯﾌｧのｱﾄﾞﾚｽ) 	*/
	Sint32		ring_size;		/* バッファサイズ[byte] 					*/
								/*  (メモリのファイルの時はファイルサイズ) 	*/
								/*  (CDのファイルの時はﾘﾝｸﾞﾊﾞｯﾌｧのサイズ 	*/
	Sint8		*pcm_addr;		/* サウンドメモリのPCMバッファアドレス 		*/
	Sint32		pcm_size;		/* サウンドメモリのPCMバッファサイズ 		*/
								/*  (1ch分のサンプル数 4096*1..4096*16) 	*/

	Sint32		command_blk_no;		/* コマンドブロック番号 				*/
	Sint32		pcm_stream_no;		/* ＰＣＭストリーム再生番号 			*/
	Sint32		pcm_pan;			/* ＰＡＮ     [0..31] 					*/
	Sint32		pcm_level;			/* ＬＥＶＥＬ [0..7] 					*/

	Sint32		start_trg_size;		/* 再生開始トリガサイズ[byte] 			*/
	Sint32		start_trg_sample;	/* 再生開始トリガサンプル[sample/1ch] 	*/
	Sint32		stop_trg_sample;	/* 再生停止トリガサンプル[sample/1ch] 	*/
} PcmPara;

/* ＰＣＭワーク */
typedef struct {
	Sint32			work_start;		/* 'WORK' ワークの開始 			*/
	PcmPara 		para;			/* パラメータ 					*/
	PcmStatus 		status;			/* 各種制御情報 				*/
	PcmFilePara 	filepara;		/* ファイルアクセスパラメータ 	*/
	PcmExecFunc 	execfunc;		/* 実行関数 					*/
} PcmWork;

/*-------------------------- 《関数宣言》 --------------------------*/
/* ライブラリの初期化 */
Bool PCM_Init(void);

/* ライブラリの終了処理 */
void PCM_Finish(void);

/* ＡＤＰＣＭ使用宣言 */
void PCM_DeclareUseAdpcm(void);

/* ハンドルの作成（メモリ） */
PcmHn PCM_CreateMemHandle(PcmCreatePara *para);

/* ハンドルの消去（メモリ） */
void PCM_DestroyMemHandle(PcmHn pcm);

/* ハンドルの作成（ファイルシステム） */
PcmHn PCM_CreateGfsHandle(PcmCreatePara *para, GfsHn gfs);

/* ハンドルの消去（ファイルシステム） */
void PCM_DestroyGfsHandle(PcmHn pcm);

/* ハンドルの作成（ストリームシステム） */
PcmHn PCM_CreateStmHandle(PcmCreatePara *para, StmHn stm);

/* ハンドルの消去（ストリームシステム） */
void PCM_DestroyStmHandle(PcmHn pcm);

/* 再生タスク */
void PCM_Task(PcmHn pcm);

/* ＶブランクＩＮ処理関数 */
void PCM_VblIn(void);

/* 再生開始 */
void PCM_Start(PcmHn pcm);

/* 再生停止 */
void PCM_Stop(PcmHn pcm);

/* 一時停止 */
void PCM_Pause(PcmHn pcm, PcmPauseCmd cmd);

/* ループ回数の指定 */
void PCM_SetLoop(PcmHn pcm, Sint32 cnt_loop);

/* 次に再生するハンドルの登録 */
void PCM_EntryNext(PcmHn pcm);

/* ハンドルの強制切り替え */
void PCM_Change(void);

/* ハンドルの切り替え状態の取得 */
PcmChangeStatus PCM_CheckChange(void);

/* ポーズ処理用ワークアドレス、サイズの設定 */
void PCM_SetPauseWork(Sint32 *addr, Sint32 size);

/* 最大転送セクタ数の設定 */
void PCM_SetLoadNum(PcmHn pcm, Sint32 read_sct);

/* 再生ＰＡＮの設定 */
void PCM_SetPan(PcmHn pcm, Sint32 pan);

/* 再生ボリュームの設定 */
void PCM_SetVolume(PcmHn pcm, Sint32 volume);

/* ＰＣＭ再生パラメータの変更 */
void PCM_ChangePcmPara(PcmHn pcm);

/* ＰＣＭ再生情報の設定 */
void PCM_SetInfo(PcmHn hn, PcmInfo *info);

/* １回のタスクで処理する量の上限の設定 [sample/1ch] */
void PCM_Set1TaskSample(PcmHn pcm, Sint32 sample);

/* 再生開始トリガサイズの設定 [byte] */
void PCM_SetStartTrgSize(PcmHn pcm, Sint32 size);

/* 再生開始トリガサンプルの設定 [sample/1ch] */
void PCM_SetStartTrgSample(PcmHn pcm, Sint32 sample);

/* 再生停止トリガサンプルの設定 [sample/1ch] */
void PCM_SetStopTrgSample(PcmHn pcm, Sint32 sample);

/* ＰＣＭコマンドブロック番号の設定 */
/* void PCM_SetPcmCmdBlockNo(PcmHn pcm, Sint32 blk_no); */

/* ＰＣＭストリーム番号の設定 */
void PCM_SetPcmStreamNo(PcmHn pcm, Sint32 stream_no);

/* データの転送方式の設定（ＣＤブロック→リングバッファ） */
void PCM_SetTrModeCd(PcmHn pcm, PcmTrMode mode);

/* データの転送方式の設定（リングバッファ→ＰＣＭバッファ） */
/* void PCM_SetTrModePcm(PcmHn pcm, PcmTrMode mode); */

/* 再生時刻の取得 */
Sint32 PCM_GetTime(PcmHn pcm);

/* 再生ステータスの取得 */
PcmPlayStatus PCM_GetPlayStatus(PcmHn pcm);

/* 書き込みバッファアドレスの取得 */
Uint32 *PCM_GetWriteBuf(PcmHn pcm, Sint32 *free_size, Sint32 *free_total);

/* バッファへの書き込みサイズの通知 */
void PCM_NotifyWriteSize(PcmHn pcm, Sint32 write_size);

/* メモリにファイルを読み込む */
Sint32 PCM_PreloadFile(PcmHn pcm, Sint32 size);

/* メモリにファイルヘッダを読み込む */
/* Sint32 PCM_PreloadHeader(PcmHn pcm); */

/* エラー関数の登録 */
void PCM_SetErrFunc(PcmErrFunc func, void *obj);

/* エラー情報の取得 */
PcmErrCode PCM_GetErr(void);

/* 非公開 */
Bool PCM_IsDeath(PcmHn pcm);

#endif	/* SEGA_PCM_H */
