#include <libxode.h>

int pkts = 0;

void node(int type, xmlnode x, void *arg)
{
    if(type > XSTREAM_CLOSE){
        printf("[%s] got err %d: %s\n",arg,type,x);
    }else{
//        printf("\b\b\b\b\b\b\b%d",pkts++);
        if(pkts++ > 1000000) exit(0);
        xmlnode_free(x);
    }
}

main()
{
    char foo[] = "my msg";
    pool p;
    xstream xs;

  while(1)
  {
    p = pool_new();
    xs = xstream_new(p, node, (void *)foo);
    xstream_eat(xs, "<root lala='foo'>", -1);
    xstream_eat(xs, " <a c='1234&quot;'>fo&amp;o</a>", -1);
    xstream_eat(xs, " <a>foo</a> <b>bar</b>", -1);
    xstream_eat(xs, " <a>b<c/>ar</a>", -1);
    xstream_eat(xs, " <a>b<c/>ar</a>", -1);
    xstream_eat(xs, " <a>foo</a> <b>bar</b>", -1);
    xstream_eat(xs, " <a>foo</a> <b>bar</b>", -1);
    pool_free(p);
  }
}
