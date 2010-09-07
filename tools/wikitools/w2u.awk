# translate wiki to udo
# usage: awk -f w2u.awk [-v INF=<infile>]
#
#
# (C) helmut karlowski 2010
############################################################

function sub_specials()
{
	gsub(/\&lt;/,"<");
	gsub(/\&gt;/,">");
	gsub(/\&quot;/,"\"");
	gsub(/\&amp;/,"&");

	gsub(/<tt><pre>/,"\n!begin_sourcecode\n");
	gsub(/<\/pre><\/tt>/,"\n!end_sourcecode");
	gsub(/<span id=\"/,"!label ");
	gsub(/\"><\/span>/,"");

#!! quick hack: ordered-list -> !itemize	-> todo
	gsub(/<ol>/,"");		# <ol>
	gsub(/<\/ol>/,"");		# </ol>
	gsub(/<li value=\"[0-9]*\">/,"*");		# <li value="n">

#
	gsub(/<!--/,"");
	gsub(/-->/,"");

#	re = 0;
#	if( gsub(/<!--/,"#<!-- ") ){
#		html_comment = 1;
#		if( match( /-->/ ) )
#			html_comment = 0;
#	}
#	else if( gsub(/-->/,"") ){
#		html_comment = 0;
#	}
}

function exchange_bold()
{
	x = first_bold;
	first_bold = second_bold;
	second_bold = x;
}
function exchange_it()
{
	x = first_it;
	first_it = second_it;
	second_it = x;
}
# translate wiki-link-syntax into udo-link-syntax in $0
function do_links()
{

	sub( /^;/, "" );	#	-> todo!

	if( sub(/<\/text>/,"") ){
		done = 1;
	}

# some inlines
	while( sub( "'''", first_bold ) ){
		if( bold_ex ){
			exchange_bold();
			bold_ex  = 0;
		}
		else if( !sub( "'''", second_bold ) ){
			exchange_bold();
			bold_ex = 1;
		}
	}
	while( sub( "''", first_it ) ){
		if( it_ex ){
			exchange_it();
			it_ex  = 0;
		}
		else if( !sub( "''", second_it ) ){
			exchange_it();
			it_ex = 1;
		}
	}
	if( !index( $0, "[" ) )
		return;

###### copy $0 into array
	for( I2 = I = 1; I <= NF; I++ ){
		if( ( idx = index( $I, "]" ) ) && substr( $I, length($I) ) != "]" ){	# ] not at eow
			for( ; substr($I, idx, 1 ) == "]"; idx++ )
			;
			idx--;
#			printf( "I=%s idx=%d\n", $I, idx);
			s[1] = substr( $I, 1, idx );
			s[2] = substr( $I, idx+1, length( $I ) - idx );
#			printf( "I=%s idx=%d s1=%s s2=%s\n", $I, idx, s[1],s[2]);

			LA[I2++] = s[1];
			if( (idx = index( s[2], "[" )) && idx != 1 ){	# [ not at bow
				LA[I2++] = substr( s[2], 1, idx - 1 );
				LA[I2++] = substr( s[2], idx, length( s[2] ) - idx );
			}
			else
				LA[I2++] = s[2];
		}
		else
			LA[I2++] = $I;

		$I = "";
	}
######
	if( index( LA[1], "=" ) ){	#node
		is_node = 1;
		I = 2;
		$1 = PREAPP[LA[1]];	#"!node";
#		printf( "!node: %s(%s)\n", i, LA[1]);
		macname = "weblink ";
		$NF = "";	# delete ==
		I2--;
	}
	else{
		is_node = 0;
		I = 1;
		macname = "webnolink ";
	}

	for( O = I; I < I2; I++ ){
		M = 0;
		if( (N = index( LA[I], "[[" )) || (M = index( LA[I], "[http://" )) ){
			PRE = "";
			if( N ){
				EPAT = "]]";
				if( N > 1 )
					PRE = substr( LA[I], 1, N-1 );
				SUB = 3;
				WEBLINK = 0;
				LOCAL = 0;
			}
			else{
				WEBLINK = 1;
				EPAT = "]";
				if( M > 1 )
					PRE = substr( LA[I], 1, M-1 );
				SUB = 2;
				N = M;
			}

			S = substr( LA[I], N );

			if( !(N = index( S, EPAT )) ){
				if( WEBLINK ){
					URL = S;
					S = "";
				}
#	collect words
				for( K = I+1; K < I2; K++ ){
					if( S == "" )
						S = LA[K];
					else
						S = S " " LA[K];
					$K = "";
					if( N = index( LA[K], EPAT ) ){
						break;
					}
				}
				I = K;
				if( WEBLINK ){
					TEXT = S;
				}
			}

			if( !WEBLINK ){
				M = index( S, "|" );
				APP = "";
				if( M ){
					URL = substr( S, SUB, M - SUB );
					TEXT = substr( S, M + 1);

					APP = substr( TEXT, index(TEXT,EPAT) + 2);
					TEXT = substr( TEXT, 1, index(TEXT,EPAT) -1 );


					if( (ai = index( URL, "#" ))){
						if( substr( URL, 1, ai -1 ) == THIS){
							LOCAL = 1;
						}
					}
				}	# no |
				else{
					TEXT = substr( S, SUB);

					N = index(TEXT,EPAT);
#					printf( "TEXT=%s\n", TEXT);
					APP = substr( TEXT, N + 2);
					TEXT = substr( TEXT, 1, N - 1 );
					URL = TEXT;
#					printf("S=%s(%d) N=%d TEXT=%s SUB=%s\n", S, length(S), N, TEXT, SUB);
				}
			}

#			printf( "%d:link:URL:%s,TEXT=%s PRE=%s APP=%s\n", lnr, URL, TEXT, PRE, APP );

			gsub( " ", "_", URL );
#			gsub( ")", "!)", TEXT);
#			gsub( /\)/, "\\&#x29;", TEXT);
#			gsub( /\(/, "\\&#x28;", TEXT);

			gsub( /\)/, "", TEXT);
			gsub( /\(/, "", TEXT);

#			printf("APP=%s\n", APP);

# note: a blank is inserted after PRE and before APP!
			if( WEBLINK )
				$O = PRE "(!" macname URL "] [" TEXT ")" APP;
			else if( LOCAL == 1 )
				$O = TEXT;
			else
				$O = PRE "(!" macname "[" BASE "/" URL "] [" TEXT "])" APP;
			O++;

		}
		else if( LA[I] != "" ){
			$O= LA[I];
			O++;
		}
	}
}


function leave_env(lvl)
{

	for( l = envlvl; l > 0 && lvl--; l-- ){
		if( ID_ENV[l] == itemize )
			print "!end_itemize\n" >>OUTF;
		else if( ID_ENV[l] == description )
			print "!end_description\n" >>OUTF;
		else if( ID_ENV[l] == enumerate )
			print "!end_enumerate\n" >>OUTF;
		else if( ID_ENV[l] == quote )
			print "!end_quote\n" >>OUTF;
		else
			printf( "ID_ENV:%d undefined:l=%d\n", ID_ENV[l], l) >"/dev/stderr";
	}
	envlvl = l;

}

BEGIN{

	FS = " ";
	UDO_HEADER = "w2u.ui";

##########################

########################
# parse-arrays
# replace-pat at beginning+end of line

	PREAPP["="] = "!node";
	PREAPP["=="] = "!node";
	PREAPP["==="] = "!subnode";
	PREAPP["===="] = "!subsubnode";

# enums
	description = 1;
	itemize = 2;
	enumerate = 3;
	quote = 4;

	first_bold = "(!B)";
	second_bold = "(!b)";
	first_it = "(!I)";
	second_it = "(!i)";
##########################
##########################

	if( INF == "" ){
		INF = ARGV[1];
	}
	if( INF == "" ){
		INF = "/dev/stdin";
		OUTF = "/dev/stdout";
	}
	else{
		split( INF, a, "." );
		OUTF= a[1] ".wu";
		if( INF == OUTF ){
			printf( "w2u:ERROR: input = output:%s\n", INF ) >"/dev/stderr";
			exit( 1 );
		}
		printf( "output to: %s\n", OUTF );
	}

	printf("") >OUTF;

	THIS = INF;
	BASE = "base-unknown/";

# xml-header
# base and title needed for local links
	for( lnr = 1; getline <INF; lnr++ ){
		if( $1 == "<text" ){
			idx = index( $0, ">" );
			if( !idx ){
				fprintf( "%s:%d:wrong xml: %s\n", INF, lnr, $0 );
				break;
			}
# text may start on same line as xml ends
			for( i = 2; !(id = index( $i, ">" )) && i < NF; i++ )
			;

			$1 = substr( $i, id + 1 );
			if( $1 != "" )
				l = 2;
			else
				l = 1;
#			print $1;
			for( i++; i <= NF; l++ && i++ )
				$l = $i;
#			printf( "l=%d:%s i=%d:%s\n", l, $l, i, $i);
			for( i = l; i <= NF; i++ ){
#				print $i;
				$i = "";
			}

			NF = l;
			break;
		}

		if( substr( $1, 1, 1 ) != "<" )
			break;
		if( substr( $1, 1, 7 ) == "<title>" ){
			split( substr( $0, 8 ), a, "<" );
			THIS = substr( a[1], index(a[1], ">") + 1 );

			printf( "THIS=%s\n", THIS );
			printf( "#! title %s\n", THIS) >>OUTF;
		}
		if( substr( $1, 1, 6 ) == "<base>" ){
			split( substr( $1, 7 ), a, "<" );
			B = a[1];
			n = split( B, b, "/" );
			BASE = b[1];
			for( i = 2; i < n; i++ )
				BASE = BASE "/" b[i];
			printf( "BASE=%s\n", BASE );
			printf( "#! base %s\n", BASE) >>OUTF;
		}
	}

	D = $0;

	if( (r=getline <UDO_HEADER) < 0 ){
		print "cannot open header "UDO_HEADER;
		UDO_HEADER = ENVIRON["HOME"]"/wiki/w2u.ui";
	}
	else
		close(UDO_HEADER);
	print "reading header "UDO_HEADER ".. ";
	for( ln = 1; getline <UDO_HEADER > 0; ln++ ){
		print >> OUTF;
	}
	if( ln == 1 ){
		printf( "%s empty or not found - stop.\n", UDO_HEADER);
		exit(1);
	}
	printf( "!docinfo [title] %s\n", THIS ) >>OUTF;
	print "!docinfo [date]    (!today)" >>OUTF;
	print "!docinfo [author]  created by w2u from " INF >>OUTF;

	print "!begin_document" >>OUTF;
	print "!maketitle" >>OUTF;
	print "!tableofcontents" >>OUTF;

# start processing #

	envlvl = 0;
	found_node = 0;
	source = 0;	# leading blanks in wiki -> sourcecode
	html_comment = 0;
	r = 1;
	$0 = D;

#	print;
	done = 0;
	for( ; done == 0 && r > 0; (r=getline <INF) && lnr++ && (OLDNF = NF) ){
		if( match( $0, /[&<>]/ ) ){
			sub_specials();
		}
#		if( html_comment == 1 ){
#			continue;
#		}
		do_links();
		if( force_br == 1 ){
			printf( "\n" ) >>OUTF;
			force_br = 0;
		}

# how detect a leading ";" ??
#		split( $0, b, ";");
#		sub( ";", "XY" );
#		if( b[1] ~ /;/ ){
#			print "semicolon";
#		}

#	detect leading blank!
		split( $0, a, "" );
		if( a[1] ~ /^[ ]$/ ){	#!!!
#			printf( "blank:a1='%s':b1='%s':$0='%s'\n", a[1], b[1], $0);
			if( source == 0 ){
				printf( "!begin_sourcecode\n" )>>OUTF;
				source = 1;
			}
		}
		else{
			if( source == 1 ){
				printf( "\n!end_sourcecode\n" )>>OUTF;
				source = 0;
			}
			if( a[1] ~ /;/ ){	# definition list
				definition_list = 1;
			}
		}

		n = split( $1, a, "" );
		if( match( a[1], /[:=*#]/ ) ){
#			print "MATCH!:" a[1] ":"$0;

# !item
# todo: ";" <> ":"  -> !begin_description
			if( a[1] == "*" || a[1] == ":"  || a[1] == "#" ){
				found = 1;
				typ = a[1];
				for( ns = 1; ns <= n && (a[ns+1] == typ || a[ns+1] == typ); ns++ )
					;
				N = ns + 1;
				n = ns;

# !begin_description (bold is parsed extra) ########################
				if( typ == "*" && a[N] == "'" && a[N+1] == "'" && a[N+2] == "'"  || a[N] == "(" && a[N+1] == "!" && a[N+2] == "B" && a[N+3] == ")"  ){
					if( n > envlvl ){
						printf( "!begin_description\n" )>>OUTF;
						ID_ENV[n] = description;
					}
					else if( n < envlvl ){
						leave_env(envlvl-n);
					}
					envlvl = n;
					sub( /'''/, "", $1 );
					sub( /\(\!B\)/,"",$1 );
					S = substr( $1, N);
					if( sub( /'''/, "", S ) > 0 || sub( /\(\!b\)/,"",S ) > 0  ){
						printf( "!item [%s]", S ) >>OUTF;
						j = 2;
					}
					else{
						printf( "!item [%s", S ) >>OUTF;
						for( j = 2; j <= NF; j++ ){
							if( sub( /'''/, "", $j ) || sub( /\(\!b\)/,"",$j) ){
								printf( " %s]", $j ) >>OUTF;
								j++;
								break;
							}
							else{
								if( index( $j, "!" ) == 1 ){
									printf( "\n%s\n", $j ) >>OUTF;
								}
								else{
									printf( " %s", $j ) >>OUTF;
								}
							}
						}
					}
				}
###############################################################################
				else{
					if( n > envlvl ){
						if( typ == "*" ){
							printf( "!begin_itemize\n" )>>OUTF;
							ID_ENV[n] = itemize;
						}
						if( typ == "#" ){
							printf( "!begin_enumerate\n" )>>OUTF;
							ID_ENV[n] = enumerate;
						}
						if( typ == ":" ){
							printf( "!begin_quote\n" )>>OUTF;
							ID_ENV[n] = quote;
						}
					}
					else if( n < envlvl ){
						leave_env(envlvl-n);
					}
					envlvl = n;

					if( typ == "*" || typ == "#" ){
						S = substr($1,ns+1);
						printf( "!item %s", S ) >>OUTF;
					}
					j = 2;
				}
				for( ; j <= NF; j++ ){
					if( index( $j, "!" ) == 1 ){
						printf( "\n%s\n", $j ) >>OUTF;
					}
					else{
						printf( " %s", $j ) >>OUTF;
					}
				}
				printf( "\n" ) >>OUTF;
			}
			else{
				leave_env(envlvl);

# ![sub..]node
				if( a[1] == "=" ){
					for( i in PREAPP ){
						if( i == $1 ){
							found = 1;
							if( i == "=" && PREAPP["=="] != "!subnode" ){
								PREAPP["="] = "!node";
								PREAPP["=="] = "!subnode";
								PREAPP["==="] = "!subsubnode";
								printf( "%s:%d:WARNING: single '=': '%s' - '==' now:%s\n", INF, lnr, $0, PREAPP["=="]) > "/dev/stderr";
							}
							if( !nodesub && found_node == 0 && length(i) > 2 ){
								nodesub = length(i) - 2;
#								PREAPP["==="] = "!node";
#								PREAPP["====="] = "!subnode";
#								PREAPP["======"] = "!subsubnode";
								printf( "%s:%d:WARNING: subnode without node:%s)\n", INF, lnr, $0 ) >"/dev/stderr";
							}

							k = substr(i,1,length(i)-nodesub);

							if( found == 0 && OLDNF != 0 )
								printf( "\n" ) >>OUTF;
							printf( PREAPP[k] ) >>OUTF;
							for( j = 2; j < NF; j++ ){
								printf( " %s", $j ) >>OUTF;
							}
							force_br = 1;
							found_node = 1;
							break;
						}
					}
				}
			}
		}
		else{	# plain text
#			split( $0, a, "" );
			leave_env(envlvl);
			print >>OUTF;
			found = 0;
		}
	}
	if( source == 1 )
		printf( "\n!end_sourcecode\n" )>>OUTF;
	leave_env(envlvl);
	print "\n!end_document" >>OUTF;
}


