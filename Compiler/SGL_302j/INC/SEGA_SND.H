/*-----------------------------------------------------------------------------
 *  FILE: snd_main.h
 *
 *      Copyright (c) by SEGA.
 *
 *  PURPOSE:
 *
 *      Sound I / F library header file.
 *
 *  DESCRIPTION:
 *
 *      Realizes the interface with the sound driver.
 *
 *  AUTHOR(S)
 *
 *      1994-05-18  N.T Ver.0.90
 *
 *  MOD HISTORY:
 *
 *      1994-09-23  N.T Ver.1.04
 *      1994-12-01  N.T Ver.1.05
 *      1996-07-10  N.T Ver.1.08
 *
 *-----------------------------------------------------------------------------
 */

#ifndef SEGA_SND_H
#define SEGA_SND_H

/*
 * C VIRTUAL TYPES DEFINITIONS
 */
#include <MACHINE.H>
#include "SEGA_XPT.H"

/*
 * USER SUPPLIED INCLUDE FILES
 */

/*
 * GLOBAL DEFINES/MACROS DEFINES
 */
/**** Constants *******************************************************************/
/******** Command execution state ******************************************************/
#define SND_RET_SET     0                       /* Successful completion                     */
#define SND_RET_NSET    1                       /* Abnormal termination                     */
/******** Sound control settings ****************************************************/
#define SND_CTRL_OUTPUT_STEREO  (0 << 7)        /* Sound output mode : STEREO*/
#define SND_CTRL_OUTPUT_MONO    (1 << 7)        /* Sound output mode : MONO  */

/******** Sequence pan control switch ********************************************/
#define SND_SEQ_PAN_OFF (0 << 7)                /* Control OFF               */
#define SND_SEQ_PAN_ON  (1 << 7)                /* Control ON                */

/******** Hardware check parameters **************************************************/
#define SND_DRAM4           0                   /* DRAM 4Mbit r/w            */
#define SND_DRAM8           1                   /* DRAM 8Mbit r/w            */
#define SND_SCSP_MIDI       2                   /* SCSP MIDI                 */
#define SND_SOUND_SRC_LR    3                   /* Source output (L/R)            */
#define SND_SOUND_SRC_L     4                   /* Source Output              */
#define SND_SOUND_SRC_R     5                   /* Source output (R)              */
/******** Hardware check status ***************************************************/
#define SND_HARD_OK     0x8000                  /* Successful completion                  */
#define SND_HARD_ERR    0x7fff                  /* Abnormal termination                  */
/******** Interrupt permission bits *************************************************/
#define SND_INT_PCM_ADR     (1 <<  7)           /* PCM play address updated   */
/******** Versus Host interrupt type *************************************************/
#define SND_FCT_PCM_ADR     0x01                /* PCM play address updated   */
/******** PCM address update interrupt status ******************************************/
#define SND_PCM_PLAY0   (1 <<  0)               /* PCM regeneration number 0              */
#define SND_PCM_PLAY1   (1 <<  1)               /* PCM regeneration number 1              */
#define SND_PCM_PLAY2   (1 <<  2)               /* PCM regeneration number 2              */
#define SND_PCM_PLAY3   (1 <<  3)               /* PCM regeneration number 3              */
#define SND_PCM_PLAY4   (1 <<  4)               /* PCM regeneration number 4              */
#define SND_PCM_PLAY5   (1 <<  5)               /* PCM regeneration number 5              */
#define SND_PCM_PLAY6   (1 <<  6)               /* PCM regeneration number 6              */
#define SND_PCM_PLAY7   (1 <<  7)               /* PCM regeneration number 7              */

/******** SndSeqStat (sequence status) ***********************************/
/************* SND_SEQ_STAT_MODE(song mode) **********************************/
#define SND_MD_STOP     0x000                           /* stop              */
#define SND_MD_PLAY     0x100                           /* play              */
#define SND_MD_FADE     0x200                           /* fade              */
#define SND_MD_PLAY_PS  0x300                           /* play pause        */
#define SND_MD_FADE_PS  0x400                           /* fade pause        */
/******** SndPcmStartPrm **********************************/
/************* SND_PRM_mode (Stereo/Monochrome + Sampling Rate) ***************************/
#define SND_MD_MONO     (0 <<  7)                   /* Monochrome                  */
#define SND_MD_STEREO   (1 <<  7)                   /* Stereo                  */
#define SND_MD_16       (0 <<  4)                   /* 16bitPCM              */
#define SND_MD_8        (1 <<  4)                   /* 8bitPCM               */
/******** SND_MoveData (Sound Data Transfer) ***********************************/
#define SND_KD_TONE         0x0                         /* Tone              */
#define SND_KD_SEQ          0x1                         /* Sequence        */
#define SND_KD_DSP_PRG      0x2                         /* DSP Program     */
#define SND_KD_DSP_RAM      0x3                         /* DSP work RAM         */
/**** Access macros *********************************************************/
/******** SndIniDt (system boot) *********************************************/
#define SND_INI_PRG_ADR(sys_ini)    ((sys_ini).prg_adr) /* 68K Program Storage... */
#define SND_INI_PRG_SZ(sys_ini)     ((sys_ini).prg_sz)  /* 68K Program Size... */
#define SND_INI_ARA_ADR(sys_ini)    ((sys_ini).ara_adr) /* Sound Area Map Retract. */
#define SND_INI_ARA_SZ(sys_ini)     ((sys_ini).ara_sz)  /* Sound Area Map Size. */
/******** SndSeqStat (sequence status) ***********************************/
#define SND_SEQ_STAT_MODE(status)   ((status).mode)     /* song mode         */
#define SND_SEQ_STAT_STAT(status)   ((status).stat)     /* song status       */
/******** SndPcmStartPrm **********************************/
#define SND_PRM_MODE(prm)       ((prm).mode)        /* Stereo monochrome + sampling rate */
#define SND_PRM_SADR(prm)       ((prm).sadr)        /* PCM stream buffer start address*/
#define SND_PRM_SIZE(prm)       ((prm).size)        /* PCM stream buffer size   */
#define SND_PRM_OFSET(prm)      ((prm).ofset)       /* PCM stream start offset */
/******** SndPcmChgPrm ************************************/
#define SND_PRM_NUM(prm)        ((prm).num)         /* PCM stream regeneration number       */
#define SND_PRM_LEV(prm)        ((prm).lev)         /* Direct sound level         */
#define SND_PRM_PAN(prm)        ((prm).pan)         /* Direct sound pan           */
#define SND_PRM_PICH(prm)       ((prm).pich)        /* The pie word              */
#define SND_R_EFCT_IN(prm)      ((prm).r_efct_in)   /* EFect in select (right)   */
#define SND_R_EFCT_LEV(prm)     ((prm).r_efct_lev)  /* Right perfect send Level  */
#define SND_L_EFCT_IN(prm)      ((prm).l_efct_in)   /* EFect in select (left)   */
#define SND_L_EFCT_LEV(prm)     ((prm).l_efct_lev)  /* Elect send Level (left)  */
#define SND_PRM_TL(prm)     	((prm).lev)  		/* Master Level			 */
/******** SndPcmPlayAdr ****************************************/
#define SND_PCM_RADR(prm)       ((prm).radr)        /* Right Output (Black & White)          */
#define SND_PCM_LADR(prm)       ((prm).ladr)        /* Left output                 */
/******** SndCdHz SrVl (Stereo Analysis Volume by Frequency Band) ******************************/
#define SND_CD_LHIGH(hz_vl)     ((hz_vl).high.left) /* Left output treble         */
#define SND_CD_LMID(hz_vl)      ((hz_vl).mid.left)  /* Left output medium tone         */
#define SND_CD_LLOW(hz_vl)      ((hz_vl).low.left)  /* Left output bass         */
#define SND_CD_RHIGH(hz_vl)     ((hz_vl).high.right)/* Right output treble         */
#define SND_CD_RMID(hz_vl)      ((hz_vl).mid.right) /* Right output medium tone         */
#define SND_CD_RLOW(hz_vl)      ((hz_vl).low.right) /* Right output bass         */

#define SND_POKE_B(adr, data)   (*((volatile Uint8 *)(adr)) = ((Uint8)(data))) /* Bytes*/
#define SND_POKE_W(adr, data)   (*((volatile Uint16 *)(adr)) = ((Uint16)(data)))
#define SND_PEEK_B(adr)         (*((volatile Uint8 *)(adr)))                   /* Bytes*/

/**** System interface work offset *********************************/
#define SND_ADR_INTR_CTRL_WORD  (0x00)          /* Interrupt control word    */
#define SND_ADR_INTR_MODE       (0x01)          /* Interrupt mode            */
#define SND_ADR_PCM_PLAY_NUM    (0x02)          /* PCM play num              */
#define SND_ADR_SND_CTRL        (0x03)          /* Sound control             */
/**** Address ***************************************************************/
#define SND_ADR_INTR_RESET  ((volatile Uint16 *)0x25b0042e)  /* SCSP interrupt reset register */

/*Interrupt *******************************************************************/
/******************************************************************************
 *
 * NAME:    SND_SET_ENA_INT()       - Interrupt enable setting
 *
 * PARAMETERS :
 *      (1) Uint8 ena_bit   - <i>   Interrupt enable bit
 *
 * DESCRIPTION:
 *      Set interrupt enable.
 *
 * PRECONDITIONS:
 *      None.
 *
 * POSTCONDITIONS:
 *      None
 *
 * CAVEATS:
 *      None.
 *
 ******************************************************************************
 */

#define SND_SET_ENA_INT(ena_bit)\
    do{\
        snd_msk_work_work = get_imask();           /* Disable sound interrupt*/\
        set_imask(15);\
        SND_POKE_B(snd_adr_sys_int_work + SND_ADR_INTR_CTRL_WORD, (ena_bit));\
        set_imask(snd_msk_work_work);\
    }while(FALSE)

/******************************************************************************
 *
 * NAME:    SND_GET_ENA_INT()       - Get interrupt enable
 *
 * PARAMETERS :
 *      None.
 *
 * DESCRIPTION:
 *      Get interrupt enable.
 *
 * PRECONDITIONS:
 *      None.
 *
 * POSTCONDITIONS:
 *      (1) Uint8           - <o>   Interrupt enable bit
 *
 * CAVEATS:
 *      None.
 *
 ******************************************************************************
 */

#define SND_GET_ENA_INT()\
        (SND_PEEK_B(snd_adr_sys_int_work + SND_ADR_INTR_CTRL_WORD))

/******************************************************************************
 *
 * NAME:    SND_GET_FCT_INT()       - Get interrupt factor
 *
 * PARAMETERS :
 *      None.
 *
 * DESCRIPTION:
 *      Get the interrupt factor.
 *
 * PRECONDITIONS:
 *      None.
 *
 * POSTCONDITIONS:
 *      (1) Uint8       - <o>   Interrupt factor
 *
 * CAVEATS:
 *      None.
 *
 ******************************************************************************
 */

#define SND_GET_FCT_INT()\
        (SND_PEEK_B(snd_adr_sys_int_work + SND_ADR_INTR_MODE))

/******************************************************************************
 *
 * NAME:    SND_RESET_INT()     - Interrupt reset
 *
 * PARAMETERS :
 *      None.
 *
 * DESCRIPTION:
 *      Reset the interrupt.
 *
 * PRECONDITIONS:
 *      None.
 *
 * POSTCONDITIONS:
 *      None
 *
 * CAVEATS:
 *      None.
 *
 ******************************************************************************
 */

#define SND_RESET_INT()\
        (SND_POKE_W(SND_ADR_INTR_RESET, (1 << 5)))

/******************************************************************************
 *
 * NAME:    SND_GET_INT_STAT()  - PCM execution address update interrupt status acquisition
 *
 * PARAMETERS :
 *      None.
 *
 * DESCRIPTION:
 *      Gets the PCM execution address update interrupt status.
 *
 * PRECONDITIONS:
 *      None.
 *
 * POSTCONDITIONS:
 *      (1) SndPcmIntStat *     - <o>   PCM execution address update interrupt status
 *
 * CAVEATS:
 *      None.
 *
 ******************************************************************************
 */
 
#define SND_GET_INT_STAT()\
    (SND_PEEK_B(snd_adr_sys_int_work + SND_ADR_PCM_PLAY_NUM))

/*Other *********************************************************************/
/******************************************************************************
 *
 * NAME:    SND_SET_SND_CTRL        - Sound control information setting
 *
 * PARAMETERS :
 *      (1) Uint8   ctrl_bit   - <i>   Interrupt enable bit
 *
 * DESCRIPTION:
 *      Set the sound control information.
 *
 * PRECONDITIONS:
 *      None.
 *
 * POSTCONDITIONS:
 *      None
 *
 * CAVEATS:
 *      None.
 *
 ******************************************************************************
 */

#define SND_SET_SND_CTRL(ctrl_bit)\
    do{\
        SND_POKE_B(snd_adr_sys_int_work + SND_ADR_SND_CTRL, (ctrl_bit));\
    }while(FALSE)

/******************************************************************************
 *
 * NAME:    SND_GET_SND_CTRL        - Sound control information acquisition
 *
 * PARAMETERS :
 *      None.
 *
 * DESCRIPTION:
 *      Gets sound control information.
 *
 * PRECONDITIONS:
 *      None.
 *
 * POSTCONDITIONS:
 *      None
 *
 * CAVEATS:
 *      None.
 *
 ******************************************************************************
 */

#define SND_GET_SND_CTRL()\
        (SND_PEEK_B(snd_adr_sys_int_work + SND_ADR_SND_CTRL))

/*
 * STRUCTURE DECLARATIONS
 */

/*
 * TYPEDEFS
 */

/**** Common *******************************************************************/
typedef struct{
    Uint16 *prg_adr;                        /* 68K first address of program store       */
    Uint16 prg_sz;                          /* 68K program size size           */
    Uint16 *ara_adr;                        /* Sound area map start address     */
    Uint16 ara_sz;                          /* Sound Area Map Size (in words)    */
}SndIniDt;                                  /* System boot data type          */

typedef Uint8 SndAreaMap;                   /* Sound area map data type          */
typedef Uint8 SndTlVl;                      /* Global volume data type              */
typedef Uint8 SndEfctBnkNum;                /* Effct bank number data type     */
typedef Uint8 SndToneBnkNum;                /* The tone bank number data type      */
typedef Uint8 SndMixBnkNum;                 /* Mixer number data type          */
typedef Uint8 SndEfctOut;                   /* Effect out select data type     */
typedef Uint8 SndLev;                       /* Level data type                 */
typedef Sint8 SndPan;                       /* Pan data type                   */
typedef Uint8 SndRet;                       /* Command execution state data type         */
typedef Uint8 SndHardPrm;                   /* Hardware check parameter data type     */
typedef Uint16 SndHardStat;                 /* Hardware check status data type      */

/**** Sequence *************************************************************/
typedef Uint8 SndSeqNum;                    /* Phonetic control number                  */
typedef Uint8 SndSeqBnkNum;                 /* Sequence bank number          */
typedef Uint8 SndSeqSongNum;                /* Sequence song number          */
typedef Uint8 SndSeqPri;                    /* Priorty level                 */
typedef Uint8 SndSeqVl;                     /* Sequence Volume               */
typedef Uint8 SndFade;                      /* fade Rate                     */
typedef Sint16 SndTempo;                    /* Tempo                         */
typedef struct{
    Uint16 mode;                            /* song mode                     */
    Uint8 stat;                             /* song status                   */
}SndSeqStat;                                /* Sequence status          */
typedef Uint16 SndSeqPlayPos;               /* Phonetic control number playback position          */

/**** PCM ********************************************************************/
typedef struct{
    Uint8 mode;                             /* Stereo monochrome + sampling rate         */
    Uint16 sadr;                            /* PCM stream buffer start address        */
    Uint16 size;                            /* PCM stream buffer size             */
}SndPcmStartPrm;                            /* PCM start parameter          */

typedef Uint8 SndPcmNum;                    /* PCM stream regeneration number               */
typedef Uint8 SndEfctIn;                    /* Select in select               */
typedef struct{
    SndPcmNum num;                          /* PCM stream regeneration number               */
    SndLev lev;                             /* Direct sound level                 */
    SndPan pan;                             /* Direct sound pan                   */
    Uint16 pich;                            /* The pie word                      */
    SndEfctIn r_efct_in;                    /* Efect in select (right output)       */
    SndLev r_efct_lev;                      /* Right Output perfect send Level      */
    SndEfctIn l_efct_in;                    /* Efect in select (Left Output)       */
    SndLev l_efct_lev;                      /* Efect send Level (Left output)      */
}SndPcmChgPrm;                              /* PCM change parameters           */

typedef struct{
    Uint8 radr;                             /* Right Output (Black \u0026 White)                  */
    Uint8 ladr;                             /* Left output                         */
}SndPcmPlayAdr;                             /* PCM regeneration address               */

typedef Uint8 SndPcmIntStat;                /* PCM address update interrupt status     */

/**** CD *********************************************************************/
typedef Uint16 SndCdVlAnl;                  /* Analysis volume                      */

typedef struct{
    SndCdVlAnl left;                        /* Left output                        */
    SndCdVlAnl right;                       /* Right output                        */
}SndCdHzVl;                                 /* Stereo analysis volume by frequency band      */

typedef struct{
    SndCdHzVl high;                         /* High-pitched                        */
    SndCdHzVl mid;                          /* Mid-range                        */
    SndCdHzVl low;                          /* Bass                        */
}SndCdHzSrVl;                               /* Analysis volume by frequency band          */

/*
 * EXTERNAL VARIABLE DECLARATIONS
 */
extern Uint8 *snd_adr_sys_int_work;             /*System interfaceworksTop address storage*/
extern Uint32 snd_msk_work_work;                /* sound priority msk        */

/*
 * EXTERNAL FUNCTION PROTOTYPE  DECLARATIONS
 */
void SND_Init(SndIniDt *);
void SND_MoveData(Uint16 *, Uint32, Uint16, Uint16);
SndRet SND_ChgMap(SndAreaMap);
SndRet SND_SetTlVl(SndTlVl);
SndRet SND_ChgEfct(SndEfctBnkNum);
SndRet SND_ChgMix(SndToneBnkNum, SndMixBnkNum);
SndRet SND_ChgMixPrm(SndEfctOut, SndLev, SndPan);
SndRet SND_ChkHard(SndHardStat *, SndHardPrm);
SndRet SND_StopDsp(void);
SndRet SND_OffAllSound(void);
SndRet SND_StartSeq(SndSeqNum, SndSeqBnkNum, SndSeqSongNum, SndSeqPri);
SndRet SND_StopSeq(SndSeqNum);
SndRet SND_PauseSeq(SndSeqNum);
SndRet SND_ContSeq(SndSeqNum);
SndRet SND_SetSeqVl(SndSeqNum, SndSeqVl, SndFade);
SndRet SND_ChgTempo(SndSeqNum, SndTempo);
SndRet SND_CtrlDirMidi(SndSeqNum, SndSeqPri, Uint8, Uint8, Uint8, Uint8);
SndRet SND_SetSeqPan(SndSeqNum, Uint8, Uint8 );
SndRet SND_StartPcm(SndPcmStartPrm *, SndPcmChgPrm *);
SndRet SND_StopPcm(SndPcmNum);
SndRet SND_ChgPcm(SndPcmChgPrm *);
SndRet SND_StartVlAnl(void);
SndRet SND_StopVlAnl(void);
SndRet SND_SetCdDaLev(SndLev, SndLev);
SndRet SND_SetCdDaPan(SndPan, SndPan);
void SND_GetSeqStat(SndSeqStat *, SndSeqNum);
void SND_GetSeqPlayPos(SndSeqPlayPos *, SndSeqNum);
void SND_GetPcmPlayAdr(SndPcmPlayAdr *, SndPcmNum);
void SND_GetAnlTlVl(SndCdVlAnl *, SndCdVlAnl *);
void SND_GetAnlHzVl(SndCdHzSrVl *);

#endif  /* ifndef SEGA_SND_H */
