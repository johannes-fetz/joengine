
/*						*/
/*  Macro define for slExtendScroll		*/

    enum  exfnc {
	_End_ ,
	_LocateA0_ ,
	_LocateA1_ ,
	_LocateB0_ ,
	_LocateB1_ ,
	_Set_ ,
	_Stock_ ,
	_Same_ ,
	_Inc_ ,
	_Inc2_ ,
	_Dec_ ,
	_Dec2_ ,
	_Vline_ ,
	_VlineMinus_ ,
	_Skip_ ,
	_Skip2_ ,
	_Copy_ ,
	_RelCopy_ ,
	_Fill_ ,
	_Bra_ ,
	_Bsr_ ,
	_Rts_ ,
	_Block_ ,
	_BlockInc_ ,
	_BlockInc2_ ,
	_One_ ,
	_High_,
	_BlockFill_,
	_Add_
    } ;

#define	    End()		_End_*4+0x80
#define	    LocateA0(adr)	_LocateA0_*4+0x80, (((adr) & 0x1fe00) >> 9) , (((adr) & 0x1fe) >> 1)
#define	    LocateA1(adr)	_LocateA1_*4+0x80, (((adr) & 0x1fe00) >> 9) , (((adr) & 0x1fe) >> 1)
#define	    LocateB0(adr)	_LocateB0_*4+0x80, (((adr) & 0x1fe00) >> 9) , (((adr) & 0x1fe) >> 1)
#define	    LocateB1(adr)	_LocateB1_*4+0x80, (((adr) & 0x1fe00) >> 9) , (((adr) & 0x1fe) >> 1)
#define	    Set(cnt)		_Set_*4+0x80, cnt - 1
#define	    Stock(cnt)		_Stock_*4+0x80, cnt - 1
#define	    Same(cnt,chr)	_Same_*4+0x80, cnt - 1 , (chr) >> 8 , (chr) & 0xff
#define	    Inc(cnt,chr)	_Inc_*4+0x80, cnt - 1 , (chr) >> 8 , (chr) & 0xff
#define	    Inc2(cnt,chr)	_Inc2_*4+0x80, cnt - 1 , (chr) >> 8 , (chr) & 0xff
#define	    Dec(cnt,chr)	_Dec_*4+0x80, cnt - 1 , (chr) >> 8 , (chr) & 0xff
#define	    Dec2(cnt,chr)	_Dec2_*4+0x80, cnt - 1 , (chr) >> 8 , (chr) & 0xff
#define	    Vline(cnt,chr)	_Vline_*4+0x80, cnt - 1 , (chr) >> 8 , (chr) & 0xff
#define	    VlineMinus(cnt,chr)	_VlineMinus_*4+0x80, cnt - 1 , (chr) >> 8 , (chr) & 0xff
#define	    Skip(offset)	_Skip_*4+0x80, offset
#define	    Skip2(offset)	_Skip2_*4+0x80, (offset) >> 8 , (offset) & 0xff
#define	    Copy(src,cnt)	_Copy_*4+0x80, ((src) >> 17) & 0xff , ((src) >> 9) & 0xff , (src) & 0xff , cnt - 1
#define	    RelCopy(src,cnt)	_RelCopy_*4+0x80, ((src) >> 8) , (src) & 0xff , cnt - 1
#define	    Fill(cnt,chr)	_Fill_*4+0x80,((cnt-1) >> 8) , (cnt-1)&0xff,(chr) >> 8,(chr) & 0xff
#define	    Bra(adr)		_Bra_*4+0x80,((adr)>>8),(adr)&0xff
#define	    Bsr(adr)		_Bsr_*4+0x80,((adr)>>8),(adr)&0xff
#define	    Rts()		_Rts_*4+0x80
#define	    Block(h,v)		_Block_*4+0x80,(h)-1,(v)-1
#define	    BlockInc(h,v,c)	_BlockInc_*4+0x80,(h)-1,(v)-1,(c) >> 8,(c)&0xff
#define	    BlockInc2(h,v,c)	_BlockInc2_*4+0x80,(h)-1,(v)-1,(c) >> 8,(c)&0xff
#define	    One(c)		_One_*4+0x80,(c) >> 8,(c)&0xff
#define	    High(cnt,chr)	_High_*4+0x80,(cnt)-1,(chr)>>8,(chr)&0xff
#define	    BlockFill(h,v,c)	_BlockFill_*4+0x80,(h)-1,(v)-1,(c) >> 8,(c)&0xff
#define	    Add(cnt,add,chr)	_Add_*4+0x80, cnt - 1 , add , (chr) >> 8 , (chr) & 0xff

#define	    Char1(c)		(c) >> 8 , (c) & 0xff
#define	    Char2(c1,c2)	Char1(c1) , Char1(c2)
#define	    Char3(c1,c2,c3)	Char2(c1,c2) , Char1(c3)
#define	    Char4(c1,c2,c3,c4)	Char2(c1,c2) , Char2(c3,c4)
#define	    Char5(c1,c2,c3,c4,c5) Char3(c1,c2,c3) , Char2(c4,c5)
#define	    Char6(c1,c2,c3,c4,c5,c6) Char3(c1,c2,c3) , Char3(c4,c5,c6)
#define	    Char7(c1,c2,c3,c4,c5,c6,c7) Char3(c1,c2,c3) , Char4(c4,c5,c6,c7)
#define	    Char8(c1,c2,c3,c4,c5,c6,c7,c8) Char4(c1,c2,c3,c4) , Char4(c5,c6,c7,c8)

#define	    LChar1(c)		(c)&0xff
#define	    LChar2(c1,c2)	(c1)&0xff,(c2)&0xff
#define	    LChar3(c1,c2,c3)	(c1)&0xff,(c2)&0xff,(c3)&0xff
#define	    LChar4(c1,c2,c3,c4)	LChar2(c1,c2),LChar2(c3,c4)
#define	    LChar5(c1,c2,c3,c4,c5) LChar2(c1,c2),LChar3(c3,c4,c5)
#define	    LChar6(c1,c2,c3,c4,c5,c6) LChar3(c1,c2,c3),LChar3(c4,c5,c6)
#define	    LChar7(c1,c2,c3,c4,c5,c6,c7) LChar4(c1,c2,c3,c4),LChar3(c5,c6,c7)
#define	    LChar8(c1,c2,c3,c4,c5,c6,c7,c8) LChar4(c1,c2,c3,c4),LChar4(c5,c6,c7,c8)

