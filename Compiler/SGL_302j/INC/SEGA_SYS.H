/*
 * 名称:        システム プログラム
 *
 * ファイル名:  sega_sys.h
 *
 * 目的:        BOOT ROM のサービスルーチンを定義します。
 *
 *              (1) SCU 割り込みサービスルーチンの登録参照
 *              (2) SH2 割り込みベクタの登録参照
 *              (3) システムセマフォの操作  (BOOT ROM V0.90以降)
 *              (4) SCU 割り込みマスクの操作(BOOT ROM V0.90以降)
 *              (5) システムクロックの切替え(BOOT ROM V0.92以降)
 *              (6) SCU 割り込みルーチン優先度の変更(SCU2.1以降)
 *              (7) CD  マルチプレーヤの起動実行(CD付きモデル用)
 *              (8) パワーオンクリアメモリの操作 (ROM V1.00以降)
 *              (9) MPEG チェック(完了復帰型)    (ROM V1.00以降)
 *
 *              sega_sys.libで関数を定義します。（Ver.2.01以降）
 *
 * 使用方法:
 *
 *      void  Vfunc(void)
 *      {
 *           // ... 実行文 ... //
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
 *           SYS_SETUINT(0x40, Vfunc);  // Vfunc() を VB-In 割込み用に登録   //
 *           tmp = SYS_GETUINT(0x41);   // 現在の VB-Out ルーチン登録を参照  //
 *           SYS_SETUINT(0x41, 0);      // VB-Out ルーチンの登録を消去       //
 *
 *           SYS_SETSINT(0x42, Hihdr);  // Hihdr() を HB-In ベクタに登録     //
 *           tmp = SYS_GETSINT(0x6e);   // 現在の DIVU ベクタ登録を参照      //
 *           SYS_SETSINT(0x6e, 0);      // DIVU ベクタを再初期化             //
 *
 *           rep = SYS_TASSEM(0x5);     // セマフォ 5 をテスト･アンド･セット //
 *           SYS_CLRSEM(0x5);           // セマフォ 5 をクリア               //
 *
 *           SYS_SETSCUIM(0xfffffffe);  // VB-In 割込みのみを許可に設定      //
 *           SYS_CHGSCUIM(0xfffffffc, 0); // VB-In, VB-Out を許可に変更      //
 *           SYS_CHGSCUIM(0xffffffff, 2); // VB-Out  を不許可に変更          //
 *           msk = SYS_GETSCUIM;        // 現在の SCU 割込みマスク値を参照   //
 *
 *           SYS_CHGSYSCK(1);           // システムクロックを２８Ｍに切替え  //
 *           clk = SYS_GETSYSCK;        // 現在の システムクロック値を参照   //
 *                                      //   0: 26 MHz       1: 28 MHz       //
 *                                      //     320/640 pix/H   352/704 pix/H //
 *
 *           SYS_CHGUIPR(PriTab);       // ＳＣＵ割り込みルーチン優先度変更  //
 *
 *           SYS_EXECDMP();             // ＣＤマルチプレーヤ起動実行        //
 *
 *           p0 = SYS_PCLRMEM;          // パワーオンクリアメモリ先頭バイト  //
 *           lval1 = *((Uint32*)&SYS_PCLRMEM+1); // と後半ロングワードを参照 //
 *
 *           rep = SYS_CHKMPEG(0);      // 引数は必ず 0. MANSYS.DOC の使用例 //
 *                                      // に従ってください                  //
 *      }
 *
 *      // ... 別のファイル ... //
 *      #pragma interrupt(Hihdr)
 *      void  Hihdr(void)
 *      {
 *           // ... 実行文 ... //
 *      }
 *
 *
 * 日付:        1994/05/06  for BOOT 0.90,0.901,0.91  by T. S.
 *              1994/07/16  for BOOT 0.92x,0.95       by T. S.
 *              1994/07/29  for BOOT 0.96x,1.00       by T. S.
 *              1994/08/31  how to exec CD multi-player
 *              1994/09/21  for Ver.1.00 BOOT ROM
 *              1995/03/31  MPEG check service
 *              1995/09/26  sega_sys.lib (Ver.2.01)   by M. M.
 *
 * バージョン:  Ver.2.01
 *
 */

#ifndef SEGA_SYS_H
#define SEGA_SYS_H

/***********************************************************************
 *      インクルードファイル
 ***********************************************************************/
#include "sega_xpt.h"

/***********************************************************************
 *      処理マクロ
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
 *      関数の宣言
 ***********************************************************************/
void SYS_CheckTrack(Sint32 tno);
void SYS_Exit(Sint32 code);

#endif
