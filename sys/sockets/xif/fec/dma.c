#include "global.h"

#include "platform/board.h"
#include "cookie.h"

DMACF_COOKIE * dmac = NULL;

DMACF_COOKIE * mc_dma_init( void )
{
    if( dmac != NULL ){
        return( dmac );
    }

    struct cookie *cookie = *CJAR;
    if (cookie)
	{
        while (cookie->tag)
		{
            if (cookie->tag == COOKIE_DMAC ){
                dmac = (DMACF_COOKIE*)cookie->value;
                if( dmac->magic != COOKIE_DMAC ){
                    dmac = NULL;
                }
                break;
            }
            cookie++;
		}
	}
	return( dmac );
}

