#define         YOKO                      0
#define         TATE                      1
#define         SIPOSX                    toFIXED(-35.0)
#define         SIPOSY                    toFIXED(-50.0)
#define         SX                        toFIXED(512.0)
#define         SY                        toFIXED(512.0)
#define         POSX_UP                   toFIXED(2.0)
#define         POSY_UP                   toFIXED(2.0) 

extern          Uint8    yama_cel[] ;
extern          Uint16   yama_map[] ;
extern          Uint16   yama_pal[] ;
extern          Uint8    kage_cel[] ;
extern          Uint16   kage_map[] ;
extern          Uint16   kage_pal[] ;
extern          Uint8    am2_cel[] ;
extern          Uint16   am2_map[] ;
extern          Uint16   am2_pal[] ;
extern          Uint16   ascii_map[] ;
extern          Uint8    sora_cel[] ;
extern          Uint16   sora_map[] ;
extern          Uint16   sora_pal[] ;
extern          Uint8    sora_cel2[] ;
extern          Uint16   sora_map2[] ;
extern          Uint16   sora_pal2[] ;
extern          Uint8    sima_cel[] ;
extern          Uint16   sima_map[] ;
extern          Uint16   sima_pal[] ;
extern          Uint8    tuti_cel[] ;
extern          Uint16   tuti_map[] ;
extern          Uint16   tuti_pal[] ;
extern          Uint8    tuti_cel2[] ;
extern          Uint16   tuti_map2[] ;
extern          Uint16   tuti_pal2[] ;

extern          void Cel2VRAM(Uint8 *  , void * , Uint32 ) ;
extern          void Map2VRAM(Uint16 * , void * , Uint16 , Uint16 , Uint16 , Uint32 ) ;
extern          void Pal2CRAM(Uint16 * , void * , Uint32 ) ;
