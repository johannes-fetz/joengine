/*
 * name:        System program
 *
 * file name:  sega_sys.h
 *
 * Purpose:        Defines the BOOT ROM service routine.
 *
 *              (1) Refer to SCU interrupt service routine registration
 *              (2) Refer to SH2 interrupt vector registration
 *              (3) System semaphore operation (BOOT ROM V0.90 or later)
 *              (4) SCU interrupt mask operation (BOOT ROM V0.90 or later)
 *              (5) System clock switching (BOOT ROM V0.92 or later)
 *              (6) Change of SCU interrupt routine priority (SCU 2.1 or later)
 *              (7) CD multiplayer startup execution (for models with CD)
 *              (8) Power-on-clear memory operation (ROM V1.00 or later)
 *              (9) MPEG check (completion return type) (ROM V1.00 or later)
 *
 *              Define the function in sega_sys.lib. (Ver.2.01 or later)
 *
 * how to use:
 *
 *      void  Vfunc(void)
 *      {
 *           /* ... Execution statement ... */
 *      }
 *
 *      extern void  Hihdr(void);
 *
 *      void  test()
 *      {
 *           void (*tmp)();
 *           Sint32 rep;
 *           Uint32 msk, clk, lval1;
 *           Uint8  p0;
 *           const Uint32 PriTab[32] = {
 *              0x00f0ffff, 0x00e0fffe, 0x00d0fffc, 0x00c0fff8,
 *              0x00b0fff0, 0x00a0ffe0, 0x0090ffc0, 0x0080ff80,
 *              0x0080ff80, 0x0070fe00, 0x0070fe00, 0x0070fe00,
 *              0x0070fe00, 0x0070fe00, 0x0070fe00, 0x0070fe00,
 *              0x0070fe00, 0x0070fe00, 0x0070fe00, 0x0070fe00,
 *              0x0070fe00, 0x0070fe00, 0x0070fe00, 0x0070fe00,
 *              0x0070fe00, 0x0070fe00, 0x0070fe00, 0x0070fe00,
 *              0x0070fe00, 0x0070fe00, 0x0070fe00, 0x0070fe00,
 *           };
 *
 *           SYS_SETUINT(0x40, Vfunc);  /* Register Vfunc() for VB-in interrupts   */
 *           tmp = SYS_GETUINT(0x41);   /* See current VB-out routine registration  */
 *           SYS_SETUINT(0x41, 0);      /* Cleared the registration of the VB-out routine       */
 *
 *           SYS_SETSINT(0x42, Hihdr);  /* Register Hihdr() with HB-in vector     */
 *           tmp = SYS_GETSINT(0x6e);   /* See current DIVU vector registration      */
 *           SYS_SETSINT(0x6e, 0);      /* Reinitialize the DIVU vector             */
 *
 *           rep = SYS_TASSEM(0x5);     /* Test and set semaphore 5 */
 *           SYS_CLRSEM(0x5);           /* Cleared semaphore 5               */
 *
 *           SYS_SETSCUIM(0xfffffffe);  /* Set only VB-in interrupts to allow      */
 *           SYS_CHGSCUIM(0xfffffffc, 0); /* Changed VB-in, VB-out to allowed      */
 *           SYS_CHGSCUIM(0xffffffff, 2); /* VB-out changed to unauthorized          */
 *           msk = SYS_GETSCUIM;        /* See current SCU interrupt mask value   */
 *
 *           SYS_CHGSYSCK(1);           /* Switch system clock to 28M  */
 *           clk = SYS_GETSYSCK;        /* See current system clock value   */
 *                                      /*   0: 26 MHz 1: 28 MHz       */
 *                                      /*     320/640 PIX/H 352/704 PIX/H. */
 *
 *           SYS_CHGUIPR(PriTab);       /* SCU interrupt routine priority change  */
 *
 *           SYS_EXECDMP();             /* CD multi-player boot performed        */
 *
 *           p0 = SYS_PCLRMEM;          /* Power-on clear memory first byte  */
 *           lval1 = *((Uint32*)&SYS_PCLRMEM+1); /* See and long words in the second half */
 *
 *           rep = SYS_CHKMPEG(0);      /* Argument must be 0. Example of using the names.DOC */
 *                                      /* Follow the instructions                  */
 *      }
 *
 *      /* ...another file... */
 *      #pragma interrupt(Hihdr)
 *      void  Hihdr(void)
 *      {
 *           /* ...Execution Statement... */
 *      }
 *
 *
 * date:        1994/05/06  for BOOT 0.90,0.901,0.91  by T. S.
 *              1994/07/16  for BOOT 0.92x,0.95       by T. S.
 *              1994/07/29  for BOOT 0.96x,1.00       by T. S.
 *              1994/08/31  how to exec CD multi-player
 *              1994/09/21  for Ver.1.00 BOOT ROM
 *              1995/03/31  MPEG check service
 *              1995/09/26  sega_sys.lib (Ver.2.01)   by M. M.
 *
 * version:  Ver.2.01
 *
 */

#ifndef SEGA_SYS_H
#define SEGA_SYS_H

/***********************************************************************
 *      Include files
 ***********************************************************************/
#include "SEGA_XPT.H"

/***********************************************************************
 *      Processing macros
 ***********************************************************************/
#define SYS_SETUINT(_Num_, _Hdr_) \
        ((**(volatile void(**)(Uint32, void*))0x6000300)((_Num_), (_Hdr_)))
#define SYS_GETUINT(_Num_) \
        ((void*)(**(void(*(**)(Uint32))(Uint32))0x6000304)(_Num_))

#define SYS_SETSINT(_Num_, _Hdr_) \
        ((**(volatile void(**)(Uint32, void*))0x6000310)((_Num_), (_Hdr_)))
#define SYS_GETSINT(_Num_) \
        ((void*)(**(void(*(**)(Uint32))(Uint32))0x6000314)(_Num_))

#define SYS_TASSEM(_Num_) \
        ((**(volatile Uint32(**)(Uint32))0x6000330)(_Num_))
#define SYS_CLRSEM(_Num_) \
        ((**(volatile void(**)(Uint32))0x6000334)(_Num_))

#define SYS_SETSCUIM(_MaskPat_) \
        ((**(volatile void(**)(Uint32))0x6000340)(_MaskPat_))
#define SYS_CHGSCUIM(_AndMask_, _OrMask_) \
        ((**(volatile void(**)(Uint32, Uint32))0x6000344)((_AndMask_), (_OrMask_)))
#define SYS_GETSCUIM \
        (*(volatile Uint32*)0x6000348)

#define SYS_CHGSYSCK(_CkMode_) \
        ((**(volatile void(**)(Uint32))0x6000320)(_CkMode_))
#define SYS_GETSYSCK \
        (*(volatile Uint32*)0x6000324)

#define SYS_CHGUIPR(_IprTab_) \
        ((**(volatile void(**)(Uint32*))0x6000280)(_IprTab_))

#define SYS_EXECDMP \
        (**(volatile void(**)(void))0x600026C)

#define SYS_PCLRMEM \
        (*(volatile Uint8*)0x6000210)

#define SYS_CHKMPEG(_dummy_) \
        ((**(volatile Sint32(**)(Sint32))0x6000274)(_dummy_))

/***********************************************************************
 *      Function declaration
 ***********************************************************************/
void SYS_CheckTrack(Sint32 tno);
void SYS_Exit(Sint32 code);

#endif
