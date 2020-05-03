/*------------------------------------------------------------------------
 *  FILE:	sega_bup.h
 *
 *	Copyright(c) 1994 SEGA
 *
 *  PURPOSE:
 *	Back Up Library
 *
 *  AUTHOR(S):
 *	K.M
 *		
 *  MOD HISTORY:
 *	Written by K.M on 1994-07-13 Ver.1.00
 *	Updated by K.M on 1994-07-29 Ver.1.00
 * 
 *------------------------------------------------------------------------
 */


#ifndef	SEGA_BUP_H
#define SEGA_BUP_H

#include	<SEGA_XPT.H>

#define	BUP_LIB_ADDRESS		(*(volatile Uint32 *)(0x6000350+8))
#define	BUP_VECTOR_ADDRESS	(*(volatile Uint32 *)(0x6000350+4))

/* Unit ID */
#define	BUP_MAIN_UNIT	(1)
#define	BUP_CURTRIDGE	(2)

/* Language type */
#define	BUP_JAPANESE	(0)
#define	BUP_ENGLISH	(1)
#define	BUP_FRANCAIS	(2)
#define	BUP_DEUTSCH	(3)
#define	BUP_ESPANOL	(4)
#define	BUP_ITALIANO	(5)

/* Machine state */
#define	BUP_NON			(1)
#define	BUP_UNFORMAT		(2)
#define	BUP_WRITE_PROTECT	(3)
#define	BUP_NOT_ENOUGH_MEMORY	(4)
#define	BUP_NOT_FOUND		(5)
#define	BUP_FOUND		(6)
#define	BUP_NO_MATCH		(7)
#define	BUP_BROKEN		(8)

/******************************************
 * Storage connection information table               *
 ******************************************/
typedef struct BupConfig{
	Uint16	unit_id;	/* Unit ID */
	Uint16	partition;	/* Number of partitions */
} BupConfig;

/******************************************
 * Status information table                 *
 ******************************************/
typedef	struct BupStat{
	Uint32	totalsize;	/* Total capacity (Byte) */
	Uint32	totalblock;	/* Total blocks */
	Uint32	blocksize;	/* Size of a block (Byte) */
	Uint32	freesize;	/* Free space */
	Uint32	freeblock;	/* Number of empty blocks */
	Uint32	datanum;
} BupStat;

/******************************************
 * Directory information table               *
 ******************************************/
typedef struct BupDir{
	Uint8	filename[12];	/* File name */
	Uint8	comment[11];	/* Comments */
	Uint8	language;	/* The language type of the comment */
	Uint32	date;		/* Time stamp */
	Uint32	datasize;	/* Data size (Byte) */
	Uint16	blocksize;	/* Data size (blocks) */
} BupDir;

typedef struct BupDate {
	Uint8	year;		/* Timestamp (year-1980) */
	Uint8	month;		/* Timestamp (month 1-12) */
	Uint8	day;		/* Time stamp (1–31 days) */
	Uint8	time;		/* Time stamp (hours 0-23) */
	Uint8	min;		/* Time stamp (minutes 0-59) */
	Uint8	week;		/* Timestamp (day of week 0-Saturday 6)*/
} BupDate;

#endif /* ifndef SEGA_BUP_H */

#ifndef	SEGA_BUP_PROTO
#define SEGA_BUP_PROTO

/* #if !(__GNUC__) */
#ifndef __GNUC__
#define	BUP_Init	((void (*)(Uint32 *lib,Uint32 *work,BupConfig tp[3])) (BUP_LIB_ADDRESS))
#else
#define	BUP_Init	((void (*)(volatile Uint32 *lib,Uint32 *work,BupConfig tp[3])) (BUP_LIB_ADDRESS))
#endif

#define	BUP_SelPart	((Sint32 (*)(Uint32 device,Uint16 num)) (*(Uint32 *)(BUP_VECTOR_ADDRESS+4)))

#define	BUP_Format	((Sint32 (*)(Uint32 device)) (*(Uint32 *)(BUP_VECTOR_ADDRESS+8)))

#define	BUP_Stat	((Sint32 (*)(Uint32 device,Uint32 datasize,BupStat *tb)) (*(Uint32 *)(BUP_VECTOR_ADDRESS+12)))

/* #if !(__GNUC__) */
#ifndef __GNUC__
#define	BUP_Write	((Sint32 (*)(Uint32 device,BupDir *tb,Uint8 *data,Uint8 wmode)) (*(Uint32 *)(BUP_VECTOR_ADDRESS+16)))
#else
#define	BUP_Write	((Sint32 (*)(Uint32 device,BupDir *tb,volatile Uint8 *data,Uint8 wmode)) (*(Uint32 *)(BUP_VECTOR_ADDRESS+16)))
#endif

/* #if !(__GNUC__) */
#ifndef __GNUC__
#define	BUP_Read	((Sint32 (*)(Uint32 device,Uint8 *filename,Uint8 *data)) (*(Uint32 *)(BUP_VECTOR_ADDRESS+20)))
#else
#define	BUP_Read	((Sint32 (*)(Uint32 device,Uint8 *filename,volatile Uint8 *data)) (*(Uint32 *)(BUP_VECTOR_ADDRESS+20)))
#endif

#define	BUP_Delete	((Sint32 (*)(Uint32 device,Uint8 *filename)) (*(Uint32 *)(BUP_VECTOR_ADDRESS+24)))

#define	BUP_Dir 	((Sint32 (*)(Uint32 device,Uint8 *filename,Uint16 tbsize,BupDir *tb)) (*(Uint32 *)(BUP_VECTOR_ADDRESS+28)))

/* #if !(__GNUC__) */
#ifndef __GNUC__
#define	BUP_Verify	((Sint32 (*)(Uint32 device,Uint8 *filename,Uint8 *data)) (*(Uint32 *)(BUP_VECTOR_ADDRESS+32)))
#else
#define	BUP_Verify	((Sint32 (*)(Uint32 device,Uint8 *filename,volatile Uint8 *data)) (*(Uint32 *)(BUP_VECTOR_ADDRESS+32)))
#endif

#define	BUP_GetDate	((void (*)(Uint32 date,BupDate *tb)) (*(Uint32 *)(BUP_VECTOR_ADDRESS+36)))

#define	BUP_SetDate	((Uint32 (*)(BupDate *tb)) (*(Uint32 *)(BUP_VECTOR_ADDRESS+40)))

#endif /* SEGA_BUP_PROTO */
