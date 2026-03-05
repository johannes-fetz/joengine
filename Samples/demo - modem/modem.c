#include "modem.h"
#include <jo/jo.h>

//#ifdef JO_COMPILE_WITH_MODEM_SUPPORT

/*
static const uint32_t modem_register_table[] =
{
    0x25895001, // DLAB(0) => RBR, THR; DLAB(1) => DLL
    0x25895005, // DLAB(0) => IER; DLAB(1) => DLH
    0x25895009, // IIR, FCR
    0x2589500D, // LCR
    0x25895011, // MCR
    0x25895015, // LSR
    0x25895019, // MSR
    0x2589501D, // Scratch
    0x2582503D  // Register access latch
};
*/

//25010100

#define MODEM_REG_SCRATCH ((volatile unsigned char*)0x2589501D)
#define MODEM_REG_LATCH ((volatile unsigned char*)0x2582503D)

//#define MODEM_REG_SCRATCH ((volatile unsigned char*)0x2501011c)
//#define MODEM_REG_LATCH ((volatile unsigned char*)0x2501013c)

static void jo_modem_confirm_reg_read_write(void)
{
    *MODEM_REG_LATCH = 0xFF;
}

static void jo_modem_write_reg(volatile unsigned char* dest_reg, unsigned char val)
{
    *dest_reg = val;
    jo_modem_confirm_reg_read_write();
}

static unsigned char jo_modem_read_reg(volatile unsigned char* src_reg)
{
    unsigned char val = 0;

    val = *src_reg;
    jo_modem_confirm_reg_read_write();

    return val;
}

int jo_modem_is_present(void)
{
    unsigned char val = 0;

    // write a value to the scratch register and read it out
    jo_modem_write_reg(MODEM_REG_SCRATCH, 0x5A);    
    val = jo_modem_read_reg(MODEM_REG_SCRATCH);
    if(val != 0xA5)
    {
        return 0;
    }

    // write a 2nd value to the scratch register and read it out
    jo_modem_write_reg(MODEM_REG_SCRATCH, 0xA1);
    val = jo_modem_read_reg(MODEM_REG_SCRATCH);
    if(val != 0xA1)
    {
        return 0;
    }

    return 1;
}

int jo_modem_init(void)
{

    return jo_modem_is_present();
}

//#endif /* !JO_COMPILE_WITH_MODEM_SUPPORT */

/*
 ** END OF FILE
*/
  
