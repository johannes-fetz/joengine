/******************************************************************************
 *      Software library
 *
 *      Copyright (c) 1994,1995,1996 SEGA
 *
 * Library:File system
 * Module :Library header
 * File   :sega_gfs.h
 * Date   :1996-02-18
 * Version:2.11
 * Author :H.T.
 *
 *************************************************************************** */

#if !defined(SEGA_GFS_H)
#define SEGA_GFS_H

/*****************************************************************************
 *      Include files
 *****************************************************************************/
#include "SEGA_CDC.H"

/* Version   */
                            /*             1 2 3 4  */
                            /*    1234567890123456789012345678901234567890 1234567890  */
                            /*    +++++++++++++++++++++++++++++++++++          */
#if defined(USE_SGL)
    #define GFS_VERSION_STR     ("GFS_SGL Version 2.11 1996-02-18")
#else
    #define GFS_VERSION_STR     ("GFS_SBL Version 2.11 1996-02-18")
#endif
#define GFS_VERSION_LEN 32

/*****************************************************************************
 *      Constant macros
 *****************************************************************************/
#define GFS_FNAME_LEN   12              /* Length of the file name                 */

/* File attributes */
#define GFS_ATR_DIR     0x80            /* Directory                     */
#define GFS_ATR_CDDA    0x40            /* CDDA file                     */
#define GFS_ATR_INTLV   0x20            /* Contains interleaved sectors       */
#define GFS_ATR_FORM2   0x10            /* Contains Form2 sectors                */
#define GFS_ATR_FORM1   0x08            /* Contains Form1 sectors                */
#define GFS_ATR_END_TBL 0x01            /* End of the directory table       */

#define GFS_BUFSIZ_INF  -1              /* There is no limit to the size of the load space   */
#define GFS_RPARA_DFL   0x7fffffff      /* The initial value of the read parameter       */

/* Demo   */
#define GFS_DDS_ADDR    0x6000ccc       /* Demo Demo Information Reference area address   */
#define GFS_DDS_EXEC    0x01            /* Indicates DDS is running                  */
#define GFS_DDS_ID()    (*(Uint8 *)(GFS_DDS_ADDR+0))  /* The value of the DDS identifier      */
#define GFS_DDS_TNO()   (*(Uint8 *)(GFS_DDS_ADDR+1))  /* First CDDA track No. */
#define GFS_DDS_FID()   (*(Uint16 *)(GFS_DDS_ADDR+2)) /* Directory identifier */
#define GFS_IS_DDS()    (GFS_DDS_ID()==GFS_DDS_EXEC)  /* DDS in Progress Decision    */

/* Error code */
#define GFS_ERR_OK              0       /* Successful completion                         */
#define GFS_ERR_CDRD            (-1)    /* CD read error                   */
#define GFS_ERR_CDNODISC        (-2)    /* CD is not inserted           */
#define GFS_ERR_CDROM           (-3)    /* The disc is not a CD-ROM           */
#define GFS_ERR_DIRTBL          (-4)    /* Directory administration table is invalid   */
#define GFS_ERR_OPENMAX         (-5)    /* Invalid value for maximum open count         */
#define GFS_ERR_DIR             (-6)    /* The specified file is not a directory */
#define GFS_ERR_CDBFS           (-7)    /* Of the CD block file system     */
                                        /* Not declared for use               */
#define GFS_ERR_NONAME          (-8)    /* Cannot handle filenames             */
#define GFS_ERR_NEXIST          (-9)    /* The specified file does not exist   */
#define GFS_ERR_FID             (-10)   /* Bad file identifier             */
#define GFS_ERR_HNDL            (-11)   /* Bad file handle           */
#define GFS_ERR_SEEK            (-12)   /* Seek position incorrect                 */
#define GFS_ERR_ORG             (-13)   /* Reference position value incorrect                   */
#define GFS_ERR_NUM             (-14)   /* Negative number of bytes                     */
#define GFS_ERR_OFS             (-15)   /* Incorrect offset                 */
#define GFS_ERR_FBUSY           (-16)   /* Processing of the specified file remains   */
#define GFS_ERR_PARA            (-17)   /* Incorrect parameter                 */
#define GFS_ERR_BUSY            (-18)   /* Library function is running             */
#define GFS_ERR_NOHNDL          (-19)   /* The file handle is full     */
#define GFS_ERR_PUINUSE         (-20)   /* Pickup in progress               */
#define GFS_ERR_ALIGN           (-21)   /* The work area is not at the 4byte boundary        */
#define GFS_ERR_TMOUT           (-22)   /* Timeout                     */
#define GFS_ERR_CDOPEN          (-23)   /* Tray is open               */
#define GFS_ERR_BFUL            (-24)   /* Buffer full read aborted       */
#define GFS_ERR_FATAL           (-25)   /* The CD is in fatal state                    */

/*****************************************************************************
 *      Enumeration constants
 *****************************************************************************/
/* Access mode (GfsFile:amode) */
enum GfsAccessMode {
    GFS_NWSTAT_NOACT = 0,               /* I have nothing to do              */
    GFS_NWSTAT_FREAD,                   /* Loading                  */
    GFS_NWSTAT_CDREAD                   /* CD reading in progress                */
};


/* Seek Mode (GFS_Seek)               */
enum GfsSeekMode {
    GFS_SEEK_SET = 0,                   /* Seek from beginning               */
    GFS_SEEK_CUR,                       /* Seek from current position         */
    GFS_SEEK_END                        /* Seek from End               */
};


/* Eject mode (GFS_SetGmode:gmode) */
enum GfsGetMode {
    GFS_GMODE_ERASE = 0,                /* Removed from buffer after transfer     */
    GFS_GMODE_RESIDENT                  /* Resides in the CD buffer           */
};


/* Transfer mode (GFS_SetTmode) */
enum GfsTransMode {
    GFS_TMODE_SCU = 0,                  /* DMA transfers via SCU    */
    GFS_TMODE_SDMA0,                    /* DMA cycle steel transfer cn=0 */
    GFS_TMODE_SDMA1,                    /*                              cn=1 */
    GFS_TMODE_CPU,                      /* Software transfer */
    GFS_TMODE_STM,                      /* Stream forwarding */
    GFS_TMODE_END
};

/* Access Server State (GFS_NwExecServer) */
enum GfsServerStatus {
    GFS_SVR_COMPLETED = 0,              /* End of access for all files     */
    GFS_SVR_BUSY,                       /* File access in progress           */
    GFS_SVR_CDPAUSE,                    /* CD buffer full, so paused */
    GFS_SVR_ERROR                       /* Error occurred */
};


/* Directory type (GFS_LoadDir, GFS_SetDir)     */
enum GfsDirType {
    GFS_DIR_ID          = 0,            /* Access by file name is not possible */
    GFS_DIR_NAME                        /* Accessible by file name   */
};


/*****************************************************************************
 *      Processing macros
 *****************************************************************************/
#define GFS_WORK_SIZE(open_max) \
            (sizeof(GfsMng) + ((open_max) - 1) * sizeof(GfsFile))

/*****************************************************************************
 *      Structure Access Processing Macro
 *****************************************************************************/

/* Directory record structure  */
#define GFS_DIR_REC(rec)        ((rec)->dirrec)
#define GFS_DIR_FAD(rec)        CDC_FILE_FAD(&GFS_DIR_REC(rec))
#define GFS_DIR_SIZE(rec)       CDC_FILE_SIZE(&GFS_DIR_REC(rec))
#define GFS_DIR_UNIT(rec)       CDC_FILE_UNIT(&GFS_DIR_REC(rec))
#define GFS_DIR_GAP(rec)        CDC_FILE_GAP(&GFS_DIR_REC(rec))
#define GFS_DIR_FN(rec)         CDC_FILE_FN(&GFS_DIR_REC(rec))
#define GFS_DIR_ATR(rec)        CDC_FILE_ATR(&GFS_DIR_REC(rec))
#define GFS_DIR_FNAME(rec)      ((rec)->fname)


/* Directory information management structure */
#define GFS_DIRTBL_TYPE(dirtbl)         ((dirtbl)->type)
#define GFS_DIRTBL_NDIR(dirtbl)         ((dirtbl)->ndir)
#define GFS_DIRTBL_DIRID(dirtbl)        ((dirtbl)->dir.dir_i)
#define GFS_DIRTBL_DIRNAME(dirtbl)      ((dirtbl)->dir.dir_n)


/* Error condition */
#define GFS_ERR_FUNC(err)       ((err)->func)
#define GFS_ERR_OBJ(err)        ((err)->obj)
#define GFS_ERR_CODE(err)       ((err)->code)


/*****************************************************************************
 *      Data type definition
 *****************************************************************************/

typedef Sint32 (*GfsTransFunc)(void *obj, Sint32 nsct);

     /* File information */
typedef struct {
    Sint32      fid;                    /* File identifier */
    CdcFile     finfo;
    Sint32      sctsz;                  /* Sector length */
    Sint32      nsct;                   /* Number of sectors */
    Sint32      lstrm;                  /* Number of invalid data in the last sector   */
} GfsFinfo;


/* CD buffer resource management */
typedef struct {
    Sint32      bufno;                  /* Buffer compartment number */
    Sint32      sctsz;                  /* Sector size of the CD buffer   */
    Sint32      flt;                    /* Stamp number */
    CdcSubh     subh;                   /* Sub-header */
    Sint32      fmode;                  /* Aperture mode */
    Sint32      puid;                   /* The user ID of the retrieval pickup */
    Sint32      filepos;                /* The position of the first data on the file */
    Sint32      sctpos;                 /* Transfer position */
    /* Playback Range */
    Sint32      sfad;                   /* Start FAD */
    Sint32      efad;                   /* End FAD */
} GfsCdRsrc;


/* Virtual CD buffer management for SCSI files */
typedef struct {
    Sint32      fid;                    /* SCSI file identifier   */
    Sint32      filepos;                /* The position of the first data on the file */
    Sint32      sctpos;                 /* Sector position on the buffer */
    Sint32      sctnum;                 /* Number of sectors in the buffer */
} GfsScsiRsrc;


/* Virtual CD buffer management for memory files */
typedef struct {
    void        *data;                  /* Data area           */
    Sint32      filepos;                /* The position of the first data on the file */
    Sint32      sctpos;                 /* Sector position on the buffer */
    Sint32      sctnum;                 /* Number of sectors in the buffer */
} GfsMemRsrc;


/* Import Source Management */
typedef struct {
    /* Import Source Management */
    Sint32      ftype;                  /* File type         */
    union {
        GfsCdRsrc       cd;             /* CD resources             */
        GfsScsiRsrc     scsi;           /* SCSI file resources   */
        GfsMemRsrc      mem;            /* Memory file resources */
    } rsrc;
} GfsDtsrc;


/* Manage CD loading */
typedef struct {
    GfsFinfo    finfo;                  /* File information         */
    GfsDtsrc    dtsrc;                  /* Import from           */
    Sint32      gmode;                  /* Eject mode       */
    Sint32      stat;                   /* Execution state             */
    /* Flow in management */
    Sint32      sct;                    /* Number of sectors to read     */
    Sint32      sctcnt;                 /* Read counter     */
    Sint32      sctmax;                 /* Maximum number of sectors to read */
} GfsFlow;


/* Data pack structure */
typedef struct {
    void        *data;                  /* Data area           */
    Sint32      adlt;                   /* The number of addresses that have increased     */
    Sint32      len;                    /* Number of bytes transferred         */
    Sint32      nsct;                   /* Number of transmission sectors         */
    Bool        use;                    /* Usage flag           */
} GfsDataPack;


typedef GfsDataPack *GfdpHn;

/* Transfer management */
/* Manages transfers from the CD buffer */
typedef struct {
    void        *buf;                   /* Destination address */
    Sint32      size;                   /* Size of the data area */
    Sint32      wp;                     /* Write offset */
    /* Transfer management */
    GfdpHn      dps;                    /* The data pack from which to transfer */
    GfdpHn      dpd;                    /* Data pack from the destination */
    Sint32      tsctmax;                /* Maximum number of sectors per transfer */
    Sint32      tsct;                   /* The number of target sectors to transfer */
    Sint32      tsctcnt;                /* Number of sectors transferred to the present time */
    Sint32      tbytcnt;                /* Number of bytes transferred in the sector */
    /* Transfer function */
    void        *obj;                   /* The first argument to the transfer function */
    GfsTransFunc tfunc;                 /* Transfer function */
    Sint32      unit;                   /* Transfer Unit */
    Bool        active;                 /* In-motion flag */
    Sint32      stat;                   /* State */
    Sint32      mode;                   /* Transfer mode */
} GfsTrans;


typedef GfsTrans *GftrHn;


/* Manage open files */
typedef struct {
    Bool        used;                   /* Whether this structure is in use */
    Sint32      amode;                  /* Access mode */
    Sint32      astat;                  /* Access status */
    GfsFlow     flow;
    GfsTrans    trans;
} GfsFile;


/* File handle */
typedef GfsFile *GfsHn;


/* User Registration Transfer function */
typedef Sint32 (*GfsTrFunc)(GfsHn gfs, Sint32 nsct);
     

/* Directory record structure (no file name)   */
typedef struct {
    CdcFile     dirrec;
} GfsDirId;


/* Directory record structure (with file name)   */
typedef struct {
    CdcFile     dirrec;
    Sint8       fname[GFS_FNAME_LEN];   /* File name */
} GfsDirName;


/* Directory information management structure */
typedef struct {
    Sint32      type;                   /* The type */
    Sint32      ndir;                   /* Maximum number of elements */
    union {
        GfsDirId *dir_i;                /* Directory information without file name */
        GfsDirName *dir_n;              /* Directory information with file names */
    } dir;
} GfsDirTbl;


/* Error handling functions */
typedef void (*GfsErrFunc)(void *obj, Sint32 ec);

/* Error condition */
typedef struct {
    GfsErrFunc  func;                   /* The calling function on error   */
    void        *obj;                   /* The first argument to the calling function   */
    Sint32      code;                   /* Error code                 */
} GfsErrStat;


/*****************************************************************************
 *      Function declaration
 *****************************************************************************/

/* File system initialization */
Sint32 GFS_Init(Sint32 open_max, void *work, GfsDirTbl *dirtbl);

/* Read directory records */
Sint32 GFS_LoadDir(Sint32 fid, GfsDirTbl *dirtbl);

/* Setting the current directory record */
Sint32 GFS_SetDir(GfsDirTbl *dirtbl);

/* Name to file identifier conversion */
Sint32 GFS_NameToId(Sint8 *fname);

/* Converting a file identifier to a file name */
Sint8 *GFS_IdToName(Sint32 fid);

/* Get directory records from file identifiers */
Sint32 GFS_GetDirInfo(Sint32 fid, GfsDirId *dirrec);

/*  Reset the file system  */
void    GFS_Reset(void);

/* File handle assignment */
GfsHn GFS_Open(Sint32 fid);

/* Unassigning file handles */
void GFS_Close(GfsHn gfs);

/* Moving the access pointer */
Sint32 GFS_Seek(GfsHn gfs, Sint32 ofs, Sint32 org);

/* Get access pointer */
Sint32 GFS_Tell(GfsHn gfs);

/* Check File Exit */
Bool GFS_IsEof(GfsHn gfs);

/* Conversion of byte size to sector size */
Sint32 GFS_ByteToSct(GfsHn gfs, Sint32 nbyte);

/* Get information about file size */
void GFS_GetFileSize(GfsHn gfs, Sint32 *sctsz, Sint32 *nsct, Sint32 *lstsz);

/* Get file information */
void GFS_GetFileInfo(GfsHn gfs, Sint32 *fid, Sint32 *fn, Sint32 *fsize,
                     Sint32 *atr);

/*  Get the number of sectors in the CD buffer compartment      */
Sint32 GFS_GetNumCdbuf(GfsHn gfs);

/* Bulk Load */
Sint32 GFS_Load(Sint32 fid, Sint32 ofs, void *buf, Sint32 bsize);

/* Completion Revert loaded */
Sint32 GFS_Fread(GfsHn gfs, Sint32 nsct, void *buf, Sint32 bsize);

/* Immediate Revert type import */
Sint32 GFS_NwFread(GfsHn gfs, Sint32 nsct, void *buf, Sint32 bsize);

/* Immediate Revert CD Buffer Read */
Sint32 GFS_NwCdRead(GfsHn gfs, Sint32 nsct);

/* Access operation completion check */
Bool GFS_NwIsComplete(GfsHn gfs);

/* Stop access operation */
Sint32 GFS_NwStop(GfsHn gfs);

/* Get access status */
void GFS_NwGetStat(GfsHn gfs, Sint32 *amode, Sint32 *ndata);

/* Perform file-by-file access operations */
Sint32 GFS_NwExecOne(GfsHn gfs);

/* Perform server access operations */
Sint32 GFS_NwExecServer(GfsHn *now_gfs);

/* Setting the eject mode from the CD buffer */
Sint32 GFS_SetGmode(GfsHn gfs, Sint32 gmode);

/* Set the transfer mode */
Sint32 GFS_SetTmode(GfsHn gfs, Sint32 tmode);

/* Setting the number of sectors read into the source buffer */
Sint32 GFS_SetReadPara(GfsHn gfs, Sint32 nsct);

/* Setting the number of transmit sectors from the source buffer to the host */
Sint32 GFS_SetTransPara(GfsHn gfs, Sint32 tsize);

/* Transfer function registration       */
void    GFS_SetTrFunc(GfsHn gfs, GfsTrFunc func);

/* Transfer start in transfer function     */
Uint32  *GFS_StartTrans(GfsHn gfs, Sint32 *dadr);

/* Move the pickup to the access pointer position */
Sint32 GFS_CdMovePickup(GfsHn gfs);

/* Register error handling functions */
void GFS_SetErrFunc(GfsErrFunc func, void *obj);

/* Get error condition */
void GFS_GetErrStat(GfsErrStat *stat);

/*  Track number conversion      */
Sint32  GFS_ConvTno(Sint32 logtno);

/* Get frame address from fid and offset */
Sint32 GFS_GetFad(Sint32 fid, Sint32 ofs);


/*****************************************************************************
 *      Information for the system
 *****************************************************************************/

/*---Constants--------------------------------------------------------------------*/
#define GFS_OPEN_MAX    24              /* Maximum open files */
#define GFS_TBUF_NR     2               /* Number of transfer buffers */
#define GFS_FTYPE_NR    3               /* Number of file types */
#define GFS_SCTBUF_SIZ  2048            /* The size of the sector buffer */
#define GFS_CDBBUF_NR   24              /* Number of blocks of CD blocks */
#define GFS_SELQ_MAX    24              /* Maximum length of the stamp configuration command queue */
#define GFS_FCONQ_MAX   24              /* Maximum length of the connection command queue */

/*---Data type----------------------------------------------------------------*/

/* Functions for working with files (administration tables, members of GfsMng) */
typedef struct {
    Sint32 (*flowin)(GfsFlow *);        /* Source buffer read function   */
    void (*stopin)(GfsFlow *, Bool);    /* Stop reading function             */
    Sint32 (*seek)(GfsFlow *, Sint32);  /* Access pointer configuration functions     */
    Sint32 (*tell)(GfsFlow *);          /* Access pointer Retrieval function     */
} GfsFileFunc;

/* Access server management structure (administration tables, members of GfsMng) */
typedef struct {
    GfsHn   access_file[GFS_OPEN_MAX];  /* Access mode specified file */
    Sint32  nfile;                      /* Number of registered files               */
} GfsSvr;


/* Directory administration structure (administration tables, members of GfsMng) */
typedef struct {
    GfsDirTbl   dirtbl;                 /* Directory Information Management */
    Sint32      nfile;                  /* Number of files */
} GfsDirMng;


/* Aperture Settings command Queue Elements (CD Block Management Structure, GfsCdbMng Members) */
typedef struct {
    Uint8 flt;                          /* Target Aperture number */
    Uint8 fmode;                        /* Aperture mode */
    CdcSubh subh;                       /* Sub-header */
    Sint32 fad;                         /* Start FAD */
    Sint32 snum;                        /* Number of FAD sectors */
} GfcdSelQu;

/* Connection command queue elements (CD block management structure, members of GfsCdbMng) */
typedef struct {
    Sint32 flt;                         /* Settings Aperture */
    Sint32 buf;                         /* True output connection buffer */
    Sint32 flnout;                      /* False Output Connection Aperture (use negative values) */
} GfcdFconQu;    

/* CD Block Management Structure (Administrative Tables, GfsMng Members) */
typedef struct {
    /* Resource Management */
    Sint8 use_buf[GFS_CDBBUF_NR];       /* Buffer compartment usage state */
    Sint8 use_filt[GFS_CDBBUF_NR];      /* Aperture usage state */
    Bool use_pu;                        /* Pickup in use state */
    Sint32 tr_bufno;                    /* Buffer compartment during data transfer   */
    Sint32 puid;                        /* The owner ID of the pickup */
    Sint32 timer;                       /* Timeout counter */
    CdcStat stat;                       /* The condition of the CD block */
    void (*func)(void *);               /* CSCT handler */
    void *obj;                          /* CSCT handler arguments */
    /* GFCD_SetFilt task */
    struct {
        Sint32 len;                     /* The length of the stamp configuration command queue */
        Sint32 stat;                    /* Command queue top state */
        GfcdSelQu selq[GFS_SELQ_MAX];   /* Stamp Configuration command Queue */
    } tsk_setflt;
    /* GFCD_SetFiltCon task */
    struct {
        Sint32 len;                     /* Length of the connection command queue */
        Sint32 stat;                    /* Task End Flag */
        GfcdFconQu fconq[GFS_FCONQ_MAX]; /* Connection command queue */
    } tsk_fltcon;
    /* GFCD_SetCon task */
    struct {
        Sint32 stat;                    /* Task End Flag */
        Sint32 flt;                     /* CD destination aperture */
    } tsk_setcon;
    /* GFCD_GetLenData task */
    struct {
        Sint32 stat;                    /* Task End Flag */
        Sint32 bufno;                   /* The target compartment */
        Sint32 spos;                    /* The first sector position */
        Sint32 usct;                    /* Number of required sectors */
        Sint32 cnt;                     /* The number of times the task is executed */
        Sint32 *nsct;                   /* Number of sectors in the compartment */
        Sint32 *nbyte;                  /* The number of bytes in the compartment */
    }tsk_getlen;
    /* GFCD_ReqData task */
    struct {
        Sint32 stat;                    /* Task End Flag */
        Sint32 bufno;                   /* The target compartment */
        Sint32 sctpos;                  /* Sector position */
        Sint32 nsct;                    /* Number of required sectors */
    } tsk_reqdat;
    /* GFCD_DelSctData task */
    struct {
        Sint32 stat;                    /* Task End Flag */
        Sint32 bufno;                   /* The target compartment */
        Sint32 sctpos;                  /* The sector position from which to delete */
        Sint32 nsct;                    /* Number of sectors to delete */
    } tsk_delsct;
    /* GFCD_MoveSctData task */
    struct {
        Sint32 stat;                    /* Task End Flag */
        Sint32 dst;                     /* Destination pane */
        Sint32 src;                     /* Source pane */
        Sint32 spos;                    /* The position of the sector from which the movement begins */
        Sint32 snum;                    /* Number of moving sectors */
        Sint32 fmode;                   /* Stamp mode storage of the destination selector */
    } tsk_movsct;
    /* GFCD_ChgDir task */
    struct {
        Sint32 stat;                    /* Task End Flag */
        Sint16 fid;                     /* Configuration file identifier */
        Sint16 work;                    /* Work buffer */
        Sint32 *ndir;                   /* Number of directory information retained */
    } tsk_chgdir;
} GfsCdbMng;

/* Administration table */
typedef struct {
    Sint32      openmax;                /* Maximum number of open files */
    GfsFileFunc functbl[GFS_FTYPE_NR];  /* Functions for file operations */
    GfsSvr      svr;                    /* Access server */
    GfsDirMng   curdir;                 /* Current directory */
    GfsHn       pickup;                 /* Pickup control in progress file */
    Sint32      sfad;                   /* Playback Range: Start FAD */
    Sint32      efad;                   /* Playback Range: End FAD+1 */
    GfsHn       trans;                  /* File in data transfer */
    GfsErrStat  error;                  /* Error condition */
    Uint32      flags;                  /* Device connection information, etc. */
    Sint32      timer;                  /* Timeout counter */
    GfsCdbMng   cdb;                    /* CD block management structure */
    GfsDataPack srcpk;                  /* Source data pack */
    GfsDataPack dstpk;                  /* Destination Data Pack */
    Uint8       sect_buf[GFS_SCTBUF_SIZ]; /* A buffer for reading one sector */
    GfsFile     file[1];                /* Array of all handles (variable length) */
} GfsMng;

#endif      /* !defined(SEGA_GFS_H) */

/* end of file */
