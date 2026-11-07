#include <engine/m4.h>
static unsigned row,column,columns,rows;static char lines[32][96];
#ifdef ENGINE_M7
static void (*capture7)(char,void *);static void *capture7_context;
void console7_capture(void (*sink)(char,void *),void *context){capture7=sink;capture7_context=context;}
#endif
extern void display_console_setup(void);
extern void display_console_cell(unsigned,unsigned,char);
static void advance(void){column=0;if(row+1<rows){++row;return;}for(unsigned r=1;r<rows;++r)memcpy(lines[r-1],lines[r],96);memset(lines[rows-1],0,96);for(unsigned r=0;r<rows;++r)for(unsigned c=0;c<columns;++c)display_console_cell(c,r,lines[r][c]?lines[r][c]:' ');}
static void character(char c){
#ifdef ENGINE_M7
    if(capture7){capture7(c,capture7_context);serial_char(c);return;}
#endif
    if(c=='\n'){serial_char(c);advance();return;}if(c==8){if(!column&&row){--row;column=columns;}if(column){--column;lines[row][column]=0;display_console_cell(column,row,' ');serial_text("\b \b");}return;}
    if(c<32||c>126)return;if(column==columns)advance();serial_char(c);lines[row][column]=c;display_console_cell(column,row,c);++column;
}
void console_begin(const os_boot_info_v0 *boot){row=column=0;memset(lines,0,sizeof(lines));columns=boot->framebuffer_width>32?(boot->framebuffer_width-32)/12:80;
#ifdef ENGINE_M73
 columns=boot->framebuffer_width>32?(boot->framebuffer_width-32)/16:60;
#endif
 if(columns>95)columns=95;if(columns<20)columns=20;
    rows=boot->framebuffer_height>160?(boot->framebuffer_height-160)/18:24;
#ifdef ENGINE_M73
 rows=boot->framebuffer_height>108?(boot->framebuffer_height-108)/24:24;
#endif
 if(rows>32)rows=32;if(rows<4)rows=4;display_console_setup();}
void console_text(const char *s){while(*s)character(*s++);}
void console_hex(uint64_t v){char b[19];b[0]='0';b[1]='x';for(unsigned i=0;i<16;++i)b[2+i]="0123456789abcdef"[(v>>(60-4*i))&15];b[18]=0;console_text(b);}
