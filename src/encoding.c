/* $Id: encoding.c,v 1.16 2003/08/08 16:08:10 jajcus Exp $ */

/*
 *  (C) Copyright 2002-2005 Jacek Konieczny [jajcus(a)jajcus,net]
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
#include <errno.h>
#include <assert.h>
#include "encoding.h"
#include "enc_win2uni.h"
#include "enc_uni2win.h"

static unsigned char *buf;
static int buf_len;

int encoding_init(){

	buf_len=16;
	buf=g_new(char,buf_len);
	return 0;
}

void encoding_done(){

	g_free(buf);
}

char *to_utf8(const char *str){
int o=0;
int i;
unsigned char c;
unsigned u;

	if (str==NULL) return NULL;
	if (buf_len<(3*strlen(str)+1)){
		buf_len=3*strlen(str)+1; /* this should always be enough */
		buf=(char *)g_realloc(buf,buf_len);
		assert(buf!=NULL);
	}
	for(i=0;str[i];i++){
		c=(unsigned char)str[i];
		if (c=='\r'){
			continue;
		}
		if (c=='\t' || c=='\n'){
			buf[o++]=c;
			continue;
		}
		if (c<32){
			buf[o++]='\xef';
			buf[o++]='\xbf';
			buf[o++]='\xbd';
			continue;
		}
		if (c<128){
			buf[o++]=c;
			continue;
		}
		u=win1250_to_unicode[c-128];
		if (u==0||u>0x10000){ /* we don't need character > U+0x10000 */
			buf[o++]='\xef';
			buf[o++]='\xbf';
			buf[o++]='\xbd';
		}
		else if (u<0x800){
			buf[o++]=0xc0|(u>>6);
			buf[o++]=0x80|(u&0x3f);
		}
		else{
			buf[o++]=0xe0|(u>>12);
			buf[o++]=0x80|((u>>6)&0x3f);
			buf[o++]=0x80|(u&0x3f);
		}
	}
	buf[o]=0;
	return (char *)buf;
}

char *from_utf8(const char *str){
unsigned char b;
unsigned u;
int o=0;
int i;

	if (str==NULL) return NULL;
	if (buf_len<(strlen(str)+1)){
		buf_len=strlen(str)*2+1; /* this should always be enough */
		buf=(char *)g_realloc(buf,buf_len);
		assert(buf!=NULL);
	}
	for(i=0;str[i];i++){
		b=(unsigned char)str[i];
		if (b=='\n'){
			buf[o++]='\r';
			buf[o++]='\n';
			continue;
		}
		if ((b&0x80)==0){ /* ASCII */
			buf[o++]=b;
			continue;
		}
		if ((b&0xc0)==0x80){ /* middle of UTF-8 char */
			continue;
		}
		if ((b&0xe0)==0xc0){
			u=b&0x1f;
			i++;
			b=(unsigned char)str[i];
			if (b==0){
				buf[o++]='?';
				break;
			}
			if ((b&0xc0)!=0x80){
				buf[o++]='?';
				continue;
			}
			u=(u<<6)|(b&0x3f);
		}
		else if ((b&0xf0)==0xe0){
			u=b&0x0f;
			b=(unsigned char)str[++i];
			if (b==0){
				buf[o++]='?';
				break;
			}
			if ((b&0xc0)!=0x80){
				buf[o++]='?';
				continue;
			}
			u=(u<<6)|(b&0x3f);
			b=(unsigned char)str[++i];
			if (b==0){
				buf[o++]='?';
				break;
			}
			if ((b&0xc0)!=0x80){
				buf[o++]='?';
				continue;
			}
			u=(u<<6)|(b&0x3f);
		}
		else{
			buf[o++]='?';
			continue;
		}
		if (u<0x00a0)
			buf[o++]='?';
		else if (u<0x0180)
			buf[o++]=unicode_to_win1250_a0_17f[u-0x00a0];
		else if (u==0x02c7)
			buf[o++]=0xa1;
		else if (u<0x02d8)
			buf[o++]='?';
		else if (u<0x02de)
			buf[o++]=unicode_to_win1250_2d8_2dd[u-0x02d8];
		else if (u<0x2013)
			buf[o++]='?';
		else if (u<0x203b)
			buf[o++]=unicode_to_win1250_2013_203a[u-0x2013];
		else if (u==0x20ac)
			buf[o++]=0x80;
		else if (u==0x2122)
			buf[o++]=0x99;
		else
			buf[o++]='?';
	}
	buf[o]=0;
	return buf;
}

#ifdef ENCODINGTEST
#include <stdio.h>

int main(int argc,char *argv[]){
char buf[1024],*p;

	encoding_init();
	while(1){
		p=fgets(buf,1024,stdin);
		if (p==NULL || buf[0]=='\n') break;
		printf("To UTF8: %s",to_utf8(buf));
		printf("From UTF8: %s",from_utf8(buf));
	}
	encoding_done();
	return 0;
}

#endif
