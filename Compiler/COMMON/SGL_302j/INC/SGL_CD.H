/*---------------------------------------------------------
 * SGL library for CD functions 1995.10.30
 *---------------------------------------------------------*/

/* sgl_cd.h */
#if	!defined(SGL_CD_H)
#define	SGL_CD_H
#include	"SL_DEF.H"
#include        <SEGA_STM.H>

/*--- A common data type -------------------------------------------------*/
/* File handle			*/
typedef	StmGrpHn	CDHN;


/*--- Key for classifying stream data -----------------------------*/
typedef struct {
    Sint16	cn;			/* Channel number			*/
    Sint16	sm;			/* Submode			*/
    Sint16	ci;			/* Coding information		*/
} CDKEY;

#define	CDKEY_NONE	STM_KEY_NONE	/* Do not select a stream key	*/
#define	CDKEY_TERM	(-2)		/* End of selection of the stream key	*/


/*--- Read area information -------------------------------------------------*/
typedef struct {			/* Copy to work RAM		*/
    void	*addr;			/* Destination area (not transferred: NULL)	*/
    Sint32	unit;			/* Unit size of the transfer area	*/
    Sint32	size;			/* Number of units in the transfer area		*/
} TRANS_COPY;

typedef struct {			/* When using transfer functions	*/
    Sint32	(*func )(void *obj, Uint32 *addr, 
                        Sint32 adinc, Sint32 nsct);	/* Transfer function	*/
    void	*obj;			/* Object			*/
} TRANS_FUNC;

typedef struct {
    Sint32	type;			/* Transfer method			*/
    union {
	TRANS_COPY	copy;		/* Copy to work RAM		*/
	TRANS_FUNC	func;		/* When using transfer functions	*/
    } trans;
} CDBUF;

/* Transfer method				*/
enum TRANS_TYPE {
    CDBUF_COPY	= 0,			/* Copy to work RAM		*/
    CDBUF_FUNC	= 1,			/* Use transfer functions		*/
    CDBUF_TERM	= 2			/* Finished specifying read area information	*/
};

/* Unit size of the transfer area		*/
enum TRANS_UNIT {
    CDBUF_FORM1	= STM_UNIT_FORM1,	/* 2048byte units			*/
    CDBUF_FORM2	= STM_UNIT_FORM2,	/* 2324 bytes			*/
    CDBUF_BYTE	= STM_UNIT_BYTE		/* In bytes			*/
};

/* Constant for retrieving the state of the CD block	*/
#define	CDREQ_FREE	((CDHN)0xffffffff)	/* The free sector of the CD block	*/
#define	CDREQ_FAD	((CDHN)0xfffffffe)	/* Pickup position	*/
#define	CDREQ_DRV	((CDHN)0xfffffffd)	/* Drive status	*/

/*--- CD drive status ---------------------------------------------------*/
enum CDDRV {
    CDDRV_BUSY	= CDC_ST_BUSY,		/* State transition in progress			*/
    CDDRV_PAUSE = CDC_ST_PAUSE,		/* Pause in progress			*/
    CDDRV_STDBY	= CDC_ST_STANDBY,	/* Standby			*/
    CDDRV_PLAY	= CDC_ST_PLAY,		/* Playing CD			*/
    CDDRV_SEEK	= CDC_ST_SEEK,		/* Seeking			*/
    CDDRV_SCAN	= CDC_ST_SCAN,		/* Scanning playing		*/
    CDDRV_OPEN	= CDC_ST_OPEN,		/* Tray is open		*/
    CDDRV_NODISC= CDC_ST_NODISC,	/* No disc available		*/
    CDDRV_RETRY	= CDC_ST_RETRY,		/* Read retry processing		*/
    CDDRV_ERROR	= CDC_ST_ERROR,		/* A read data error has occurred	*/
    CDDRV_FATAL	= CDC_ST_FATAL		/* A fatal error has occurred	*/
};

/*--- Status -------------------------------------------------------*/
enum CDSTAT {
    CDSTAT_PAUSE	= STM_EXEC_PAUSE,	/* Loading stopped	*/
    CDSTAT_DOING	= STM_EXEC_DOING,	/* Loading		*/
    CDSTAT_WAIT		= STM_EXEC_WAIT,	/* Waiting for transfer		*/
    CDSTAT_COMPLETED	= STM_EXEC_COMPLETED	/* Read complete		*/
};

/*--- Error code -----------------------------------------------------*/
enum CDERR {
    CDERR_OK	= STM_ERR_OK,		/* Successful completion			*/
    CDERR_RDERR	= STM_ERR_CDRD,		/* Read error			*/
    CDERR_NODISC= STM_ERR_CDNODISC,	/* No disk is inserted	*/
    CDERR_CDROM	= GFS_ERR_CDROM,	/* The disc is not a CD-ROM       */
    CDERR_IPARA	= GFS_ERR_DIRTBL,	/* Initialization parameters incorrect         */
    CDERR_DIR	= GFS_ERR_DIR,		/* Move to a directory other than     */
    CDERR_NEXIST= GFS_ERR_NEXIST,	/* File does not exist		*/
    CDERR_NUM	= STM_ERR_NUM,		/* Negative bytes, etc.             */
    CDERR_PUINUSE=STM_ERR_PUINUSE,	/* Pickup in progress           */
    CDERR_ALIGN	= GFS_ERR_ALIGN,	/* The work area is not at the 4byte boundary    */
    CDERR_TMOUT	= STM_ERR_TMOUT,	/* Timeout			*/
    CDERR_OPEN	= STM_ERR_CDOPEN,	/* Tray open		*/
    CDERR_FATAL	= STM_ERR_FATAL,	/* The CD drive is in the \u003cfatal\u003e state	*/
    CDERR_BUSY	= (-50)			/* State transition in progress                   */
};

/*--- The size of the work area -------------------------------------------------*/

/* Size of the work area at initialization		*/
#define	SLCD_MAX_OPEN	24
#define	SLCD_WORK_SIZE(nfile)	(nfile * sizeof(GfsDirName))

/*--- Prototype -----------------------------------------------------*/

/* Initialization				*/
Sint32	slCdInit(Sint32 nfile, void *work);

/* Directory move			*/
Sint32	slCdChgDir(Sint8 *pathname);

/* File Open			*/
CDHN	slCdOpen(Sint8 *pathname, CDKEY key[]);

/* Import files			*/
Sint32	slCdLoadFile(CDHN cdhn, CDBUF buf[]);

/* Stream forwarding			*/
Sint32	slCdTrans(CDHN cdhn, CDBUF buf[], Sint32 ndata[]);

/* Reset the transfer area			*/
Bool	slCdResetBuf(CDHN cdhn, CDKEY *key);

/* Load aborted				*/
Sint32	slCdAbort(CDHN cdhn);

/* Load paused			*/
Sint32	slCdPause(CDHN cdhn);

/* Get Status			*/
Sint32	slCdGetStatus(CDHN cdhn, Sint32 ndata[]);

/* Event functions for CD			*/
void	slCdEvent(void);

#endif
