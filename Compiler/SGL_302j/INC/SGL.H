#ifndef	_SGL_H_   /* SGL.H */
#define	_SGL_H_

#include <machine.h>
#include "sl_def.h"

#ifdef	SATURN
#include "sega_int.h"
#endif

#ifdef	ST_V
#include "syscall.h"
#endif

/********************************/
/*				*/
/********************************/
#define	NORMAL(x,y,z)		{POStoFIXED(x,y,z)
#define	VERTICES(v0,v1,v2,v3)	{v0 , v1 , v2 , v3}}
#define	C_RGB(r,g,b)		(((b)&0x1f)<<10|((g)&0x1f)<<5|((r)&0x1f)|0x8000)

typedef	Uint16	TEXDAT;

/********************************/
/*	Old Texture Table	*/
/********************************/
#define	cgaddress	0x10000
#define pal COL_32K
#define TEXDEF(h,v,presize)		{h,v,(cgaddress+(((presize)*4)>>(pal)))/8,(((h)&0x1f8)<<5 | (v))}
#define	PICDEF(texno,cmode,pcsrc)	{(Uint16)(texno),(Uint16)(cmode),(void *)(pcsrc)}

/********************************/
/*	New Texture Table	*/
/********************************/
#define	CGADDRESS	0x10000
#define	AdjCG(cga,hs,vs,col)		((cga) + (((((hs)*(vs)*4)>>(col))+0x1f) &0x7ffe0))
#define	TEXTBL(hs,vs,cga)		{hs , vs , (cga)>>3 , ((hs)&0x1f8)<<5|(vs)}
#define	PICTBL(texno,cmode,pcsrc)	{(Uint16)(texno),(Uint16)(cmode),(void *)(pcsrc)}

/********************************/
/*	Object structure	*/
/********************************/
/*	Modified by T.Ota 1.Oct.95
typedef struct object{
  PDATA  *myself;
  FIXED  pos[XYZ] ;
  ANGLE  ang[XYZ] ;
  FIXED  scl[XYZ] ;  
  struct object *child;
  struct object *sibling;
} OBJECT;
*/
#define TRANSLATION(x,y,z)	{toFIXED(x),toFIXED(y),toFIXED(z)}
#define ROTATION(x,y,z)		{DEGtoANG(x),DEGtoANG(y),DEGtoANG(z)}
#define SCALING(x,y,z)		{toFIXED(x),toFIXED(y),toFIXED(z)}

#endif /* _SGL_H_ */
