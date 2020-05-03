/*******************************************************************
*
*                       Cinepak for SATURN Player 
*                                by SOJ
*
*                      Copyright(c) 1994,1995 SEGA
*
*   Comment: User header
*   File   : SEGA_CPK.H
*   Date   : 1995-10-05
*   Version: 1.20
*   Author : Y.T
*
*******************************************************************/
#ifndef SEGA_CPK_H
#define SEGA_CPK_H


/*-------------------------- Treatment Selection --------------------------*/

/* Use codebook address table
 * If you enable this, cpk_ra15.src and cpk_er15.src are also set to 1 at the same time.
 */
/* #define CPK_USE_CBTBL */


/*-------------------- Include File --------------------*/
#include <SEGA_XPT.H>
#if 0
#include <sega_gfs.h>
#include <sega_stm.h>
#else
#include <SGL_CD.H>
#endif


/*------------------------- Macro constants -------------------------*/

/* Version */
#define CPK_VERSION				("CPK Version 1.20 1995-10-05")
							/*	           1 2 3 4	*/
							/*	  1234567890123456789012345678901234567890 1234567890	*/
							/*	  +++++++++++++++++++++++++++++++++++			*/
#define CPK_VERSION_LEN			(32)

/* Total handle count */
#define CPK_HN_MAX				(32)

/* Error code	*/
typedef enum {
	CPK_ERR_OK					= 0x00000000,/* No error occurred		*/

	/* Error in CPK_Create */
	CPK_ERR_OUT_OF_HANDLE		= 0x00000101,/* Handle sold out 			*/
	CPK_ERR_TOO_SMALL_BUF		= 0x00000102,/* Ring buffer size too small	*/
	CPK_ERR_NO_INIT				= 0x00000103,/* Initialization function not called 	*/

	/* Error in CPK_Destroy */
	CPK_ERR_INVALID_HN			= 0x00000201,/* Erase with wrong handle		*/
	CPK_ERR_ILL_CREATE_MODE		= 0x00000202,/* Not in Create mode		*/

	/* Error in cpk_HeaderProcess */
	CPK_ERR_TOO_SMALL_FLOP		= 0x00000301,/* The flop initial value is too low		*/
	CPK_ERR_TOO_LARGE_HEADER	= 0x00000302,/* Header too large			*/
											/* (Ring buffer size too small)	*/
	CPK_ERR_HEADER_DATA			= 0x00000303,/* Header data error 			*/
	CPK_ERR_TOO_SMALL_PCMBUF	= 0x00000304,/* PCM buffer size too small	*/
	CPK_ERR_ILL_SIZE_PCMBUF		= 0x00000305,/* PCM buffer size incorrect 4096*2 to 16	*/

	/* Error in cpk_VideoSample */
	CPK_ERR_SAMPLE_ID			= 0x00000401,/* Identifier error. Not 00h,01h.*/
	CPK_ERR_SAMPLE_DATA			= 0x00000402,/* Bad data.				*/

	/*  */
	CPK_ERR_ILLEGAL_PARA		= 0x00000503,/* The argument is not specified correctly 	*/
	CPK_ERR_ILLEGAL_HANDLE		= 0x00000504,/* Handle is faulty 				*/

	/* Error with continuous playback */
	CPK_ERR_NEXT_MOVI_STATUS	= 0x00000601,/* Continuous movie condition is abnormal 	*/
	CPK_ERR_NEXT_MOVI_AUDIO		= 0x00000602,/* Audio conditions do not match */
	CPK_ERR_NEXT_MOVI_SCALE		= 0x00000603,/* Timescales do not match 	*/
	CPK_ERR_CHANGE_NO_DATA		= 0x00000604,/* Change in lack of data supply */
	CPK_ERR_CHANGE_NO_ENTRY		= 0x00000605,/* Change without entry */

	/* Error with pause */
	CPK_ERR_PAUSE_STATUS		= 0x00000701,/* CPK_STAT_PLAY_TIME, Or
											 * CPK_STAT_PLAY_PAUSE Or
											 * The pose function was called in the state.
											 */

	/* Error with DMA transfer */
	CPK_ERR_DMA_MODE			= 0x00000801,/* Unsupported transport method 			*/
	CPK_ERR_DMA_CPU_PCM			= 0x00000802,/* CPUDMA terminated abnormally 			*/

	/* Error reading file */
	CPK_ERR_GFS_READ			= 0x00000901,/* GFS read failed 		*/
	CPK_ERR_STM_FAD				= 0x00000902,/* Inconsistent fad in STM transfer function 	*/
	CPK_ERR_STM_HN				= 0x00000903,/* STM transfer function, STM handle failure 	*/

	/* Error with CPK_DecodeFrame */
	CPK_ERR_DF_NO_DATA			= 0x00000A01,/* No data supply for header 	*/
	CPK_ERR_DF_IS_NOT_VIDEO		= 0x00000A02,/* Not a video sample 		*/


	CPK_ERR_END
} CpkErrCode;

/* Playback status */
typedef enum {
	CPK_STAT_PLAY_ERR_STOP		= -1,		/* Abnormal stop					*/
	CPK_STAT_PLAY_STOP			= 0,		/* Stop (Create) 		*/
	CPK_STAT_PLAY_PAUSE			= 1,		/* Pause		 			*/
	CPK_STAT_PLAY_START			= 2,		/* Start						*/
	CPK_STAT_PLAY_HEADER		= 3,		/* Header processing				*/
	CPK_STAT_PLAY_TIME			= 4,		/* During playback (timer start)	*/
	CPK_STAT_PLAY_END			= 5			/* Playback complete 				*/
} CpkPlayStatus;

/* Control pause command */
typedef enum {
	CPK_PAUSE_ON_AT_ONCE,					/* Immediate pause 			*/
	CPK_PAUSE_ON_KEYFRAME,					/* Pause waiting for keyframe	*/
	CPK_PAUSE_OFF							/* Unpause 			*/
} CpkPauseCmd;

/* Number of colors [bit] */
typedef enum {
	CPK_COLOR_24BIT				= 24,		/* 16 million colors 				*/
	CPK_COLOR_15BIT				= 15		/* 32000 colors 					*/
} CpkColorType;

/* Force switchability check value */
typedef enum {
	CPK_CHANGE_OK_AT_ONCE		= 0,	/* Can be switched on immediately.			*/
	CPK_CHANGE_NO_DATA			= 1,	/* Insufficient data supply.				*/
	CPK_CHANGE_NO_ENTRY			= 2,	/* No entry.				*/
	CPK_CHANGE_TOO_LARGE_HEADER	= 3		/* Header too large.			*/
										/* (Buffer size too small)	*/
} CpkChangeStatus;

/* Image stretch timing setting */
typedef enum {
	CPK_DEC_TIMING_DECODE_IN_HASTE = 0,	/* Keep it stretched		*/
	CPK_DEC_TIMING_WAIT_AND_DECODE = 1	/* Wait for the display time to stretch		*/
} CpkDecTiming;

/* Method of data transfer */
typedef enum {
	CPK_TRMODE_CPU = 0,					/* Software transfer 			*/
	CPK_TRMODE_SDMA,					/* DMA Cycle Steel 		*/
	CPK_TRMODE_SCU,						/* DMA for SCU				*/
	CPK_TRMODE_SCUDSPDMA				/* SCU - DMA for the DSP		*/
} CpkTrMode;

/* [Note 1] The transfer method settings have the restrictions shown in the following table.
 *　　　　　　　　　　　Data transfer method setting restrictions
 *  ====================+=================================================
 *     Processing \ method   |  Software transfer  CPU-DMA  SCU-DMA  SCU-DSP-DMA
 *  ====================+=================================================
 *    CPK_SetTrModeCd   |      ○         ○        ○          ×
 *    CPK_SetTrModePcm  |      ○         ○        ×          ○
 *  ====================+=================================================
 * [Note 2] When CPK_TRMODE_SCUDSPDMA is specified as the transfer method,
 *  The right to use SCU-DSP is in Cinepak library. SCU-DSP as Cinepak library
 *  Use CPK_AllocScuDsp, CPK_FreeScuDsp to use both in the application.
 */

/* 4BYTE character code is expressed in Uint32 type.
 *   (If you write'cvid 'etc., the wording will come out, so to avoid this)
 */
#define CPK_4BYTE_CHAR(a,b,c,d)					\
	(	((Uint32)((a) & 0x000000FF) << 24) | 	\
		((Uint32)((b) & 0x000000FF) << 16) | 	\
		((Uint32)((c) & 0x000000FF) <<  8) | 	\
		 (Uint32)((d) & 0x000000FF)				\
	)

/* Compression method */
typedef enum {
	CPK_CTYPE_NOVIDEO 	= 0,								/* No image 	*/
	CPK_CTYPE_NONE 		= CPK_4BYTE_CHAR('r','a','w',' '),	/* Uncompressed 		*/
	CPK_CTYPE_CVID 		= CPK_4BYTE_CHAR('c','v','i','d'),	/* Cinepak compression 	*/
	CPK_CTYPE_END
} CpkCType;

/* CPU usage conditions */
typedef enum {
	CPK_CPU_MAIN 		= 1, 		/* main cpu only (default)			*/
	CPK_CPU_SLAVE 		= 2, 		/* slave cpu only (Not supported) 	*/
	CPK_CPU_DUAL 		= 3, 		/* dual cpu 						*/
	CPK_CPU_END
} CpkCpu;
/* [Note] 
 *  When CPK_CPU_DUAL is specified as the CPU usage condition, slave cpu is Cinepak
 *  It is exclusively used by the library. Applications cannot use slave cpu.
 */


/*---------------------- Various default values ----------------------*/

/* Default value for chroma key cutoff */
#define CPK_DEFAULT_CKEY_CUTOFF_15		(3 * 7)
#define CPK_DEFAULT_CKEY_CUTOFF_24		(CPK_DEFAULT_CKEY_CUTOFF_15 * 8)

/* Default silence value [1/s] */
#define CPK_DEFAULT_SILENCE_RATE	(8)

/* Default playback speed (normal playback speed) */
#define CPK_SPEED_ORIGINAL		(0)

/* The unit of measurement for playback speed */
#define CPK_SPEED_UNIT			(1024)

/* Output start address for debug information (default value) */
#define CPKD_START_ADDR			(0x04000000)

/* Output end address for debug information (default value) */
#define CPKD_END_ADDR			(0x047FFFF0)

/* Margin at which debug information begins to output [dword] */
#define CPKD_HEAD_MARGIN		(4)

/* Identification string length [dword], [byte] of the CPKD system structure */
#define CPKD_STRING_LEN			(4)
#define CPKD_STRING_LEN_BYTE	(4 * CPKD_STRING_LEN)


/*------------------------- Processing Macro -------------------------*/
/* Access macro to creation parameters */
#define	CPK_PARA_WORK_ADDR(para)	((para)->work_addr)
#define	CPK_PARA_WORK_SIZE(para)	((para)->work_size)
#define	CPK_PARA_BUF_ADDR(para)		((para)->buf_addr)
#define	CPK_PARA_BUF_SIZE(para)		((para)->buf_size)
#define	CPK_PARA_PCM_ADDR(para)		((para)->pcm_addr)
#define	CPK_PARA_PCM_SIZE(para)		((para)->pcm_size)

/* Handle access */
#define	CPK_HN_DONT_DEC_VIDEO(hn) ((*(CpkWork **)(hn))->status.dont_dec_video)

/* Code1Book size [byte],[dbyte] */
#define CPK_BSIZE_CODE1BOOK				(0x800)
#define CPK_BSIZE_CODE1BOOK_FUL			(2 * CPK_BSIZE_CODE1BOOK)
#define CPK_DSIZE_CODE1BOOK				(CPK_BSIZE_CODE1BOOK     / 4)
#define CPK_DSIZE_CODE1BOOK_FUL			(CPK_BSIZE_CODE1BOOK_FUL / 4)

/* Work buffer size for 24-bit full color */
#define	CPK_24WORK_BSIZE	sizeof(CpkWork)
#define	CPK_24WORK_DSIZE	((CPK_24WORK_BSIZE + 3) / 4)

/* Work buffer size for 15-bit color */
#define	CPK_15WORK_BSIZE	(sizeof(CpkWork) - (4 * CPK_BSIZE_CODE1BOOK))
#define	CPK_15WORK_DSIZE	((CPK_15WORK_BSIZE + 3) / 4)

/* CPKD System Structure Access */
#define CPKD_SYS_STRING			(cpkd_sys.string)
#define CPKD_SYS_MODE			(cpkd_sys.mode)
#define CPKD_SYS_START_ADDR		(cpkd_sys.start_addr)
#define CPKD_SYS_END_ADDR		(cpkd_sys.end_addr)
#define CPKD_SYS_PRINT_ADDR		(cpkd_sys.print_addr)
#define CPKD_SYS_RETURN_CNT		(cpkd_sys.return_cnt)


/*-------------------------- Data type --------------------------*/
/* The moving handle */
typedef void *CpkHn;

/* Creation parameters */
typedef struct {
	Uint32		*work_addr;		/* The first address of the work */
								/* The work area in which the library plays the movie*/
	Uint32		work_size;		/* Number of bytes in the work */
								/* Specify CPK_BSIZE_work */
	Uint32		*buf_addr;		/* The first address of the buffer that contains the movie file */
								/* Address of the memory movie file, if any */
								/* The address of the ring buffer when the CD movie file is in */
	Uint32		buf_size;		/* Buffer size */
								/* File size for memory movie files*/
								/* The size of the ring buffer when the CD movie file is in */
	Uint16		*pcm_addr;		/* PCM buffer address of the sound memory 		*/
	Uint32		pcm_size;		/* PCM buffer size of the sound memory		*/
								/* (Number of samples per channel (4096*1..4096*16))		*/
} CpkCreatePara;


/* Film Sample Table Item */
typedef struct {
	Uint32		offset;				/* Offset from media start position */
	Sint32		size;				/* Sample size 					*/
	Sint32		time;				/* Frame display time, -1: Audio sample */
	Sint32		duration;			/* Frame display duration 			*/
} CpkFilmSample;


/* Film header */
typedef struct {
	/* FilmHeader */
	Uint32		film;				/* "FILM" 								*/
	Uint32		size_header;		/* Header Size (Offset to Media Start)	*/
	Uint32		version;			/* Version 							*/
	Uint32		reserved;			/* Reservations 								*/
	/* FrameDescription */
	Uint32		fdsc;				/* "FDSC" 								*/
	Uint32		size_fdsc;			/* FDSC size 0x0000001C = 7*4[byte]	*/
	CpkCType	c_type;				/* Compression method \"cvid\" 						*/
	Uint32		height;				/* Width [pixel]						*/
	Uint32		width;				/* Width [pixel] 						*/
	Uint8		color;				/* Number of colors [bit] 						*/
	Uint8		sound_channel;		/* Number of channels 							*/
	Uint8		sound_smpling_bit;	/* Sampling Bits [bit] 			*/
	Uint8		sound_compress;		/* Compression 00h: None 01h:ADPCM 				*/
	Uint32		sound_smpling_rate;	/* Sampling rate [Hz] lower 2byte is a decimal number 	*/
	Uint32		ckey;				/* Chroma key information	 					*/
	/* FilmSampleTable */
	Uint32		stab;				/* "STAB" 								*/
	Uint32		size_stab;			/* STAB size (total samples + 1)*16[byte] 	*/
	Uint32		time_scale_film;	/* Timescale 						*/
	Uint32		sample_total;		/* Total number of samples 						*/
	CpkFilmSample sample[1];		/* Sample table arbitrary size 			*/
} CpkHeader;

/* Error Registration function */
typedef void (*CpkErrFunc)(void *obj, Sint32 err_code);


/* Parameters */
typedef struct {
	Uint32		AddrBuf;		/* Buffer start position. 0x4000000 for shims 	*/
	Uint32		SizeBuf;		/* Buffer size. 0x1000000 for shim 	*/
	Uint32		size_trigger;	/* Start playback trigger size						*/
	Uint32		AddrVram;		/* Image output address 						*/
	Uint32		SizeVramLine;	/* Image output buffer line size [byte]			*/
	/* Sound driver related */
	Uint32		play_pcm;		/* PCM Play Flag 0: No sound processing 1: Play		*/
	Uint32		command_blk_no;	/* Sound driver command block number 	*/
	Uint32		pcm_stream_no;	/* PCM stream regeneration number 					*/
	Uint16		*addr_pcmbuf;	/* PCM buffer address of the sound memory 			*/
	Uint32		size_pcmbuf;	/* PCM buffer size for sound memory [sample/1ch] 	*/
	Uint32		pcm_pan;		/* 0..31	*/
	Uint32		pcm_level;		/* 0..7		*/
	Uint32		frequency_vbl;	/* Frequency to Call CPK_Vblin() [256*Hz] 	*/
} CpkPara;

/* DMA transfer information */
typedef struct {
	Uint32		dst;				/* Destination address 						*/
	Uint32		src;				/* Source address 						*/
	Uint32		cnt;				/* Number of transfers 							*/
} CpkDmaStock;

/* Various states */
typedef struct {
	/* Overall control information */
	Sint32		stat_start;			/* Start of status						*/
	Sint32		play;				/* Playback state 							*/
	Uint32		RequestDispFrame;	/* Frame display request flag 				*/
	Uint32		CntSample;			/* Sample counter 					*/
	Uint32		CntFrame;			/* Frame counter 					*/
	Uint32		offset_media;		/* Offset to media start position		*/
	Uint32		display_keyframe;	/* Display image keyframe flag 			*/
	Uint32		standby_keyframe;	/* Waiting image keyframe flag 			*/
	Uint32		standby_video_flag;	/* Display Waiting Image present flag 				*/
	Uint32		standby_time_display;/* Time the waited image was displayed					*/
	Uint32		standby_time_duration;/* The display duration of the waiting image				*/
	Uint32		only_audio;			/* flag that only audio sample 			*/
	Uint32		only_video;			/* flag that only video sample 			*/
	Uint32		color;				/* Output color depth [bit]. 15 or 24				*/
	Uint32		wait_keyframe_to_pause;/* Pause key frame wait flag 	*/
	Uint32		wait_and_decode;	/* 1: Wait for the display time and then stretch		*/
	Sint32		flag_prohibit_komaotoshi;/* 1: Frame drop kinnshi flag		*/
	Sint32		ckey_cutoff;		/* chroma key cutoff level 				*/
	Sint32		final_audio_sample;	/* Last audio sample 			*/
	Sint32		silence_rate;		/* Silence [1/s]							*/
	
	/* Ring buffer control information */
	Sint32		ring_start;			/* Start of								*/
	Uint32		OffsetWrite;		/* Buffer read/write position [byte]			*/
	Uint32		OffsetRead;			/* (Offset from beginning of file)		*/
	Uint32		AddrWrite;			/* Buffer Read and Write Locations (addresses)		*/
	Uint32		AddrRead;			/* 										*/
	Uint32		AddrRing;			/* Ring buffer start address 			*/
	Uint32		AddrRingEnd;		/* Ring buffer end address 			*/
	Sint32		SizeHead;			/* Header buffer size [byte] sector integer times */
	Uint32		SizeRing;			/* Ring buffer size [byte] sector integer times */
	Uint32		SizeFlop;			/* Flow buffer size [byte] 		*/
	
	/* PCM regeneration information */
	Sint32		audio_start;		/* Start of								*/
	Uint32		play_pcm;			/* PCM Play Flag 0: No sound processing 1: Play	*/
	Uint32		bsize_pcmbuf;		/* PCM buffer size in sound memory [byte/1ch]*/
	Uint16		*addr_pcmbuf_end;	/* PCM buffer end address 				*/
	Uint16		*addr_pcm_write;	/* PCM buffer write address 			*/
	Uint32		smpling_rate;		/* Sampling rate [Hz] lower 1byte is a decimal number 	*/
	Uint32		cnt_4ksample;		/* PCM play address 4k per sample update counter */
	Sint32		flag_fill_silence;	/* Silence-set flag					*/
	Sint32		sample_write;		/* Number of samples supplied [sample]				*/
									/* (Total current supply from the first film)	*/
	Sint32		sample_film_start;	/* Sample number of samples at the start of the film		*/
									/* (From the first film to the current film start)	*/
	Sint32		sample_pause;		/* Sample number at pause [sample]			*/
									/* (From the first film to the most recent pause)	*/
	Sint32		count_start;		/* CPU clock timer start time 		*/
									/* 						CPU Clock 15 	*/
	Sint32		clock_scale;		/* CPU clock timer scale 		*/
	Sint32		dont_stop_pcm;		/* Flag that does not stop PCM at the end. 		*/

	/* Next Movie Management Information */
	Sint32		next_start;			/* Start of								*/
/*	CpkHn hn_next;*/			/* Next handle (not registered, null after start) 	*/
	CpkHn		hn_next_start;		/* Next handle (after start) 					*/
	Sint32		dont_dec_video;		/* Flag that does not stop PCM at the end. 		*/
	
	
	/* Information about DMA transfers of PCM data */
	Sint32		cppcm_start;		/* Start of								*/
	CpkTrMode	copy_mode_pcm;		/* Method of data transfer 					*/
	Uint32		flag_dma_hold;		/* 1: This movie has DMA resources*/
	Uint32		pcm_renew_size;		/* Us\u003ethe PCM buffer write pointer update size at the end of DMA*/
	Uint32		buf_renew_size;		/* DMA end-time buffer read pointer update size 	*/
	Uint32		idx_dma_pcm_tbl;	/* Index of DMA_PCM_tbl 			*/
	CpkDmaStock	dma_pcm_tbl[4];		/* DMA transfer information entry table 			*/
	
	/* Time Management Information (Video only movies) */
	Sint32		time_start;			/* Start of								*/
									/* 95-08-17 Y. TT : CPK_SetSpeed capable 	*/
									/*   Have a Vbl counter per handle				*/
	Sint32		cnt_vbl_in;			/* VLB-in Interrupt Counter [vbl]			*/
	Sint32		vblcnt_regulator;	/* VLB-in counter regeneration speed regulator		*/
	Sint32		vbl_film_start;		/* Film Start Time [vbl] 				*/
	Sint32		vbl_pause;			/* Pause Start Time [vbl] 					*/
	Sint32		vbl_pcm_start;		/* PCM start time
									 * Accidentally counting cnt_4ksample at the beginning
									 * Don't up.
									 */
	Sint32		time_frame;			/* The time displayed + duration of the displayed frame	*/

	/* Playback speed control information */
	Sint32		speed;				/* Playback speed [Ratio to Constant Speed * 1024] 			*/
	Bool		audio_sw;			/* Audio output switch when changing playback speed		*/

	/* Error information */
	Uint32		err_start;			/* Start error information                     */
	Uint32		CntLossFrame;		/* Frame drop counter 					*/
	Uint32		CntPcmLoadMiss;		/* PCM load miss counter 				*/
	Uint32		CntCallTask;		/* Task call counters 				*/
	Uint32		CntOverCallTask;	/* Excessive task call counters 			*/
	Uint32		cnt_buf_empty;		/* Buffer empty task counter 			*/
	Uint32		max_late_time;		/* Maximum Delay Time [Film Time Unit] 		*/
	Uint32		max_late_frame;		/* Maximum delay frame number					*/
} CpkStatus;


/* File access relationships */
typedef struct {
	GfsHn		gfs;			/* File handle */
	Sint32		load_sect;		/* Maximum number of sectors to transfer */
	Bool		called_cdread;	/* True if calling GFS_NwCdRead */
	Bool		exec_one_state;	/* True if ExecOne is running */
	Sint32		exec_load_size;	/* Number of bytes read by ExecOne */
	Sint32		now_load_size;	/* Number of bytes that ExecOne is reading */
	Sint32		load_total_sect;/* Total number of sectors to read */
	Sint32		file_sect;		/* Number of sectors in the file */
	Sint32		tr_mode;		/* Transfer mode */
} CpkFileGfsPara;

typedef struct {
	StmHn		stm;			/* Stream handle */
	Sint32		load_sect;		/* Maximum number of sectors to transfer */
	Sint32		old_cd_buf_num;	/* Number of previous CD buffers */
	Bool		dma_state;		/* True if DMA transfer is in process */
	Sint32		dma_sect;		/* Number of sectors to DMA transfer */
	Uint32		*write_addr;	/* Address to write to */
	Sint32		buf_bsize;		/* Number of bytes in the free buffer */
	Sint32		write_bsize;	/* Number of bytes transferred */
	Sint32		load_total_sect;/* Total number of sectors to read */
	StmTrFunc	load_func;		/* Transfer function */
} CpkFileStmPara;

typedef struct {
	Sint32		faccess_type;		/* File access type */
	/* File local data */
	union	{
		CpkFileGfsPara	gfs;
		CpkFileStmPara	stm;
	} data;
} CpkFilePara;

/* Execution functions */
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
	Sint32		work_start;				/* Start work					*/
	CpkPara 	para;					/* Cinepak startup parameters */
	CpkStatus 	status;					/* Various states (public part) 		*/
	CpkHeader 	*header;				/* Film file header 		*/
	CpkFilePara	filepara;				/* File access parameters	*/
	CpkExecFunc	execfunc;				/* Execution functions						*/
#ifdef CPK_USE_CBTBL
	Uint32		*cbtblp[4];				/* The central pointer to the code book address table 	*/
	Uint32		cbtbl1a[256];			/* Code Book Address Table 			*/
	Uint32		cbtbl2a[256];			/* 								*/
	Uint32		cbtbl1b[256];			/* 								*/
	Uint32		cbtbl2b[256];			/* 								*/
#endif
	Uint32 		code_book[4 * CPK_DSIZE_CODE1BOOK_FUL];
										/* Upper code1book, code2book	*/
										/* Lower code1book, code2book	*/
} CpkWork;


/* CPKD system structure */
typedef struct {
	Sint32 string[CPKD_STRING_LEN];	/* Identification string */
	Sint32 mode;					/* Output mode 0: No output (default) */
	Sint32 *start_addr;				/* Beginning of the output address */
	Sint32 *end_addr;				/* End of output address */
	Sint32 *print_addr;				/* Current output address */
	Sint32 return_cnt;				/* Fold-over counter */
} CpkdSys;


/*------------------------ Declaration for Debugging ------------------------*/

/* CPKD system structure */
extern CpkdSys cpkd_sys;

/* Setting debug output mode 0N:printing, off:not printing (default) */
void CPKD_SetMode(Bool mode);

/* Setting the debug output address */
void CPKD_SetPrintAddr(Sint32 *start, Sint32 *end);

/* Debug string output (string length is standardized to an integer multiple of 4) */
void CPKD_PrintStr(char *str);

/* Debug Data Output (4 bytes) */
void CPKD_PrintData(Sint32 data);


/*-------------------------- Function Declaration --------------------------*/

/* Library initialization */
Bool CPK_Init(void);

/* The process of closing the library */
void CPK_Finish(void);

/* Create handle (memory) */
CpkHn CPK_CreateMemMovie(CpkCreatePara *para);

/* Clear handle (memory) */
void CPK_DestroyMemMovie(CpkHn cpk);

/* Create handles (File System) */
CpkHn CPK_CreateGfsMovie(CpkCreatePara *para, GfsHn gfs);

/* Clear handles (File System) */
void CPK_DestroyGfsMovie(CpkHn cpk);

/* Creating handles (Stream System) */
CpkHn CPK_CreateStmMovie(CpkCreatePara *para, StmHn stm);

/* Clear handle (Stream System) */
void CPK_DestroyStmMovie(CpkHn cpk);

/* Playback task */
void CPK_Task(CpkHn cpk);

/* Determining display timing */
Bool CPK_IsDispTime(CpkHn cpk);

/* Determine the display frame type */
Bool CPK_IsKeyframe(CpkHn cpk);

/* Notification of completion of the display */
void CPK_CompleteDisp(CpkHn cpk);

/* V blank in processing function */
void CPK_VblIn(void);

/* Start playback */
void CPK_Start(CpkHn cpk);

/* Stop playback */
void CPK_Stop(CpkHn cpk);

/* Pause */
Bool CPK_Pause(CpkHn cpk, CpkPauseCmd cmd);

/* Register the next movie to play */
void CPK_EntryNext(CpkHn cpk);

/* Force switch of the movement */
void CPK_Change(void);

/* Get the transition state of the movie */
CpkChangeStatus CPK_CheckChange(void);

/* Expand the specified frame */
void CPK_DecodeFrame(CpkHn cpk, Sint32 sample_no);

/* Expand specified Frames (Differential Frame Assurance) */
void CPK_DecodeFrameSafety(CpkHn cpk, Sint32 sample_no);

/* Set destination address */
void CPK_SetDecodeAddr(CpkHn cpk, void *dec_addr, Sint32 dec_line_size);

/* Set the number of colors to display */
void CPK_SetColor(CpkHn cpk, CpkColorType color);

/* Set the maximum number of transmit sectors */
void CPK_SetLoadNum(CpkHn cpk, Sint32 read_sct);

/* Playback pan settings */
void CPK_SetPan(CpkHn cpk, Sint32 pan);

/* Playback volume settings */
void CPK_SetVolume(CpkHn cpk, Sint32 volume);

/* Set playback speed */
void CPK_SetSpeed(CpkHn cpk, Sint32 speed, Bool audio_sw);

/* PCM command block number configuration */
/* Void CPK_SetPcmCmdBlockNo(CpkHn cpk, Sint32 blk_no) */

/* PCM stream number configuration */
void CPK_SetPcmStreamNo(CpkHn cpk, Sint32 stream_no);

/* Setting the key-out (color to make transparent) range */
void CPK_SetKeyOutRange(CpkHn cpk, Sint32 range);

/* Silence setting */
void CPK_SetSilenceRate(CpkHn cpk, Sint32 silence_rate);

/* Setting the method for transferring data (CD blocks → ring buffers) */
void CPK_SetTrModeCd(CpkHn cpk, CpkTrMode mode);

/* Set the data transfer method (ring buffer → PCM buffer) */
void CPK_SetTrModePcm(CpkHn cpk, CpkTrMode mode);

/* Set the start playback trigger size [byte] */
void CPK_SetStartTrgSize(CpkHn cpk, Sint32 bsize);

/* Get timescale */
Sint32 CPK_GetTimeScale(CpkHn cpk);

/* Get playback time [timescale] */
Sint32 CPK_GetTime(CpkHn cpk);

/* Get playback status */
CpkPlayStatus CPK_GetPlayStatus(CpkHn cpk);

/* Get film header address */
CpkHeader *CPK_GetHeader(CpkHn cpk);

/* Get write buffer address */
Uint32 *CPK_GetWriteBuf(CpkHn cpk, Sint32 *free_size, Sint32 *free_total);

/* Notification of the write size to the buffer */
void CPK_NotifyWriteSize(CpkHn cpk, Sint32 write_size);

/* Reads a movie into memory */
Sint32 CPK_PreloadMovie(CpkHn cpk, Sint32 size);

/* Load film headers into memory */
Sint32 CPK_PreloadHeader(CpkHn cpk);

/* Register error function */
void CPK_SetErrFunc(CpkErrFunc func, void *obj);

/* Get error information */
CpkErrCode CPK_GetErr(void);

/* SCU-DSP resource reserve */
Sint32 CPK_AllocScuDsp(void);

/* Release of SCU-DSP resources */
void CPK_FreeScuDsp(void);

/* Setting CPU usage conditions */
void CPK_SetCpu(CpkCpu cpu);

/* Call settings for stream server functions */
void CPK_SetStmServerCall(Bool mode);

/* Get data storage [byte] */
/* Only available if the GFS version of the SGL is version ver1.21 or higher */
void CPK_GetStockSize(CpkHn cpk, Sint32 *ring_bsize, Sint32 *all_bsize);

/* Data storage capacity retrieval [timescale] */
/* Only available if the GFS version of the SGL is version ver1.21 or higher */
void CPK_GetStockTime(CpkHn cpk, Sint32 *ring_time, Sint32 *all_time);

#endif /* SEGA_CPK_H */
