/******************************************************************************
 *	ソフトウェアライブラリ
 *
 *	Copyright 1991 CSK Research Institute Corp.
 *	Copyright (c) 1994 SEGA
 *
 * Library	:ＡＤＰＣＭ伸張ライブラリ
 * Module	:ライブラリヘッダ
 * File		:sega_adp.h
 * Date		:1994-12-14
 * Version	:1.01
 * Auther	:Y.T
 *
 ****************************************************************************/
#if !defined(SEGA_ADP_H)
#define SEGA_ADP_H


/*-------------------- 《インクルードファイル》 --------------------*/
#include "sega_xpt.h"


/*------------------------- 《マクロ定数》 -------------------------*/

/* １サウンドグループの圧縮時サイズ */
#define ADP_SIZE_SG_CODE    (128)

/* １サウンドグループの伸張時サイズ */
#define ADP_SIZE_SG_DECO    (448)

/* １サウンドグループの伸張時サイズ[byte/1ch] */
#define ADP_SIZE_SG_DECO_1  (ADP_SIZE_SG_DECO / 2)

/* １サウンドグループの伸張時サイズ[dword/1ch] */
#define ADP_DSIZE_SG_DECO_1  (ADP_SIZE_SG_DECO_1 / 4)

/* １サウンドグループの圧縮時サイズ（ワード数） */
#define ADP_WSIZE_SG_CODE   (ADP_SIZE_SG_CODE/2)

/* １サウンドグループの伸張時サイズ（ワード数） */
#define ADP_WSIZE_SG_DECO    (ADP_SIZE_SG_DECO/2)


/*-------------------------- 《関数宣言》 --------------------------*/

/*    ストリーム形式数ＳＧのステレオ伸張        */
/* extern void ADP_DecStereoStm(Uint16 *in, void *out, Sint32 nsg); */
                                                                               
/*    ストリーム形式数ＳＧのモノラル伸張        */
/* extern void ADP_DecMonoStm(Uint16 *in, void *out, Sint32 nsg); */
                                                                               
/*    バッファ形式数ＳＧのステレオ伸張          */
extern void ADP_DecStereoBuf(void *in, void *out, Sint32 nsg);
                                                                               
/*    バッファ形式数ＳＧのモノラル伸張          */
extern void ADP_DecMonoBuf(void *in, void *out, Sint32 nsg);
                                                                               
/*    バッファ形式１ＳＧのステレオ伸張          */
extern void ADP_DecStereo(void *in, void *out);
                                                                               
/*    バッファ形式１ＳＧのモノラル伸張          */
extern void ADP_DecMono(void *in, void *out);

#endif	/* SEGA_ADP_H */
