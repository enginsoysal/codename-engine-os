/* Basic static HTTPS reader. HTML parsing stays in a bounded CPL3 worker. */
enum { BROWSER_BACK74=15000,BROWSER_HOME74,BROWSER_ADDRESS74,BROWSER_GO74,BROWSER_STOP74,BROWSER_LINKS74,
       BROWSER_PREV74,BROWSER_NEXT74,BROWSER_RETRY74,BROWSER_LINK74=15100 };
static const char browser_home74[]="<h1>Welcome to your browser</h1><p>A small, readable space for static HTTPS pages.</p>"
    "<p>This first browser reads HTML and plain text. JavaScript, forms, CSS layout, images and downloads inside pages are not supported.</p>"
    "<p>Choose an address above, or open Links to visit one of these pages:</p>"
    "<p><a href='https://example.com/'>Example Domain</a></p>"
    "<p><a href='https://www.iana.org/help/example-domains'>About example domains</a></p>"
    "<p>Pages are limited to 128 KiB of source and 7679 visible text bytes. Use Downloads for other files.</p>";
static unsigned prefix74(const char *value,const char *prefix){
    unsigned i=0;while(prefix[i]){unsigned c=(unsigned char)value[i];if(c>='A'&&c<='Z')c+=32;if(c!=(unsigned char)prefix[i])return 0;++i;}return 1;
}
static unsigned browser_snapshot74(application *app){
    if(app->number<0||app->number>16||app->length>APP74_HTML_TEXT||app->operation>1||app->calc_error)return V6_INVALID;
    app->count74=(unsigned)app->number;return 0;
}
static void browser_home_page74(application *app,unsigned mode){
    static const char address[]="engine://home";
    if(send_app(app,APP74_HTML_BEGIN,mode,address,sizeof(address)-1)||send_app(app,APP74_HTML_FEED,4,browser_home74,sizeof(browser_home74)-1)||browser_snapshot74(app))return;
    copy(app->pending,address,256);app->offset=app->month74=app->year74=app->day74=0;app->name_cursor=length(app->pending);app->name_selected=0;notice(app,"");
}
static void browser_restore_address74(application *app){
    if(!app->stopped&&!send_app(app,APP74_HTML_BASE,0,0,0))copy(app->pending,app->text+APP74_HTML_LINK_OFFSET,256);
    app->name_cursor=length(app->pending);app->name_selected=0;
}
static unsigned browser_parse74(application *app,const char *address,unsigned plain){
    const v6_entry *e=entry(app->path);if(!e||e->kind!=1||e->size>128u*1024||v6_verify(m6_volume(),app->path))return V6_INVALID;
    uint64_t size=e->size;unsigned status=send_app(app,APP74_HTML_BEGIN,(unsigned)app->offset74,address,length(address));if(status)return status;
    uint8_t input[4096];uint64_t offset=0;
    do{unsigned n=(unsigned)(size-offset);if(n>sizeof(input))n=sizeof(input);
        if(n&&read74(app->path,offset,input,n))return V6_IO;
        status=send_app(app,APP74_HTML_FEED,(plain?2:0)|((offset+n==size)?4:0),input,n);if(status)return status;offset+=n;
    }while(offset<size);
    status=browser_snapshot74(app);if(status)return status;
    app->offset=app->month74=app->year74=app->day74=0;copy(app->pending,address,256);app->name_cursor=length(app->pending);
    notice(app,app->operation?"Showing the first 7679 text bytes. This basic reader does not run scripts.":"Static page loaded. Open Links to follow a link; arrows scroll the text.");
    serial_text("M74_BROWSER_PAGE_LOADED text_bytes=");serial_hex(app->length);serial_text("\n");return 0;
}
static void browser_cache_remove74(application *app){
    if(app->existed==1&&app->path[0]){(void)v6_remove(m6_volume(),app->path);app->path[0]=0;}app->existed=0;
}
static void browser_stop74(application *app,uint64_t token){
    if(transfer_owner74==token){download7_cancel();transfer_owner74=0;}browser_cache_remove74(app);
}
static void browser_go74(application *app,uint64_t token,const char *source,unsigned mode){
    char address[256];copy(address,source,256);
    app->month74=0;
    if(equal(address,"engine://home")){browser_stop74(app,token);browser_home_page74(app,mode);whole();return;}
    if(!prefix74(address,"https://")){
        unsigned colon=0;for(unsigned i=0;address[i];++i)if(address[i]==':')colon=1;
        if(colon||length(address)>247){notice(app,"Enter an https:// address, or a host name such as example.com.");return;}
        copy(address,"https://",256);append(address,source,256);
    }else for(unsigned i=0;i<8;++i)address[i]="https://"[i];
    net7_url parsed;if(net7_parse_url(address,&parsed)){notice(app,"Check the address. This browser opens HTTPS pages only.");return;}
    download7_info d=download7_current();
    if(transfer_owner74&&transfer_owner74!=token){notice(app,"Another window is using the connection. Wait for it to finish.");return;}
    if(d.state>=DOWNLOAD7_RESOLVING&&d.state<=DOWNLOAD7_RECEIVING&&transfer_owner74!=token){notice(app,"Wait for the current download to finish first.");return;}
    browser_stop74(app,token);copy(app->pending,address,256);app->offset74=mode;app->month74=0;app->day74=0;app->name_selected=0;
    net7_configuration network=net7_current();
    if(network.state!=NET7_CONNECTED){
        if(network.state==NET7_NO_INTERFACE){notice(app,"No supported network adapter is available. Check the VM network settings.");browser_restore_address74(app);return;}
        if(network.state!=NET7_CONFIGURING)net7_retry();app->existed=2;transfer_owner74=token;notice(app,"Connecting to the network...");return;
    }
    char folder[256];root_path(folder,"AppData");unsigned status=v6_mkdir(m6_volume(),folder);
    if(status&&status!=V6_EXISTS){notice(app,"Cannot create the private browser cache.");return;}
    append(folder,"/Browser",256);status=v6_mkdir(m6_volume(),folder);
    if(status&&status!=V6_EXISTS){notice(app,"Cannot create the private browser cache.");return;}
    unique_in(app->path,folder,"Page",".html");
    if(!app->path[0]){notice(app,"Cannot create a new private page file.");browser_restore_address74(app);return;}
    if(download74_start(m6_volume(),address,app->path,128u*1024)){notice(app,download7_current().message?download7_current().message:"Could not start this page request.");app->path[0]=0;browser_restore_address74(app);return;}
    transfer_owner74=token;app->existed=1;app->total74=download7_current().revision;notice(app,"Opening a checked HTTPS connection...");
}
static unsigned browser_link_address74(application *app,unsigned link,char *out){
    char target[512],base[256];if(send_app(app,APP74_HTML_LINK,link,0,0))return 1;copy(target,app->text+APP74_HTML_LINK_OFFSET,512);
    if(send_app(app,APP74_HTML_BASE,0,0,0))return 1;copy(base,app->text+APP74_HTML_LINK_OFFSET,256);
    if(prefix74(target,"https://")){if(length(target)>=256)return 1;copy(out,target,256);return 0;}
    if(target[0]=='#'||!target[0])return 1;
    if(target[0]=='/'&&target[1]=='/'){if(length(target)>249)return 1;copy(out,"https:",256);append(out,target,256);return 0;}
    for(unsigned i=0;target[i];++i){if(target[i]==':')return 1;if(target[i]=='/'||target[i]=='?'||target[i]=='#')break;}
    net7_url parsed;if(net7_parse_url(base,&parsed))return 1;
    unsigned origin=8;while(base[origin]&&base[origin]!='/')++origin;
    if(target[0]=='/'){if(origin+length(target)>=256)return 1;base[origin]=0;copy(out,base,256);append(out,target,256);}
    else{unsigned end=origin;while(base[end]&&base[end]!='?'&&base[end]!='#')++end;while(end>origin&&base[end-1]!='/')--end;
        if(end==origin){if(end+1>=256)return 1;base[end++]='/';}if(end+length(target)>=256)return 1;base[end]=0;copy(out,base,256);append(out,target,256);}
    return 0;
}
static void browser_init74(unsigned kind,application *app){
    if(kind!=BROWSER74)return;uint64_t token=desktop_token(&model,(unsigned)(app-apps));
    desktop_place(&model,token,(desktop_rect){((int)model.width-850)/2,35,850,650});
    if(app->path[0]){app->offset74=2;if(browser_parse74(app,"engine://local-file",0)){browser_home_page74(app,2);notice(app,"Cannot read this local HTML file. The original file was not changed.");}}
    else browser_home_page74(app,2);
}
static void browser_draw74(desktop_window *w,application *app,uint64_t token){
    if(w->app!=BROWSER74)return;int x=w->bounds.x+24,y=w->bounds.y+55,width=w->bounds.width-48;
    button(token,x,y,100,"Back",BROWSER_BACK74,0);button(token,x+112,y,100,"Home",BROWSER_HOME74,0);button(token,x+224,y,100,"Stop",BROWSER_STOP74,0);
    text(x+345,y+10,"Basic static-page browser",1,ink());
    ui73_round(x,y+49,width-94,36,6,app->name_selected?preferences.accent:preferences.dark?0x10212a:0xffffff);
    char visible[256];unsigned start=0;copy(visible,app->pending,256);
    while(visible[start]&&ui73_measure(visible+start,1)>(unsigned)(width-120))++start;
    text(x+10,y+59,visible+start,1,app->name_selected?0x102d29:ink());
    if(control_count<DESKTOP_CONTROLS)controls[control_count++]=(control){{x,y+49,width-94,36},token,BROWSER_ADDRESS74};
    button(token,x+width-82,y+49,82,"Go",BROWSER_GO74,1);
    int body=y+108;unsigned rows=(unsigned)(w->bounds.height-255)/24;ui73_round(x,body,width,w->bounds.height-223,7,preferences.dark?0x10212a:0xffffff);
    if(app->month74){
        for(unsigned i=0;i<8&&app->year74+i<app->count74;++i){unsigned index=app->year74+i;
            const char *label=app->text+APP74_HTML_LINK_OFFSET+index*32;
            button(token,x+12,body+12+(int)i*42,width-24,label[0]?label:"Open link",BROWSER_LINK74+index,0);}
        if(!app->count74)text(x+16,body+18,"This page has no supported links.",1,ink());
    }else{unsigned at=0;char line[256];for(unsigned i=0;i<app->offset&&app->text[at];++i){unsigned next=line74(app->text,at,(unsigned)width-28,line);if(next<=at)break;at=next;}
        app->source_crc=page74(x+14,body+14,(unsigned)width-28,rows,app->text,at);}
    int bottom=w->bounds.y+w->bounds.height-99;
    button(token,x,bottom,105,"Previous",BROWSER_PREV74,0);button(token,x+117,bottom,100,"Next",BROWSER_NEXT74,0);button(token,x+230,bottom,130,app->month74?"Read page":"Links",BROWSER_LINKS74,app->month74);
    const char *message=app->notice;
    if(transfer_owner74==token&&app->existed==1){const char *stage=download7_current().message;if(stage)message=stage;}
    ui73_paragraph(x,bottom+49,(unsigned)width,2,message[0]?message:"HTML/plain text only. Scripts, forms and CSS layout are not supported.",ink());
}
static unsigned browser_action74(desktop_window *w,application *app,uint64_t token,unsigned command){
    if(w->app!=BROWSER74||command<15000||command>=15200)return 0;
    if(command==BROWSER_ADDRESS74){app->day74=1;app->name_selected=1;app->name_cursor=length(app->pending);}
    if(command==BROWSER_GO74){char address[256];copy(address,app->pending,256);browser_go74(app,token,address,1);}
    if(command==BROWSER_HOME74){browser_stop74(app,token);browser_home_page74(app,1);}
    if(command==BROWSER_BACK74){app->month74=0;if(!send_app(app,APP74_HTML_BACK,0,0,0)){char address[256];copy(address,app->text+APP74_HTML_LINK_OFFSET,256);
        if(address[0])browser_go74(app,token,address,0);else notice(app,"This is the first page in this window.");}}
    if(command==BROWSER_STOP74){app->month74=0;browser_stop74(app,token);browser_restore_address74(app);notice(app,"Stopped. The previous page was kept.");}
    if(command==BROWSER_LINKS74){app->month74^=1;app->year74=0;app->day74=0;if(app->month74)send_app(app,APP74_HTML_LINKS,0,0,0);}
    if(command==BROWSER_PREV74){unsigned rows=(unsigned)(w->bounds.height-255)/24;if(app->month74)app->year74=app->year74>=8?app->year74-8:0;else app->offset=app->offset>=rows?app->offset-rows:0;}
    if(command==BROWSER_NEXT74){unsigned rows=(unsigned)(w->bounds.height-255)/24;if(app->month74){if(app->year74+8<app->count74)app->year74+=8;}else if(app->source_crc<app->length)app->offset+=rows;}
    if(command>=BROWSER_LINK74&&command<BROWSER_LINK74+16){char address[256];unsigned link=command-BROWSER_LINK74+1;app->month74=0;
        if(browser_link_address74(app,link,address))notice(app,"This link is not a supported HTTPS address.");else browser_go74(app,token,address,1);}
    invalidate(w->bounds);return 1;
}
static unsigned browser_key74(desktop_window *w,application *app,uint64_t token,int key){
    if(w->app!=BROWSER74)return 0;
    if(key==12){app->day74=1;app->name_selected=1;app->name_cursor=length(app->pending);invalidate(w->bounds);return 1;}
    if(key==3){const char *source=app->day74?app->pending:app->text;clipboard_length=length(source);if(clipboard_length>=sizeof(clipboard))clipboard_length=sizeof(clipboard)-1;memcpy(clipboard,source,clipboard_length);clipboard[clipboard_length]=0;notice(app,"Copied to the clipboard.");invalidate(w->bounds);return 1;}
    if(!app->day74){if(key==133&&app->offset)--app->offset;else if(key==134&&app->source_crc<app->length)++app->offset;else return 0;invalidate(w->bounds);return 1;}
    if(key==10){browser_action74(w,app,token,BROWSER_GO74);return 1;}
    if(key==22){if(clipboard_length>255)notice(app,"This address is longer than the 255-character limit.");else{copy(app->pending,clipboard,256);app->name_cursor=length(app->pending);app->name_selected=0;}}
    else if(key==128){if(app->name_cursor)--app->name_cursor;app->name_selected=0;}
    else if(key==129){if(app->name_cursor<length(app->pending))++app->name_cursor;app->name_selected=0;}
    else if(key==130){app->name_cursor=0;app->name_selected=0;}
    else if(key==131){app->name_cursor=length(app->pending);app->name_selected=0;}
    else if(key==8||key==132){unsigned n=length(app->pending);if(app->name_selected){app->pending[0]=0;app->name_cursor=app->name_selected=0;}
        else if((key==8&&app->name_cursor)||(key==132&&app->name_cursor<n)){if(key==8)--app->name_cursor;for(unsigned i=app->name_cursor;i<n;++i)app->pending[i]=app->pending[i+1];}}
    else if(key>=32&&key<=126){unsigned n=length(app->pending);if(app->name_selected){app->pending[0]=0;app->name_cursor=app->name_selected=n=0;}
        if(n<255){for(unsigned i=n+1;i>app->name_cursor;--i)app->pending[i]=app->pending[i-1];app->pending[app->name_cursor++]=(char)key;}}
    else return 0;
    invalidate(w->bounds);return 1;
}
static void browser_pump74(void){
    desktop_window *w=desktop_lookup(&model,transfer_owner74);if(!w||w->app!=BROWSER74)return;
    application *app=&apps[w-model.windows];uint64_t token=transfer_owner74;
    if(app->existed==2){net7_configuration net=net7_current();
        if(net.state==NET7_CONNECTED){char address[256];copy(address,app->pending,256);unsigned mode=(unsigned)app->offset74;app->existed=0;transfer_owner74=0;browser_go74(app,token,address,mode);}
        else if(net.state==NET7_FAILED||net.state==NET7_NO_INTERFACE){app->existed=0;transfer_owner74=0;notice(app,net.message);browser_restore_address74(app);}
        else return;invalidate(w->bounds);return;
    }
    download7_info info=download7_current();
    if(info.state<DOWNLOAD7_DONE){if(app->total74==info.revision)return;app->total74=info.revision;
        invalidate((desktop_rect){w->bounds.x+24,w->bounds.y+w->bounds.height-50,w->bounds.width-48,48});return;}
    if(info.state==DOWNLOAD7_DONE){char address[256];const char *mime=download74_mime();unsigned plain=prefix74(mime,"text/plain"),html=prefix74(mime,"text/html")||prefix74(mime,"application/xhtml+xml")||!mime[0];
        unsigned status=(!plain&&!html)||download74_url(address,sizeof(address))?V6_INVALID:browser_parse74(app,address,plain);
        if(status&&!app->stopped){notice(app,"Could not display this page. HTML/plain text up to 128 KiB is supported.");browser_restore_address74(app);}
        browser_cache_remove74(app);transfer_owner74=0;
    }else if(info.state==DOWNLOAD7_FAILED||info.state==DOWNLOAD7_CANCELLED){notice(app,info.message?info.message:"The page request did not finish.");browser_cache_remove74(app);transfer_owner74=0;browser_restore_address74(app);}
    invalidate(w->bounds);
}
