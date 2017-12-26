/* 
    C言語で必要な初期化処理サンプルルーチン(SGLサンプルプログラム用)
      1.BSS(未初期化領域)のクリア
      2.ROM領域からRAM領域へのコピー(存在しない場合はいらない)
      3.SGLシステム変数領域のクリア
*/

#include	"sgl.h"

#define		SystemWork		0x060ffc00		/* System Variable Address */
#define		SystemSize		(0x06100000-0x060ffc00)		/* System Variable Size */
/* sl.lnk で指定した.bssセクションの開始、終了シンボル */
extern Uint32 _bstart, _bend;
/* */
extern void ss_main( void );

// GNUSH: void to int
int	main( void )
{
	Uint8	*dst;
	Uint32	i;

	/* 1.Zero Set .bss Section */
	for( dst = (Uint8 *)&_bstart; dst < (Uint8 *)&_bend; dst++ ) {
		*dst = 0;
	}
	/* 2.ROM has data at end of text; copy it. */

	/* 3.SGL System Variable Clear */
	for( dst = (Uint8 *)SystemWork, i = 0;i < SystemSize; i++) {
		*dst = 0;
	}

	/* Application Call */
	ss_main();
	return 0;
}
