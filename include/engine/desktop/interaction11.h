#ifndef ENGACER_INTERACTION11_H
#define ENGACER_INTERACTION11_H
#include <stdint.h>
enum {I11_SELECT=1,I11_ACTIVATE=2,I11_CONTEXT=4,I11_DRAG=8,I11_CANCEL=16};
enum {I11_NONE,I11_ITEM,I11_BUTTON,I11_CAPTION,I11_BACKGROUND};
typedef struct {uint64_t window;unsigned action,kind;} i11_target;
typedef struct {
 i11_target down,last;int x,y,last_x,last_y;
 uint64_t last_ms;unsigned buttons,captured,dragged,last_valid,double_ms;
 uint64_t target_identity,down_identity,last_identity;
} i11_state;
static inline int i11_same(i11_target a,i11_target b){return a.window==b.window&&a.action==b.action&&a.kind==b.kind;}
static inline int i11_far(int x,int y,int a,int b){int64_t dx=(int64_t)x-a,dy=(int64_t)y-b;return dx>4||dx< -4||dy>4||dy< -4;}
static inline void i11_cancel(i11_state*s){s->captured=0;s->dragged=0;s->last_valid=0;}
/* Time is milliseconds; callers convert the kernel's 100 Hz monotonic clock. */
static inline unsigned i11_pointer(i11_state*s,i11_target target,int x,int y,unsigned buttons,uint64_t ms){
 unsigned old=s->buttons;s->buttons=buttons;
 if((buttons&3)&&!(old&3)){s->down=target;s->down_identity=s->target_identity;s->x=x;s->y=y;s->captured=buttons&3;s->dragged=0;return 0;}
 if(s->captured&&(buttons&3)&&i11_far(x,y,s->x,s->y)){s->last_valid=0;if(!s->dragged){s->dragged=1;return I11_DRAG;}return 0;}
 if(s->captured&&!(buttons&3)){
  unsigned held=s->captured;s->captured=0;
  if(s->dragged){s->dragged=0;return I11_CANCEL;}
  if(!i11_same(s->down,target)||s->down_identity!=s->target_identity||target.kind==I11_NONE){s->last_valid=0;return I11_CANCEL;}
  if(held==2){s->last_valid=0;return I11_CONTEXT;}
  if(held!=1){s->last_valid=0;return I11_CANCEL;}
  if(target.kind==I11_BUTTON){s->last_valid=0;return I11_ACTIVATE;}
  unsigned interval=s->double_ms;if(interval<200||interval>900)interval=500;
  unsigned twice=s->last_valid&&s->last_identity==s->target_identity&&i11_same(s->last,target)&&ms>=s->last_ms&&ms-s->last_ms<=interval&&!i11_far(x,y,s->last_x,s->last_y);
  s->last=target;s->last_identity=s->target_identity;s->last_ms=ms;s->last_x=x;s->last_y=y;s->last_valid=!twice;
  return I11_SELECT|(twice?I11_ACTIVATE:0);
 }
 return 0;
}
#endif
