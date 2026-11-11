#include <engine/desktop/start11.h>
#include <engine/desktop/power11.h>
static s11_preferences p;
static void reset(void){for(unsigned i=0;i<sizeof(p);++i)((uint8_t*)&p)[i]=0;p.magic=S11_MAGIC;p.version=1;p.categories=2;p.order[1]=1;p.names[0][0]='A';p.names[1][0]='B';}
unsigned start11_checks(void){unsigned n=0;
#define CHECK(x) do{++n;if(!(x))return n;}while(0)
 reset();CHECK(s11_valid(&p));p.magic=0;CHECK(!s11_valid(&p));reset();p.categories=13;CHECK(!s11_valid(&p));reset();p.order[1]=0;CHECK(!s11_valid(&p));reset();p.order[1]=2;CHECK(!s11_valid(&p));reset();p.hidden[1]=2;CHECK(!s11_valid(&p));reset();p.assignment[19]=2;CHECK(!s11_valid(&p));reset();p.names[1][0]='a';CHECK(!s11_valid(&p));reset();p.names[1][0]=' ';CHECK(!s11_valid(&p));reset();for(unsigned i=0;i<32;++i)p.names[1][i]='C';CHECK(!s11_valid(&p));
 reset();p.pins=1;p.pinned[0].kind=1;p.pinned[0].app=19;CHECK(s11_valid(&p));p.pinned[0].app=20;CHECK(!s11_valid(&p));p.pinned[0].app=2;p.pinned[0].path[0]='/';CHECK(!s11_valid(&p));reset();p.pins=1;p.pinned[0].kind=2;p.pinned[0].path[0]='/';CHECK(s11_valid(&p));p.pinned[0].path[0]='x';CHECK(!s11_valid(&p));reset();p.pins=33;CHECK(!s11_valid(&p));reset();p.pins=2;p.pinned[0].kind=p.pinned[1].kind=1;p.pinned[0].app=p.pinned[1].app=2;CHECK(!s11_valid(&p));reset();p.tasks=1;p.task[0]=2;CHECK(s11_valid(&p));p.task[0]=0;CHECK(!s11_valid(&p));p.task[0]=20;CHECK(!s11_valid(&p));reset();p.tasks=9;CHECK(!s11_valid(&p));reset();p.tasks=2;p.task[0]=p.task[1]=2;CHECK(!s11_valid(&p));reset();p.centered=2;CHECK(!s11_valid(&p));reset();p.view=2;CHECK(!s11_valid(&p));reset();p.tab=3;CHECK(!s11_valid(&p));
 unsigned a=0,b=0;uint8_t good[]={8,'_','S','5','_',0x12,6,4,5,5,0,0};
 /* BytePrefix is required for literal values greater than One. */
 good[6]=8;good[8]=0x0a;good[9]=5;uint8_t valid[]={8,'_','S','5','_',0x12,8,4,0x0a,5,0x0a,5,0,0};
 CHECK(p11_s5(valid,sizeof(valid),&a,&b)&&a==5&&b==5);
 for(unsigned length=0;length<sizeof(valid);++length)CHECK(!p11_s5(valid,length,&a,&b));
 valid[6]=0xff;CHECK(!p11_s5(valid,sizeof(valid),&a,&b));valid[6]=8;valid[9]=8;CHECK(!p11_s5(valid,sizeof(valid),&a,&b));valid[9]=5;
 uint8_t duplicate[28];for(unsigned i=0;i<14;++i)duplicate[i]=duplicate[14+i]=valid[i];CHECK(!p11_s5(duplicate,sizeof(duplicate),&a,&b));
 valid[3]='4';CHECK(!p11_s5(valid,sizeof(valid),&a,&b));CHECK(!p11_s5(good,sizeof(good),&a,&b));
 return 0;
}
