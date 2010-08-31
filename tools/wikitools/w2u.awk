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
#	print;
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
# unfortunately udo inserts links into links (macros) :-(
function do_links()
{

	sub(/<\/text>/,"");

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
# copy $0 into array
	for( I = 1; I <= NF; I++ ){
		LA[I] = $I;
	}

	for( I = 1; I <= NF; I++ ){
#		printf( "do_links:%s,I=%d NF=%d\n", LA[I],I,NF );
		M = 0;
		if( (N = index( LA[I], "[[" )) || (M = index( LA[I], "[http://" )) ){
			if( N ){
				EPAT = "]]";
				if( N > 1 )
					PRE = substr( LA[I], 1, N-1 );
				else PRE = "";
				SUB = 3;
				WEBLINK = 0;
				LOCAL = 0;
			}
			else{
				WEBLINK = 1;
				EPAT = "]";
				SUB = 2;
				N = M;
			}

#			printf( "link:%s N=%d\n", LA[I], N );

			S = substr( LA[I], N );

			if( !(N = index( S, EPAT )) ){
				if( WEBLINK ){
					URL = S;
					S = "";
				}
#	collect words
				for( K = I+1; K <= NF; K++ ){
					if( S == "" )
						S = LA[K];
					else
						S = S " " LA[K];
					$K = "";
					if( N = index( LA[K], EPAT ) ){
						break;
					}
				}
				if( WEBLINK ){
					TEXT = S;
				}
			}

			if( !WEBLINK ){
				M = index( S, "|" );
				APP = "";
#			printf("S=%s LA[%d]=%s M=%d N=%d l=%d\n", S, I, LA[I], M, N,length(S));
				if( M ){
					URL = substr( S, SUB, M - SUB );
					TEXT = substr( S, M + 1);

					APP = substr( TEXT, index(TEXT,EPAT) + 2);
					TEXT = substr( TEXT, 1, index(TEXT,EPAT) -1 );
					if( (ai = index( URL, "#" ))){
						if( substr( URL, 1, ai -1 ) == THIS){
#							printf( "local link:%s:%s(%d)\n", URL, substr( URL, 1, ai -1 ), ai );
							LOCAL = 1;
						}
					}
				}	# no |
				else{
					TEXT = substr( S, SUB);
#				printf("S=%s(%d) N=%d TEXT=%s\n", S, length(S), N, TEXT);

					N = index(TEXT,EPAT);
					APP = substr( TEXT, N + 2);
					TEXT = substr( TEXT, 1, N - 1 );
					URL = TEXT;
#				printf("APP=%s\n", APP);
				}
			}

#			printf( "link:URL:%s,TEXT=%s PRE=%s APP=%s\n", URL, TEXT, PRE, APP );

			gsub( " ", "_", URL );

			if( WEBLINK )
				$I = PRE "\n(!weblink " URL "] [" TEXT ")" APP;
			else if( LOCAL == 1 )
				$I = TEXT;
			else
				$I = PRE "\n(!weblink [" BASE "/" URL "] [" TEXT "])" APP;

		}
	}
}


function leave_env(lvl)
{

#	printf( "leave_env:lvl=%d:%d\n", lvl, envlvl );
	for( l = envlvl; l > 0 && lvl--; l-- ){
#		printf( "ID_ENV[%d]=%d\n", l, ID_ENV[l] );
		if( ID_ENV[l] == itemize )
			print "!end_itemize\n" >>OUTF;
		else if( ID_ENV[l] == description )
			print "!end_description\n" >>OUTF;
		else
			printf( "ID_ENV:%d undefined:l=%d\n", ID_ENV[l], l) >"/dev/stderr";
	}
	envlvl = l;

}

BEGIN{

	UDO_HEADER = "w2u.ui";

##########################

########################
# parse-arrays
# replace-pat at beginning+end of line

	PREAPP["=="] = "!node";
	PREAPP["==="] = "!subnode";
	PREAPP["===="] = "!subsubnode";

# enums
	description = 1;
	itemize = 2;

	first_bold = "(!B)";
	second_bold = "(!b)";
	first_it = "(!I)";
	second_it = "(!i)";
##########################
##########################

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


# xml-header
# base and title needed for local links
	for( lnr = 1; getline <INF; lnr++ ){
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
#	printf( "%d:%s\n", lnr, $0);

	for( lnr = 1; getline <UDO_HEADER; lnr++ ){
		print >> OUTF;
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
	r = 1;
	$0 = D;
	for( lnr = 1; r > 0; (r=getline <INF) && lnr++ && (OLDNF = NF) ){
		if( match( $0, /[&<]/ ) ){
			sub_specials();
		}
		do_links();
		if( force_br == 1 ){
			printf( "\n" ) >>OUTF;
			force_br = 0;
		}
		n = split( $1, a, "" );
		if( match( a[1], /[=*]/ ) ){
#			printf( "wiki:%s\n", $0 );

# !item
			if( a[1] == "*" ){
				found = 1;
				for( ns = 1; ns <= n && a[ns+1] == "*"; ns++ )
					;
				N = ns + 1;
				n = ns;

#				printf("!item:%s %s N=%d ns=%d a=%s:%s\n", $1,$2,N, ns,a[n], a[N]);
# !begin_description (bold is parsed extra) ########################
				if( a[N] == "'" && a[N+1] == "'" && a[N+2] == "'"  || a[N] == "(" && a[N+1] == "!" && a[N+2] == "B" && a[N+3] == ")"  ){
#					printf( "desc:n=%d envlvl=%d\n", n, envlvl);
					if( n > envlvl ){
						printf( "!begin_description\n" )>>OUTF;
						ID_ENV[n] = description;
#						printf( "!begin_description: n=%d->%d\n", n, ID_ENV[n]);
					}
					else if( n < envlvl ){
#						printf( "!end_description\n" )>>OUTF;
						leave_env(envlvl-n);
					}
					envlvl = n;
					sub( /'''/, "", $1 );
					sub( /\(\!B\)/,"",$1 );
					S = substr( $1, N);
#					printf("S=%s\n",S);
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
						printf( "!begin_itemize\n" )>>OUTF;
						ID_ENV[n] = itemize;
#						printf( "!begin_itemize: n=%d->%d\n", n, ID_ENV[n]);
					}
					else if( n < envlvl ){
						leave_env(envlvl-n);
					}
					envlvl = n;

					S = substr($1,ns+1);
#					printf("item:%s S=%s ns=%d\n", $1, S, ns);
					printf( "!item %s", S ) >>OUTF;
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
#							printf( "PREAPP:%s(%d)\n", PREAPP[i],length(i));
							if( found_node == 0 && length(i) > 2 ){
								printf( "%s:%d:WARNING: subnode without node:%s\n", INF, lnr, $0 ) >"/dev/stderr";
								PREAPP["==="] = "!node";
								PREAPP["===="] = "!subnode";
								PREAPP["====="] = "!subsubnode";
							}
							if( found == 0 && OLDNF != 0 )
								printf( "\n" ) >>OUTF;
							printf( PREAPP[i] ) >>OUTF;
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
			leave_env(envlvl);
			print >>OUTF;
			found = 0;
		}
	}
	print "\n!end_document" >>OUTF;
}


