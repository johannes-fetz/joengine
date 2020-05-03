/******************************************************************************
 *	Software library
 *
 *	Copyright 1991 CSK Research Institute Corp.
 *	Copyright (c) 1994 SEGA
 *
 * Library	:ADPCM decompression library
 * Module	:Library header
 * File		:sega_adp.h
 * Date		:1994-12-14
 * Version	:1.01
 * Auther	:Y.T
 *
 ****************************************************************************/
#if !defined(SEGA_ADP_H)
#define SEGA_ADP_H


/*-------------------- Include File --------------------*/
#include "SEGA_XPT.H"


/*------------------------- Macro constants -------------------------*/

/* 1 the compressed size of the sound group */
#define ADP_SIZE_SG_CODE    (128)

/* 1 the extension size of the sound group */
#define ADP_SIZE_SG_DECO    (448)

/* 1 Sound Group Extended Size [byte/1ch] */
#define ADP_SIZE_SG_DECO_1  (ADP_SIZE_SG_DECO / 2)

/* 1 Sound Group Extension Size [dword/1ch] */
#define ADP_DSIZE_SG_DECO_1  (ADP_SIZE_SG_DECO_1 / 4)

/* 1 Sound Group Compression Size (words) */
#define ADP_WSIZE_SG_CODE   (ADP_SIZE_SG_CODE/2)

/* 1 Sound Group Extended Size (words) */
#define ADP_WSIZE_SG_DECO    (ADP_SIZE_SG_DECO/2)


/*-------------------------- Function Declaration --------------------------*/

/*    Stereo extension of SG stream formats        */
/* Extern void ADP_DecStereoStm(Uint16 *in, void *out, Sint32 nsg) */
                                                                               
/*    Mono extension of SG stream formats        */
/* Extern void ADP_DecMonoStm(Uint16 *in, void *out, Sint32 nsg) */
                                                                               
/*    Stereo extension of SG buffer formats          */
extern void ADP_DecStereoBuf(void *in, void *out, Sint32 nsg);
                                                                               
/*    Mono extension of SG buffer formats          */
extern void ADP_DecMonoBuf(void *in, void *out, Sint32 nsg);
                                                                               
/*    Buffered 1SG stereo extension          */
extern void ADP_DecStereo(void *in, void *out);
                                                                               
/*    Mono extension in buffer format 1SG          */
extern void ADP_DecMono(void *in, void *out);

#endif	/* SEGA_ADP_H */
