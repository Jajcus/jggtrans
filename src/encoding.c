#include <iconv.h>
#include <errno.h>
#include <assert.h>
#include "encoding.h"
#include <glib.h>

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


static char *convert(iconv_t conv,const char *str){
char *inbuf;
size_t inbytesleft;
char *outbuf;
char *oldbuf;
size_t outbytesleft;
int r;

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

