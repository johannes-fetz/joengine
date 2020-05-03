/******************************************************************************
 *	Software library
 *
 *	Copyright (c) 1994,1995 SEGA
 *
 * Library	:PCM / ADPCM playback library
 * Module 	:Library header
 * File		:sega_pcm.h
 * Date		:1995-03-31
 * Version	:1.16
 * Author	:Y.T
 *
 ****************************************************************************/
#if !defined(SEGA_PCM_H)
#define SEGA_PCM_H

/* from Ver1.10, adpcm function support */
#define ADPCM_FUNCTION

/*-------------------- Include File --------------------*/
#include "SEGA_XPT.H"
#include "SEGA_GFS.H"
#include "SEGA_STM.H"

/*------------------------- Macro constants -------------------------*/

/* Version */
#define PCM_VERSION				("PCM Version 1.16 1995-03-31")
							/*	           1 2 3 4	*/
							/*	  1234567890123456789012345678901234567890 1234567890	*/
							/*	  +++++++++++++++++++++++++++++++++++			*/
#define PCM_VERSION_LEN			(32)

/* Total handle count */
#define PCM_HN_MAX				(32)

/* 1024, 2048, 4096,... */
#define PCM_SIZE_1K				(1024)
#define PCM_SIZE_2K				(2 * PCM_SIZE_1K)
#define PCM_SIZE_3K				(3 * PCM_SIZE_1K)
#define PCM_SIZE_4K				(4 * PCM_SIZE_1K)
#define PCM_SIZE_8K				(8 * PCM_SIZE_1K)

/* Flag */
typedef enum {
    PCM_OFF = 0,
    PCM_ON  = 1
} PcmFlag;

/* Error code	*/
typedef enum {
	PCM_ERR_OK					= 0x00000000,/* No error occurred		*/

	/* Error in PCM_Create */
	PCM_ERR_OUT_OF_HANDLE		= 0x00000101,/* Handle sold out 			*/
	PCM_ERR_NO_INIT				= 0x00000103,/* Initialization function not called 	*/

	/* Error in PCM_Destroy */
	PCM_ERR_INVALID_HN			= 0x00000201,/* Erase with wrong handle		*/
	PCM_ERR_ILL_CREATE_MODE		= 0x00000202,/* Not in Create mode		*/

	/* Error in PCM_HeaderProcess */
	PCM_ERR_TOO_LARGE_HEADER	= 0x00000302,/* Header too large			*/
											/* (Buffer size too small)	*/
	PCM_ERR_HEADER_DATA			= 0x00000303,/* Header data error 			*/
	PCM_ERR_TOO_SMALL_PCMBUF	= 0x00000304,/* PCM buffer size too small	*/
	PCM_ERR_ILL_SIZE_PCMBUF		= 0x00000305,/* PCM buffer size incorrect 4096*2 to 16	*/
	PCM_ERR_AFI_NO_COMMON		= 0x00000306,/* CommonChunk is missing 			*/
	PCM_ERR_AFI_COMMPRESS		= 0x00000307,/* Unsupported compression type			*/
	PCM_ERR_NOT_DECLARE_ADPCM	= 0x00000308,/* ADPCM use declaration missing			*/

	/*  */
	PCM_ERR_ILLEGAL_PARA		= 0x00000503,/* The argument is not specified correctly 	*/
	PCM_ERR_ILLEGAL_HANDLE		= 0x00000504,/* Handle is faulty 				*/

	/* Error with continuous playback */
	PCM_ERR_NEXT_HN_STATUS		= 0x00000601,/* Continuous regeneration handle condition fault 	*/
	PCM_ERR_NEXT_HN_AUDIO		= 0x00000602,/* Audio conditions do not match */
	PCM_ERR_CHANGE_NO_ENTRY		= 0x00000605,/* Change without entry */

	/* Error with pause */
	PCM_ERR_PAUSE_STATUS		= 0x00000701,/* PCM_STAT_PLAY_TIME, Or
											 * PCM_STAT_PLAY_PAUSE Other than
											 * The pose function was called in the state.
											 */
	PCM_ERR_PAUSE_WORK_NULL		= 0x00000702,/* Work fault for pause processing		*/
	PCM_ERR_PAUSE_WORK_SIZE		= 0x00000703,/* Work fault for pause processing		*/
	PCM_ERR_PAUSE_WORK_SET		= 0x00000704,/* Incorrect work specification for pause processing*/

	/* Error with DMA transfer */
	PCM_ERR_DMA_MODE			= 0x00000801,/* Unsupported transport method 			*/
	PCM_ERR_DMA_CPU_PCM			= 0x00000802,/* DMA terminated abnormally 			*/

	/* Error reading file */
	PCM_ERR_GFS_READ			= 0x00000901,/* GFS read failed 		*/

	/* Error in supplying data to ring buffer */
	PCM_ERR_RING_SUPPLY			= 0x00000a01,/* Supply was present after regeneration was completed    */
											 /* (Supply interrupted in time)*/

	PCM_ERR_END
} PcmErrCode;

/* Playback status */
typedef enum {
	PCM_STAT_PLAY_ERR_STOP		= -1,		/* Abnormal stop					*/
	PCM_STAT_PLAY_CREATE		= 0,		/* Initial state					*/
	PCM_STAT_PLAY_PAUSE			= 1,		/* Pause		 			*/
	PCM_STAT_PLAY_START			= 2,		/* Start						*/
	PCM_STAT_PLAY_HEADER		= 3,		/* Header settings				*/
	PCM_STAT_PLAY_TIME			= 4,		/* Play (timer start)		*/
	PCM_STAT_PLAY_END			= 5			/* Playback complete 				*/
} PcmPlayStatus;

/* File type */
typedef enum {
	PCM_FILE_TYPE_NO_HEADER		= 1,	/* No header.
										 * (Including the case of playing the file format that the PCM library does not support)
										 */
	PCM_FILE_TYPE_AIFF			= 2,	/* AIFF							*/
	PCM_FILE_TYPE_AIFC			= 3 	/* AIFC							*/
} PcmFileType;
/* [Caution] AIFC is not available.*/

/* Data type */
typedef enum {
	PCM_DATA_TYPE_LRLRLR		= 1,	/* Left, right, left, right, and right samples	*/
	PCM_DATA_TYPE_RLRLRL		= 2,	/* Left and right, one sample	*/
	PCM_DATA_TYPE_LLLRRR		= 3,	/* Left right and left right in blocks	*/
	PCM_DATA_TYPE_RRRLLL		= 4, 	/* Right left right left in block	*/
	PCM_DATA_TYPE_ADPCM_SG		= 5, 	/* ADPCM, continuous data for sound groups*/
	PCM_DATA_TYPE_ADPCM_SCT		= 6 	/* ADPCM,1 sector 18 sound group		*/
} PcmDataType;

/* Control pause command */
typedef enum {
	PCM_PAUSE_ON_AT_ONCE,				/* Immediate pause 			*/
	PCM_PAUSE_OFF						/* Unpause 			*/
} PcmPauseCmd;

/* Force switchability check value */
typedef enum {
	PCM_CHANGE_OK_AT_ONCE		= 0,	/* Can be switched on immediately.			*/
	PCM_CHANGE_NO_DATA			= 1,	/* Insufficient data supply.				*/
	PCM_CHANGE_NO_ENTRY			= 2,	/* No entry.				*/
	PCM_CHANGE_TOO_LARGE_HEADER	= 3		/* Header too large.			*/
										/* (Buffer size too small)	*/
} PcmChangeStatus;

/* Method of data transfer */
typedef enum {
	PCM_TRMODE_CPU = 0,					/* Software transfer 			*/
	PCM_TRMODE_SDMA,					/* DMA Cycle Steel 		*/
	PCM_TRMODE_SCU						/* DMA for SCU				*/
} PcmTrMode;

/* [Note] 
	Transfer from ring buffer to PCM buffer is fixed to software transfer.
*/

/*------------------------- Default value -------------------------*/

/* Maximum amount of work to be done in a single task [sample/1ch] */
#define PCM_DEFAULT_SAMPLE_1TASK		(PCM_SIZE_1K)

/* Start playback trigger size [byte] */
#define PCM_DEFAULT_SIZE_START_TRG		(0)

/* Start Playback Trigger Sample [sample/1ch] */
#define PCM_DEFAULT_SAMPLE_START_TRG	(PCM_SIZE_4K)

/* Stop Playback Trigger Sample [sample/1ch] */
#define PCM_DEFAULT_SAMPLE_STOP_TRG		(PCM_SIZE_2K)


/*------------------------- Processing Macro -------------------------*/

/* 4BYTE character code is expressed in Uint32 type.
 *   (If you write'cvid 'etc., the wording will come out, so to avoid this)
 */
#define PCM_4BYTE_CHAR(a,b,c,d)					\
	(	((Uint32)((a) & 0x000000FF) << 24) | 	\
		((Uint32)((b) & 0x000000FF) << 16) | 	\
		((Uint32)((c) & 0x000000FF) <<  8) | 	\
		 (Uint32)((d) & 0x000000FF)				\
	)

/* SEGA_PCM compatible */
#define PCM_PARA_WORK_ADDR(para)	PCM_PARA_WORK(para)
#define PCM_PARA_BUF_ADDR(para)		PCM_PARA_RING_ADDR(para)
#define PCM_PARA_BUF_SIZE(para)		PCM_PARA_RING_SIZE(para)
#define PCM_CreateMemMovie			PCM_CreateMemHandle
#define PCM_DestroyMemMovie			PCM_DestroyMemHandle

/*-------------------- Structure Access Macro --------------------*/

/* Access macro to creation parameters */
#define PCM_PARA_WORK(para)			((para)->work)
#define PCM_PARA_RING_ADDR(para)	((para)->ring_addr)
#define PCM_PARA_RING_SIZE(para)	((para)->ring_size)
#define PCM_PARA_PCM_ADDR(para)		((para)->pcm_addr)
#define PCM_PARA_PCM_SIZE(para)		((para)->pcm_size)

/* Macro for accessing playback information configuration parameters */
#define PCM_INFO_FILE_TYPE(info)		((info)->file_type)
#define PCM_INFO_DATA_TYPE(info)		((info)->data_type)
#define PCM_INFO_FILE_SIZE(info)		((info)->file_size)
#define PCM_INFO_CHANNEL(info)			((info)->channel)
#define PCM_INFO_SAMPLING_BIT(info)		((info)->sampling_bit)
#define PCM_INFO_SAMPLING_RATE(info)	((info)->sampling_rate)
#define PCM_INFO_SAMPLE_FILE(info)		((info)->sample_file)
#define PCM_INFO_COMPRESSION_TYPE(info)	((info)->compression_type)

/* Access to the handle (work) */
#define PCM_HN_START_TRG_SIZE(hn)	(((*(PcmWork **)hn))->para.start_trg_size)
#define PCM_HN_START_TRG_SAMPLE(hn)	(((*(PcmWork **)hn))->para.start_trg_sample)
#define PCM_HN_STOP_TRG_SAMPLE(hn)	(((*(PcmWork **)hn))->para.stop_trg_sample)
#define PCM_HN_CNT_LOOP(hn)			(((*(PcmWork **)hn))->status.cnt_loop)


/*-------------------------- Data type --------------------------*/
/* The handle */
typedef void *PcmHn;

/* Error Registration function */
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

/* Contents of each last command issued */
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

/* Playback information */
typedef struct {
	PcmFileType		file_type;			/* File type 					*/
	PcmDataType		data_type;			/* Data type 					*/
	Sint32			file_size;			/* file size[byte]
										 * Allow more supply to the ring buffer
										 * But that part is not processed.
										 */
	Sint32			channel;			/* Number of channels 						*/
	Sint32			sampling_bit;		/* Number of sampling bits 			*/
	Sint32			sampling_rate;		/* Sampling Rate [Hz] 			*/
	Sint32			sample_file;		/* Sample number of files [sample/1ch]	*/
	Sint32			compression_type;	/* Compression type 						*/
} PcmInfo;

/* PCM transfer information */
typedef struct {
	Sint8		*dst1;				/* Destination address 1 						*/
	Sint8		*dst2;				/* Destination address 2 						*/
	Sint8		*src;				/* Source address 						*/
	Sint32		size;				/* Number of bytes transferred 						*/
} PcmCopyTbl;

/* Various control information */
typedef struct {
	/* Overall control information */
	Sint32			stat_start;		/* 'STAT' Start of status 				*/
	PcmPlayStatus	play;			/* Playback state 							*/
	PcmFlag			ignore_header;	/* 1: Ignore header information 					*/
	PcmFlag			need_ci;		/* 1: Read CI from sub-header 			*/
	Sint32			cnt_loop;		/* Loop Count 			*/

	/* Playback information */
	Sint32		info_start;			/* 'INFO' Start playback information 				*/
	PcmInfo		info;				/* Playback information 							*/

	/* Ring buffer control information */
	Sint32		ring_start;			/* 'RING' Start of ring buffer control information 	*/
	Sint32		ring_write_offset;	/* Ring buffer write position [byte] 	*/
									/*	(Offset from beginning of file)			*/
	Sint32		ring_read_offset;	/* Ring buffer read offset 	*/
									/*	(Offset from beginning of file)			*/
	Sint8		*ring_end_addr;		/* Ring buffer end address 			*/
	Sint8		*ring_write_addr;	/* Ring buffer write address 		*/
	Sint8		*ring_read_addr;	/* Ring buffer read address 		*/
	Sint32		media_offset;		/* Offset from the beginning of the file to the data 	*/

	/* PCM regeneration information */
	Sint32		pcm_start;			/* 'PCM ' Start of PCM playback information 			*/
	Sint8		*pcm2_addr;			/* PCM second channel buffer start address 	*/
	Sint32		pcm_bsize;			/* PCM buffer size in sound memory [byte/1ch]*/
	Sint32		pcm_write_offset;	/* PCM buffer write position [byte]		*/
									/*  (Offset from the beginning of the PCM buffer)	*/
	Sint32		cnt_4ksample;		/* PCM play address 4k per sample update counter */
	Sint32		sample_write_file;	/* Number of samples supplied with this file [sample/1ch]*/
									/* (Total amount of supply to the present date of this file)		*/
	Sint32		sample_write;		/* Number of Supply Samples [sample/1ch]			*/
									/* (Total current supply from the first film)	*/
	Sint32		sample_film_start;	/* Sample at Film Start [sample/1ch]	*/
									/* (From the first film to the current film start)	*/
	Sint32		sample_pause;		/* Sample at Pause [sample/1ch]		*/
									/* (From the first film to the most recent pause)	*/
	Sint32		count_start;		/* CPU clock timer start time 		*/
									/* 						CPU Clock 15 	*/
	Sint32		clock_scale;		/* CPU clock timer scale 		*/
	PcmFlag		fill_silence;		/* Silence-set flag					*/
	Sint32		*pwork_addr;		/* Work for pause processing					*/
	Sint32		pwork_size;			/* The work size [byte] for the pause processing 		*/
									/*  (Required if using the Pause feature)	*/
	Sint32		cnt_sg;				/* Sound group counter				*/
	Sint32		onetask_sample;		/* How much to handle in one task [sample/1ch]	*/
	Sint32		onetask_size;		/* How much to handle in one task [byte/1ch]	*/
	Sint32		onetask_sg;			/* How much to process in one task [sg/1ch]		*/

	/* Sound driver related */
	Sint32		sddrv_start;		/* 'SDRV' Sound driver related start 	*/
	PcmCommand	command;			/* Contents of each last command issued 		*/

	/* Information about PCM data transfer */
	Sint32		copy_start;			/* 'COPY' Start related to PCM data transfer 		*/
	PcmTrMode	copy_mode_pcm;		/* Method of data transfer 					*/
	PcmFlag		dma_hold;			/* 1: This handle holds DMA resources*/
	Uint32		pcm_renew_size;		/* Use the PCM buffer write pointer update size at the end of DMA*/
	Uint32		buf_renew_size;		/* DMA end-time buffer read pointer update size 	*/
	Sint32		copy_idx;			/* The index of the PCM transfer information table 	*/
	PcmCopyTbl 	copy_tbl[4];		/* PCM transfer information table 					*/

	/* Time management information */
	Sint32		vbl_start;			/* 'VBL ' Start time management information 			*/
	Sint32		vbl_film_start;		/* Film Start Time [vbl] 				*/
	Sint32		vbl_pause;			/* Pause Start Time [vbl] 					*/
	Sint32		vbl_pcm_start;		/* PCM start time [vbl]
									 * Accidentally counting cnt_4ksample at the beginning
									 */

	/* ADPCM isolation function pointer */
	void (*audio_process_fp)(PcmHn hn);
									/* 1 Audio sample processing
									 * pcm_AudioProcess      Or
									 * pcm_AudioProcessAdpcm Registered
									 * Be done
									 */

	/* Error information */
	Uint32		err_start;			/* 'ERR ' Start of error information 				*/
	Uint32		cnt_load_miss;		/* PCM load miss counter 				*/
	Uint32		cnt_task_call;		/* Task call counters 				*/
	Uint32		cnt_over_task_call;	/* Excessive task call counters 			*/
	Uint32		cnt_buf_empty;		/* Buffer empty task counter 			*/
	Uint32		max_late_time;		/* Maximum delay time [sample] 				*/
	Uint32		max_late_sample;	/* Maximum delay time [sample] 				*/
	Sint32		cnt_fill_silence;	/* Silence setting counter						*/
} PcmStatus;

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
	PcmTrMode	tr_mode;		/* Transfer mode */
} PcmFileGfsPara;

typedef struct {
	StmHn		stm;			/* Stream handle */
	Sint32		load_sect;		/* Maximum number of sectors to transfer */
	Sint32		old_cd_buf_num;	/* Number of previous CD buffers */
	Bool		dma_state;		/* True if DMA transfer is in process */
	Sint32		dma_sect;		/* Number of sectors to DMA transfer */
	Uint32		*write_addr;	/* Address to write to */
	Sint32		buf_bsize;		/* Number of bytes in the free buffer */
	Sint32		write_bsize;	/* Number of bytes transferred */
	Sint32		sect_bsize;		/* Number of bytes in a sector */
	Sint32		audio_1st_sect;	/* The first audio sector */
	StmTrFunc	load_func;		/* Transfer function */
} PcmFileStmPara;

typedef struct {
	Sint32		faccess_type;		/* File access type */
	/* File local data */
	union	{
		PcmFileGfsPara	gfs;
		PcmFileStmPara	stm;
	} data;
} PcmFilePara;

/* Execution functions */
typedef struct {
	void	(*start)(PcmHn pcm);
	void	(*task)(PcmHn pcm);
	Sint32	(*preload_file)(PcmHn pcm, Sint32 size);
	void	(*set_load_num)(PcmHn pcm, Sint32 load_sct);
	void	(*set_trmode)(PcmHn pcm, PcmTrMode mode);
} PcmExecFunc;

/* Empty declaration for cross-references */
struct PcmWork;

/* Creation parameters */
typedef struct {
	struct PcmWork *work;		/* Work address 						*/
								/*  (The work area for each handle used by the library) */
	Sint8		*ring_addr;		/* The first address of the buffer that stores the file */
								/*  (Address of the file in memory, if it is a file) 	*/
								/*  (Ring buffer address for CD files) 	*/
	Sint32		ring_size;		/* Buffer size [byte] 					*/
								/*  (File size for memory files) 	*/
								/*  (For CD files, the size of the ring buffer 	*/
	Sint8		*pcm_addr;		/* PCM buffer address of the sound memory 		*/
	Sint32		pcm_size;		/* PCM buffer size in sound memory [sample]*/
								/*  (Number of samples per channel: 4096*1..4096*16) 	*/
} PcmCreatePara;

/* Creation parameters */
typedef struct {
	Sint8		*ring_addr;		/* The first address of the buffer that stores the file */
								/*  (Address of the file in memory, if it is a file) 	*/
								/*  (Ring buffer address for CD files) 	*/
	Sint32		ring_size;		/* Buffer size [byte] 					*/
								/*  (File size for memory files) 	*/
								/*  (For CD files, the size of the ring buffer 	*/
	Sint8		*pcm_addr;		/* PCM buffer address of the sound memory 		*/
	Sint32		pcm_size;		/* PCM buffer size of the sound memory 		*/
								/*  (Number of samples per channel: 4096*1..4096*16) 	*/

	Sint32		command_blk_no;		/* Command block number 				*/
	Sint32		pcm_stream_no;		/* PCM stream regeneration number 			*/
	Sint32		pcm_pan;			/* Pan [0..31] 					*/
	Sint32		pcm_level;			/* Level [0..7] 					*/

	Sint32		start_trg_size;		/* Start playback trigger size [byte] 			*/
	Sint32		start_trg_sample;	/* Start Playback Trigger Sample [sample/1ch] 	*/
	Sint32		stop_trg_sample;	/* Stop Playback Trigger Sample [sample/1ch] 	*/
} PcmPara;

/* PCM work */
typedef struct {
	Sint32			work_start;		/* 'WORK' Start of work 			*/
	PcmPara 		para;			/* Parameters 					*/
	PcmStatus 		status;			/* Various control information 				*/
	PcmFilePara 	filepara;		/* File access parameters 	*/
	PcmExecFunc 	execfunc;		/* Execution functions 					*/
} PcmWork;

/*-------------------------- Function Declaration --------------------------*/
/* Library initialization */
Bool PCM_Init(void);

/* The process of closing the library */
void PCM_Finish(void);

/* ADPCM Declaration of use */
void PCM_DeclareUseAdpcm(void);

/* Create handle (memory) */
PcmHn PCM_CreateMemHandle(PcmCreatePara *para);

/* Clear handle (memory) */
void PCM_DestroyMemHandle(PcmHn pcm);

/* Create handles (File System) */
PcmHn PCM_CreateGfsHandle(PcmCreatePara *para, GfsHn gfs);

/* Clear handles (File System) */
void PCM_DestroyGfsHandle(PcmHn pcm);

/* Creating handles (Stream System) */
PcmHn PCM_CreateStmHandle(PcmCreatePara *para, StmHn stm);

/* Clear handle (Stream System) */
void PCM_DestroyStmHandle(PcmHn pcm);

/* Playback task */
void PCM_Task(PcmHn pcm);

/* V blank in processing function */
void PCM_VblIn(void);

/* Start playback */
void PCM_Start(PcmHn pcm);

/* Stop playback */
void PCM_Stop(PcmHn pcm);

/* Pause */
void PCM_Pause(PcmHn pcm, PcmPauseCmd cmd);

/* Specify the number of loops */
void PCM_SetLoop(PcmHn pcm, Sint32 cnt_loop);

/* Register the next handle to play */
void PCM_EntryNext(PcmHn pcm);

/* Force switch of the handle */
void PCM_Change(void);

/* Get handle toggle status */
PcmChangeStatus PCM_CheckChange(void);

/* Work address for pause processing, setting size */
void PCM_SetPauseWork(Sint32 *addr, Sint32 size);

/* Set the maximum number of transmit sectors */
void PCM_SetLoadNum(PcmHn pcm, Sint32 read_sct);

/* Playback pan settings */
void PCM_SetPan(PcmHn pcm, Sint32 pan);

/* Playback volume settings */
void PCM_SetVolume(PcmHn pcm, Sint32 volume);

/* PCM regeneration parameter changes */
void PCM_ChangePcmPara(PcmHn pcm);

/* PCM regeneration information configuration */
void PCM_SetInfo(PcmHn hn, PcmInfo *info);

/* Set the limit for the amount of work you want to handle in a single task [sample/1ch] */
void PCM_Set1TaskSample(PcmHn pcm, Sint32 sample);

/* Set the start playback trigger size [byte] */
void PCM_SetStartTrgSize(PcmHn pcm, Sint32 size);

/* Start Playback Trigger Set Sample [sample/1ch] */
void PCM_SetStartTrgSample(PcmHn pcm, Sint32 sample);

/* Stop Playback Trigger Sample Settings [sample/1ch] */
void PCM_SetStopTrgSample(PcmHn pcm, Sint32 sample);

/* PCM command block number configuration */
/* Void PCM_SetPcmCmdBlockNo(PcmHn PCM, Sint32 blk_no) */

/* PCM stream number configuration */
void PCM_SetPcmStreamNo(PcmHn pcm, Sint32 stream_no);

/* Setting the method for transferring data (CD blocks → ring buffers) */
void PCM_SetTrModeCd(PcmHn pcm, PcmTrMode mode);

/* Set the data transfer method (ring buffer → PCM buffer) */
/* Void PCM_SetTrModePcm (PcmHn PCM, PcmTrMode mode) */

/* Get playback time */
Sint32 PCM_GetTime(PcmHn pcm);

/* Get playback status */
PcmPlayStatus PCM_GetPlayStatus(PcmHn pcm);

/* Get write buffer address */
Uint32 *PCM_GetWriteBuf(PcmHn pcm, Sint32 *free_size, Sint32 *free_total);

/* Notification of the write size to the buffer */
void PCM_NotifyWriteSize(PcmHn pcm, Sint32 write_size);

/* Load files into memory */
Sint32 PCM_PreloadFile(PcmHn pcm, Sint32 size);

/* Read file headers into memory */
/* Sint32 PCM_PreloadHeader(PcmHn PCM) */

/* Register error function */
void PCM_SetErrFunc(PcmErrFunc func, void *obj);

/* Get error information */
PcmErrCode PCM_GetErr(void);

/* Private */
Bool PCM_IsDeath(PcmHn pcm);

#endif	/* SEGA_PCM_H */
