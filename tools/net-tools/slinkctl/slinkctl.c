/*
 * Daynaport SCSI/Link control tool for FreeMiNT.
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
 *	slinkctl: user interface to unique SCSILINK.XIF functions
 *
 *	syntax:	slinkctl [-c] [-r] [-tN] <interface> [filename]
 *		. default: report statistics for <interface>
 *			-r	resets the device instead
 *			-c	clears the statistics counters instead
 *			-tN	sets the number of trace table entries to N instead
 *				(0 <= N <= 10000)
 *		. if any of the optional arguments is specified, statistics
 *		  are NOT reported.
 *		. output is to stdout, unless a filename is present, in which
 *		  case the report will be written to it instead
 *
 *	v0.10	february/2007	rfb
 *		original version, based on slnetx v0.48 (equivalent program for magxnet)
 *
 *	v0.15	february/2007	rfb
 *		update for new version of SCSILINK_STATS
 *
 *	v0.35	february/2007	rfb
 *		support trace data in SCSILINK_STATS
 *
 *	v0.40	march/2007		rfb
 *		support additional counts in SCSILINK_STATS
 *
 *	v0.50	march/2007		rfb
 *		handle new dynamic trace table size
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <mintbind.h>
#include <string.h>				/* for memcpy(), memset(), strcmp() [builtin versions] */

#include <sockios.h>
#include <net/if.h>
#include <getopt.h>
#include <macros.h>

#include "scsilink.h"


#define PROGRAM		"slinkctl"
#define VERSION		"v0.50"

/*
 *	other definitions required for SLINKCTL
 */
#define SOCKDEV		"u:\\dev\\socket"
#define O_RDWR		0x02

/*
 *	internal function prototypes
 */
static void display_statistics(SCSILINK_STATS *stats);
static void display_trace(struct trace_entry *trace_table,long trace_entries,long current_entry);
static void quit(char *s);
//static int socket(int domain,int type,int proto);
static void usage(void);

/*
 *	globals
 */
SCSILINK_STATS *stats;
int clear_counts = 0;
int reset_device = 0;
int trace_entries = -1;
int opt_specified = 0;
FILE *report = NULL;


/************************************
*									*
*		INITIALISATION ROUTINES		*
*									*
************************************/

int main(int argc,char **argv)
{
int c, sock, i;
long rc, stats_length, driver_version;
long rc1 = 0L, rc2 = 0L, rc3 = 0L;
struct ifreq ifr;
struct trace_entry *t;

	fprintf(stderr,"%s %s: Copyright 2007 by Roger Burrows, Anodyne Software\n\n",PROGRAM,VERSION);

	while((c=getopt(argc,argv,"crt:")) != -1) {
		switch(c) {
		case 'c':
			clear_counts++;
			break;
		case 'r':
			reset_device++;
			break;
		case 't':
			trace_entries = atoi(optarg);
			if ((trace_entries < 0) || (trace_entries > MAX_TRACE_ENTRIES))
				quit("trace entries must be between 0 & 10000 inclusive");
			break;
		default:
			usage();
		}
	}

	if (optind >= argc)
		quit("you must specify an interface");

	if (clear_counts || reset_device || (trace_entries >= 0))
		opt_specified = 1;

	sock = socket(PF_INET,SOCK_DGRAM,0);
	if (sock < 0)
		quit("cannot open socket: MintNet not loaded?");

	strcpy(ifr.ifr_name,argv[optind++]);

	if (optind < argc)
		report = fopen(argv[optind],"wa");

	if (!report)
		report = stdout;

	fprintf(report,"Processing log for interface %s\n",ifr.ifr_name);
	fprintf(report,"--------------------------------\n");

	ifr.ifr_ifru.ifru_data = (char *)SL_GET_VERSION;
	driver_version = Fcntl(sock,(long)&ifr,SIOCGLNKFLAGS);
	if (driver_version >= 0L)
		fprintf(report,"Driver version %d.%02d\n\n",(int)driver_version>>8,(int)driver_version&0xff);
	else fprintf(report,"  Error: cannot determine driver version (rc=%ld)\n",driver_version);

	if (clear_counts) {
		ifr.ifr_ifru.ifru_data = (char *)SL_CLEAR_COUNTS;
		rc1 = Fcntl(sock,(long)&ifr,SIOCGLNKFLAGS);
		if (rc1 == 0L)
			fprintf(report,"Statistics have been cleared\n");
		else fprintf(report,"  Error: cannot clear statistics (rc=%ld)\n",rc1);
	}

	if (reset_device) {
		ifr.ifr_ifru.ifru_data = (char *)SL_RESET_DEVICE;
		rc2 = Fcntl(sock,(long)&ifr,SIOCGLNKFLAGS);
		if (rc2 == 0L)
			fprintf(report,"Device has been reset\n");
		else fprintf(report,"  Error: cannot reset device (rc=%ld)\n",rc2);
	}

	if (trace_entries >= 0) {
		ifr.ifr_ifru.ifru_data = (char *)(SL_SET_TRACE | (trace_entries<<8));
		rc3 = Fcntl(sock,(long)&ifr,SIOCGLNKFLAGS);
		if (rc3 == 0L)
			fprintf(report,"%d entries allocated for trace table\n",trace_entries);
		else fprintf(report,"  Error: cannot allocate %d trace entries (rc=%ld)\n",trace_entries,rc3);
	}

	if (opt_specified)
	{
		rc2 = min(rc2,rc3);
		return min(rc1,rc2);
	}

	ifr.ifr_ifru.ifru_data = (char *)SL_STATS_LENGTH;
	rc = Fcntl(sock,(long)&ifr,SIOCGLNKFLAGS);
	if (rc < 0L) {
		fprintf(report,"  Error: cannot determine size of statistics data (rc=%ld)\n",rc);
		return rc;
	}
	stats_length = rc;

	stats = malloc(min(stats_length,sizeof(SCSILINK_STATS)));	/* malloc area for statistics */
	memset(stats,0x00,sizeof(SCSILINK_STATS));					/* & ensure basic stats are zeroed */

	ifr.ifr_ifru.ifru_data = (char *)stats;
	rc = Fcntl(sock,(long)&ifr,SIOCGLNKSTATS);
	if (rc != 0L) {
		fprintf(report,"  Error: cannot get statistics data (rc=%ld)\n",rc);
		return rc;
	}
	if (stats->magic != STATS_MAGIC) {
		fprintf(report,"  Error: wrong magic number for statistics data\n");
		return -1L;
	}

	fprintf(report,"Statistics\n");
	fprintf(report,"----------\n");
	display_statistics(stats);

	if (stats->trace_entries == 0L) {
		fprintf(report,"Tracing is inactive\n\n");
		return 0L;
	}
	if (stats_length != sizeof(SCSILINK_STATS)+stats->trace_entries*sizeof(struct trace_entry)) {
		fprintf(report,"  Error: wrong trace table length, trace table not dumped\n\n");
		return -1L;
	}

	for (i = 0, t = stats->trace_table; i < stats->trace_entries; i++, t++)
		if (t->type)
			break;
	if (i >= stats->trace_entries) {
		fprintf(report,"Trace table is empty\n\n");
		return 0L;
	}

	fprintf(report,"Trace table\n");
	fprintf(report,"-----------\n");
	display_trace(stats->trace_table,stats->trace_entries,stats->current_entry);

	return 0L;
}

static void display_statistics(SCSILINK_STATS *statistics)
{
	fprintf(report,"  Input counts:\n");
	fprintf(report,"    %7ld input calls\n",statistics->count.input_calls);
	fprintf(report,"    %7ld interface down\n",statistics->count.input_ifdown);
	fprintf(report,"    %7ld device active\n",statistics->count.input_dev_active);
	fprintf(report,"    %7ld device resetting\n",statistics->count.input_dev_resetting);
	fprintf(report,"    %7ld reads\n",statistics->count.scsi_reads);
	fprintf(report,"    %7ld reads failed\n",statistics->count.read_fails);
	fprintf(report,"    %7ld reads recovered\n",statistics->count.read_fixups);
	fprintf(report,"    %7ld zero reads\n",statistics->count.zero_reads);
	fprintf(report,"    %7ld negative flag\n",statistics->count.negative_flag);
	fprintf(report,"    %7ld invalid length\n",statistics->count.invalid_length);
	fprintf(report,"    %7ld receive errors\n",statistics->count.receive_errors);
	fprintf(report,"    %7ld packets received\n",statistics->count.packets_received);

	fprintf(report,"  Output counts:\n");
	fprintf(report,"    %7ld output calls\n",statistics->count.output_calls);
	fprintf(report,"    %7ld interface down\n",statistics->count.output_ifdown);
	fprintf(report,"    %7ld device active\n",statistics->count.output_dev_active);
	fprintf(report,"    %7ld device resetting\n",statistics->count.output_dev_resetting);
	fprintf(report,"    %7ld write enqueues\n",statistics->count.write_enqueues);
	fprintf(report,"    %7ld write dequeues\n",statistics->count.write_dequeues);
	fprintf(report,"    %7ld writes\n",statistics->count.scsi_writes);
	fprintf(report,"    %7ld writes failed\n",statistics->count.write_fails);
	fprintf(report,"    %7ld writes recovered\n",statistics->count.write_fixups);
	fprintf(report,"    %7ld packets sent\n",statistics->count.packets_sent);

	fprintf(report,"  Device statistics:\n");
	fprintf(report,"    %ld reads, %ld writes, %ld resets\n",
			statistics->count.sl_count[0],statistics->count.sl_count[1],statistics->count.sl_count[2]);
	if (statistics->count.sl_count[3]+statistics->count.sl_count[4]+statistics->count.sl_count[5]) {
		if (statistics->count.sl_count[3])
			fprintf(report,"    *** %ld type1 errors reported by SCSI/Link ***\n",statistics->count.sl_count[3]);
		if (statistics->count.sl_count[4])
			fprintf(report,"    *** %ld type2 errors reported by SCSI/Link ***\n",statistics->count.sl_count[4]);
		if (statistics->count.sl_count[5])
			fprintf(report,"    *** %ld type3 errors reported by SCSI/Link ***\n",statistics->count.sl_count[5]);
	} else fprintf(report,"    No errors reported by SCSI/Link\n");

	fprintf(report,"\n");
}

static void display_trace(struct trace_entry *trace_table,long traceentries,long current_entry)
{
struct trace_entry *t;
int i;

	fprintf(report,"  Trace table size = %ld entries\n\n",traceentries);

	fprintf(report,"    ---Ticks--- Typ -Len- Retcd -uid- -euid-\n");

	for (i = current_entry, t = &trace_table[i]; i < traceentries; i++, t++)
		if (t->type)
			fprintf(report,"    %11ld  %c  %5d %5ld %5d %5d\n",t->ticks,t->type,t->len,t->rc,t->uid,t->euid);

	for (i = 0, t = trace_table; i < current_entry; i++, t++)
		if (t->type)
			fprintf(report,"    %11ld  %c  %5d %5ld %5d %5d\n",t->ticks,t->type,t->len,t->rc,t->uid,t->euid);

	fprintf(report,"\n");
}

/*
 *	utility routines
 */
static void quit(char *s)
{
	if (s)
		fprintf(stderr,"%s: %s\n",PROGRAM,s);
	exit(-1);
}

/*
 *	the following function is stolen from lib/socket.c
 */
/*
static int socket(int domain,int type,int proto)
{
struct socket_cmd cmd;
short sockfd;
long rc;

	rc = Fopen(SOCKDEV,O_RDWR);
	if (rc < 0L)
		return -1;
	sockfd = (short)rc;

	cmd.cmd = SOCKET_CMD;
	cmd.domain = domain;
	cmd.type = type;
	cmd.protocol = proto;

	rc = Fcntl(sockfd,(long)&cmd,SOCKETCALL);
	if (rc < 0L) {
		Fclose(sockfd);
		return -1;
	}

	return sockfd;
}
*/

static void usage()
{
	fprintf(stderr,"%s [-c] [-r] [-tN] <interface> [filename]\n",PROGRAM);
	fprintf(stderr,"   default: report statistics for <interface>\n");
	fprintf(stderr,"   -c   clear the <interface> statistics counters instead\n");
	fprintf(stderr,"   -r   reset the <interface> instead\n");
	fprintf(stderr,"   -tN  set the number of trace entries for <interface> instead\n");
	fprintf(stderr,"        (N must be between 0 & %d inclusive)\n",MAX_TRACE_ENTRIES);
	fprintf(stderr,"   output is to stdout, unless a filename is present, in\n");
	fprintf(stderr,"   which case all output will be written to it instead\n");
	quit(NULL);
}
