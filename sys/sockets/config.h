/*
 *	Global configuration file for MiNT-Net.
 *
 *	10/26/93, kay roemer
 */

# ifndef _sockets_config_h
# define _sockets_config_h


/*
 * PART 1: common stuff
 */

/*
 * use the new way for Fselect()
 */
# define NEW_SELECT


/*
 * PART 2: AF_UNIX specific stuff
 */

/*
 * Use unix file name lookup cache. This speeds up on filesystems where
 * Fxattr() works for XATTR.index :-) and slows down on filesystems where
 * Fxattr() doesn't work for XATTR.index :-(.
 * That means:
 * - undefine if your unix sockets reside on TosFs
 * - define if your unix sockets reside on MinixFs
 */
# define USE_UN_LOOKUP_CACHE


/*
 * PART 3: AF_INET specific stuff
 */

/*
 * Define this if TCP should use delayed acks to combine many ack segments
 * and window updates in fewer segments.
 */
# define USE_DELAYED_ACKS

/*
 * Define this if TCP should use `Nagle' algorithm to combine
 * many small segments into few larger ones.
 */
# define USE_NAGLE

/*
 * Defines this if TCP should detect duplicate acks and use them
 * to speed up retransmissions of lost segments.
 */
# define USE_DUPLICATE_ACKS

/*
 * Define this if TCP should detect whether segments were dropped
 * (and clear backoff in this case) or rtt was too small (and keep
 * backoff in this case).
 */
# define USE_DROPPED_SEGMENT_DETECTION


# endif /* _sockets_config_h */
