#ifndef CF_BOARD_H
#define CF_BOARD_H

#define __MBAR ((volatile unsigned char*)0xff000000)

/*
    FireTOS Flash paramters, like MAC Address
    (configure via FireBee CPX)
*/
#define PARAMS_ADDRESS 0xE0002000

/*
 * System Bus Clock Info
 */
#define SYSTEM_CLOCK 133

/*
	CPU Type
*/
#define MCF547X

#include "types.h"
#include "mcf548x_fec.h"
#include "mcf548x_dma.h"
#include "mcf548x_gpio.h"
#include "mcf548x_intc.h"
#include "dma.h"

/*
 * Structure definition for the Parameters
 */
#define FILENAME_SIZE   (40)
typedef struct
{
    unsigned long  baud;
    unsigned char   client[4];
    unsigned char   server[4];
    unsigned char   gateway[4];
    unsigned char   netmask[4];
    unsigned char   netcfg[4];
    unsigned char   ethaddr[6];
    char    filename[FILENAME_SIZE];
} PARAM;

#endif
