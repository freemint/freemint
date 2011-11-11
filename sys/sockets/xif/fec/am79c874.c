
#include "buf.h"
#include "inet4/if.h"
#include "inet4/ifeth.h"
#include "netinfo.h"
#include "mint/sockio.h"



#include "platform/board.h"
#include "fec.h"
#include "am79c874.h"



/********************************************************************/
/* Initialize the AM79C874 PHY
 *
 * This function sets up the Auto-Negotiate Advertisement register
 * within the PHY and then forces the PHY to auto-negotiate for
 * it's settings.
 *
 * Params:
 *  fec_ch      FEC channel
 *  phy_addr    Address of the PHY.
 *  speed       Desired speed (10BaseT or 100BaseTX)
 *  duplex      Desired duplex (Full or Half)

 * Return Value:
 *  0 if MII commands fail
 *  1 otherwise
 */
long am79c874_init(uint8 fec_ch, uint8 phy_addr, uint8 speed, uint8 duplex)
{
	long timeout;
	uint16 settings;

	duplex = duplex;

	/* Initialize the MII interface */
	fec_mii_init(fec_ch, SYSTEM_CLOCK);
	/* Reset the PHY */
	if(!fec_mii_write(fec_ch, phy_addr, MII_AM79C874_CR, MII_AM79C874_CR_RESET))
	{
		KDEBUG(("PHY error reset"));
		return 0;
	}

	/* Wait for the PHY to reset */
	for(timeout = 0; timeout < FEC_MII_TIMEOUT; timeout++)
	{
		fec_mii_read(fec_ch, phy_addr, MII_AM79C874_CR, &settings);
		if(!(settings & MII_AM79C874_CR_RESET))
			break;
	}
	if(timeout >= FEC_MII_TIMEOUT)
	{
		KDEBUG(("PHY error reset timeout"));
		return 0;
	}

	if( speed != FEC_MII_10BASE_T ){
		KDEBUG(("PHY Enable Auto-Negotiation"));
		/* Enable Auto-Negotiation */
		if(!fec_mii_write(fec_ch, phy_addr, MII_AM79C874_CR, MII_AM79C874_CR_AUTON | MII_AM79C874_CR_RST_NEG))
		{
			KDEBUG(("PHY error enable Auto-Negotiation"));
			return 0;
		}
		KDEBUG(("PHY Wait for auto-negotiation to complete"));
		/* Wait for auto-negotiation to complete */
		for(timeout = 0; timeout < FEC_MII_TIMEOUT; timeout++)
		{
			settings = 0;
			fec_mii_read(fec_ch, phy_addr, MII_AM79C874_SR, &settings);
			if((settings & AUTONEGLINK) == AUTONEGLINK)
				break;
		}
		if(timeout >= FEC_MII_TIMEOUT)
		{

			/* Set the default mode (Full duplex, 100 Mbps) */
			if(!fec_mii_write(fec_ch, phy_addr, MII_AM79C874_CR, MII_AM79C874_CR_100MB | MII_AM79C874_CR_DPLX))
			{
				KDEBUG(("PHY error set default moden"));
				return 0;
			}
		}
	} else {
		if(!fec_mii_write(fec_ch, phy_addr, MII_AM79C874_CR, 0))
		{
			KDEBUG(("PHY error set force mode"));
			return 0;
		}
	}
	settings = 0;
	fec_mii_read(fec_ch, phy_addr, MII_AM79C874_DR, &settings);
	KDEBUG(("PHY Mode: "));
	if(settings & MII_AM79C874_DR_DATA_RATE)
		KDEBUG(("100Mbps "));
	else
		KDEBUG(("10Mbps "));
	if(settings & MII_AM79C874_DR_DPLX)
		KDEBUG(("Full-duplex"));
	else
		KDEBUG(("Half-duplex"));
	return 1;
}
/********************************************************************/




