# ifndef _igmp_h
# define _igmp_h

# include "global.h"

# include "buf.h"

/* IGMP timer */
#define IGMP_TMR_INTERVAL              100 /* Milliseconds */
#define IGMP_V1_DELAYING_MEMBER_TMR   (1000/IGMP_TMR_INTERVAL)
#define IGMP_JOIN_DELAYING_MEMBER_TMR (500 /IGMP_TMR_INTERVAL)

/* MAC Filter Actions, these are passed to a netif's
 * igmp_mac_filter callback function. */
#define IGMP_DEL_MAC_FILTER            0
#define IGMP_ADD_MAC_FILTER            1

/**
 * igmp group structure - there is
 * a list of groups for each interface
 * these should really be linked from the interface, but
 * if we keep them separate we will not affect the lwip original code
 * too much
 * 
 * There will be a group for the all systems group address but this 
 * will not run the state machine as it is used to kick off reports
 * from all the other groups
 */
struct igmp_group {
  /** next link */
  struct igmp_group *next;
  /** interface on which the group is active */
  struct netif      *nif;
  /** multicast address */
  ulong             group_address;
  /** signifies we were the last person to report */
  char              last_reporter_flag;
  /** current state of the group */
  char              group_state;
  /** timer for reporting, negative is OFF */
  short             timer;
  /** counter of simultaneous uses */
  char              use;
};

struct igmp_dgram
{
	char		type;		/* IGMP message type */
	char		maxresp;
	short		chksum;		/* checksum */
	long		group_address;
};


void	igmp_init (void);
long  igmp_joingroup(ulong ifaddr, ulong groupaddr, unsigned short ifindex);
long  igmp_leavegroup(ulong ifaddr, ulong groupaddr, unsigned short ifindex);
long	igmp_start(struct netif *nif);
long	igmp_stop(struct netif *nif);
void	igmp_report_groups(struct netif *nif);

# endif /* _igmp_h */
