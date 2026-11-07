enum { CALC74_KEY=13000,CALC74_BASIC=13100,CALC74_SCIENTIFIC,CALC74_CONVERT };
static const char keys74[]="789/C456*\b123-n0.=+%solrqikmgbft";
static void draw_calculator74(desktop_window *w,application *app,uint64_t token){
    int x=w->bounds.x+24,y=w->bounds.y+56,width=w->bounds.width-48;
    const char *tabs[]={"Calculator","Scientific","Convert"};
    for(unsigned i=0;i<3;++i)button(token,x+(int)i*(width/3),y,width/3-8,tabs[i],CALC74_BASIC+i,app->offset==i);
    ui73_round(x,y+53,width,72,9,preferences.dark?0x10212a:0xe7f0ee);
    ui73_fit(x+16,y+73,(unsigned)width-32,app->calc_error?"Cannot calculate. Clear or edit the value.":app->length?app->text:"0",3,ink());
    int keyboard=y+151;
    if(app->offset==1){
        const char *names[]={"sin (degrees)","cos (degrees)","ln","Square root","Square","1 / x"};
        for(unsigned i=0;i<6;++i)button(token,x+(int)(i%3)*(width/3),y+146+(int)(i/3)*44,width/3-8,names[i],CALC74_KEY+20+i,0);
        keyboard=y+246;
    }else if(app->offset==2){
        const char *names[]={"km to miles","miles to km","kg to pounds","pounds to kg","Celsius to Fahrenheit","Fahrenheit to Celsius"};
        for(unsigned i=0;i<6;++i)button(token,x+(int)(i%2)*(width/2),y+146+(int)(i/2)*44,width/2-8,names[i],CALC74_KEY+26+i,0);
        keyboard=y+286;
    }
    const char *labels[]={"7","8","9","/","Clear","4","5","6","x","Backspace","1","2","3","-","+ / -","0",".","=","+","%"};
    for(unsigned i=0;i<20;++i)button(token,x+(int)(i%5)*(width/5),keyboard+(int)(i/5)*44,width/5-8,labels[i],CALC74_KEY+i,i==17);
    ui73_paragraph(x,w->bounds.y+w->bounds.height-65,(unsigned)width,2,app->offset==1?"Six decimal places. Trigonometry uses degrees; scientific results are approximate.":"Six decimal places. Results are rounded towards zero. Clear resets the calculation.",ink());
}
static unsigned calculator_action74(desktop_window *w,application *app,unsigned command){
    if(w->app!=CALCULATOR)return 0;
    if(command>=CALC74_BASIC&&command<=CALC74_CONVERT)app->offset=command-CALC74_BASIC;
    else if(command>=CALC74_KEY&&command<CALC74_KEY+sizeof(keys74)-1)send_app(app,APP7_KEY,(unsigned char)keys74[command-CALC74_KEY],0,0);
    else return 0;
    invalidate(w->bounds);return 1;
}
