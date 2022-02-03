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
 *	syntax:	slinkctl [-c] [-r] [-tN] <interface>
 *		. default: report statistics for <interface>
 *			-r	resets the device instead
 *			-c	clears the statistics counters instead
 *			-tN	sets the number of trace table entries to N instead
 *				(0 <= N <= 1000)
 *		. if any of the optional arguments is specified, statistics
 *		  are NOT reported.
 *		. output is to stdout
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
 *	v0.60	november/2007		rfb
 *	  .	retrieve MAC address for interface via new ioctl()
 *	  .	the option to output to a file has been dropped because:
 *		a) the bash shell can redirect stdout to a file if required
 *		b) the code seems to change the default directory behind bash's back
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <mintbind.h>
#include <string.h>				/* for memcpy(), memset(), strcmp() [builtin versions] */

#include <sockios.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <getopt.h>
#include <macros.h>

#include "scsilink.h"

#undef min
#define min(a, b)       ((a) < (b) ? (a) : (b))
#undef max
#define max(a, b)       ((a) > (b) ? (a) : (b))

#define PROGRAM		"slinkctl"
#define VERSION		"v0.60"

/*
 *	other definitions required for SLINKCTL
 */
#define ETH_HDR_LEN		(2*ETH_ALEN+2)

/*
 *	internal function prototypes
 */
static void display_statistics(SCSILINK_STATS *stats);
static void display_trace(SLINK_TRACE *trace_table,long trace_entries,long current_entry);
static void quit(const char *s);
static void usage(void);

static void display_hex(FILE *report,char *start,char *end);		/* trace-related */
static void display_ip_header(FILE *report,char *start,char *end);
static void display_packet(FILE *report,char *start,char *end);
static void display_trace_entry(SLINK_TRACE *t);
static char *format_macaddr(char *macaddr);

/*
 *	globals
 */
static SCSILINK_STATS *g_stats;
static int clear_counts = 0;
static int reset_device = 0;
static int g_trace_entries = -1;
static int opt_specified = 0;
#define g_report stdout	/* get rid of "initializer element is not constant" error */


/************************************
*									*
*		INITIALISATION ROUTINES		*
*									*
************************************/

int main(int argc,char **argv)
{
int c, sock, i, n;
long rc, stats_length, driver_version;
long rc1 = 0L, rc2 = 0L, rc3 = 0L;
char *p;
struct ifreq ifr;
SLINK_TRACE *t;

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
			g_trace_entries = atoi(optarg);
			if ((g_trace_entries < 0) || (g_trace_entries > MAX_TRACE_ENTRIES))
				quit("trace entries must be between 0 & 10000 inclusive");
			break;
		default:
			usage();
		}
	}

	if (optind >= argc)
		quit("you must specify an interface, e.g. en0");

	if (clear_counts || reset_device || (g_trace_entries >= 0))
		opt_specified = 1;

	sock = socket(PF_INET,SOCK_DGRAM,0);
	if (sock < 0)
		quit("cannot open socket: MintNet not loaded?");

	strcpy(ifr.ifr_name,argv[optind++]);

	printf("Processing log for interface %s\n",ifr.ifr_name);
	printf("--------------------------------\n");

	ifr.ifr_ifru.ifru_data = (char *)SL_GET_VERSION;
	driver_version = Fcntl(sock,(long)&ifr,SIOCGLNKFLAGS);
	if (driver_version >= 0L)
		printf("Driver version %d.%02d\n\n",(int)driver_version>>8,(int)driver_version&0xff);
	else printf("  Error: cannot determine driver version (rc=%ld)\n",driver_version);

	ifr.ifr_ifru.ifru_data = (char *)SL_GET_MACADDR;
	if ((rc=Fcntl(sock,(long)&ifr,SIOCGLNKFLAGS)) == 0L) {
		p = (char *)&ifr.ifr_ifru.ifru_data;
		printf("MAC address %02x:%02x:%02x:%02x:%02x:%02x\n\n",
				p[0],p[1],p[2],p[3],p[4],p[5]);
	} else printf("  Error: cannot retrieve MAC address (rc=%ld)\n",rc);

	if (clear_counts) {
		ifr.ifr_ifru.ifru_data = (char *)SL_CLEAR_COUNTS;
		rc1 = Fcntl(sock,(long)&ifr,SIOCGLNKFLAGS);
		if (rc1 == 0L)
			printf("Statistics have been cleared\n");
		else printf("  Error: cannot clear statistics (rc=%ld)\n",rc1);
	}

	if (reset_device) {
		ifr.ifr_ifru.ifru_data = (char *)SL_RESET_DEVICE;
		rc2 = Fcntl(sock,(long)&ifr,SIOCGLNKFLAGS);
		if (rc2 == 0L)
			printf("Device has been reset\n");
		else printf("  Error: cannot reset device (rc=%ld)\n",rc2);
	}

	if (g_trace_entries >= 0) {
		ifr.ifr_ifru.ifru_data = (char *)(SL_SET_TRACE | (g_trace_entries<<8));
		rc3 = Fcntl(sock,(long)&ifr,SIOCGLNKFLAGS);
		if (rc3 == 0L)
			printf("%d entries allocated for trace table\n",g_trace_entries);
		else printf("  Error: cannot allocate %d trace entries (rc=%ld)\n",g_trace_entries,rc3);
	}

	if (opt_specified)
	{
		rc2 = min(rc2,rc3);
		return min(rc1,rc2);
	}

	ifr.ifr_ifru.ifru_data = (char *)SL_STATS_LENGTH;
	rc = Fcntl(sock,(long)&ifr,SIOCGLNKFLAGS);
	if (rc < 0L) {
		printf("  Error: cannot determine size of statistics data (rc=%ld)\n",rc);
		return rc;
	}
	stats_length = rc;

	n = max(stats_length,sizeof(SCSILINK_STATS));	/* size of area for return data */
	g_stats = malloc(n);							/* malloc it */
	memset(g_stats,0,sizeof(SCSILINK_STATS));		/* & ensure basic stats are zeroed */

	ifr.ifr_ifru.ifru_data = (char *)g_stats;
	rc = Fcntl(sock,(long)&ifr,SIOCGLNKSTATS);
	if (rc != 0L) {
		printf("  Error: cannot get statistics data (rc=%ld)\n",rc);
		return rc;
	}
	if (g_stats->magic != STATS_MAGIC) {
		printf("  Error: wrong magic number for statistics data\n");
		return -1L;
	}

	printf("Statistics %s\n",ifr.ifr_name);
	printf("----------\n");
	display_statistics(g_stats);

	if (g_stats->trace_entries == 0L) {
		printf("Tracing is inactive\n\n");
		return 0L;
	}
	if (stats_length != sizeof(SCSILINK_STATS)+g_stats->trace_entries*sizeof(SLINK_TRACE)) {
		printf("  Error: wrong trace table length, trace table not dumped\n\n");
		return -1L;
	}

	for (i = 0, t = g_stats->trace_table; i < g_stats->trace_entries; i++, t++)
		if (t->type)
			break;
	if (i >= g_stats->trace_entries) {
		printf("Trace table is empty\n\n");
		return 0L;
	}

	printf("Trace table\n");
	printf("-----------\n");
	display_trace(g_stats->trace_table,g_stats->trace_entries,g_stats->current_entry);

	return 0L;
}

static void display_statistics(SCSILINK_STATS *stats)
{
	printf("  Input counts:\n");
	printf("    %7ld input calls\n",stats->count.input_calls);
	printf("    %7ld interface down\n",stats->count.input_ifdown);
	printf("    %7ld device active\n",stats->count.input_dev_active);
	printf("    %7ld device resetting\n",stats->count.input_dev_resetting);
	printf("    %7ld reads\n",stats->count.scsi_reads);
	printf("    %7ld reads failed\n",stats->count.read_fails);
	printf("    %7ld reads recovered\n",stats->count.read_fixups);
	printf("    %7ld zero reads\n",stats->count.zero_reads);
	printf("    %7ld negative flag\n",stats->count.negative_flag);
	printf("    %7ld invalid length\n",stats->count.invalid_length);
	printf("    %7ld receive errors\n",stats->count.receive_errors);
	printf("    %7ld packets received\n",stats->count.packets_received);

	printf("  Output counts:\n");
	printf("    %7ld output calls\n",stats->count.output_calls);
	printf("    %7ld interface down\n",stats->count.output_ifdown);
	printf("    %7ld device active\n",stats->count.output_dev_active);
	printf("    %7ld device resetting\n",stats->count.output_dev_resetting);
	printf("    %7ld write enqueues\n",stats->count.write_enqueues);
	printf("    %7ld write dequeues\n",stats->count.write_dequeues);
	printf("    %7ld writes\n",stats->count.scsi_writes);
	printf("    %7ld writes failed\n",stats->count.write_fails);
	printf("    %7ld writes recovered\n",stats->count.write_fixups);
	printf("    %7ld packets sent\n",stats->count.packets_sent);

	printf("  Device statistics:\n");
	printf("    %ld reads, %ld writes, %ld resets\n",
			stats->count.sl_count[0],stats->count.sl_count[1],stats->count.sl_count[2]);
	if (stats->count.sl_count[3]+stats->count.sl_count[4]+stats->count.sl_count[5]) {
		if (stats->count.sl_count[3])
			printf("    *** %ld type1 errors reported by SCSI/Link ***\n",stats->count.sl_count[3]);
		if (stats->count.sl_count[4])
			printf("    *** %ld type2 errors reported by SCSI/Link ***\n",stats->count.sl_count[4]);
		if (stats->count.sl_count[5])
			printf("    *** %ld type3 errors reported by SCSI/Link ***\n",stats->count.sl_count[5]);
	} else printf("    No errors reported by SCSI/Link\n");

	printf("\n");
}

static void display_trace(SLINK_TRACE *trace_table,long trace_entries,long current_entry)
{
SLINK_TRACE *t;
int i;

	printf("Size = %ld entries\n\n",trace_entries);

	for (i = current_entry, t = &trace_table[i]; i < trace_entries; i++, t++)
		if (t->type)
			display_trace_entry(t);

	for (i = 0, t = trace_table; i < current_entry; i++, t++)
		if (t->type)
			display_trace_entry(t);

	printf("\n");
}


/*
 *	utility routines
 */
static void quit(const char *s)
{
	if (s)
		fprintf(stderr,"%s: %s\n",PROGRAM,s);
	exit(-1);
}


static void usage(void)
{
	fprintf(stderr,"%s [-c] [-r] [-tN] <interface>\n",PROGRAM);
	fprintf(stderr,"   default: report statistics for <interface>\n");
	fprintf(stderr,"   -c   clear the <interface> statistics counters instead\n");
	fprintf(stderr,"   -r   reset the <interface> instead\n");
	fprintf(stderr,"   -tN  set the number of trace entries for <interface> instead\n");
	fprintf(stderr,"        (N must be between 0 & %d inclusive)\n",MAX_TRACE_ENTRIES);
	fprintf(stderr,"   output is to stdout\n");
	quit(NULL);
}

/*
 *	trace display routines (very similar to STinG version!)
 */
static void display_trace_entry(SLINK_TRACE *t)
{
char *end;

	end = t->data + min(t->length,SLINK_TRACE_LEN);

	fprintf(g_report,"%08lx %c %5ld %4d ",t->time,t->type,t->rc,t->length);

	if ((t->type == TRACE_READ_1) || (t->type == TRACE_READ_2)
	 || (t->type == TRACE_WRITE_1) || (t->type == TRACE_WRITE_2))
		display_packet(g_report,t->data,end);
	else display_hex(g_report,t->data,end);
}

static void display_packet(FILE *report,char *start,char *end)
{
char *p;
int i;

	fprintf(report," %s",format_macaddr(start));		/* ethernet header */
	fprintf(report," <- %s",format_macaddr(start+ETH_ALEN));
	i = *(short *)(start+2*ETH_ALEN);
	switch(i) {
	case 0x0800:
		p = "IP";
		break;
	case 0x0806:
		p = "ARP";
		break;
	default:
		p = "???";
	}
	fprintf(report," %04x (%s)",i,p);
	p = start + ETH_HDR_LEN;
	if (i == 0x0800) {
		display_ip_header(report,p,end);
		p += (*p&0x0f)*4;
	}

	display_hex(report,p,end);
}

static void display_ip_header(FILE *report,char *start,char *end)
{
char *p;
int len;

	len = (*start & 0x0f) * 4;
	if (start+len < end)
		end = start + len;

	fprintf(report,"\n        ");
	for (p = start; p < end; p++)
		fprintf(report," %02x",*p);
}

static void display_hex(FILE *report,char *start,char *end)
{
int i;
char *p;

	for (i = 0, p = start; p < end; i++) {
		if (i%32 == 0)
			fprintf(report,"\n        ");
		fprintf(report," %02x",*p++);
	}

	fprintf(report,"\n\n");
}

static char *format_macaddr(char *macaddr)
{
static char s[18];

	sprintf(s,"%02x:%02x:%02x:%02x:%02x:%02x",
			macaddr[0],macaddr[1],macaddr[2],macaddr[3],macaddr[4],macaddr[5]);
	return s;
}
