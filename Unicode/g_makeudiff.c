/* g_makeudiff.c, v0.1 : Copyright (C) 2018 by Jose Da Silva
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see http://www.gnu.org/licenses/
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BUFFSIZE 1024

typedef struct {
    long first,last,count;
    long val[0x120000];
} tab_codes;
tab_codes tabcodes[11];
char *tab_name[0x120000];

/* if ver=0, then get latest Unicodedata.txt for main reference list, */
/* else get codes from older Unicodedata.txt to compare to main list. */
static int get_UnicodeData(char *filenamein, char **tabname, tab_codes *tabcode, int ver, int v) {
    FILE *infile;
    long i, count, lineno, firstu, lastu, unicode;
    char buffer[BUFFSIZE+1], *p, *end;
    long val[0x120000];

    firstu = 10000000; lastu = -1; count = lineno = 0;
    buffer[BUFFSIZE]='\0';
    count = 0;
    for ( i=0; i<0x120000; ++i ) val[i] = -1;

    if ( (infile=fopen(filenamein,"rb"))==NULL ) {
	fprintf( stderr, "Can't find or read file %s\n", filenamein );
	return( -1 );
    }

    if ( v ) fprintf( stdout, "Read %s for latest Unicode values and names\n", filenamein );

    while ( fgets(buffer,BUFFSIZE,infile)!=NULL ) {
	++lineno;
	if ( (i=strlen(buffer))>=BUFFSIZE ) {
	    fprintf( stderr, "Error with file %s. Found line %d too long:\n%s\nMax allowed is string_len=%d\n", filenamein, lineno, buffer, BUFFSIZE );
	    fclose(infile);
	    return( -2 );
	}
	if ( strlen(buffer)<6 || buffer[0]=='#' || buffer[0]==' ' )
	    continue;
	/* get unicode value */
	unicode = strtol(buffer,&end,16);
	if ( unicode < 0 || unicode >= 0x120000 ) {
	    fprintf( stderr, "Error with file %s. Found line %d with unicode value out of range=0x%lx in string: %s\n", filenamein, lineno, unicode, buffer );
	    fclose(infile);
	    return( -3 );
	}
	/* expect to find ';' followed by a name string */
	if ( *end!=';' || *++end=='\0' || *end==';' ) {
	    fprintf( stderr, "Error with file %s. Expected line %d to have ';' followed by a name_string. Found:%s\n", filenamein, lineno, buffer );
	    fclose(infile);
	    return( -4 );
	}
	p = end;
	while ( *end!='\0' && *end!=';') ++end;
	*end = '\0';

	if ( ver ) {
	    /* ver!=0, compare these codes with the current UnicodeData.txt names list */
	    if ( val[unicode]!=-1 ) {
		fprintf( stderr, "Error with file %s. Found line %d to point to an already in use unicode 0x%lx listed earlier\n", filenamein, lineno, unicode );
		fclose(infile);
		return( -7 );
	    }

	    if ( tabname[unicode]!=NULL && strcmp(p,tabname[unicode])==0 ) {
		val[unicode] = unicode;
	    } else {
		for ( i=0; i<0x120000; ++i ) {
		    if ( tabname[i]!=NULL && strcmp(p,tabname[i])==0 ) {
			val[unicode] = i;
			if ( v>1 ) fprintf( stdout, "mismatched 0x%lx points to value 0x%lx\n", unicode, i );
		    }
		}
	    }
	} else {
	    /* ver==0, load initial UnicodeData.txt list into names for comparing afterwards */
	    if ( tabname[unicode]!=NULL ) {
		fprintf( stderr, "Error with file %s. Found line %d to have another copy of unicode 0x%lx followed by another name_string\nold=%s\nnew=%s\n", filenamein, lineno, unicode, tabname[unicode], p );
		fclose(infile);
		return( -5 );
	    }
	    if ( (tabname[unicode]=calloc(1,strlen(p)+1))==NULL ) {
		fprintf( stderr, "Error. No more memory.\n" );
		fclose(infile);
		return( -6 );
	    }
	    strcpy(tabname[unicode],p);
	}

	if ( v>2 ) fprintf( stdout, "value 0x%lx:%s\n", unicode, p );

	/* stats */
	++count;
	if ( unicode<firstu )
	    firstu = unicode;
	else if ( unicode>lastu )
	    lastu = unicode;
    }
    fclose(infile);

    if ( v ) fprintf( stdout, "Done %s, first code=0x%lx, last code=0x%lx, total count=%d\n", filenamein, firstu, lastu, count );

    /* if done safely then transfer temp into tabcodes as new values else maintain old codes */
    if ( ver ) {
	tabcode[ver].first = firstu;
	tabcode[ver].last = lastu;
	tabcode[ver].count = count;
	for ( i=0; i<0x120000; ++i ) tabcode[ver].val[i] = val[i];
    }
    return( 0 );
}

int main(int argc, char **argv) {
    int e, v;
    long i;
    char *p;

    v = 0;
    for ( i=1; i<argc; ++i ) {
	if ( *(p=argv[i])=='-' ) {
	    if ( *++p=='-' ) ++p;
	    if ( strcmp(p,"v")==0 || strcmp(p,"verbose")==0 )
		++v; /* verbose 0,1,2 */
	}
    }

    for ( i=0; i<0x120000; ++i )
	tab_name[i] = NULL;

    for ( e=0; e<11; ++e ) {
	tabcodes[e].first = 10000000;
	tabcodes[e].last = -1;
	tabcodes[e].count = 0;
	for ( i=0; i<0x120000; ++i )
	    tabcodes[e].val[i] = -1;
    }

    /* get latest UnicodeData for comparisons */
    if ( (e=get_UnicodeData("UnicodeData.txt",tab_name,tabcodes,0,v)) ) goto mainerr;
    if ( (e=get_UnicodeData("UnicodeData10.txt",tab_name,tabcodes,10,v)) ) goto mainerr;

mainerr:
    for ( i=0; i<0x120000; ++i )
	if ( tab_name[i]!= NULL ) free(tab_name[i]);

    return( e );
}
