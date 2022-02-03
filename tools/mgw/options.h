/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 1999 Jens Heitmann
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * 
 * Author: Jens Heitmann
 * Started: 1999-05
 * 
 * please send suggestions or bug reports to me or
 * the MiNT mailing list
 * 
 */

# ifndef _options_h
# define _options_h

typedef struct options CFG_OPT;
struct options
{
	long dcfg;
	char provider[21];
	
	/* Connect options */
	char user[36];
	char password[36];
	char phone_no[21];
	char term_before;
	char term_after;
	
	/* Modem configuration */
	char modem_init[50];
	char modem_dial[50];
	char modem_hangup[50];
	char modem_cmdok[50];
	char modem_conmsg[50];
	char modem_fail1[50];
	char modem_fail2[50];
	
	/* RS232 settings */
	short port;
	short baud;
	short parity;
	short bits;
	short stopbits;
	
	/* Protocol options */
	long prot_cookie;
	short  protocol;
	char ip_compression;
	char soft_compression;  
	
	/* Gateway options */
	unsigned long nameserver1;
	unsigned long nameserver2;
	unsigned long target1, gateway1;
	unsigned long target2, gateway2;
	unsigned long target3, gateway3;
	unsigned long std_gateway;
	short subnet1, subnet2, subnet3;
	char default_gw;
	
	/* TCP options */
	char ip_assign;
	unsigned long ip;
	unsigned long subnet;	   
	char host[21];
	char domain[21];
	
	/* Proxys */
	char http[41];
	char http_port[5];
	char ftp[41];
	char ftp_port[5];
	char gopher[41];
	char gopher_port[5];
	char secure[41];
	char secure_port[5];
	char exclude[3][47];
	
	/* Mail */
	char email[43];
	char email2[43];
	char mailpass[43];
	char popuser[43];
	char popserver[43];
	char smtpserver[43];
	
	short  timeout;
	char netlogon, force_enc, tonline;
	
};

# endif /* _options_h */
