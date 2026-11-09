#include <engine/desktop/window.h>
#include <limits.h>
unsigned window92_cases(void){
 desktop_model m;desktop_init(&m,1024,768);uint64_t t=desktop_open(&m,1,(desktop_rect){100,90,600,400});if(!t)return 1;
 if(!desktop_place(&m,t,(desktop_rect){-200,40,600,400})||desktop_lookup(&m,t)->bounds.x!=-200||desktop_hit(&m,0,50)!=t)return 2;
 desktop_place(&m,t,(desktop_rect){INT_MIN,40,600,400});if(desktop_lookup(&m,t)->bounds.x!=-440||desktop_hit(&m,159,50)!=t||desktop_hit(&m,160,50))return 3;
 desktop_place(&m,t,(desktop_rect){INT_MAX,INT_MAX,600,400});desktop_rect r=desktop_lookup(&m,t)->bounds;if(r.x!=864||r.y!=673)return 4;
 desktop_rect damage;if(!desktop_take_damage(&m,&damage)||damage.x<0||damage.y<0||damage.x+damage.width>1024||damage.y+damage.height>768)return 5;
 desktop_place(&m,t,(desktop_rect){100,90,600,400});r=desktop_lookup(&m,t)->bounds;if(r.x!=100||r.y!=90||r.width!=600||r.height!=400)return 6;
 desktop_place(&m,t,(desktop_rect){INT_MIN,INT_MIN,INT_MAX,INT_MAX});r=desktop_lookup(&m,t)->bounds;if(r.width!=1024||r.height!=708||r.x!=-864||r.y!=0)return 7;
 desktop_close(&m,t);if(desktop_lookup(&m,t))return 8;return 0;
}
