/*
 * Daynaport SCSI/Link driver for FreeMiNT.
 * GNU C conversion by Miro Kropacek, <miro.kropacek@gmail.com>
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * Copyright (c) 2007 Roger Burrows.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 *	slink.h: header for MiNTnet SCSI/Link driver
 */

/*
 *	additional driver statistics
 *	these are retrieved by SIOCGLNKSTATS & cleared by SIOCGLNKFLAGS
 */
#define MAX_TRACE_ENTRIES	1000
#define SLINK_TRACE_LEN		52
typedef struct {
	long time;
	long rc;
	char type;
#define TRACE_CLOSE		'C'
#define TRACE_IOCTL		'I'
#define TRACE_OPEN		'O'
#define TRACE_READ_1	'R'
#define TRACE_READ_2	'r'
#define TRACE_STATS		'S'
#define TRACE_WRITE_1	'W'
#define TRACE_WRITE_2	'w'
#define TRACE_RESET		'Z'
	char reserved;
	short length;
	char data[SLINK_TRACE_LEN];
} SLINK_TRACE;

struct scsilink_counts {
	long input_calls;
	long input_ifdown;
	long input_dev_active;			/* interference from ourselves */
	long input_dev_resetting;
	long scsi_reads;
	long read_fails;
	long read_fixups;
	long zero_reads;
	long negative_flag;
	long invalid_length;
	long receive_errors;
	long packets_received;
	long output_calls;
	long output_ifdown;
	long output_dev_active;			/* interference from ourselves */
	long output_dev_resetting;
	long write_enqueues;
	long write_dequeues;
	long scsi_writes;
	long write_fails;
	long write_fixups;
	long packets_sent;
	long sl_count[6];				/* from scsilink.lib & device itself */
};

typedef struct {				/* data returned by SIOCGLNKSTATS */
	long magic;						/* STATS_MAGIC */
	unsigned char major_version;	/* of driver */
	unsigned char minor_version;
	short reserved;
	struct scsilink_counts count;
	long trace_entries;				/* number of entries in trace table */
	long current_entry;
	SLINK_TRACE trace_table[0];		/* trace table (variable length) */
} SCSILINK_STATS;

#define STATS_MAGIC		0x18670701

/*
 *	special arg values for SIOCGLNKFLAGS
 *
 *	note that for SL_SET_TRACE, the number of entries is passed in the
 *	high-order 3 bytes; therefore, to avoid conflict, no other arg values
 *	are allowed to end in a zero byte
 */
#define SL_SET_TRACE	0x00000000L	/* set number of trace entries */
#define SL_GET_VERSION	0x00000001L	/* get driver version number */
#define SL_STATS_LENGTH	0x00000002L	/* get length of statistics+trace */
#define SL_CLEAR_COUNTS	0x00000003L	/* clear internal counts */
#define SL_GET_MACADDR	0x00000004L	/* get MAC address */
#define SL_RESET_DEVICE	0x000000ffL	/* reset device */
