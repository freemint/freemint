# translate udo to wiki
# usage: awk -f u2w.awk [-v INF=<infile>] [-v TITLE=<title>]
#
# known commands see below
#
# (C) helmut karlowski 2010
############################################################

# search for labels and nodes in every line to provide links in the output
# this may slow things down a bit
function is_node(L)
{
	for( I in NODES )
		sub( I, NODES[I], L );
	return L;
}


BEGIN{

##########################
# constants (enum)

	begin_itemize = 1;
	end_itemize = 2;

	begin_description = 4;
	end_description = 5;

	begin_quote = 6;
	end_quote = 7;

	item = 11;

	docinfo = 111;

	label = 200;

	begin_exclude = 333;
	end_exclude = 334;

	weblink = 1000;
	maillink = 1001;
	wikilink = 1002;

########################
# parse-arrays
# replace-pat at beginning+end of line

	PREAPP["!node"] = "==";
	PREAPP["!subnode"] = "===";
	PREAPP["!subsubnode"] = "====";


# replace-pat at beginning of line

	PRE["!begin_sourcecode"] = "<tt><pre>";
	PRE["!end_sourcecode"] = "</pre></tt>\n";


# environment+commands with args

	COMMAND["!begin_itemize"] = begin_itemize;
	COMMAND["!end_itemize"] = end_itemize;
	COMMAND["!begin_description"] = begin_description;
	COMMAND["!end_description"] = end_description;
	COMMAND["!begin_quote"] = begin_quote;
	COMMAND["!end_quote"] = end_quote;
	COMMAND["!item"] = item;
	COMMAND["!docinfo"] = docinfo;

	COMMAND["!label"] = label;


# macros

	MACRO["(!weblink"] = weblink;
	MACRO["(!maillink"] = maillink;
	MACRO["(!wikilink"] = wikilink;


# wiki-special: #!wik com

	WIKI["begin_exclude"] = begin_exclude;
	WIKI["end_exclude"] = end_exclude;


# ignored commands

	IGNORED["!language"] = 1;
	IGNORED["!ifdest"] = 1;
	IGNORED["!if"] = 1;
	IGNORED["!else"] = 1;
	IGNORED["!endif"] = 1;
	IGNORED["!define"] = 1;
	IGNORED["!macro"] = 1;
	IGNORED["!no_preamble"] = 1;
	IGNORED["!use_nodes_inside_index"] = 1;
	IGNORED["!use_alias_inside_index"] = 1;
	IGNORED["!use_label_inside_index"] = 1;
	IGNORED["!no_headlines"] = 1;
	IGNORED["!html_merge_nodes"] = 1;
	IGNORED["!html_merge_subnodes"] = 1;
	IGNORED["!html_merge_subsubnodes"] = 1;
	IGNORED["!tex_tetex"] = 1;
	IGNORED["!tex_2e"] = 1;
	IGNORED["!no_effects"] = 1;
	IGNORED["!use_justification"] = 1;
	IGNORED["!maketitle"] = 1;
	IGNORED["!tableofcontents"] = 1;
	IGNORED["!begin_document"] = 1;
	IGNORED["!end_document"] = 1;

	IGNORED_DI["[program]"] = 1;
	IGNORED_DI["[date]"] = 1;
	IGNORED_DI["[author]"] = 1;



##########################
##########################



# start processing #
	if( INF == "" ){
		INF = "/dev/stdin";
		OUTF = "/dev/stdout";
	}
	else{
# read nodes (for local links)
		split( INF, a, "." );
		OUTF= a[1] ".wik";
		NODF = INF".n";

		if( INF == OUTF ){
			printf( "u2w:ERROR: input = output:%s\n", INF ) >"/dev/stderr";
			exit( 1 );
		}

		printf( "output to: %s\n", OUTF ) >"/dev/stderr";

		print "reading nodes " NODF  >"/dev/stderr";
		for( lnr = 1; getline <NODF > 0; lnr++ ){
			N = $2;
			for( i = 3; i <= NF; i++ )
				N = N " " $i;
			if( TITLE != "" )
				NODES[N] = "[[" TITLE "#" N "|" N "]]";
			else
				NODES[N] = "[[" N "]]";
		}
	}

	close(NODF);
	printf("") >OUTF;

	print "reading "INF >"/dev/stderr";

# base and title needed for local- and wiki- links
	for( lnr = 1; ((r=getline <INF) > 0) && lnr < 3; lnr++ ){
#		printf( "%d:%s,r=%d\n",lnr,$0,r) >"/dev/stderr";
		if( $1 != "#!" )
			break;
		if( $2 == "base" ){
			BASE = $3;
			printf( "BASE=%s\n", BASE) >"/dev/stderr";
			printf( "<base>%s</base>\n", BASE) >>OUTF;
		}
		if( $2 == "title" ){
			TITLE = "";
			for( i = 3; i <= NF; i++ )
				TITLE = TITLE " " $i;
			printf( "TITLE=%s\n", TITLE) >>"/dev/stderr";
			printf( "<title>%s</title>\n", TITLE) >>OUTF;
		}
	}

	print "header done" >"/dev/stderr";
	if( "$0" == "" )
		r = getline < INF;
	else r = 1;
	print;
	for(  ; r; (r=getline <INF ) > 0 && lnr++ && (OLDNF = NF) ){

# inlines
		gsub( /\(\!B\)/, "'''" );
		gsub( /\(\!b\)/, "'''" );
		gsub( /\(\!I\)/, "''" );
		gsub( /\(\!i\)/, "''" );
#
		split( $1, a, "" );
		if( a[1] == "#" ){
			if( match( $1, /#\!wik/ ) ){
				for( i in WIKI )
					if( $2 == i ){
						if( WIKI[i] == begin_exclude ){
							if( OLDNF > 0 )
								printf( "\n" ) >>OUTF;
							printf( "<!--%s\n", $0 ) >>OUTF;
							exclude++;
						}
						else if( WIKI[i] == end_exclude ){
							if( exclude > 0 ){
								exclude--;
								if( exclude == 0 ){
									if( OLDNF > 0 )
										printf( "\n" ) >>OUTF;
									printf( "%s-->\n", $0 ) >>OUTF;
								}
							}
							else
								printf( "%d:end_exclude  without begin_exclude \n", lnr ) >"/dev/stderr";
						}
					}
			}
			continue;	#comment ignored
		}
		if( a[1] == "!" ){

			found = 0;
			for( i in IGNORED ){
				if( i == $1 ){
					found = 1;
					break;
				}
			}
			if( found )
				continue;

			found = 0;
			for( i in PREAPP ){
				if( i == $1 ){
					found = 1;
					for( j = 0; j < length(PREAPP[i]); j++ )
						printf("\n")>>OUTF;
					printf( PREAPP[i] ) >>OUTF;
					for( j = 2; j <= NF; j++ ){
						printf( " %s", $j ) >>OUTF;
					}
					printf( " %s", PREAPP[i] ) >>OUTF;
					force_br = 1;
					break;
				}
			}
			if( found == 0 ){
				for( i in COMMAND ){
					if( i == $1 ){
						found = 1;
						if( COMMAND[i] == item ){
							printf( "\n" ) >>OUTF;
							for( j = 0; j < nstars; j++ )
								printf( "*" ) >>OUTF;

							j = 2;
							n = split( $2, b, "[" );
#							printf( "%d:item:%s,n=%d(on=%d off=%d)\n", lnr, $2, n, bold_on, bold_off);
							if( n > 1 ){
								split( $0, b, "[" );
								split( b[2], c, "]" );
								printf( "'''%s''' ", c[1] ) >>OUTF;
								j = split( c[1], d, " ") + 2;
#								printf( "%d:BOLD: %s->%d\n", lnr, c[1], j ) >"/dev/stderr";
							}

							for( S = ""; j <= NF; j++ ){
								S = S " " $j;
							}
							S = is_node(S);
							printf( "%s ", S ) >>OUTF;

							is_item = 2;
							break;
						}
						else if( COMMAND[i] == begin_description ){
							nstars++;
							bold_on++;
							break;
						}
						else if( COMMAND[i] == begin_itemize ){
							nstars++;
							bold_off = bold_on + 1;
							break;
						}
						else if( COMMAND[i] == end_description ){
							nstars--;
							bold_on--;
							break;
						}
						else if( COMMAND[i] == end_itemize ){
							nstars--;
							bold_off--;
							break;
						}
						else if( COMMAND[i] == begin_quote ){
							ncolons++;
							break;
						}
						else if( COMMAND[i] == end_quote ){
							ncolons--;
							break;
						}
						else if( COMMAND[i] == label ){
							if( OLDNF == 0 )
								printf("\n") >>OUTF;
							printf( "<span id=\"%s\"></span>", $2) >>OUTF;
							break;
						}
						else if( COMMAND[i] == docinfo ){
							for( i in IGNORED_DI ){
								if( i == $2 ){
									found = 1;
									break;
								}
							}
							if( found == 0 ){
								if( $2 == "[title]" ){
									printf( "=" ) >>OUTF;
									for( j = 3; j <= NF; j++ ){
										printf( " %s", $j ) >>OUTF;
									}
									printf( " =" ) >>OUTF;
									printf( "\n" ) >>OUTF;
								}
								else
									printf( "%s:%d:unknown docinfo:%s(ignored)\n", INF, lnr, $1) >"/dev/stderr";
								break;
							}
						}
					}
				}
			}
			if( found == 0 )
				for( i in PRE ){
					if( i == $1 ){
						found = 1;
						printf( PRE[i] ) >>OUTF;
						for( j = 2; j <= NF; j++ ){
							printf( " %s", $j ) >>OUTF;
						}
						break;
					}
				}

			if( !found )
				printf( "%s:%d:unknown command:%s(ignored)\n", INF, lnr, $1) >"/dev/stderr";
		}
		else if( a[1] == "(" && a[2] == "!" ){
# macros
			found = 0;
			for( i in MACRO ){
				if( i == $1 ){
					found = 1;
					split( $0, b, "[" );
					split( b[2], c1, "]" );
					split( b[3], c2, "]" );
					if( MACRO[i] == weblink )
						printf( "[%s %s] ", c2[1], c1[1] ) >>OUTF;
					else if( MACRO[i] == wikilink )
						printf( "[[%s]] ", c1[1] ) >>OUTF;
					else
						printf( "[%s %s] ", c2[1], c1[1] ) >>OUTF;
					n = split( $0, a, ")" );
					for( j = 2; j <= n; j++ )
						printf( "%s", a[j] ) >>OUTF;
					printf( "\n" ) >>OUTF;
					OLDNF = 1;
					break;
				}
			}
			if( !found )
				printf( "%s:%d:unknown macro:%s(ignored)\n", INF, lnr, $1) >"/dev/stderr";
		}
		else{	# plain text

			if( NF == 0 ){
				if( nstars == 0 )
					printf( "\n" ) >>OUTF;
			}
			else{
				if( force_br == 1 ){
					printf( "\n" ) >>OUTF;
					force_br = 0;
				}
				for( i = 0; i < ncolons; i++ )
					printf( ":" ) >>OUTF;
				$0 = is_node($0);
				for( j = 1; j <= NF; j++ ){
					printf( "%s", $j ) >>OUTF;
					if( j < NF || nstars > 0 )
						printf( " " ) >>OUTF;
				}
				if( nstars == 0 )
					printf( "\n" ) >>OUTF;
			}
		}
	}
	if( lnr == 1 )
		printf( "%s: not found (output empty)\n", INF) > "/dev/stderr";
}


