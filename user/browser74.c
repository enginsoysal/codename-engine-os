#define APP7_USER
#include <engine/desktop/app.h>

typedef struct {char url[512],label[32];} html_link74;
typedef struct {
    uint32_t magic,in_tag,quote,tag_length,entity_length,skip,links,truncated,anchor,anchor_start,history_count;
    char tag[512],entity[16],base[256],history[4][256];
    html_link74 link[16];
} html74_state;
_Static_assert(sizeof(html74_state)<=3*4096,"bounded basic HTML state");
static unsigned lower74(unsigned c){return c>='A'&&c<='Z'?c+32:c;}
static unsigned same74(const char *a,const char *b){unsigned n=0;while(a[n]&&lower74((unsigned char)a[n])==(unsigned char)b[n])++n;return !a[n]&&!b[n];}
static void copy74(char *out,const char *in,unsigned cap){unsigned n=0;while(n+1<cap&&in[n]){out[n]=in[n];++n;}out[n]=0;}
static void clear74(void *out,unsigned n){volatile uint8_t *p=out;for(unsigned i=0;i<n;++i)p[i]=0;}
static void emit74(html74_state *s,app7_view *v,unsigned c){
    if(c=='\r')return;if(c=='\t')c=' ';if(c!='\n'&&(c<32||c>126))c='?';
    if(c==' '&&(!v->length||v->text[v->length-1]==' '||v->text[v->length-1]=='\n'))return;
    if(c=='\n'&&v->length&&v->text[v->length-1]==' '){v->text[v->length-1]='\n';return;}
    if(c=='\n'&&(!v->length||v->text[v->length-1]=='\n'))return;
    if(v->length==APP74_HTML_TEXT){s->truncated=1;return;}v->text[v->length++]=(char)c;v->text[v->length]=0;
}
static unsigned entity74(const char *s){
    if(same74(s,"amp"))return '&';if(same74(s,"lt"))return '<';if(same74(s,"gt"))return '>';
    if(same74(s,"quot"))return '"';if(same74(s,"apos"))return '\'';if(same74(s,"nbsp"))return ' ';
    if(s[0]=='#'){unsigned value=0,i=1,base=10;if(s[i]=='x'||s[i]=='X'){base=16;++i;}
        unsigned first=i;for(;s[i];++i){unsigned c=lower74((unsigned char)s[i]),digit=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:99;
            if(digit>=base||value>127/base)return '?';value=value*base+digit;}if(i>first&&value>=32&&value<=126)return value;
    }return '?';
}
static unsigned href74(const char *tag,char *out){
    for(unsigned i=0;tag[i];++i){if(i&&tag[i-1]!=' '&&tag[i-1]!='\t'&&tag[i-1]!='\n')continue;
        if(lower74((unsigned char)tag[i])!='h')continue;unsigned n=i;const char *name="href";unsigned j=0;
        while(name[j]&&tag[n]&&lower74((unsigned char)tag[n])==(unsigned char)name[j]){++n;++j;}if(name[j])continue;
        while(tag[n]==' '||tag[n]=='\t')++n;if(tag[n++]!='=')continue;while(tag[n]==' '||tag[n]=='\t')++n;
        unsigned quote=tag[n]=='\''||tag[n]=='"'?(unsigned char)tag[n++]:0,k=0;
        while(tag[n]&&(quote?(unsigned char)tag[n]!=quote:tag[n]!=' '&&tag[n]!='\t'&&tag[n]!='\n')){
            unsigned c=(unsigned char)tag[n++];if(c<32||c>126||k==511)return 0;
            if(c=='&'){char entity[16];unsigned e=0,start=n;while(tag[n]&&tag[n]!=';'&&e<15)entity[e++]=tag[n++];entity[e]=0;
                if(tag[n]==';'){++n;c=entity74(entity);}else n=start;}
            out[k++]=(char)c;
        }out[k]=0;return k!=0;
    }return 0;
}
static void anchor_end74(html74_state *s,app7_view *v){
    if(s->anchor<s->links){unsigned n=0;char *label=s->link[s->anchor].label;
        for(unsigned i=s->anchor_start;i<v->length&&n<31;++i){unsigned c=(unsigned char)v->text[i];label[n++]=(char)(c=='\n'?' ':c);}label[n]=0;
        if(!n)copy74(label,"Open link",32);
    }s->anchor=UINT32_MAX;
}
static void tag74(html74_state *s,app7_view *v){
    s->tag[s->tag_length]=0;char name[20];unsigned i=0,n=0;while(s->tag[i]==' '||s->tag[i]=='\t')++i;
    unsigned closing=s->tag[i]=='/';if(closing)++i;
    while(s->tag[i]&&s->tag[i]!=' '&&s->tag[i]!='\t'&&s->tag[i]!='\n'&&s->tag[i]!='/'&&n<19)name[n++]=(char)lower74((unsigned char)s->tag[i++]);name[n]=0;
    if(s->skip){if(closing&&((s->skip==1&&same74(name,"script"))||(s->skip==2&&same74(name,"style"))))s->skip=0;return;}
    if(!closing&&(same74(name,"script")||same74(name,"style"))){s->skip=same74(name,"script")?1:2;return;}
    if(same74(name,"a")){
        if(closing)anchor_end74(s,v);
        else if(s->links<16&&href74(s->tag,s->link[s->links].url)){
            anchor_end74(s,v);unsigned index=s->links++;emit74(s,v,' ');emit74(s,v,'[');if(index+1>=10)emit74(s,v,'1');emit74(s,v,'0'+(index+1)%10);emit74(s,v,']');emit74(s,v,' ');s->anchor=index;s->anchor_start=v->length;
        }
    }
    if(same74(name,"br")||same74(name,"p")||same74(name,"div")||same74(name,"li")||same74(name,"tr")||same74(name,"section")||same74(name,"article")||(name[0]=='h'&&name[1]>='1'&&name[1]<='6'&&!name[2]))emit74(s,v,'\n');
}
#ifdef APP74_HOST
__declspec(dllexport)
#endif
void browser74_event(app7_mailbox *m,void *storage){
    html74_state *s=storage;app7_view *v=&m->view;
    if(m->magic!=APP7_MAGIC||m->bytes>=APP7_TEXT){v->calc_error=1;return;}
    if(m->command==APP74_HTML_BEGIN){
        if(m->bytes>=256){v->calc_error=1;return;}
        if(s->magic!=0x48373431)clear74(s,sizeof(*s));
        if(s->magic&&s->base[0]&&m->key==1){if(s->history_count==4){for(unsigned i=1;i<4;++i)copy74(s->history[i-1],s->history[i],256);--s->history_count;}copy74(s->history[s->history_count++],s->base,256);}
        else if(s->magic&&!m->key&&s->history_count)--s->history_count;
        s->magic=0x48373431;s->in_tag=s->quote=s->tag_length=s->entity_length=s->skip=s->links=s->truncated=0;s->anchor=UINT32_MAX;
        for(unsigned i=0;i<m->bytes;++i)s->base[i]=m->input[i];s->base[m->bytes]=0;
        v->length=v->cursor=v->dirty=v->calc_error=v->operation=v->entered=0;v->number=v->operand=0;v->text[0]=0;return;
    }
    if(s->magic!=0x48373431){v->calc_error=1;return;}
    if(m->command==APP74_HTML_LINK||m->command==APP74_HTML_BACK||m->command==APP74_HTML_BASE||m->command==APP74_HTML_LINKS){
        char *out=v->text+APP74_HTML_LINK_OFFSET;clear74(out,512);
        if(m->command==APP74_HTML_BASE)copy74(out,s->base,512);
        if(m->command==APP74_HTML_BACK&&s->history_count)copy74(out,s->history[s->history_count-1],512);
        if(m->command==APP74_HTML_LINK&&m->key&&m->key<=s->links)copy74(out,s->link[m->key-1].url,512);
        if(m->command==APP74_HTML_LINKS)for(unsigned i=0;i<s->links;++i)copy74(out+i*32,s->link[i].label,32);
        return;
    }
    if(m->command!=APP74_HTML_FEED){v->calc_error=1;return;}
    for(unsigned i=0;i<m->bytes;++i){unsigned c=(unsigned char)m->input[i];
        if(m->key&2){emit74(s,v,c);continue;}
        if(s->in_tag){
            if(c=='>'&&!s->quote){tag74(s,v);s->in_tag=s->tag_length=0;continue;}
            if(c=='\''||c=='"'){if(s->quote==c)s->quote=0;else if(!s->quote)s->quote=c;}
            if(s->tag_length<511)s->tag[s->tag_length++]=(char)c;continue;
        }
        if(c=='<'){s->in_tag=1;s->tag_length=s->quote=0;continue;}if(s->skip)continue;
        if(s->entity_length){if(c==';'){s->entity[s->entity_length-1]=0;emit74(s,v,entity74(s->entity));s->entity_length=0;}
            else if(s->entity_length<16&&c>32){s->entity[s->entity_length++-1]=(char)c;}
            else{s->entity_length=0;emit74(s,v,'?');emit74(s,v,c);}continue;}
        if(c=='&'){s->entity_length=1;continue;}if(c=='\n'||c=='\r')c=' ';emit74(s,v,c);
    }
    if(m->key&4){anchor_end74(s,v);if(s->entity_length){emit74(s,v,'?');s->entity_length=0;}}
    v->cursor=v->dirty=v->calc_error=v->entered=0;v->number=s->links;v->operation=s->truncated;
}
#ifndef APP74_HOST
__attribute__((noreturn)) void app7_main(void){
    browser74_event((app7_mailbox *)(uintptr_t)APP7_MAILBOX,(void *)(uintptr_t)APP74_STATE);
    __asm__ volatile("int $0x80"::"a"((uint64_t)APP7_DONE):"memory");for(;;)__asm__ volatile("ud2");
}
#endif
