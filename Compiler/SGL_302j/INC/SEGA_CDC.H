/*
 *     Software library
 *
 *     Copyright (c) 1994 SEGA
 *
 *  Library: CD communication interface (CDC)
 *  Module : Header file for CDC library
 *  File   : sega_cdc.h
 *  Date   : 1995-03-31
 *  Version: 1.20
 *  Author : M.M.
 *
 */

/* Support for multiple includes */
#ifndef SEGA_CDC_H
#define SEGA_CDC_H

/*******************************************************************
*       Include files
*******************************************************************/
#include    "SEGA_XPT.H"

/*******************************************************************
*       Constant macros
*******************************************************************/

/* Bit name of the interrupt factor register relationship (HIRQREQ, HIRQMSK) */
#define CDC_HIRQ_CMOK   0x0001  /* Bit0—command-issue capable */
#define CDC_HIRQ_DRDY   0x0002  /* Bit1—Data transfer ready */
#define CDC_HIRQ_CSCT   0x0004  /* Bit2: 1 sector read complete */
#define CDC_HIRQ_BFUL   0x0008  /* Bit3: CD buffer full */
#define CDC_HIRQ_PEND   0x0010  /* Bit4: End of CD play */
#define CDC_HIRQ_DCHG   0x0020  /* Bit5: Disk replacement occurred */
#define CDC_HIRQ_ESEL   0x0040  /* Bit6: End of selector configuration process */
#define CDC_HIRQ_EHST   0x0080  /* Bit7: End of host I/O processing */
#define CDC_HIRQ_ECPY   0x0100  /* Bit8: End of copy/move process */
#define CDC_HIRQ_EFLS   0x0200  /* Bit9: End of file system processing */
#define CDC_HIRQ_SCDQ   0x0400  /* Bit 10: Update of subcode Q complete */
#define CDC_HIRQ_MPED   0x0800  /* Bit11: MPEG-related processing is finished */
#define CDC_HIRQ_MPCM   0x1000  /* Bit12: End of MPEG motion indefinite interval */
#define CDC_HIRQ_MPST   0x2000  /* Bit13: MPEG interrupt status notification */
    
/* Special specification of the sector range (sector position and number of sectors) of the buffer compartment */
/* (Both have lower 16 bits enabled and internally equivalent to 0xffff) */
#define CDC_SPOS_END    -1  /* Indicates the sector position at the end of the compartment */
#define CDC_SNUM_END    -1  /* Indicates the number of sectors from the specified sector position to the end of the compartment */

/* Other special specifications */
#define CDC_PARA_DFL    0       /* Parameter abbreviation */
#define CDC_PARA_NOCHG  -1      /* Unspecified change of the parameter */
#define CDC_NUL_SEL     0xff    /* Special value of the selector number */
#define CDC_NUL_FID     -1      /* Special value of the file identifier (0xffffff) */

/* CD Flag */
#define CDC_CDFLG_ROM   0x80    /* CD-ROM decoding */

/* Hardware Flag (in hardware information) */
#define CDC_HFLAG_MPEG  0x02    /* MPEG available */
#define CDC_HFLAG_HERR  0x80    /* Hardware error occurred */

/* Play mode (in the CD play parameter) */
#define CDC_PM_DFL          0x00    /* Play mode abbreviation */
#define CDC_PM_REP_NOCHG    0x7f    /* Do not change the maximum number of repeats */
#define CDC_PM_PIC_NOCHG    0x80    /* Do not change pickup position */
#define CDC_PM_NOCHG        -1      /* Do not change playback mode (0xff) */

/* Submode */
#define CDC_SM_EOR      0x01    /* The last sector in the record */
#define CDC_SM_VIDEO    0x02    /* Video sector */
#define CDC_SM_AUDIO    0x04    /* Audio sector */
#define CDC_SM_DATA     0x08    /* Data sectors */
#define CDC_SM_TRIG     0x10    /* Trigger on */
#define CDC_SM_FORM     0x20    /* Form bits (1:Form2, 0:Form1)*/
#define CDC_SM_RT       0x40    /* Real-time sectors */
#define CDC_SM_EOF      0x80    /* The last sector of the file */

/* File attributes (in file information) */
#define CDC_ATR_DIRFG   0x02    /* Directory */
#define CDC_ATR_FORM1   0x08    /* Contains Form1 sectors */
#define CDC_ATR_FORM2   0x10    /* Contains Form2 sectors */
#define CDC_ATR_INTLV   0x20    /* Contains interleaved sectors */
#define CDC_ATR_CDDA    0x40    /* CD-DA files */
#define CDC_ATR_DIRXA   0x80    /* Directory files */

/* Standby time */
#define CDC_STNBY_MIN   60      /* Minimum value */
#define CDC_STNBY_MAX   900     /* Maximum value */

/* The subcode flag */
#define CDC_SCD_PACK    0x01    /* Pack data error */
#define CDC_SCD_OVER    0x02    /* Overrun error */

/* Number of CD block transfer words */
#define CDC_DEND_ERR    0xffffff    /* Error in data transfer */

/* Actual data size */
#define CDC_ACTSIZ_ERR  0xffffff    /* The calculation could not be performed */

/* Frame Address Search Results (Sector Location and Frame Address) */
#define CDC_SPOS_ERR    0xffff      /* Search fails */
#define CDC_FAD_ERR     0xffffff    /* The search could not be performed */

/* Mask bit pattern */
#define CDC_STC_MSK     0x0f    /* Status code */

/*******************************************************************
*       Enumeration constants
*******************************************************************/

/* Error code */
enum CdcErrCode {
    CDC_ERR_OK = OK,    /* Function completed successfully */

    CDC_ERR_CMDBUSY=-1, /* Command exit flag is not 1 */
    CDC_ERR_CMDNG  =-2, /* When issuing the command, the CMOK flag is not 1 */
    CDC_ERR_TMOUT  =-3, /* Timeout (waiting for response, waiting for data transfer)*/
    CDC_ERR_PUT    =-4, /* Write preparation failed to free sectors */
    CDC_ERR_REJECT =-5, /* The response to the command is reject */
    CDC_ERR_WAIT   =-6, /* The response to the command was wait */
    CDC_ERR_TRNS   =-7, /* The data transfer size is abnormal */
    CDC_ERR_PERI   =-8  /* Failed to get periodic response */
};

/* Status */
enum CdcStatus {
    /* Status code (CD drive status) */
    CDC_ST_BUSY     = 0x00,     /* State transition in progress */
    CDC_ST_PAUSE    = 0x01,     /* Pause */
    CDC_ST_STANDBY  = 0x02,     /* Standby (Drive down) */
    CDC_ST_PLAY     = 0x03,     /* Playing CD */
    CDC_ST_SEEK     = 0x04,     /* Seeking */
    CDC_ST_SCAN     = 0x05,     /* Scanning playing */
    CDC_ST_OPEN     = 0x06,     /* Tray is open */
    CDC_ST_NODISC   = 0x07,     /* No disc available */
    CDC_ST_RETRY    = 0x08,     /* Read retry processing */
    CDC_ST_ERROR    = 0x09,     /* A read data error has occurred */
    CDC_ST_FATAL    = 0x0a,     /* A fatal error has occurred */

    /* Other */
    CDC_ST_PERI     = 0x20,     /* Periodic response */
    CDC_ST_TRNS     = 0x40,     /* Data transfer request present */
    CDC_ST_WAIT     = 0x80,     /* WAIT */
    CDC_ST_REJECT   = 0xff      /* REJECT */
};

/* Position type (play, specified by CD position parameter during seek) */
enum CdcPosType {
    CDC_PTYPE_DFL,          /* Specify an abbreviation */
    CDC_PTYPE_FAD,          /* Frame addressing */
    CDC_PTYPE_TNO,          /* Track \u003e Index */
    CDC_PTYPE_NOCHG,        /* Unchanged designation (do not change settings) */

    CDC_PTYPE_END
};

/* Ready to transfer type (specified in Ready to transfer data) */
enum CdcDrdyType {
    CDC_DRDY_GET,           /* Eject data (CD block → host) */
    CDC_DRDY_PUT,           /* Write data (Host → CD Block) */

    CDC_DRDY_END
};

/* Scan direction */
enum CdcScanDir {
    CDC_SCAN_FWD = 0x00,    /* Fast forward playback */
    CDC_SCAN_RVS = 0x01     /* Play back quickly */
};

/* Sector length */
enum CdcSctLen {
    CDC_SLEN_2048  = 0,     /* 2048 bytes and 2324 bytes (user data)*/
    CDC_SLEN_2336  = 1,     /* 2336 bytes (up to subheader)*/
    CDC_SLEN_2340  = 2,     /* 2340 bytes (up to header)*/
    CDC_SLEN_2352  = 3,     /* 2352 bytes (entire sector)*/
    CDC_SLEN_NOCHG = -1     /* Do not change settings (0xff) */
};

/* Copy/Move Error Information */
enum CdcCopyErr {
    CDC_COPY_OK   = 0,      /* Successful completion */
    CDC_COPY_NG   = 1,      /* Error occurred */
    CDC_COPY_BUSY = 0xff    /* Copying/moving in progress */
};

/*******************************************************************
*       Structure Access Processing Macro
*******************************************************************/

/* CD status information */
#define CDC_STAT_STATUS(stat)       ((stat)->status)
#define CDC_STAT_FLGREP(stat)       ((stat)->report.flgrep)
#define CDC_STAT_CTLADR(stat)       ((stat)->report.ctladr)
#define CDC_STAT_TNO(stat)          ((stat)->report.tno)
#define CDC_STAT_IDX(stat)          ((stat)->report.idx)
#define CDC_STAT_FAD(stat)          ((stat)->report.fad)

/* Hardware information */
#define CDC_HW_HFLAG(hw)            ((hw)->hflag)
#define CDC_HW_VER(hw)              ((hw)->ver)
#define CDC_HW_MPVER(hw)            ((hw)->mpver)
#define CDC_HW_DRV(hw)              ((hw)->drv)
#define CDC_HW_REV(hw)              ((hw)->rev)

/* CD position parameters */
#define CDC_POS_PTYPE(pos)          ((pos)->ptype)
#define CDC_POS_FAD(pos)            ((pos)->pbody.fad)
#define CDC_POS_TNO(pos)            ((pos)->pbody.trkidx.tno)
#define CDC_POS_IDX(pos)            ((pos)->pbody.trkidx.idx)

/* CD playback parameters */
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

/* Sub-header condition */
#define CDC_SUBH_FN(subh)           ((subh)->fn)
#define CDC_SUBH_CN(subh)           ((subh)->cn)
#define CDC_SUBH_SMMSK(subh)        ((subh)->smmsk)
#define CDC_SUBH_SMVAL(subh)        ((subh)->smval)
#define CDC_SUBH_CIMSK(subh)        ((subh)->cimsk)
#define CDC_SUBH_CIVAL(subh)        ((subh)->cival)

/* Sector information */
#define CDC_SCT_FAD(sct)            ((sct)->fad)
#define CDC_SCT_FN(sct)             ((sct)->fn)
#define CDC_SCT_CN(sct)             ((sct)->cn)
#define CDC_SCT_SM(sct)             ((sct)->sm)
#define CDC_SCT_CI(sct)             ((sct)->ci)

/* File information */
#define CDC_FILE_FAD(file)          ((file)->fad)
#define CDC_FILE_SIZE(file)         ((file)->size)
#define CDC_FILE_UNIT(file)         ((file)->unit)
#define CDC_FILE_GAP(file)          ((file)->gap)
#define CDC_FILE_FN(file)           ((file)->fn)
#define CDC_FILE_ATR(file)          ((file)->atr)

/*******************************************************************
*       Processing macros
*******************************************************************/

/* Obtain the status code from the CD status information */
#define CDC_GET_STC(stat)       (CDC_STAT_STATUS(stat) & CDC_STC_MSK)

/* Retrieve the number of repeats from the CD status information */
#define CDC_GET_REPEAT(stat)    (CDC_STAT_FLGREP(stat) & 0x0f)

/* Check if MPEG is available (TRUE: Enabled; FALSE: Disabled) */
/* Note: This macro is not written in the manual */
#define CDC_IS_MPEG_ENA(hw)     (CDC_HW_MPVER(hw) != 0)

/*******************************************************************
*       Data type declaration
*******************************************************************/

/* CD Status Information (Status + CD Report) */
typedef struct {
    Uint8   status;         /* Status */
    struct {                /* CD reports */
        Uint8   flgrep;     /* CD flag and number of repeats */
        Uint8   ctladr;     /* Control/ADR bytes */
        Uint8   tno;        /* Track number */
        Uint8   idx;        /* Index number */
        Sint32  fad;        /* Frame address */
    } report;
} CdcStat;

/* Hardware information */
typedef struct {
    Uint8   hflag;          /* Hardware flags */
    Uint8   ver;            /* About CD blocks */
    Uint8   mpver;          /* MPEG version information */
    Uint8   drv;            /* CD drive information */
    Uint8   rev;            /* Revision information for the CD block */
} CdcHw;

/* CD position parameters */
typedef struct {
    Sint32 ptype;           /* Position type (specify the type of position parameter)*/
    union {
        Sint32 fad;         /* Frame address, number of sectors */
        struct {
            Uint8 tno;      /* Track number */
            Uint8 idx;      /* Index number */
        } trkidx;
    } pbody;
} CdcPos;

/* CD playback parameters */
typedef struct {
    CdcPos  start;          /* Start position */
    CdcPos  end;            /* End position */
    Uint8   pmode;          /* Play mode (pickup movement, repeat count)*/
} CdcPly;

/* Sub-header condition */
typedef struct {
    Uint8   fn;             /* File number */
    Uint8   cn;             /* Channel number */
    Uint8   smmsk;          /* Mask pattern in submode */
    Uint8   smval;          /* Comparison value for the submode */
    Uint8   cimsk;          /* The mask pattern of coding information */
    Uint8   cival;          /* The comparison value for coding information */
} CdcSubh;

/* Sector information */
typedef struct {
    Sint32  fad;            /* Frame address */
    Uint8   fn;             /* File number */
    Uint8   cn;             /* Channel number */
    Uint8   sm;             /* Submode */
    Uint8   ci;             /* Coding information */
} CdcSct;

/* File information */
typedef struct {
    Sint32  fad;            /* File start frame address */
    Sint32  size;           /* File size in bytes */
    Uint8   unit;           /* File unit size */
    Uint8   gap;            /* Gap size */
    Uint8   fn;             /* File number */
    Uint8   atr;            /* File attributes */
} CdcFile;

/*******************************************************************
*       Variable declaration
*******************************************************************/

/*******************************************************************
*       Function declaration
*******************************************************************/

/* CDP_cmn.c */
Sint32  CDC_GetCurStat(CdcStat *stat);
Sint32  CDC_GetLastStat(CdcStat *stat);
Sint32  CDC_GetHwInfo(CdcHw *hw);
Sint32  CDC_TgetToc(Uint32 *toc);
Sint32  CDC_GetSes(Sint32 sesno, Uint32 *ses);
Sint32  CDC_CdInit(Sint32 iflag, Sint32 stnby, Sint32 ecc, Sint32 retry);
Sint32  CDC_CdOpen(void);
Sint32  CDC_DataReady(Sint32 dtype);
Sint32  CDC_DataEnd(Sint32 *cdwnum);
Sint32  CDC_GetPeriStat(CdcStat *stat);

/* CDP_drv.c */
Sint32  CDC_CdPlay(CdcPly *ply);
Sint32  CDC_CdSeek(CdcPos *pos);
Sint32  CDC_CdScan(Sint32 scandir);

/* CDP_SCD.c. */
Sint32  CDC_TgetScdQch(Uint16 *qcode);
Sint32  CDC_TgetScdRwch(Uint16 *rwcode, Sint32 *scdflag);

/* CDP_dev.c */
Sint32  CDC_CdSetCon(Sint32 filtno);
Sint32  CDC_CdGetCon(Sint32 *filtno);
Sint32  CDC_CdGetLastBuf(Sint32 *bufno);

/* CDP_sel.c. */
Sint32  CDC_SetFiltRange(Sint32 filtno, Sint32 fad, Sint32 fasnum);
Sint32  CDC_GetFiltRange(Sint32 filtno, Sint32 *fad, Sint32 *fasnum);
Sint32  CDC_SetFiltSubh(Sint32 filtno, CdcSubh *subh);
Sint32  CDC_GetFiltSubh(Sint32 filtno, CdcSubh *subh);
Sint32  CDC_SetFiltMode(Sint32 filtno, Sint32 fmode);
Sint32  CDC_GetFiltMode(Sint32 filtno, Sint32 *fmode);
Sint32  CDC_SetFiltCon(Sint32 filtno, Sint32 cflag, Sint32 bufno,
                       Sint32 flnout);
Sint32  CDC_GetFiltCon(Sint32 filtno, Sint32 *bufno, Sint32 *flnout);
Sint32  CDC_ResetSelector(Sint32 rflag, Sint32 bufno);

/* CDP_bif.c. */
Sint32  CDC_GetBufSiz(Sint32 *totalsiz, Sint32 *bufnum, Sint32 *freesiz);
Sint32  CDC_GetSctNum(Sint32 bufno, Sint32 *snum);
Sint32  CDC_CalActSiz(Sint32 bufno, Sint32 spos, Sint32 snum);
Sint32  CDC_GetActSiz(Sint32 *actwnum);
Sint32  CDC_GetSctInfo(Sint32 bufno, Sint32 spos, CdcSct *sct);
Sint32  CDC_ExeFadSearch(Sint32 bufno, Sint32 spos, Sint32 fad);
Sint32  CDC_GetFadSearch(Sint32 *bufno, Sint32 *spos, Sint32 *fad);

/* CDP_bio.c. */
Sint32  CDC_SetSctLen(Sint32 getslen, Sint32 putslen);
Sint32  CDC_GetSctData(Sint32 bufno, Sint32 spos, Sint32 snum);
Sint32  CDC_DelSctData(Sint32 bufno, Sint32 spos, Sint32 snum);
Sint32  CDC_GetdelSctData(Sint32 bufno, Sint32 spos, Sint32 snum);
Sint32  CDC_PutSctData(Sint32 filtno, Sint32 snum);
Sint32  CDC_CopySctData(Sint32 srcbn, Sint32 spos, Sint32 snum,
                        Sint32 dstfln);
Sint32  CDC_MoveSctData(Sint32 srcbn, Sint32 spos, Sint32 snum,
                        Sint32 dstfln);
Sint32  CDC_GetCopyErr(Sint32 *cpyerr);

/* CDP_CFS.c. */
Sint32  CDC_ChgDir(Sint32 filtno, Sint32 fid);
Sint32  CDC_ReadDir(Sint32 filtno, Sint32 fid);
Sint32  CDC_GetFileScope(Sint32 *fid, Sint32 *infnum, Bool *drend);
Sint32  CDC_TgetFileInfo(Sint32 fid, CdcFile *file);
Sint32  CDC_ReadFile(Sint32 filtno, Sint32 fid, Sint32 offset);
Sint32  CDC_AbortFile(void);

/* CDP_reg.c */
Uint32  *CDC_GetDataPtr(void);
Sint32  CDC_GetHirqReq(void);
void    CDC_ClrHirqReq(Sint32 bitpat);
Sint32  CDC_GetHirqMsk(void);
void    CDC_SetHirqMsk(Sint32 bitpat);
Uint32  *CDC_GetMpegPtr(void);



/********************************************************************/
/********************************************************************/
/********************************************************************/
/*------------------------------------------------------------------*/
/*------------------------- MPEG Part -------------------------*/
/*------------------------------------------------------------------*/
/********************************************************************/
/********************************************************************/
/********************************************************************/



/*******************************************************************
*       Constant macros
*******************************************************************/

/* Maximum window size and coordinates */
#define CDC_MPNT_NSX    352           /* NTSC Normal X Size (352) */
#define CDC_MPNT_NSY    240           /* NTSC Normal Y Size (240) */
#define CDC_MPNT_NPX (CDC_MPNT_NSX-1) /* NTSC Normal X Coordinate (351) */
#define CDC_MPNT_NPY (CDC_MPNT_NSY-1) /* NTSC Normal Y Coordinate (239) */

#define CDC_MPNT_HSX (2*CDC_MPNT_NSX) /* NTSC high-definition X-direction size (704) */
#define CDC_MPNT_HSY (2*CDC_MPNT_NSY) /* NTSC High-Definition Y-Direction Size (480) */
#define CDC_MPNT_HPX (CDC_MPNT_HSX-1) /* NTSC high-definition X-direction coordinates (703) */
#define CDC_MPNT_HPY (CDC_MPNT_HSY-1) /* NTSC High-Definition Y-Direction Coordinates (479) */

#define CDC_MPPL_NSX    352           /* PAL Normal X Size (352) */
#define CDC_MPPL_NSY    288           /* PAL Normal Y-direction size (288) */
#define CDC_MPPL_NPX (CDC_MPPL_NSX-1) /* PAL Normal X Coordinate (351) */
#define CDC_MPPL_NPY (CDC_MPPL_NSY-1) /* PAL Normal Y Coordinate (287) */

#define CDC_MPPL_HSX (2*CDC_MPPL_NSX) /* PAL High Definition X-direction size (704) */
#define CDC_MPPL_HSY (2*CDC_MPPL_NSY) /* PAL High-Definition Y-Direction Size (576) */
#define CDC_MPPL_HPX (CDC_MPPL_HSX-1) /* PAL High-Definition X-Direction Coordinates (703) */
#define CDC_MPPL_HPY (CDC_MPPL_HSY-1) /* PAL High-Definition Y Coordinate (575) */

/* MPEG/Video operating state (MPEG operation status bit0-2) */
#define CDC_MPASTV_STOP     0x01    /* Stop */
#define CDC_MPASTV_PRE1     0x02    /* Preparation 1 */
#define CDC_MPASTV_PRE2     0x03    /* Preparation 2 */
#define CDC_MPASTV_TRNS     0x04    /* Transfer (Play) */
#define CDC_MPASTV_CHG      0x05    /* Toggle */
#define CDC_MPASTV_RCV      0x06    /* Reinstatement */

/* MPEG decode state (bit3 of MPEG operation status) */
#define CDC_MPASTD_STOP     0x08    /* MPEG decode stopped */

/* MPEG/Audio Operational State (MPEG Operational Status bit4-6) */
#define CDC_MPASTA_STOP     0x10    /* Stop */
#define CDC_MPASTA_PRE1     0x20    /* Preparation 1 */
#define CDC_MPASTA_PRE2     0x30    /* Preparation 2 */
#define CDC_MPASTA_TRNS     0x40    /* Transfer (Play) */
#define CDC_MPASTA_CHG      0x50    /* Toggle */
#define CDC_MPASTA_RCV      0x60    /* Reinstatement */

/* MPEG/Audio status */
#define CDC_MPSTA_DEC       0x01    /* Audio decode activity flag */
#define CDC_MPSTA_ILG       0x08    /* Audio illegal flag */
#define CDC_MPSTA_BEMPTY    0x10    /* Buffer compartment empty flag for audio */
#define CDC_MPSTA_ERR       0x20    /* Audio error flag */
#define CDC_MPSTA_OUTL      0x40    /* Left channel output flag */
#define CDC_MPSTA_OUTR      0x80    /* Right channel output flag */

/* MPEG/Video status */
#define CDC_MPSTV_DEC       0x0001  /* Video decode activity flag */
#define CDC_MPSTV_DISP      0x0002  /* Display flag */
#define CDC_MPSTV_PAUSE     0x0004  /* Pause flag */
#define CDC_MPSTV_FREEZE    0x0008  /* Freeze flag */
#define CDC_MPSTV_LSTPIC    0x0010  /* Last picture display flag */
#define CDC_MPSTV_FIELD     0x0020  /* Odd field flag */
#define CDC_MPSTV_UPDPIC    0x0040  /* Picture update flag */
#define CDC_MPSTV_ERR       0x0080  /* Video error flag */
#define CDC_MPSTV_RDY       0x0100  /* Ready to output flag */
#define CDC_MPSTV_1STPIC    0x0800  /* First picture display flag */
#define CDC_MPSTV_BEMPTY    0x1000  /* Buffer compartment empty flag for video */

/* MPEG interrupt factor */
#define CDC_MPINT_VSRDY  0x00000001 /* Video stream ready */
#define CDC_MPINT_VSCHG  0x00000002 /* Video stream switching complete */
#define CDC_MPINT_VORDY  0x00000004 /* Ready for video output */
#define CDC_MPINT_VOSTRT 0x00000008 /* Video output started */
#define CDC_MPINT_VDERR  0x00000010 /* Video decoding error */
#define CDC_MPINT_VSERR  0x00000020 /* Video stream data error */
#define CDC_MPINT_VBERR  0x00000040 /* Video buffer compartment connection error */
#define CDC_MPINT_VNERR  0x00000080 /* Data error in the primary video stream */
#define CDC_MPINT_PSTRT  0x00000100 /* Picture Start Detection */
#define CDC_MPINT_GSTRT  0x00000200 /* GOP start detection */
#define CDC_MPINT_SQEND  0x00000400 /* Sequence End Detection */
#define CDC_MPINT_SQSTRT 0x00000800 /* Sequence start detection */
#define CDC_MPINT_VTRG   0x00001000 /* Trigger bit detection for video sectors */
#define CDC_MPINT_VEOR   0x00002000 /* EOR bit detection for video sectors */
#define CDC_MPINT_ATRG   0x00004000 /* Trigger bit detection in the audio sector */
#define CDC_MPINT_AEOR   0x00008000 /* EOR bit detection in audio sectors */
#define CDC_MPINT_ASRDY  0x00010000 /* Audio stream ready */
#define CDC_MPINT_ASCHG  0x00020000 /* Audio stream switching complete */
#define CDC_MPINT_AORDY  0x00040000 /* Ready for audio output */
#define CDC_MPINT_AOSTRT 0x00080000 /* Audio output started */
#define CDC_MPINT_ADERR  0x00100000 /* Audio decode error */
#define CDC_MPINT_ASERR  0x00200000 /* Audio stream data error */
#define CDC_MPINT_ABERR  0x00400000 /* Audio buffer compartment connection error */
#define CDC_MPINT_ANERR  0x00800000 /* Data error in the primary audio stream */

/* Connection mode (in MPEG decoder destination parameter) */
#define CDC_MPCMOD_EOR      0x01    /* Toggle with EOR bit */
#define CDC_MPCMOD_SEC      0x02    /* Switch at the system end (SEC) */
#define CDC_MPCMOD_DEL      0x04    /* Erase sectors */
#define CDC_MPCMOD_IGPTS    0x08    /* Does not identify PTS */
#define CDC_MPCMOD_VCLR     0x10    /* Clear VBV */
#define CDC_MPCMOD_VWCLR    0x20    /* Clear VVVV+WBC */
#define CDC_MPCMOD_BEF      0x40    /* Exit criteria determination before the back stamp */

/* Layer designation (in MPEG decoder destination parameters) */
#define CDC_MPLAY_SYS       0x00    /* The Cosmetic Layer */
#define CDC_MPLAY_AUDIO     0x01    /* Audio layer */
#define CDC_MPLAY_VIDEO     0x01    /* Video layer */

/* Specify MPEG/Video Picture Search (in MPEG Decoder Destination Parameters) */
#define CDC_MPSRCH_OFF      0x00    /* Do not search for pictures */
#define CDC_MPSRCH_VIDEO    0x80    /* Search for pictures */
#define CDC_MPSRCH_AV       0xc0    /* Discard audio data for picture search */

/* Stream mode (in MPEG stream parameters) */
#define CDC_MPSMOD_SNSET    0x01    /* Configure stream number settings */
#define CDC_MPSMOD_SNIDF    0x02    /* Identify the stream number */
#define CDC_MPSMOD_CNSET    0x10    /* Configure channel number settings */
#define CDC_MPSMOD_CNIDF    0x20    /* Identify the channel number */

/* Interpolation mode (in Screen Special Effects Parameters) */
#define CDC_MPITP_YH        0x01    /* Horizontal interpolation of Y */
#define CDC_MPITP_CH        0x02    /* Horizontal interpolation of C. */
#define CDC_MPITP_YV        0x04    /* Make a vertical interpolation of Y */
#define CDC_MPITP_CV        0x08    /* Perform a vertical interpolation of C. */

/* Transparent bit mode (in the Screen Special Effects parameter) */
#define CDC_MPTRP_DFL       0x00    /* Normal (no transparent bit processing) */
#define CDC_MPTRP_64        0x01    /* Brightness 64 */
#define CDC_MPTRP_128       0x02    /* Brightness 128 */
#define CDC_MPTRP_256       0x03    /* Brightness 256 */
#define CDC_MPTRP_MAG       0x04    /* Zoom in on transparent areas */

/* Blur mode (in Screen Special Effects parameters) */
#define CDC_MPSOFT_ON       0x01    /* Blur */

/* Mute MPEG/Audio */
#define CDC_MPMUT_DFL       0x04    /* Abbreviated (not muted) */
#define CDC_MPMUT_R         0x01    /* Mute the right channel */
#define CDC_MPMUT_L         0x02    /* Mute the left channel */

/*******************************************************************
*       Enumeration constants
*******************************************************************/

/* Error code */
enum CdcMpErrCode {
    CDC_ERR_MP_COMU = -20   /* MPCM flag is not 1 */
};

/* Next stream flag (specify current stream and next stream) */
enum CdcMpStf {
    CDC_MPSTF_CUR  = 0,     /* Specify current Destination/Stream (first-time) */
    CDC_MPSTF_NEXT = 1      /* Specify next destination/stream (next) */
};

/* Picture type (Low 3 bits are enabled) */
enum CdcMpPict {
    CDC_MPPICT_I = 0x01,    /* I-picture */
    CDC_MPPICT_P = 0x02,    /* P-Picture */
    CDC_MPPICT_B = 0x03,    /* B Picture */
    CDC_MPPICT_D = 0x04     /* D-Picture */
};

/* MPEG operating mode */
enum CdcMpAct {
    CDC_MPACT_NMOV = 0,     /* Video playback mode */
    CDC_MPACT_NSTL = 1,     /* Still image playback mode */
    CDC_MPACT_HMOV = 2,     /* High-definition video playback mode (not supported) */
    CDC_MPACT_HSTL = 3,     /* High-definition still image playback mode */
    CDC_MPACT_SBUF = 4      /* MPEG sector buffer mode */
};

/* Decode timing */
enum CdcMpDec {
    CDC_MPDEC_VSYNC = 0,    /* Decode with VSYNC synchronization */
    CDC_MPDEC_HOST  = 1     /* Host-synchronous decoding */
};

/* The destination of the image data */
enum CdcMpOut {
    CDC_MPOUT_VDP2 = 0,     /* Output to VDP2 */
    CDC_MPOUT_HOST = 1      /* Output to host */
};

/* MPEG playback mode */
enum CdcMpPly {
    CDC_MPPLY_SYNC = 0,     /* Synchronous playback mode */
    CDC_MPPLY_INDP = 1      /* Independant playback mode */
};

/* The MPEG decoder\u0027s transfer mode */
enum CdcMpTrn {
    CDC_MPTRN_AUTO  = 0,    /* Call forwarding mode */
    CDC_MPTRN_FORCE = 1     /* Forced transfer mode */
};

/* MPEG decoder connection switching flag */
enum CdcMpCof {
    CDC_MPCOF_ABT = 0,      /* Detach (Force Quit) */
    CDC_MPCOF_CHG = 1       /* Force switch */
};

/* MPEG/Audio clear method */
enum CdcMpCla {
    CDC_MPCLA_OFF = 0,      /* Do not clear */
    CDC_MPCLA_ON  = 1       /* Immediately clears a single sector buffer */
};

/* MPEG/Video clearing method */
enum CdcMpClv {
    CDC_MPCLV_FRM = 0,      /* Immediately clears VBV and MPEG frame buffers */
    CDC_MPCLV_VBV = 2       /* Clear the VBV at the next I or P-Picture Start */
};

/*******************************************************************
*       Structure Access Processing Macro
*******************************************************************/

/* MPEG status information */
#define CDC_MPSTAT_STS(mpstat)      ((mpstat)->status)
#define CDC_MPSTAT_AST(mpstat)      ((mpstat)->report.actst)
#define CDC_MPSTAT_PICT(mpstat)     ((mpstat)->report.pict)
#define CDC_MPSTAT_STA(mpstat)      ((mpstat)->report.stat_a)
#define CDC_MPSTAT_STV(mpstat)      ((mpstat)->report.stat_v)
#define CDC_MPSTAT_VCNT(mpstat)     ((mpstat)->report.vcnt)

/* Timecode */
#define CDC_MPTC_HOUR(mptc)         ((mptc)->hour)
#define CDC_MPTC_MIN(mptc)          ((mptc)->min)
#define CDC_MPTC_SEC(mptc)          ((mptc)->sec)
#define CDC_MPTC_PIC(mptc)          ((mptc)->pic)

/* MPEG decoder destination parameter */
#define CDC_MPCON_CMOD(mpcon)       ((mpcon)->cmod)
#define CDC_MPCON_LAY(mpcon)        ((mpcon)->lay)
#define CDC_MPCON_BN(mpcon)         ((mpcon)->bn)

/* MPEG stream parameters */
#define CDC_MPSTM_SMOD(mpstm)       ((mpstm)->smod)
#define CDC_MPSTM_ID(mpstm)         ((mpstm)->id)
#define CDC_MPSTM_CN(mpstm)         ((mpstm)->cn)

/* Screen special effects parameters */
#define CDC_MPVEF_ITP(mpvef)        ((mpvef)->itp)
#define CDC_MPVEF_TRP(mpvef)        ((mpvef)->trp)
#define CDC_MPVEF_MOZH(mpvef)       ((mpvef)->moz_h)
#define CDC_MPVEF_MOZV(mpvef)       ((mpvef)->moz_v)
#define CDC_MPVEF_SOFTH(mpvef)      ((mpvef)->soft_h)
#define CDC_MPVEF_SOFTV(mpvef)      ((mpvef)->soft_v)

/*******************************************************************
*       Processing macros
*******************************************************************/

/* Get status codes from MPEG status information */
#define CDC_MPGET_STC(mpstat)   (CDC_MPSTAT_STS(mpstat) & CDC_STC_MSK)

/* Gets the MPEG/Video operating state from the MPEG status information */
#define CDC_MPGET_ASTV(mpstat)   (CDC_MPSTAT_AST(mpstat) & 0x07)

/* Gets the MPEG/Audio operating state from the MPEG status information */
#define CDC_MPGET_ASTA(mpstat)   (CDC_MPSTAT_AST(mpstat) & 0x70)

/*******************************************************************
*       Data type declaration
*******************************************************************/

/* MPEG Status Information (Status + MPEG Report) */
typedef struct {
    Uint8   status;         /* Status */
    struct {                /* MPEG Report */
        Uint8   actst;      /* MPEG operational status */
        Uint8   pict;       /* Picture information */
        Uint8   stat_a;     /* MPEG/Audio status */
        Uint16  stat_v;     /* MPEG/Video status */
        Uint16  vcnt;       /* Operating interval (VSYNC) counter */
    } report;
} CdcMpStat;

/* Timecode */
typedef struct {
    Uint8   hour;           /* Time */
    Uint8   min;            /* Minutes */
    Uint8   sec;            /* Seconds */
    Uint8   pic;            /* Picture */
} CdcMpTc;

/* MPEG decoder destination parameter */
typedef struct {
    Uint8   cmod;           /* Connection mode */
    Uint8   lay;            /* Layer and picture search specifications */
    Uint8   bn;             /* Buffer compartment number */
} CdcMpCon;

/* MPEG stream parameters */
typedef struct {
    Uint8   smod;           /* Stream mode */
    Uint8   id;             /* Stream number */
    Uint8   cn;             /* Channel number */
} CdcMpStm;

/* Screen special effects parameters */
typedef struct {
    Uint8   itp;            /* Interpolation mode */
    Uint8   trp;            /* Transparent bit mode */
    Uint8   moz_h;          /* Horizontal mosaic mode */
    Uint8   moz_v;          /* Vertical Mosaic mode */
    Uint8   soft_h;         /* Horizontal blur mode */
    Uint8   soft_v;         /* Vertical Blur Mode */
} CdcMpVef;

/*******************************************************************
*       Variable declaration
*******************************************************************/

/*******************************************************************
*       Function declaration
*******************************************************************/

/* CDP_MDC.c. */
Sint32  CDC_MpGetCurStat(CdcMpStat *mpstat);
Sint32  CDC_MpGetLastStat(CdcMpStat *mpstat);
Sint32  CDC_MpGetInt(Sint32 *intreq);
Sint32  CDC_MpSetIntMsk(Sint32 intmsk);
Sint32  CDC_MpInit(Bool sw);
Sint32  CDC_MpSetMode(Sint32 actmod, Sint32 dectim, Sint32 out,
                      Sint32 scnmod);
Sint32  CDC_MpPlay(Sint32 plymod, Sint32 tmod_a, Sint32 tmod_v,
                   Sint32 dec_v);
Sint32  CDC_MpSetDec(Sint32 mute, Sint32 pautim, Sint32 frztim);
Sint32  CDC_MpOutDsync(Sint32 fbn);
Sint32  CDC_MpGetTc(Sint32 *bnk, Sint32 *pictyp, Sint32 *tr, CdcMpTc *mptc);
Sint32  CDC_MpGetPts(Sint32 *pts_a);

/* CDP_mst.c */
Sint32  CDC_MpSetCon(Sint32 next, CdcMpCon *mpcon_a, CdcMpCon *mpcon_v);
Sint32  CDC_MpGetCon(Sint32 next, CdcMpCon *mpcon_a, CdcMpCon *mpcon_v);
Sint32  CDC_MpChgCon(Sint32 chg_a, Sint32 chg_v,
                     Sint32 clr_a, Sint32 clr_v);
Sint32  CDC_MpSetStm(Sint32 next, CdcMpStm *mpstm_a, CdcMpStm *mpstm_v);
Sint32  CDC_MpGetStm(Sint32 next, CdcMpStm *mpstm_a, CdcMpStm *mpstm_v);
Sint32  CDC_MpGetPictSiz(Sint32 *siz_h, Sint32 *siz_v);

/* CDP_mwn.c */
Sint32  CDC_MpDisp(Bool dspsw, Sint32 fbn);
Sint32  CDC_MpSetWinFpos(Bool chgflg, Sint32 fpx, Sint32 fpy);
Sint32  CDC_MpSetWinFrat(Bool chgflg, Sint32 frx, Sint32 fry);
Sint32  CDC_MpSetWinDpos(Bool chgflg, Sint32 dpx, Sint32 dpy);
Sint32  CDC_MpSetWinDsiz(Bool chgflg, Sint32 dsx, Sint32 dsy);
Sint32  CDC_MpSetWinDofs(Bool chgflg, Sint32 dox, Sint32 doy);
Sint32  CDC_MpSetBcolor(Sint32 bcolor);
Sint32  CDC_MpSetFade(Sint32 gain_y, Sint32 gain_c);
Sint32  CDC_MpSetVeff(CdcMpVef *mpvef);

/* CDP_mfb.c. */
Sint32  CDC_MpGetImg(Sint32 *dwnum);
Sint32  CDC_MpSetImgPos(Sint32 fbn, Sint32 ipx, Sint32 ipy);
Sint32  CDC_MpSetImgSiz(Sint32 fbn, Sint32 isx, Sint32 isy);
Sint32  CDC_MpReadImg(Sint32 srcfbn,
                      Sint32 fln_y, Sint32 fln_cr, Sint32 fln_cb);
Sint32  CDC_MpWriteImg(Sint32 bn_y, Sint32 bn_cr, Sint32 bn_cb,
                       Sint32 dstfbn, Sint32 clrmod);

/* CDP_Mbu.c. */
Sint32  CDC_MpReadSct(Sint32 srcmsp, Sint32 snum, Sint32 dstfln);
Sint32  CDC_MpWriteSct(Sint32 srcbn, Sint32 sp, Sint32 snum,
                       Sint32 dstmsp);

/* CDP_mls.c. */
/* Note: The following functions are private */
Sint32  CDC_MpGetLsi(Sint32 r_sw, Sint32 reg_no, Sint32 *rdata);
Sint32  CDC_MpSetLsi(Sint32 rwsw, Sint32 reg_no, Sint32 wdata,
                     Sint32 *rdata);

#endif  /* ifndef SEGA_CDC_H */

