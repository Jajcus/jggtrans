/* $Id: encoding.c,v 1.9 2002/12/08 15:35:41 jajcus Exp $ */

/*
 *  (C) Copyright 2002 Jacek Konieczny <jajcus@pld.org.pl>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License Version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "ggtrans.h"
#include <iconv.h>
#include <errno.h>
#include <assert.h>
#include "encoding.h"

#define ENCODING "windows-1250"

static iconv_t to_utf8_c;
static iconv_t from_utf8_c;
static char *buf;
static int buf_len;


int encoding_init(){

	to_utf8_c=iconv_open("utf-8",ENCODING);
	if (to_utf8_c==(iconv_t)-1)
		g_error("Couldn't open 'to Unicode' converter (%s)",g_strerror(errno));
	from_utf8_c=iconv_open(ENCODING,"utf-8");
	if (from_utf8_c==(iconv_t)-1)
		g_error("Couldn't open 'from Unicode' converter (%s)",g_strerror(errno));
	buf_len=16;
	buf=g_new(char,buf_len);
	return 0;
}

void encoding_done(){

	iconv_close(to_utf8_c);
	iconv_close(from_utf8_c);
	g_free(buf);
}

static char *convert(iconv_t conv,const char *str){
char *inbuf;
size_t inbytesleft;
char *outbuf;
char *oldbuf;
size_t outbytesleft;
int r;

	if (str==NULL) return NULL;
	inbuf=(char *)str;
	inbytesleft=strlen(str);
	outbuf=buf;
	outbytesleft=buf_len-1;
	iconv(conv,NULL,&inbytesleft,&outbuf,&outbytesleft);
	while(inbytesleft>0){
		r=iconv(conv,&inbuf,&inbytesleft,&outbuf,&outbytesleft);
		if (r>=0){
			*outbuf=0;
			break;
		}
		switch(errno){
			case EILSEQ:
				if (!*inbuf) break;
				inbuf++;
				*(outbuf++)='?';
				outbytesleft--;
				inbytesleft--;
				if (outbytesleft>0) break;
			case E2BIG:
				buf_len+=1024;
				oldbuf=buf;
				buf=(char *)g_realloc(oldbuf,buf_len);
				assert(buf!=NULL);
				outbytesleft+=1024;
				outbuf=buf+(outbuf-oldbuf);
				break;
			case EINVAL:
				inbytesleft=0;
				break;
			default:
				*buf=0;
				inbytesleft=0;
				break;
		}
	}
	return buf;
}

char *to_utf8(const char *str){

	return convert(to_utf8_c,str);
}

char *from_utf8(const char *str){

	return convert(from_utf8_c,str);
}

