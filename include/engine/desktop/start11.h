#ifndef ENGACER_START11_H
#define ENGACER_START11_H
#include <stdint.h>
#define S11_CATEGORIES 12u
#define S11_PINS 32u
#define S11_TASK_PINS 8u
#define S11_APPS 20u
#define S11_MAGIC UINT64_C(0x3154524154533145)
typedef struct {unsigned kind,app;char path[256],label[32];} s11_pin;
typedef struct {
 uint64_t magic;unsigned version,categories,pins,tasks,view,tab,centered,reserved;
 char names[S11_CATEGORIES][32];unsigned order[S11_CATEGORIES],hidden[S11_CATEGORIES],assignment[S11_APPS];
 s11_pin pinned[S11_PINS];unsigned task[S11_TASK_PINS];
} s11_preferences;
_Static_assert(sizeof(s11_preferences)<16*1024,"bounded Start preferences");
static inline unsigned s11_text_valid(const char*p,unsigned cap){unsigned n=0;for(;n<cap&&p[n];++n)if((unsigned char)p[n]<32||(unsigned char)p[n]>126)return 0;return n&&n<cap;}
static inline unsigned s11_name_valid(const char*p,unsigned cap){if(!s11_text_valid(p,cap)||p[0]==' ')return 0;unsigned n=0;while(p[n])++n;return p[n-1]!=' ';}
static inline unsigned s11_same_text(const char*a,const char*b){for(unsigned n=0;;++n){unsigned x=(unsigned char)a[n],y=(unsigned char)b[n];if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return 0;if(!x)return 1;}}
static inline unsigned s11_valid(const s11_preferences*p){
 if(!p||p->magic!=S11_MAGIC||p->version!=1||!p->categories||p->categories>S11_CATEGORIES||p->pins>S11_PINS||p->tasks>S11_TASK_PINS||p->view>1||p->tab>2||p->centered>1||p->reserved)return 0;
 unsigned seen=0;
 for(unsigned i=0;i<p->categories;++i){if(!s11_name_valid(p->names[i],32)||p->hidden[i]>1||p->order[i]>=p->categories||(seen&(1u<<p->order[i])))return 0;seen|=1u<<p->order[i];for(unsigned k=0;k<i;++k)if(s11_same_text(p->names[k],p->names[i]))return 0;}
 for(unsigned i=0;i<S11_APPS;++i)if(p->assignment[i]>=p->categories)return 0;
 for(unsigned i=0;i<p->pins;++i){const s11_pin*q=&p->pinned[i];if(q->label[0]&&!s11_name_valid(q->label,32))return 0;if(q->kind==1){if(!q->app||q->app>=S11_APPS||q->path[0])return 0;}else if(q->kind==2){if(q->app||q->path[0]!='/'||!s11_text_valid(q->path,256))return 0;}else return 0;for(unsigned k=0;k<i;++k)if(q->kind==p->pinned[k].kind&&(q->kind==1?q->app==p->pinned[k].app:s11_same_text(q->path,p->pinned[k].path)))return 0;}
 for(unsigned i=0;i<p->tasks;++i){if(!p->task[i]||p->task[i]>=S11_APPS)return 0;for(unsigned k=0;k<i;++k)if(p->task[k]==p->task[i])return 0;}
 return 1;
}
#endif
