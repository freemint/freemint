/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

# ifndef _isofs_rrip_h
# define _isofs_rrip_h

/*
 *	Analyze function flag (similar to RR field bits)
 */
#define	ISO_SUSP_ATTR		0x0001
#define	ISO_SUSP_DEVICE		0x0002
#define	ISO_SUSP_SLINK		0x0004
#define	ISO_SUSP_ALTNAME	0x0008
#define	ISO_SUSP_CLINK		0x0010
#define	ISO_SUSP_PLINK		0x0020
#define	ISO_SUSP_RELDIR		0x0040
#define	ISO_SUSP_TSTAMP		0x0080
#define	ISO_SUSP_IDFLAG		0x0100
#define	ISO_SUSP_EXTREF		0x0200
#define	ISO_SUSP_CONT		0x0400
#define	ISO_SUSP_OFFSET		0x0800
#define	ISO_SUSP_STOP		0x1000
#define	ISO_SUSP_UNKNOWN	0x8000

typedef u_int32_t daddr_t;
typedef u_int32_t off_t;
typedef u_int32_t ino_t;

typedef struct {
	struct iso_node	*inop;
	long		fields;		/* interesting fields in this analysis */
	daddr_t		iso_ce_blk;	/* block of continuation area */
	off_t		iso_ce_off;	/* offset of continuation area */
	long		iso_ce_len;	/* length of continuation area */
	struct iso_super*super;		/* mount structure */
	ino_t		*inump;		/* inode number pointer */
	char		*outbuf;	/* name/symbolic link output area */
	ushort		*outlen;	/* length of above */
	ushort		maxlen;		/* maximum length of above */
	long		cont;		/* continuation of above */
} ISO_RRIP_ANALYZE;

long cd9660_rrip_analyze(struct iso_directory_record *isodir,
			 struct iso_node *inop, struct iso_super *super);

long cd9660_rrip_getname(struct iso_directory_record *isodir,
			 char *outbuf, ushort *outlen,
			 ino_t *inump, struct iso_super *super);

long cd9660_rrip_getsymname(struct iso_directory_record *isodir,
			    char *outbuf, ushort *outlen,
			    struct iso_super *super);

long cd9660_rrip_offset(struct iso_directory_record *isodir,
			struct iso_super *super);

# endif /* _isofs_rrip_h */
