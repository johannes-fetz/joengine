/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2017, Johannes Fetz (johannesfetz@gmail.com)
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the Johannes Fetz nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL Johannes Fetz BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/** @file sgl_prototypes.h
 *  @author Johannes Fetz
 *
 *  @brief SGL prototypes
 */

#ifndef __JO_SGL_PROTOTYPES_H__
# define __JO_SGL_PROTOTYPES_H__

#if !JO_COMPILE_USING_SGL

//HACK
#define SEGA_XPT_H
#define SEGA_GFS_H
#define SEGA_CDC_H
#define SEGA_BUP_H

/*
#include <sgl.h>
*/
#define	    VDP2_VRAM_A0	0x25e00000
#define	    VDP2_VRAM_A1	0x25e20000
#define	    VDP2_VRAM_B0	0x25e40000
#define	    VDP2_VRAM_B1	0x25e60000

#define	    VDP2_COLRAM    		0x25f00000
#define		CHAR_SIZE_1x1		0

#define		COL_TYPE_256		0x10

#define	    KTBL0_RAM		VDP2_VRAM_A1		/* ��]�X�N���[���p�W���e�[�u�� */
#define	    BACK_CRAM		(KTBL0_RAM + 0x1fffe)	/* Back color */

#define	    CL_Half		2		/* ���P�x���[�h */
#define	    CL_Trans		3		/* ���������[�h */
#define	    CL_Gouraud		4		/* �O�[���[�V�F�[�f�B���O���[�h */

#define	PER_ID_NotConnect	0xff	/* ���ڑ�				*/
#define	PER_ID_StnPad		0x02	/* �T�^�[���W���p�b�h			*/
#define	PER_ID_StnMouse		0x23	/* �T�^�[���}�E�X			*/
#define	PER_ID_ShuttleMouse	0xe3	/* �V���g���}�E�X			*/

#define	    toFIXED(a)		((FIXED)(65536.0 * (a)))
#define	    POStoFIXED(x,y,z)	{toFIXED(x),toFIXED(y),toFIXED(z)}
#define	    DEGtoANG(d)		((ANGLE)((65536.0 * (d)) / 360.0))
#define	NORMAL(x,y,z)		{POStoFIXED(x,y,z)
#define	VERTICES(v0,v1,v2,v3)	{v0 , v1 , v2 , v3}}
#define	C_RGB(r,g,b)		(((b)&0x1f)<<10|((g)&0x1f)<<5|((r)&0x1f)|0x8000)

#define     No_Window   (0 << 9)   /* No sprite clipping */
#define     Window_In   (2 << 9)   /* Clip everything outside bounds */
#define     Window_Out  (3 << 9)  /* Clip everything inside bounds */
#define	    ECdis		(1 << 7)	/* �G���h�R�[�h���p���b�g�̂ЂƂƂ��Ďg�p */

#define	    SPdis		(1 << 6)	/* �X�y�[�X�R�[�h���p���b�g�̂ЂƂƂ��Ďg�p */

#define	    sprPolygon		(FUNC_Polygon | ((ECdis | SPdis) << 24))

#define	    FUNC_Texture	2
#define	    FUNC_Polygon	4
#define	    FUNC_PolyLine	5

#define	    MESHon		(1 << 8)	/* ���b�V���ŕ\�� */
#define	    CL32KRGB		(5 << 3)	/* �q�f�a�R�Q�j�F���[�h */

#define	    UseTexture		(1 << 2)	/* �e�N�X�`����\��|���S�� */
#define	    UseLight		(1 << 3)	/* �����̉e�����󂯂�|���S�� */
#define	    UsePalette		(1 << 5)	/* �|���S���̃J���[ */

#define	    No_Texture		0		/* �e�N�X�`�����g�p���Ȃ��� */
#define	    No_Option		0		/* �I�v�V�������g�p���Ȃ��� */
#define	    No_Gouraud		0		/* �O�[���[�V�F�[�f�B���O���g�p���Ȃ��� */
#define	    No_Palet	        0		/* �J���[�p���b�g�̎w�肪����Ȃ��� */

#define	    sprNoflip		((0) | FUNC_Texture | (UseTexture << 16))

typedef unsigned char	Uint8 ;		/* �����Ȃ��P�o�C�g���� */
typedef signed   char	Sint8 ;		/* �������P�o�C�g���� */
typedef unsigned short	Uint16 ;	/* �����Ȃ��Q�o�C�g���� */
typedef signed   short	Sint16 ;	/* �������Q�o�C�g���� */
typedef unsigned long	Uint32 ;	/* �����Ȃ��S�o�C�g���� */
typedef signed   long	Sint32 ;	/* �������S�o�C�g���� */
typedef int		Bool ;		/* �_���^�i�_���萔��l�ɂƂ�j	*/
typedef float		Float32 ;	/* �S�o�C�g���� */

enum ps {X, Y, Z, XYZ, XYZS, XYZSS, XY = Z, S = XYZ, Sh = S, Sv = XYZS} ;
enum base
{
    SORT_BFR,			/* ���O�ɕ\�������|���S���̈ʒu���g�� */
    SORT_MIN,			/* �S�_�̓��A��Ԏ�O�̓_���g�� */
    SORT_MAX,			/* �S�_�̓���ԉ����_���g�� */
    SORT_CEN			/* �S�_�̕��ψʒu���g�� */
} ;

enum pln
{
    Single_Plane,			/* �Жʃ|���S�� */
    Dual_Plane			/* ���ʃ|���S��(�\������̌��ʂ����Ȃ�) */
} ;

typedef	    Sint16	ANGLE ;
typedef	    Sint32	FIXED ;
typedef     FIXED	MATRIX[4][3] ;		/* �}�g���N�X�f�[�^ */
typedef     FIXED	VECTOR[XYZ] ;		/* �x�N�g���f�[�^ */
typedef     FIXED	POINT[XYZ] ;		/* ���_�̈ʒu�f�[�^ */

#define		_Stereo		0x80
#define		_Mono		0x00

#define		_PCM16Bit	0x00
#define		_PCM8Bit	0x10

typedef struct
{
    Uint8	mode ;		/* Mode */
    Uint8	channel ;	/* PCM Channel Number */
    Uint8	level ;		/* 0 ~ 127 */
    Sint8	pan ;		/* -128 ~ +127 */
    Uint16	pitch ;
    Uint8	eflevelR ;	/* Effect level for Right(mono) 0 ~ 7 */
    Uint8	efselectR ;	/* Effect select for Right(mono) 0 ~ 15 */
    Uint8	eflevelL ;	/* Effect level for Left 0 ~ 7 */
    Uint8	efselectL ;	/* Effect select for Left 0 ~ 15 */
} PCM ;

enum SmpcCommand  			/* �r�l�o�b�R�}���h�ԍ�			*/
{
    SMPC_MSHON,			/* (00) �}�X�^�@�r�g�Q�@�n�m		*/
    SMPC_SSHON,			/* (01) �X���[�u�@�r�g�Q�@�n�m		*/
    SMPC_SSHOFF,			/* (02) �X���[�u�@�r�g�Q�@�n�e�e	*/
    SMPC_SNDON,			/* (03) �T�E���h�@�n�m			*/
    SMPC_SNDOFF,			/* (04) �T�E���h�@�n�e�e		*/
    SMPC_CDON,			/* (05) �b�c�@�n�m			*/
    SMPC_CDOFF,			/* (06) �b�c�@�n�e�e			*/
    SMPC_SYSRES,			/* (07) �V�X�e���S�̃��Z�b�g		*/
    SMPC_CKC352,			/* (08) �N���b�N�`�F���W�R�T�Q���[�h	*/
    SMPC_CKC320,			/* (09) �N���b�N�`�F���W�R�Q�O���[�h	*/
    SMPC_NMIREQ,			/* (0a) �m�l�h���N�G�X�g		*/
    SMPC_RESENA,			/* (0b) ���Z�b�g�C�l�[�u��		*/
    SMPC_RESDIS,			/* (0c) ���Z�b�g�f�B�X�G�[�u��		*/
    SMPC_GETSTS,			/* (0d) �X�e�[�^�X�擾			*/
    SMPC_GETPER,			/* (0e) �y���t�F�����擾		*/
    SMPC_SETMEM,			/* (0f) �r�l�o�b�������ݒ�		*/
    SMPC_SETTIM,			/* (10) �����ݒ�			*/
    SMPC_CMDMAX			/* (11) �R�}���h�ԍ��ő�l		*/
};

enum SmpcWaitMode  			/* �r�l�o�b�R�}���h���s���[�h		*/
{
    SMPC_NO_WAIT,			/* (00) �R�}���h���s�I���҂�����	*/
    SMPC_WAIT			/* (01) �R�}���h���s�I���҂��L��	*/
};


enum BooleanSwitch  			/* �_���萔�Q�i�X�C�b�`�j */
{
    OFF = 0,
    ON  = 1
};

typedef struct
{
    Uint16	 texno ;		/* �e�N�X�`���ԍ� */
    Uint16	 atrb ;			/* �A�g���r���[�g�f�[�^(�\�����[�h) */
    Uint16	 colno ;		/* �J���[�i���o�[ */
    Uint16	 gstb ;			/* �O�[���[�V�F�[�f�B���O�e�[�u�� */
    Uint16	 dir ;			/* �e�N�X�`���̔��]�ƃt�@���N�V�����i���o�[ */
} SPR_ATTR ;

typedef struct
{
    Uint8	 flag ;			/* �Жʂ����ʂ��̃t���O */
    Uint8	 sort ;			/* �\�[�g�̊�ʒu�ƃI�v�V�����ݒ� */
    Uint16	 texno ;		/* �e�N�X�`���ԍ� */
    Uint16	 atrb ;			/* �A�g���r���[�g�f�[�^(�\�����[�h) */
    Uint16	 colno ;		/* �J���[�i���o�[ */
    Uint16	 gstb ;			/* �O�[���[�V�F�[�f�B���O�e�[�u�� */
    Uint16	 dir ;			/* �e�N�X�`���̔��]�ƃt�@���N�V�����i���o�[ */
} ATTR ;

typedef struct
{
    VECTOR	 norm ;			/* �@���x�N�g�� */
    Uint16	 Vertices[4] ;		/* �|���S�����\�����钸�_�ԍ� */
} POLYGON ;

typedef struct
{
    POINT	*pntbl ;		/* ���_�̈ʒu�f�[�^�e�[�u�� */
    Uint32	 nbPoint ;		/* ���_�̐� */
    POLYGON	*pltbl ;		/* �|���S����`�e�[�u�� */
    Uint32	 nbPolygon ;		/* �|���S���̐� */
    ATTR	*attbl ;		/* �|���S���̃A�g���r���[�g�e�[�u�� */
} PDATA ;

typedef struct
{
    Uint16	 Hsize ;		/* �X�v���C�g�̂g�T�C�Y */
    Uint16	 Vsize ;		/* �X�v���C�g�̂u�T�C�Y */
    Uint16	 CGadr ;		/* �X�v���C�g�b�f�A�h���X / �W */
    Uint16	 HVsize ;		/* �g�T�C�Y/�W�A�u�T�C�Y(�n�[�h�p) */
} TEXTURE ;

typedef struct
{
    Uint16	 texno ;		/* �e�N�X�`���ԍ� */
    Uint16	 cmode ;		/* �J���[���[�h */
    void	*pcsrc ;		/* ���f�[�^�̏��� */
} PICTURE ;

typedef struct  			/* �f�W�^���f�o�C�X			*/
{
    Uint8	id;			/* �y���t�F�����h�c			*/
    Uint8	ext;			/* �g���f�[�^�T�C�Y			*/
    Uint16	data;			/* �{�^�����݃f�[�^			*/
    Uint16	push;			/* �{�^�������f�[�^			*/
    Uint16	pull;			/* �{�^������f�[�^			*/
    Uint32	dummy2[4];		/* �_�~�[�Q				*/
} PerDigital;

enum BooleanLogic  			/* �_���萔�P�i�U�A�^�j */
{
    FALSE = 0,
    TRUE  = 1
};

enum tvsz
{
    TV_320x224, TV_320x240, TV_320x256, TV_dummy1,
    TV_352x224, TV_352x240, TV_352x256, TV_dummy2,
    TV_640x224, TV_640x240, TV_640x256, TV_dummy3,
    TV_704x224, TV_704x240, TV_704x256, TV_dummy4,

    TV_320x448, TV_320x480, TV_320x512, TV_dummy5,
    TV_352x448, TV_352x480, TV_352x512, TV_dummy6,
    TV_640x448, TV_640x480, TV_640x512, TV_dummy7,
    TV_704x448, TV_704x480, TV_704x512, TV_dummy8
} ;

#define		PNB_2WORD		0
#define		PNB_1WORD		0x8000
#define		CN_10BIT       		0
#define		CN_12BIT		0x4000
#define		COL_TYPE_32768		0x30

#define		PL_SIZE_1x1		0
#define		PL_SIZE_2x1		1
#define		PL_SIZE_2x2		3

#define		NBG0ON			(1<<0)
#define		NBG1ON			(1<<1)

#define		BM_512x256		0x02
#define		BM_512x512		0x06
#define		BM_1024x256		0x0a
#define		BM_1024x512		0x0e

extern void    slCurColor(Uint16) ;
extern	PerDigital*	Smpc_Peripheral;
extern	Uint8		Per_Connect1, Per_Connect2;
extern  void    slPlaneNbg1(Uint16) ;
extern  void   slUnitMatrix(MATRIX) ;
extern  Bool   slDispSprite(FIXED *, SPR_ATTR *attr, ANGLE) ;
extern  void    slScrPosNbg1(FIXED x,FIXED y) ;
extern  void    slZoomNbg1(FIXED x,FIXED y) ;
extern void    slDMACopy(void *, void *, Uint32) ;
extern  void    slBack1ColSet(void *, Uint16) ;
extern  void    slCharNbg0(Uint16 type,Uint16 size) ;
extern  void    slCharNbg1(Uint16 type,Uint16 size) ;
extern  void    slTVOn(void) ;
extern  void    slPageNbg1(void *,void *,Uint16) ;
extern  void    slMapNbg1(void *,void *,void *,void *) ;
extern void    slPrint(char *, void *) ;
extern  void    slBitMapNbg1(Uint16, Uint16, void *) ;
extern  void    slTVOff(void) ;
extern  Bool    slScrAutoDisp(Uint32) ;
extern  void   slInitSystem(Uint16, TEXTURE *, Sint8) ;
extern  void   slSynch(void) ;
extern  void    slPriority(Sint16, Uint16) ;
extern void   *slLocate(Uint16, Uint16) ;
#define		scnNBG0			1
#define		scnNBG1			0

extern  void   slLookAt(FIXED *camera, FIXED *target, ANGLE) ;
extern  void   slLight(VECTOR) ;
extern  Bool   slPushMatrix(void) ;
extern  Bool   slPopMatrix(void) ;
extern  void   slRotX(ANGLE) ;
extern  void   slRotY(ANGLE) ;
extern  void   slRotZ(ANGLE) ;
extern  void   slScale(FIXED, FIXED, FIXED) ;
extern  Bool   slPutPolygon(PDATA *) ;
extern  Bool   slWindow(Sint16, Sint16, Sint16, Sint16, Sint16, Sint16, Sint16) ;
extern  void   slPerspective(ANGLE) ;
extern  void   slTranslate(FIXED, FIXED, FIXED) ;
extern  void   slZdspLevel(Uint16) ;

extern	Bool	slRequestCommand(Uint8, Uint8);

#define CDC_POS_PTYPE(pos)          ((pos)->ptype)
#define CDC_POS_FAD(pos)            ((pos)->pbody.fad)
#define CDC_POS_TNO(pos)            ((pos)->pbody.trkidx.tno)
#define CDC_POS_IDX(pos)            ((pos)->pbody.trkidx.idx)

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

#define CDC_SM_EOR      0x01    /* ���R�[�h�Ō�̃Z�N�^ */
#define CDC_SM_VIDEO    0x02    /* �r�f�I�Z�N�^ */
#define CDC_SM_AUDIO    0x04    /* �I�[�f�B�I�Z�N�^ */
#define CDC_SM_DATA     0x08    /* �f�[�^�Z�N�^ */
#define CDC_SM_TRIG     0x10    /* �g���K�n�m */
#define CDC_SM_FORM     0x20    /* �t�H�[���r�b�g�i1:Form2,  0:Form1�j*/
#define CDC_SM_RT       0x40    /* ���A���^�C���Z�N�^ */
#define CDC_SM_EOF      0x80    /* �t�@�C���Ō�̃Z�N�^ */

#define CDC_PM_DFL          0x00    /* �Đ����[�h�̏ȗ��l */

/* �G���[�R�[�h */
#define GFS_ERR_OK              0       /* ����I��                         */
#define GFS_ERR_CDRD            (-1)    /* CD���[�h�G���[                   */
#define GFS_ERR_CDNODISC        (-2)    /* CD���Z�b�g����Ă��Ȃ�           */
#define GFS_ERR_CDROM           (-3)    /* �f�B�X�N��CD-ROM�łȂ�           */
#define GFS_ERR_DIRTBL          (-4)    /* �f�B���N�g���Ǘ��e�[�u�����s��   */
#define GFS_ERR_OPENMAX         (-5)    /* �ő�I�[�v�����̒l���s��         */
#define GFS_ERR_DIR             (-6)    /* �w��t�@�C���̓f�B���N�g���łȂ� */
#define GFS_ERR_CDBFS           (-7)    /* CD�u���b�N�t�@�C���V�X�e����     */

#define GFS_ERR_NONAME          (-8)    /* �t�@�C�����������Ȃ�             */
#define GFS_ERR_NEXIST          (-9)    /* �w�肳�ꂽ�t�@�C���͑��݂��Ȃ�   */
#define GFS_ERR_FID             (-10)   /* �s���ȃt�@�C�����ʎq             */
#define GFS_ERR_HNDL            (-11)   /* �t�@�C���n���h�����s��           */
#define GFS_ERR_SEEK            (-12)   /* �V�[�N�ʒu���s��                 */
#define GFS_ERR_ORG             (-13)   /* ��ʒl���s��                   */
#define GFS_ERR_NUM             (-14)   /* �o�C�g������                     */
#define GFS_ERR_OFS             (-15)   /* �I�t�Z�b�g���s��                 */
#define GFS_ERR_FBUSY           (-16)   /* �w��t�@�C���̏������c���Ă���   */
#define GFS_ERR_PARA            (-17)   /* �p�����[�^���s��                 */
#define GFS_ERR_BUSY            (-18)   /* ���C�u�����֐����s��             */
#define GFS_ERR_NOHNDL          (-19)   /* �t�@�C���n���h���ɋ󂫂��Ȃ�     */
#define GFS_ERR_PUINUSE         (-20)   /* �s�b�N�A�b�v���쒆               */
#define GFS_ERR_ALIGN           (-21)   /* ��Ɨ̈悪4byte���E�ɂȂ�        */
#define GFS_ERR_TMOUT           (-22)   /* �^�C���A�E�g                     */
#define GFS_ERR_CDOPEN          (-23)   /* �g���C���J���Ă���               */
#define GFS_ERR_BFUL            (-24)   /* �o�b�t�@�t���œǂݍ��ݒ��~       */
#define GFS_ERR_FATAL           (-25)   /* CD��FATAL���                    */


/* �X�e�[�^�X */
enum CdcStatus
{
    /* �X�e�[�^�X�R�[�h�i�b�c�h���C�u��ԁj */
    CDC_ST_BUSY     = 0x00,     /* ��ԑJ�ڒ� */
    CDC_ST_PAUSE    = 0x01,     /* �|�[�Y���i�ꎞ��~�j */
    CDC_ST_STANDBY  = 0x02,     /* �X�^���o�C�i�h���C�u��~��ԁj */
    CDC_ST_PLAY     = 0x03,     /* �b�c�Đ��� */
    CDC_ST_SEEK     = 0x04,     /* �V�[�N�� */
    CDC_ST_SCAN     = 0x05,     /* �X�L�����Đ��� */
    CDC_ST_OPEN     = 0x06,     /* �g���C���J���Ă��� */
    CDC_ST_NODISC   = 0x07,     /* �f�B�X�N���Ȃ� */
    CDC_ST_RETRY    = 0x08,     /* ���[�h���g���C������ */
    CDC_ST_ERROR    = 0x09,     /* ���[�h�f�[�^�G���[���������� */
    CDC_ST_FATAL    = 0x0a,     /* �v���I�G���[���������� */

    /* ���̑� */
    CDC_ST_PERI     = 0x20,     /* ������X�|���X */
    CDC_ST_TRNS     = 0x40,     /* �f�[�^�]���v������ */
    CDC_ST_WAIT     = 0x80,     /* WAIT */
    CDC_ST_REJECT   = 0xff      /* REJECT */
};

/* �]�����[�h(GFS_SetTmode) */
enum GfsTransMode
{
    GFS_TMODE_SCU = 0,                  /* �r�b�t�ɂ��c�l�`�]��    */
    GFS_TMODE_SDMA0,                    /* �c�l�`�T�C�N���X�`�[���]��   cn=0 */
    GFS_TMODE_SDMA1,                    /*                              cn=1 */
    GFS_TMODE_CPU,                      /* �\�t�g�E�F�A�]�� */
    GFS_TMODE_STM,                      /* �X�g���[���p�]�� */
    GFS_TMODE_END
};


enum CdcPosType
{
    CDC_PTYPE_DFL,          /* �ȗ��l�̎w�� */
    CDC_PTYPE_FAD,          /* �t���[���A�h���X�w�� */
    CDC_PTYPE_TNO,          /* �g���b�N�^�C���f�b�N�X�w�� */
    CDC_PTYPE_NOCHG,        /* ���ύX�̎w��i�ݒ�l��ύX���Ȃ��j */

    CDC_PTYPE_END
};

/* �b�c�ʒu�p�����[�^ */
typedef struct
{
    Sint32 ptype;           /* �ʒu�^�C�v�i�ʒu�p�����[�^�̎�ނ̎w��j*/
    union
    {
        Sint32 fad;         /* �t���[���A�h���X�A�Z�N�^�� */
        struct
        {
            Uint8 tno;      /* �g���b�N�ԍ� */
            Uint8 idx;      /* �C���f�b�N�X�ԍ� */
        } trkidx;
    } pbody;
} CdcPos;


/* �b�c�Đ��p�����[�^ */
typedef struct
{
    CdcPos  start;          /* �J�n�ʒu */
    CdcPos  end;            /* �I���ʒu */
    Uint8   pmode;          /* �Đ����[�h�i�߯����߈ړ��A�J��Ԃ��񐔁j*/
} CdcPly;


extern  void   slInitSound(Uint8 *, Uint32, Uint8 *, Uint32) ;
extern  Bool   slCDDAOn(Uint8, Uint8, Sint8, Sint8) ;
extern  Sint8  slPCMOn(PCM *, void *, Uint32) ;
extern  Bool   slPCMOff(PCM *) ;
extern  Sint8  slSndPCMNum() ;
extern  void   slSndFlush() ;
extern  Bool   slPCMStat(PCM *) ;
extern  Bool   slSndVolume(Uint8) ;

Sint32  CDC_CdPlay(CdcPly *ply);
Sint32  CDC_CdSeek(CdcPos *pos);
Sint32  CDC_CdInit(Sint32 iflag, Sint32 stnby, Sint32 ecc, Sint32 retry);

/* �t�@�C����� */
typedef struct
{
    Sint32  fad;            /* �t�@�C���擪�t���[���A�h���X */
    Sint32  size;           /* �t�@�C���T�C�Y�i�o�C�g���j */
    Uint8   unit;           /* �t�@�C�����j�b�g�T�C�Y */
    Uint8   gap;            /* �M���b�v�T�C�Y */
    Uint8   fn;             /* �t�@�C���ԍ� */
    Uint8   atr;            /* �t�@�C���A�g���r���[�g */
} CdcFile;

/* �T�u�w�b�_���� */
typedef struct
{
    Uint8   fn;             /* �t�@�C���ԍ� */
    Uint8   cn;             /* �`���l���ԍ� */
    Uint8   smmsk;          /* �T�u���[�h�̃}�X�N�p�^�[�� */
    Uint8   smval;          /* �T�u���[�h�̔�r�l */
    Uint8   cimsk;          /* �R�[�f�B���O���̃}�X�N�p�^�[�� */
    Uint8   cival;          /* �R�[�f�B���O���̔�r�l */
} CdcSubh;


enum GfsSeekMode {
    GFS_SEEK_SET = 0,                   /* �擪����V�[�N               */
    GFS_SEEK_CUR,                       /* ���݂̈ʒu����V�[�N         */
    GFS_SEEK_END                        /* �I�[����V�[�N               */
};

typedef Sint32 (*GfsTransFunc)(void *obj, Sint32 nsct);



/* �t�@�C����� */
typedef struct
{
    Sint32      fid;                    /* �t�@�C�����ʎq */
    CdcFile     finfo;
    Sint32      sctsz;                  /* �Z�N�^�� */
    Sint32      nsct;                   /* �Z�N�^�� */
    Sint32      lstrm;                  /* �ŏI�Z�N�^���̖����f�[�^��   */
} GfsFinfo;


/* CD�o�b�t�@�̎����Ǘ� */
typedef struct
{
    Sint32      bufno;                  /* �o�b�t�@���ԍ� */
    Sint32      sctsz;                  /* �b�c�o�b�t�@�̃Z�N�^�T�C�Y   */
    Sint32      flt;                    /* �i��ԍ� */
    CdcSubh     subh;                   /* �T�u�w�b�_ */
    Sint32      fmode;                  /* �i�胂�[�h */
    Sint32      puid;                   /* �擾�s�b�N�A�b�v�̃��[�UID */
    Sint32      filepos;                /* �擪�f�[�^�̃t�@�C����̈ʒu */
    Sint32      sctpos;                 /* �]���ʒu */
    /* �Đ��͈� */
    Sint32      sfad;                   /* �J�nFAD */
    Sint32      efad;                   /* �I��FAD */
} GfsCdRsrc;


/* SCSI�t�@�C���̉��zCD�o�b�t�@�Ǘ� */
typedef struct
{
    Sint32      fid;                    /* SCSI�t�@�C�����ʎq   */
    Sint32      filepos;                /* �擪�f�[�^�̃t�@�C����̈ʒu */
    Sint32      sctpos;                 /* �o�b�t�@��̃Z�N�^�ʒu */
    Sint32      sctnum;                 /* �o�b�t�@���̃Z�N�^�� */
} GfsScsiRsrc;


/* �������t�@�C���̉��zCD�o�b�t�@�Ǘ� */
typedef struct
{
    void        *data;                  /* �f�[�^�̈�           */
    Sint32      filepos;                /* �擪�f�[�^�̃t�@�C����̈ʒu */
    Sint32      sctpos;                 /* �o�b�t�@��̃Z�N�^�ʒu */
    Sint32      sctnum;                 /* �o�b�t�@���̃Z�N�^�� */
} GfsMemRsrc;


/* �ǂݍ��݌��Ǘ� */
typedef struct
{
    /* �ǂݍ��݌��Ǘ� */
    Sint32      ftype;                  /* �t�@�C�����         */
    union
    {
        GfsCdRsrc       cd;             /* CD�̎���             */
        GfsScsiRsrc     scsi;           /* SCSI�t�@�C���̎���   */
        GfsMemRsrc      mem;            /* �������t�@�C���̎��� */
    } rsrc;
} GfsDtsrc;


/* CD����̓ǂݍ��݂��Ǘ����� */
typedef struct
{
    GfsFinfo    finfo;                  /* �t�@�C�����         */
    GfsDtsrc    dtsrc;                  /* �ǂݍ��݌�           */
    Sint32      gmode;                  /* ���o�����[�h       */
    Sint32      stat;                   /* ���s���             */
    /* flow in�Ǘ� */
    Sint32      sct;                    /* �ǂݍ��݃Z�N�^��     */
    Sint32      sctcnt;                 /* �ǂݍ��݃J�E���^     */
    Sint32      sctmax;                 /* �ǂݍ��ݍő�Z�N�^�� */
} GfsFlow;


/* �f�[�^�p�b�N�\���� */
typedef struct
{
    void        *data;                  /* �f�[�^�̈�           */
    Sint32      adlt;                   /* �A�h���X�̑�����     */
    Sint32      len;                    /* �]���o�C�g��         */
    Sint32      nsct;                   /* �]���Z�N�^��         */
    Bool        use;                    /* �g�p�t���O           */
} GfsDataPack;


typedef GfsDataPack *GfdpHn;

/* �]���Ǘ� */
/* CD�o�b�t�@����̓]�����Ǘ����� */
typedef struct
{
    void        *buf;                   /* �]����A�h���X */
    Sint32      size;                   /* �f�[�^�̈�̃T�C�Y */
    Sint32      wp;                     /* �������݃I�t�Z�b�g */
    /* �]���Ǘ� */
    GfdpHn      dps;                    /* �]�����̃f�[�^�p�b�N */
    GfdpHn      dpd;                    /* �]���悩��̃f�[�^�p�b�N */
    Sint32      tsctmax;                /* �P��̓]���̍ő�Z�N�^�� */
    Sint32      tsct;                   /* �]���ڕW�Z�N�^�� */
    Sint32      tsctcnt;                /* ���݂܂œ]���Z�N�^�� */
    Sint32      tbytcnt;                /* �Z�N�^���̓]���o�C�g�� */
    /* �]���֐� */
    void        *obj;                   /* �]���֐��ւ̑�P���� */
    GfsTransFunc tfunc;                 /* �]���֐� */
    Sint32      unit;                   /* �]���P�� */
    Bool        active;                 /* ���쒆�t���O */
    Sint32      stat;                   /* ��� */
    Sint32      mode;                   /* �]�����[�h */
} GfsTrans;


typedef GfsTrans *GftrHn;


/* �I�[�v�������t�@�C�����Ǘ����� */
typedef struct
{
    Bool        used;                   /* ���̍\���̂��g�p�����ǂ��� */
    Sint32      amode;                  /* �A�N�Z�X���[�h */
    Sint32      astat;                  /* �A�N�Z�X��� */
    GfsFlow     flow;
    GfsTrans    trans;
} GfsFile;

#define GFS_FNAME_LEN   12              /* �t�@�C�����̒���                 */



/* �f�B���N�g�����R�[�h�\����(�t�@�C�����Ȃ�)   */
typedef struct
{
    CdcFile     dirrec;
} GfsDirId;


/* �f�B���N�g�����R�[�h�\����(�t�@�C��������)   */
typedef struct
{
    CdcFile     dirrec;
    Sint8       fname[GFS_FNAME_LEN];   /* �t�@�C���� */
} GfsDirName;


/* �f�B���N�g�����Ǘ��\���� */
typedef struct
{
    Sint32      type;                   /* ��� */
    Sint32      ndir;                   /* �ő�v�f�� */
    union
    {
        GfsDirId *dir_i;                /* �t�@�C�����Ȃ��f�B���N�g����� */
        GfsDirName *dir_n;              /* �t�@�C�������f�B���N�g����� */
    } dir;
} GfsDirTbl;


/* �t�@�C���n���h�� */
typedef GfsFile *GfsHn;

#define GFS_FTYPE_NR    3               /* �t�@�C����ʐ� */
#define GFS_SCTBUF_SIZ  2048            /* �Z�N�^�o�b�t�@�̑傫�� */
#define GFS_OPEN_MAX    24              /* �ő�I�[�v���t�@�C���� */
#define GFS_FCONQ_MAX   24              /* �ڑ��R�}���h�L���[�̍ő咷 */

/* �G���[�����֐� */
typedef void (*GfsErrFunc)(void *obj, Sint32 ec);

/* �G���[��� */
typedef struct
{
    GfsErrFunc  func;                   /* �G���[�������̌Ăяo���֐�   */
    void        *obj;                   /* �Ăяo���֐��ɓn��������   */
    Sint32      code;                   /* �G���[�R�[�h                 */
} GfsErrStat;

#define GFS_CDBBUF_NR   24              /* CD�u���b�N�̋�搔 */
#define GFS_SELQ_MAX    24              /* �i��ݒ�R�}���h�L���[�̍ő咷 */

/* �f�B���N�g�����(GFS_LoadDir, GFS_SetDir)     */
enum GfsDirType
{
    GFS_DIR_ID          = 0,            /* �t�@�C�����ɂ��A�N�Z�X�s�� */
    GFS_DIR_NAME                        /* �t�@�C�����ɂ��A�N�Z�X��   */
};

enum GfsServerStatus
{
    GFS_SVR_COMPLETED = 0,              /* �S�t�@�C���̃A�N�Z�X�I��     */
    GFS_SVR_BUSY,                       /* �t�@�C���A�N�Z�X��           */
    GFS_SVR_CDPAUSE,                    /* �b�c�o�b�t�@�t���̈׈ꎞ��~ */
    GFS_SVR_ERROR                       /* �G���[���� */
};



/* �ڑ��R�}���h�L���[�̗v�f(CD�u���b�N�Ǘ��\����, GfsCdbMng�̃����o) */
typedef struct
{
    Sint32 flt;                         /* �ݒ�i�� */
    Sint32 buf;                         /* �^�o�͐ڑ��o�b�t�@ */
    Sint32 flnout;                      /* �U�o�͐ڑ��i��(���̒l���g�p) */
} GfcdFconQu;


/* �b�c�X�e�[�^�X���i�X�e�[�^�X�{�b�c���|�[�g�j */
typedef struct
{
    Uint8   status;         /* �X�e�[�^�X */
    struct                  /* �b�c���|�[�g */
    {
        Uint8   flgrep;     /* �b�c�t���O�ƃ��s�[�g�� */
        Uint8   ctladr;     /* CONTROL/ADR�o�C�g */
        Uint8   tno;        /* �g���b�N�ԍ� */
        Uint8   idx;        /* �C���f�b�N�X�ԍ� */
        Sint32  fad;        /* �t���[���A�h���X */
    } report;
} CdcStat;

/* �i��ݒ�R�}���h�L���[�̗v�f(CD�u���b�N�Ǘ��\����, GfsCdbMng�̃����o) */
typedef struct
{
    Uint8 flt;                          /* �ݒ��i��ԍ� */
    Uint8 fmode;                        /* �i�胂�[�h */
    CdcSubh subh;                       /* �T�u�w�b�_ */
    Sint32 fad;                         /* �J�nFAD */
    Sint32 snum;                        /* FAD�Z�N�^�� */
} GfcdSelQu;


/* CD�u���b�N�Ǘ��\����(�Ǘ��e�[�u��, GfsMng�̃����o) */
typedef struct
{
    /* �����Ǘ� */
    Sint8 use_buf[GFS_CDBBUF_NR];       /* �o�b�t�@���g�p��� */
    Sint8 use_filt[GFS_CDBBUF_NR];      /* �i��g�p��� */
    Bool use_pu;                        /* �s�b�N�A�b�v�g�p��� */
    Sint32 tr_bufno;                    /* �f�[�^�]�����̃o�b�t�@���   */
    Sint32 puid;                        /* �s�b�N�A�b�v�̏��L��ID */
    Sint32 timer;                       /* �^�C���A�E�g�J�E���^ */
    CdcStat stat;                       /* CD�u���b�N�̏�� */
    void (*func)(void *);               /* CSCT�n���h�� */
    void *obj;                          /* CSCT�n���h���̈��� */
    /* GFCD_SetFilt�^�X�N */
    struct
    {
        Sint32 len;                     /* �i��ݒ�R�}���h�L���[�̒��� */
        Sint32 stat;                    /* �R�}���h�L���[�擪�̏�� */
        GfcdSelQu selq[GFS_SELQ_MAX];   /* �i��ݒ�R�}���h�L���[ */
    } tsk_setflt;
    /* GFCD_SetFiltCon�^�X�N */
    struct
    {
        Sint32 len;                     /* �ڑ��R�}���h�L���[�̒��� */
        Sint32 stat;                    /* �^�X�N�I���t���O */
        GfcdFconQu fconq[GFS_FCONQ_MAX]; /* �ڑ��R�}���h�L���[ */
    } tsk_fltcon;
    /* GFCD_SetCon�^�X�N */
    struct
    {
        Sint32 stat;                    /* �^�X�N�I���t���O */
        Sint32 flt;                     /* CD�ڑ���i�� */
    } tsk_setcon;
    /* GFCD_GetLenData�^�X�N */
    struct
    {
        Sint32 stat;                    /* �^�X�N�I���t���O */
        Sint32 bufno;                   /* �Ώۋ�� */
        Sint32 spos;                    /* �擪�Z�N�^�ʒu */
        Sint32 usct;                    /* �v���Z�N�^�� */
        Sint32 cnt;                     /* �^�X�N�̎��s�� */
        Sint32 *nsct;                   /* �����Z�N�^���i�[�� */
        Sint32 *nbyte;                  /* �����o�C�g���i�[�� */
    } tsk_getlen;
    /* GFCD_ReqData�^�X�N */
    struct
    {
        Sint32 stat;                    /* �^�X�N�I���t���O */
        Sint32 bufno;                   /* �Ώۋ�� */
        Sint32 sctpos;                  /* �Z�N�^�ʒu */
        Sint32 nsct;                    /* �v���Z�N�^�� */
    } tsk_reqdat;
    /* GFCD_DelSctData�^�X�N */
    struct
    {
        Sint32 stat;                    /* �^�X�N�I���t���O */
        Sint32 bufno;                   /* �Ώۋ�� */
        Sint32 sctpos;                  /* �폜�J�n�Z�N�^�ʒu */
        Sint32 nsct;                    /* �폜�Z�N�^�� */
    } tsk_delsct;
    /* GFCD_MoveSctData�^�X�N */
    struct
    {
        Sint32 stat;                    /* �^�X�N�I���t���O */
        Sint32 dst;                     /* �ړ����� */
        Sint32 src;                     /* �ړ������ */
        Sint32 spos;                    /* �ړ��J�n�Z�N�^�ʒu */
        Sint32 snum;                    /* �ړ��Z�N�^�� */
        Sint32 fmode;                   /* �ړ���Z���N�^�̍i�胂�[�h�ۑ� */
    } tsk_movsct;
    /* GFCD_ChgDir�^�X�N */
    struct
    {
        Sint32 stat;                    /* �^�X�N�I���t���O */
        Sint16 fid;                     /* �ݒ�t�@�C�����ʎq */
        Sint16 work;                    /* ��ƃo�b�t�@ */
        Sint32 *ndir;                   /* �ێ��f�B���N�g������ */
    } tsk_chgdir;
} GfsCdbMng;


/* �A�N�Z�X�T�[�o�Ǘ��\����(�Ǘ��e�[�u��, GfsMng�̃����o) */
typedef struct
{
    GfsHn   access_file[GFS_OPEN_MAX];  /* �A�N�Z�X���[�h�w��σt�@�C�� */
    Sint32  nfile;                      /* �o�^�t�@�C����               */
} GfsSvr;

/* �f�B���N�g���Ǘ��\����(�Ǘ��e�[�u��, GfsMng�̃����o) */
typedef struct
{
    GfsDirTbl   dirtbl;                 /* �f�B���N�g�����Ǘ� */
    Sint32      nfile;                  /* �t�@�C���� */
} GfsDirMng;

/* �t�@�C������p�֐�(�Ǘ��e�[�u��, GfsMng�̃����o) */
typedef struct
{
    Sint32 (*flowin)(GfsFlow *);        /* �\�[�X�o�b�t�@�ǂݍ��݊֐�   */
    void (*stopin)(GfsFlow *, Bool);    /* �ǂݍ��ݒ��~�֐�             */
    Sint32 (*seek)(GfsFlow *, Sint32);  /* �A�N�Z�X�|�C���^�ݒ�֐�     */
    Sint32 (*tell)(GfsFlow *);          /* �A�N�Z�X�|�C���^�擾�֐�     */
} GfsFileFunc;


/* �Ǘ��e�[�u�� */
typedef struct
{
    Sint32      openmax;                /* �I�[�v���t�@�C���̍ő吔 */
    GfsFileFunc functbl[GFS_FTYPE_NR];  /* �t�@�C������p�֐� */
    GfsSvr      svr;                    /* �A�N�Z�X�T�[�o */
    GfsDirMng   curdir;                 /* �J�����g�f�B���N�g�� */
    GfsHn       pickup;                 /* �s�b�N�A�b�v���䒆�t�@�C�� */
    Sint32      sfad;                   /* �Đ��͈�:�J�nFAD */
    Sint32      efad;                   /* �Đ��͈�:�I��FAD+1 */
    GfsHn       trans;                  /* �f�[�^�]�����t�@�C�� */
    GfsErrStat  error;                  /* �G���[��� */
    Uint32      flags;                  /* �f�o�C�X�ڑ����Ȃ� */
    Sint32      timer;                  /* �^�C���A�E�g�J�E���^ */
    GfsCdbMng   cdb;                    /* CD�u���b�N�Ǘ��\���� */
    GfsDataPack srcpk;                  /* �]�����f�[�^�p�b�N */
    GfsDataPack dstpk;                  /* �]����f�[�^�p�b�N */
    Uint8       sect_buf[GFS_SCTBUF_SIZ]; /* �P�Z�N�^�̓ǂݍ��ݗp�o�b�t�@ */
    GfsFile     file[1];                /* �S�n���h���̔z��i�ϒ��j */
} GfsMng;

#define GFS_WORK_SIZE(open_max) \
            (sizeof(GfsMng) + ((open_max) - 1) * sizeof(GfsFile))
#define GFS_DIRTBL_TYPE(dirtbl)         ((dirtbl)->type)
#define GFS_DIRTBL_DIRNAME(dirtbl)      ((dirtbl)->dir.dir_n)
#define GFS_DIRTBL_NDIR(dirtbl)         ((dirtbl)->ndir)

/* �t�@�C���V�X�e���̏����� */
Sint32 GFS_Init(Sint32 open_max, void *work, GfsDirTbl *dirtbl);
/* �������A�^�ǂݍ��� */
Sint32 GFS_NwFread(GfsHn gfs, Sint32 nsct, void *buf, Sint32 bsize);
/* �t�@�C���P�ʂł̃A�N�Z�X����̎��s */
Sint32 GFS_NwExecOne(GfsHn gfs);
/* �������A�^�ǂݍ��� */
Sint32 GFS_Fread(GfsHn gfs, Sint32 nsct, void *buf, Sint32 bsize);
/* �A�N�Z�X��Ԃ̎擾 */
void GFS_NwGetStat(GfsHn gfs, Sint32 *amode, Sint32 *ndata);
/* �t�@�C���n���h���̊����� */
GfsHn GFS_Open(Sint32 fid);
/* �t�@�C���n���h���̊����ĉ��� */
void GFS_Close(GfsHn gfs);
void GFS_GetFileSize(GfsHn gfs, Sint32 *sctsz, Sint32 *nsct, Sint32 *lstsz);
Sint32 GFS_NwCdRead(GfsHn gfs, Sint32 nsct);
Sint32 GFS_SetTransPara(GfsHn gfs, Sint32 tsize);

Sint32 GFS_LoadDir(Sint32 fid, GfsDirTbl *dirtbl);
Sint32 GFS_SetDir(GfsDirTbl *dirtbl);
Sint32 GFS_NameToId(Sint8 *fname);
Sint32 GFS_Load(Sint32 fid, Sint32 ofs, void *buf, Sint32 bsize);
Sint32 GFS_Seek(GfsHn gfs, Sint32 ofs, Sint32 org);

typedef struct  			/* �q�s�b����				*/
{
    Uint16	year;			/* ����N��				*/
    Uint8	month;			/* �j���E��				*/
    Uint8	date;			/* ��					*/
    Uint8	hour;			/* ��					*/
    Uint8	minute;			/* ��					*/
    Uint8	second;			/* �b					*/
    Uint8	dummy;			/* �_�~�[				*/
} SmpcDateTime;


typedef struct  			/* �r�l�o�b�X�e�[�^�X			*/
{
    Uint8	cond;			/* ��ԃX�e�[�^�X			*/
    Uint8	dummy1;			/* �_�~�[�P				*/
    Uint16	dummy2;			/* �_�~�[�Q				*/
    SmpcDateTime	rtc;		/* �q�s�b����				*/
    Uint8	ctg;			/* �J�[�g���b�W�R�[�h			*/
    Uint8	area;			/* �G���A�R�[�h				*/
    Uint16	system;			/* �V�X�e���X�e�[�^�X			*/
    Uint32	smem;			/* �r�l�o�b�������ێ��f�[�^		*/
} SmpcStatus;

enum SmpcLanguage  			/* �r�l�o�b����������ԍ�		*/
{
    SMPC_ENGLISH,			/* (00) �p��				*/
    SMPC_DEUTSCH,			/* (01) �h�C�c��			*/
    SMPC_FRANCAIS,			/* (02) �t�����X��			*/
    SMPC_ESPANOL,			/* (03) �X�y�C����			*/
    SMPC_ITALIANO,			/* (04) �C�^���A��			*/
    SMPC_JAPAN			/* (05) ���{��				*/
};

extern  void   slIntFunction(void (*func)()) ;
extern	Uint8	slGetLanguage(void);

extern	SmpcStatus*	Smpc_Status;
#define	slGetStatus()			slRequestCommand(SMPC_GETSTS, SMPC_NO_WAIT)


#define	    COL_256	(2+0)		/* �J���[�o���N�Q�T�U�F���[�h */
#define	    COL_32K	(2-1)		/* �q�f�a�R�Q�j�F���[�h */

#define	    SPR_ATTRIBUTE(t,c,g,a,d)	{t,(a)|(((d)>>24)&0xc0),c,g,(d)&0x0f3f}
#define	    FUNC_Sprite		1
#define	    CL256Bnk		(4 << 3)	/* �J���[�o���N�Q�T�U�F���[�h */

#define	    SpriteVRAM		0x25c00000

#define	CGADDRESS	0x10000
#define	AdjCG(cga,hs,vs,col)		((cga) + (((((hs)*(vs)*4)>>(col))+0x1f) &0x7ffe0))
#define	TEXTBL(hs,vs,cga)		{hs , vs , (cga)>>3 , ((hs)&0x1f8)<<5|(vs)}
#define	PICTBL(texno,cmode,pcsrc)	{(Uint16)(texno),(Uint16)(cmode),(void *)(pcsrc)}

/*
#include <SGL_CPK.H>
*/

#else /* JO_COMPILE_USING_SGL */

#include <SGL.H>
#include <SL_DEF.H>
#include <SEGA_CDC.H>
#include <SEGA_GFS.H>

#endif

/*
#include <SEGA_SYS.H>
*/
void SYS_Exit(Sint32 code);

#endif /* !__JO_SGL_PROTOTYPES_H__ */

/*
** END OF FILE
*/
