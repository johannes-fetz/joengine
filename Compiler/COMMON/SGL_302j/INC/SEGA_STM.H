/*****************************************************************************
 *      Software library
 *
 *      Copyright (c) 1994,1995,1996 SEGA
 *
 * Library: Stream system
 * Module : Public header file
 * File   : sega_stm.h
 * Date   : 1996-03-21
 * Version: 2.11
 * Auther : H.T
 *
 *****************************************************************************/
#if !defined(SEGA_STM_H)
#define     SEGA_STM_H

/*****************************************************************************
 *      Include files
 *****************************************************************************/
#include        "SEGA_GFS.H"

/* Version   */
                            /*             1 2 3 4  */
                            /*    1234567890123456789012345678901234567890 1234567890  */
                            /*    +++++++++++++++++++++++++++++++++++          */
#if defined(USE_SGL)
    #define STM_VERSION_STR     ("STM_SGL Version 2.11 1996-03-21")
#else
    #define STM_VERSION_STR     ("STM_SBL Version 2.11 1996-03-21")
#endif
#define STM_VERSION_LEN 32

/*****************************************************************************
 *      Constant macros
 *****************************************************************************/

#define STM_LONG_MAX    0x7fffffff

/* Load specifications during loop playback           */
enum StmLoopRead {
    STM_LOOP_READ       = TRUE,         /* Load on loop playback   */
    STM_LOOP_NOREAD     = FALSE,        /* Do not load on loop playback */
    STM_LOOP_END
};

/* Set the loop stream to the first stream   */
#define STM_LOOP_DFL    ((StmHn)(0xffffffff))

/* Infinite loop specification                   */
#define STM_LOOP_ENDLESS        STM_LONG_MAX

/* Stream Access State (acstat)       */
enum StmAcStat {
    STM_EXEC_COMPLETED  = 0x100,        /* Access finished                 */
    STM_EXEC_PAUSE      = 0x101,        /* Access suspended           */
    STM_EXEC_DOING      = 0x102,        /* Accessing                   */
    STM_EXEC_WAIT       = 0x103,        /* Waiting for transfer gate to open     */
    STM_EXEC_TSKEND     = 0x104,        /* Immediate Revert process completed             */
    STM_EXEC_END
};


/* Transfer unit size           */
enum StmUnitSize {
    STM_UNIT_FORM1      = 2048,         /* Mode 1, mode 2Form1 sector size */
    STM_UNIT_FORM2      = 2324,         /* MODE2Form2 sector size       */
    STM_UNIT_WORD       = 2,            /* Word                       */
    STM_UNIT_BYTE       = 1,            /* Bytes                       */
    STM_UNIT_END
};


/* Stream information                       */
#define STM_KEY_NONE    (-1)            /* Ignore each stream key selection   */

/* The range of sectors to read                   */
enum StmFad {
    STM_FAD_CDTOP       = 150,          /* Disk first FAD              */
    STM_FAD_CDEND       = STM_LONG_MAX  /* Number of sectors to read until the end of the disk*/
};


/* CD buffer sector location                 */
enum StmSctPos {
    STM_CDBUF_TOP       = 0,            /* Buffer compartment top             */
    STM_CDBUF_END       = 0xffff        /* Total number of sectors                   */
};

/* For open load-Zave mode settings   */
enum StmSctType {
    STM_SM_EOR          = CDC_SM_EOR,           /* The last sector in the record */
    STM_SM_VIDEO        = CDC_SM_VIDEO,         /* Video sector         */
    STM_SM_AUDIO        = CDC_SM_AUDIO,         /* Audio sector     */
    STM_SM_DATA         = CDC_SM_DATA,          /* Data sectors         */
    STM_SM_TRIG         = CDC_SM_TRIG,          /* Trigger bits       */
    STM_SM_FORM         = CDC_SM_FORM,          /* Form bits       */
    STM_SM_RT           = CDC_SM_RT,            /* Real-time sectors   */
    STM_SM_EOF          = CDC_SM_EOF,           /* The last sector of the file */
    STM_SM_END
};


/* Maximum number of transmit sectors                     */
#define STM_TR_ALL      STM_LONG_MAX


/* Transfer gate state                       */
enum StmGate {
    STM_GATE_OPEN       = 0x300,                /* Open                 */
    STM_GATE_CLOSE      = 0x301,                /* Close                 */
    STM_GATE_END
};


/* Transfer mode                   */
enum StmTrMode {
    STM_TR_SCU          = GFS_TMODE_SCU,        /* DMA with SCU         */
    STM_TR_SDMA0        = GFS_TMODE_SDMA0,      /* CPU Cycle Steel  */
    STM_TR_SDMA1        = GFS_TMODE_SDMA1,      /* CPU Cycle Steel  */
    STM_TR_CPU          = GFS_TMODE_CPU,        /* Software transfer     */
    STM_TR_END
};


/* Function ID                       */
enum StmFuncId {
    STM_INIT            = 0x300,        /* Stream system initialization     */
    STM_OPENGRP         = 0x301,        /* Open stream group */
    STM_CLOSEGRP        = 0x302,        /* Close stream group */
    STM_GETSTMNUM       = 0x303,        /* Get number of streams           */
    STM_GETSTMHNDL      = 0x304,        /* Get stream handle     */
    STM_SETCDBUFFUNC    = 0x305,        /* Register buffer size monitoring function */
    STM_OPENFID         = 0x306,        /* Open by identifier         */
    STM_OPENFRANGE      = 0x307,        /* Open by Sector Position     */
    STM_OPENRESI        = 0x308,        /* Open resident stream     */
    STM_CLOSE           = 0x309,        /* Close stream         */
    STM_SETKEY          = 0x310,        /* Stream key settings         */
    STM_GETINFO         = 0x311,        /* Get stream information         */
    STM_SCTTOWORD       = 0x312,        /* Convert number of sectors to number of words */
    STM_WORDTOSCT       = 0x313,        /* Conversion of words to sectors */
    STM_SETALIAS        = 0x314,        /* Alias configuration             */
    STM_UNSETALIAS      = 0x315,        /* Remove alias             */
    STM_SETTRBUF        = 0x316,        /* Set the transfer area               */
    STM_SETTRFUNC       = 0x317,        /* Transfer function settings               */
    STM_STARTTRANS      = 0x318,        /* Transfer start in transfer function     */
    STM_SETTRGATE       = 0x319,        /* Opening and closing the transfer gate             */
    STM_SETTRPARA       = 0x320,        /* Set the maximum number of transmit sectors       */
    STM_SETTRFAD        = 0x321,        /* Configuration of the start forwarding FAD            */
    STM_SETTRMODE       = 0x322,        /* Set the transfer mode             */
    STM_RESETTRBUF      = 0x323,        /* Reset the transfer area           */
    STM_GETNUMCDBUF     = 0x324,        /* Get number of valid sectors in the CD buffer */
    STM_GETSCTINFO      = 0x325,        /* Retrieve read sector information     */
    STM_GETLENTRBUF     = 0x326,        /* Get the number of data in the transfer area     */
    STM_ISTRBUFFULL     = 0x327,        /* Check the transfer area full       */
    STM_ISTRANS         = 0x328,        /* Check during transfer             */
    STM_SETEXECGRP      = 0x329,        /* Specify the server execution group     */
    STM_EXECSERVER      = 0x330,        /* Running the server                 */
    STM_MOVEPICKUP      = 0x331,        /* Pickup movement           */
    STM_SETLOOP         = 0x332,        /* Specify the loop stream       */
    STM_GETLOOPCNT      = 0x333,        /* Get loop counter         */
    STM_GETEXECSTAT     = 0x334,        /* Get Execution State               */
    STM_ISCOMPLETE      = 0x335,        /* Check for end of execution           */
    STM_EXECTRANS       = 0x336,        /* Stream data transfer       */
    STM_CONNECTCDBUF    = 0x337,        /* Connection between the stamp and the CD buffer compartment   */
    STM_MOVECDBUF       = 0x338,        /* CD buffer data movement       */
    STM_ERASECDBUF      = 0x339,        /* Erasing CD buffer data       */
    STM_SETERRFUNC      = 0x340,        /* Register error function             */
    STM_GETERRSTAT      = 0x341,        /* Get error condition             */
    STM_FUNC_END
};


/* Error code         */
enum StmErrCode {
    STM_ERR_OK          = GFS_ERR_OK,           /* Successful completion             */
    STM_ERR_CDRD        = GFS_ERR_CDRD,         /* CD read error       */
    STM_ERR_CDNODISC    = GFS_ERR_CDNODISC,     /* CD is not inserted */
    STM_ERR_FID         = GFS_ERR_FID,          /* File identifier incorrect   */
    STM_ERR_HNDL        = GFS_ERR_HNDL,         /* Handle incorrect       */
    STM_ERR_NUM         = GFS_ERR_NUM,          /* Negative bytes, etc.     */
    STM_ERR_PARA        = GFS_ERR_PARA,         /* Incorrect parameter     */
    STM_ERR_NOHNDL      = GFS_ERR_NOHNDL,       /* Handle is full */
    STM_ERR_PUINUSE     = GFS_ERR_PUINUSE,      /* Pickup in progress   */
    STM_ERR_TMOUT       = GFS_ERR_TMOUT,        /* Timeout         */
    STM_ERR_CDOPEN      = GFS_ERR_CDOPEN,       /* Tray is open   */
    STM_ERR_FATAL       = GFS_ERR_FATAL,        /* The CD is in fatal state        */
    STM_ERR_END
};


/*****************************************************************************
 *      Data type definition
 *****************************************************************************/

/* Stream handle           */
typedef struct stmobj {
    void            *grp;           /* The stream group to which you belong           */
    struct stmobj   *alias;         /* The actual stream to operate on             */
    struct stmobj   *sfadlst;       /* From FAD Ascending List                    */
    struct stmobj   *fltlst;        /* List of connection order for the stamp                   */
    GfsHn           gfs;            /* File handle                     */
    GfsFlow         *flow;          /* Load management                         */
    Sint32          (*trfunc)(void *obj, struct stmobj *stm, Sint32 nsct);
                                    /* Transfer function (StmTrFunc)                  */
    void            *obj;           /* The first argument of the transfer function        */
    Uint32          *trsrc;         /* Source address                       */
    Sint32          adlt;           /* Source address change                 */
    Sint32          trfad;          /* Start Forwarding FAD                          */
    Uint8           atr;            /* Attribute                       */
    Uint8           svrstat;
} StmObj;

typedef StmObj      *StmHn;


/* Transfer function     */
typedef Sint32  (*StmTrFunc)(void *obj, StmHn stm, Sint32 nsct);
#define STM_TR_NULLFUNC         ((StmTrFunc)0)

/* Error function   */
typedef void    (*StmErrFunc)(void *obj, Sint32 ec);
#define STM_ERR_NULLFUNC        ((StmErrFunc)0)

/* Buffer monitoring functions  */
typedef void    (*StmCdbufFunc)(void *obj);
#define STM_CDBUF_NULLFUNC      ((StmCdbufFunc)0)
#define STM_CDBUF_REST          NULL

/* Stream group           */
typedef struct {
    StmHn       sfadlst;        /* List by FAD from which to start reading          */
    StmHn       fltlst;         /* Aperture connection order list                     */
    StmHn       loopstm;        /* Loop stream                     */
    StmHn       lststm;         /* Final stream                       */
    StmHn       nexttrns;       /* Next transport stream           */
    Sint32      fad;            /* FAD when loading, paused      */
    Sint32      acstat;         /* Access status                         */
                                /* (Completed, pause, wait)      */
    StmHn       chkstm;         /* Buffer size monitoring stream         */
    Sint32      chksctnum;      /* Buffer size monitoring function called number of sectors */
    StmCdbufFunc  chkfunc;      /* Buffer size monitoring function               */
    void        *chkobj;        /* Buffer size monitoring object       */
    Sint32      loopmax;        /* Number of loops                           */
    Sint32      loopcnt;        /* Loop counter                       */
    Sint32      puid;           /* Pickup user ID                 */
    Uint16      atr;            /* Attribute                       */
    Uint8       svrstat;
} StmGrp;

typedef StmGrp  *StmGrpHn;

/* Stream key               */
typedef struct {
    Sint16      fn;             /* File number                         */
    Sint16      cn;             /* Channel number                         */
    Sint16      smmsk;          /* Submode mask pattern             */
    Sint16      smval;          /* Submode comparison value                     */
    Sint16      cimsk;          /* Coding information mask pattern       */
    Sint16      cival;          /* Coding information comparison value               */
} StmKey;

#define STM_KEY_FN(stmkey)      ((stmkey)->fn)
#define STM_KEY_CN(stmkey)      ((stmkey)->cn)
#define STM_KEY_SMMSK(stmkey)   ((stmkey)->smmsk)
#define STM_KEY_SMVAL(stmkey)   ((stmkey)->smval)
#define STM_KEY_CIMSK(stmkey)   ((stmkey)->cimsk)
#define STM_KEY_CIVAL(stmkey)   ((stmkey)->cival)


/* Stream playback range           */
typedef struct {
    Sint32      sfad;           /* Start regeneration FAD                          */
    Sint32      fasnum;         /* Number of regeneration sectors                         */
} StmFrange;

#define STM_FRANGE_SFAD(frange)         ((frange)->sfad)
#define STM_FRANGE_FASNUM(frange)       ((frange)->fasnum)


/* Sector information                   */
typedef struct {
    Sint32      fad;            /* Frame address                     */
    Sint32      fn;             /* File number                         */
    Sint32      cn;             /* Channel number                         */
    Uint8       sm;             /* Submode                           */
    Uint8       ci;             /* Coding information                     */
} StmSct;

#define STM_SCT_FAD(sct)        ((sct)->fad)
#define STM_SCT_FN(sct)         ((sct)->fn)
#define STM_SCT_CN(sct)         ((sct)->cn)
#define STM_SCT_SM(sct)         ((sct)->sm)
#define STM_SCT_CI(sct)         ((sct)->ci)


/* Error management                   */
typedef struct {
    Sint32      code;           /* Error code                         */
    Sint32      where;          /* Location of the error                       */
    StmErrFunc  func;           /* The call function when an error occurs           */
    void        *obj;           /* The first argument of the calling function               */
} StmErrStat;

#define STM_ERR_CODE(err)       ((err)->code)
#define STM_ERR_WHERE(err)      ((err)->where)
#define STM_ERR_FUNC(err)       ((err)->func)
#define STM_ERR_OBJ(err)        ((err)->obj)


/* Stream system management       */
typedef struct {
    StmGrp      *stmgrptbl;     /* Stream group allocation area         */
    Sint32      stmgrpmax;      /* Maximum number of stream groups             */
    StmObj      *stmtbl;        /* Stream allocation area                 */
    Sint32      stmmax;         /* Maximum number of streams                     */
    StmGrpHn    curgrp;         /* The stream group currently being processed         */
    Sint32      sfad;           /* Start regeneration FAD                          */
    Sint32      efad;           /* End of regeneration FAD                          */
    StmErrStat  err;            /* Error management                           */
    Bool        svrexec;        /* STM_ExecServer in progress                 */
} StmMng;


/*****************************************************************************
 *      Processing macros
 *****************************************************************************/

/* Get the work area size                 */
#define STM_WORK_SIZE(grpmax, stmmax)   \
    (sizeof(StmMng) + sizeof(StmGrp) * (grpmax) + sizeof(StmObj) * (stmmax))

/*****************************************************************************
 *      Function declaration
 *****************************************************************************/

/* Stream system initialization             */
Bool    STM_Init(Sint32 grpmax, Sint32 stmmax, void *work);

/* Reset the stream system         */
void    STM_Reset(void);

/* Open stream group         */
StmGrpHn        STM_OpenGrp(void);

/* Close stream group         */
void    STM_CloseGrp(StmGrpHn grp);

/* Get number of streams                   */
Sint32  STM_GetStmNum(StmGrpHn grp);

/* Get stream handle             */
StmHn   STM_GetStmHndl(StmGrpHn grp, Sint32 nstm);

/* Register buffer size monitoring function         */
void    STM_SetCdbufFunc(StmGrpHn grp, StmHn stm, Sint32 nsct, 
                                       StmCdbufFunc func, void *obj);

/* Open a file by identifier       */
StmHn   STM_OpenFid(StmGrpHn grp, Sint32 fid, StmKey *key, Sint32 lpread);

/* Open a file by sector location   */
StmHn   STM_OpenFrange(StmGrpHn grp, StmFrange *frange, StmKey *key,
                                                           Sint32 lpread);

/* Open resident stream             */
StmHn   STM_OpenResi(StmGrpHn grp, Sint32 fid, StmKey *key, Sint32 fad);

/* Close stream                 */
void    STM_Close(StmHn stm);

/* Stream key settings                 */
void    STM_SetKey(StmHn stm, StmKey *stmkey);

/* Get stream information                 */
StmGrpHn        STM_GetInfo(StmHn stm, Sint32 *fid, StmFrange *range, 
                                                Sint32 *bn, StmKey *stmkey);

/* Convert number of sectors to number of words         */
Sint32  STM_SctToWord(StmHn stm, Sint32 nsct);

/* Conversion of words to sectors         */
Sint32  STM_WordToSct(StmHn stm, Sint32 nword);

/* Alias configuration for the stream handle   */
Bool    STM_SetAlias(StmHn alias, StmHn stm);

/* Alias release of stream handle   */
void    STM_UnsetAlias(StmHn alias);

/* Set the transfer area                       */
void    STM_SetTrBuf(StmHn stm, void *buffer, Sint32 nunit, Sint32 unitsize);

/* Transfer function settings                       */
void    STM_SetTrFunc(StmHn stm, StmTrFunc func, void *obj);

/* Transfer start in transfer function             */
Uint32  *STM_StartTrans(StmHn stm, Sint32 *dadr);

/* Opening and closing the transfer gate                     */
void    STM_SetTrGate(StmHn stm, Sint32 gate);

/* Set the maximum number of transmit sectors               */
void    STM_SetTrPara(StmHn stm, Sint32 tsct);

/* Configuration of the start forwarding FAD                    */
void    STM_SetTrFad(StmHn stm, Sint32 fad);

/* Set the transfer mode                     */
void    STM_SetTrMode(StmHn stm, Sint32 tmode);

/* Reset the transfer area                   */
void    STM_ResetTrBuf(StmHn stm);

/* Get number of valid sectors in the CD buffer       */
Sint32  STM_GetNumCdbuf(StmHn stm);

/* Retrieve read sector information             */
Bool    STM_GetSctInfo(StmHn stm, Sint32 spos, StmSct *sinfo);

/* Get the number of data (words) in the transfer area     */
Sint32  STM_GetLenTrBuf(StmHn stm);

/* Check the transfer area full               */
Bool    STM_IsTrBufFull(StmHn stm);

/* Check during stream data transfer     */
Bool    STM_IsTrans(StmHn stm);

/* Specifying a Server Execution Group (Immediate Revert)   */
Bool    STM_NwSetExecGrp(StmGrpHn grp);

/* Specifying the Server Execution Group (Completion Reversion)   */
Bool    STM_SetExecGrp(StmGrpHn grp);

/* Running the server                         */
Sint32  STM_ExecServer(void);

/* Pickup movement                   */
void    STM_MovePickup(StmHn stm, Sint32 off);

/* Recovery in the event of a read error         */
Sint32  STM_Recover(void);

/* Specify the loop stream               */
Bool    STM_SetLoop(StmGrpHn grp, StmHn loopstm, Sint32 loopmax);

/* Get loop counter                 */
Sint32  STM_GetLoopCnt(StmGrpHn grp);

/* Get Execution State                       */
Sint32  STM_GetExecStat(StmGrpHn grp, Sint32 *fad);

/* Check for end of execution                   */
Bool    STM_IsComplete(StmHn stm);

/* Stream data transfer               */
Bool    STM_ExecTrans(StmHn stm);

/* Stream connection                     */
void    STM_ConnectCdbuf(StmHn keystm, StmHn bufstm);

/* CD buffer data movement               */
void    STM_MoveCdbuf(StmHn src, Sint32 spos, Sint32 snum, StmHn dst);

/* Erasing CD buffer data               */
void    STM_EraseCdbuf(StmHn stm, Sint32 spos, Sint32 snum);

/* Register the call function when an error occurs     */
void    STM_SetErrFunc(StmErrFunc func, void *obj);

/* Get error condition                     */
void    STM_GetErrStat(StmErrStat *stat);

#endif  /* SEGA_STM_H   */

/* end of sega_stm.h            */
