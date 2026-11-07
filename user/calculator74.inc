/* Six-place decimal arithmetic without a floating-point runtime or heap. */
#define SCALE74 INT64_C(1000000)
static uint64_t magnitude74(int64_t n){return n<0?(uint64_t)(-(n+1))+1:(uint64_t)n;}
static int ratio74(int64_t a,int64_t b,int64_t divisor,int64_t *out){
    if(!divisor)return 1;
    unsigned negative=(a<0)^(b<0)^(divisor<0);uint64_t d=magnitude74(divisor),q=0,r=0;
    __uint128_t product=(__uint128_t)magnitude74(a)*magnitude74(b);
    for(int bit=127;bit>=0;--bit){r=(r<<1)|(uint64_t)((product>>bit)&1);
        if(r>=d){r-=d;if(bit>=64)return 1;q|=UINT64_C(1)<<bit;}}
    if(q>(uint64_t)INT64_MAX+negative)return 1;
    *out=negative?(q==(UINT64_C(1)<<63)?INT64_MIN:-(int64_t)q):(int64_t)q;return 0;
}
static void format74(app7_view *v){
    char reverse[32];unsigned n=0,i=0;uint64_t value=magnitude74(v->number),whole=value/SCALE74,fraction=value%SCALE74;
    if(v->number<0)v->text[i++]='-';do{reverse[n++]=(char)('0'+whole%10);whole/=10;}while(whole);
    while(n)v->text[i++]=reverse[--n];
    if(fraction){v->text[i++]='.';uint64_t place=100000;while(place){v->text[i++]=(char)('0'+fraction/place);fraction%=place;place/=10;}while(i&&v->text[i-1]=='0')--i;}
    v->text[i]=0;v->length=v->cursor=i;
}
static int parse74(app7_view *v){
    uint64_t whole=0,fraction=0,place=100000;unsigned dot=0,negative=v->text[0]=='-',i=negative;
    for(;i<v->length;++i){unsigned c=(unsigned char)v->text[i];if(c=='.'){if(dot)return 1;dot=1;continue;}
        if(c<'0'||c>'9')return 1;
        if(dot){if(!place)return 1;fraction+=(c-'0')*place;place/=10;}
        else{if(whole>((uint64_t)INT64_MAX/SCALE74-(c-'0'))/10)return 1;whole=whole*10+c-'0';}}
    uint64_t value=whole*SCALE74+fraction;if(value>(uint64_t)INT64_MAX+negative)return 1;
    v->number=negative?(value==(UINT64_C(1)<<63)?INT64_MIN:-(int64_t)value):(int64_t)value;return 0;
}
static int root74(int64_t value,int64_t *out){
    if(value<0)return 1;__uint128_t n=(__uint128_t)(uint64_t)value*SCALE74,root=0,bit=(__uint128_t)1<<126;
    while(bit>n)bit>>=2;
    while(bit){if(n>=root+bit){n-=root+bit;root=(root>>1)+bit;}else root>>=1;bit>>=2;}
    if(root>INT64_MAX)return 1;*out=(int64_t)root;return 0;
}
static int sine74(int64_t degrees,int64_t *out){
    degrees%=360*SCALE74;if(degrees>180*SCALE74)degrees-=360*SCALE74;if(degrees< -180*SCALE74)degrees+=360*SCALE74;
    int64_t x;if(ratio74(degrees,3141593,180*SCALE74,&x))return 1;
    int64_t square,term=x,sum=x;if(ratio74(x,x,SCALE74,&square))return 1;
    for(int64_t n=1;n<10;++n){if(ratio74(term,-square,SCALE74*(2*n)*(2*n+1),&term)||__builtin_add_overflow(sum,term,&sum))return 1;}
    *out=sum;return 0;
}
static int logarithm74(int64_t value,int64_t *out){
    if(value<=0)return 1;int64_t shift=0;
    while(value>=2*SCALE74){value/=2;++shift;}while(value<SCALE74){value*=2;--shift;}
    int64_t z,square,term,sum=0;if(ratio74(value-SCALE74,SCALE74,value+SCALE74,&z)||ratio74(z,z,SCALE74,&square))return 1;
    term=z;for(int64_t n=1;n<32;n+=2){sum+=term/n;if(ratio74(term,square,SCALE74,&term))return 1;}
    *out=2*sum+shift*693147;return 0;
}
static void calculate74(app7_view *v,unsigned key){
    if(key=='C'||key=='c'){v->number=v->operand=0;v->operation=v->entered=v->calc_error=0;format74(v);return;}
    if((key>='0'&&key<='9')||key=='.'){
        if(!v->entered){v->length=v->cursor=0;v->text[0]=0;v->number=0;v->entered=1;v->calc_error=0;}
        if(key=='.'){for(unsigned i=0;i<v->length;++i)if(v->text[i]=='.')return;if(!v->length)v->text[v->length++]='0';}
        if(v->length>=24){v->calc_error=1;return;}v->text[v->length++]=(char)key;v->text[v->length]=0;v->cursor=v->length;v->calc_error=(unsigned)parse74(v);return;
    }
    if(key==8){if(v->length)--v->length;v->text[v->length]=0;v->cursor=v->length;v->calc_error=(unsigned)parse74(v);v->entered=1;return;}
    int64_t result=v->number;int error=0;
    if(key=='='){
        if(v->operation=='+')error=__builtin_add_overflow(v->operand,v->number,&result);
        else if(v->operation=='-')error=__builtin_sub_overflow(v->operand,v->number,&result);
        else if(v->operation=='*')error=ratio74(v->operand,v->number,SCALE74,&result);
        else if(v->operation=='/')error=ratio74(v->operand,SCALE74,v->number,&result);
        else return;v->operation=0;
    }else if(key=='+'||key=='-'||key=='*'||key=='/'){
        if(v->operation&&v->entered){calculate74(v,'=');if(v->calc_error)return;}
        v->operand=v->number;v->operation=key;v->entered=0;return;
    }
    else if(key=='n')error=__builtin_sub_overflow((int64_t)0,v->number,&result);
    else if(key=='q')error=ratio74(v->number,v->number,SCALE74,&result);
    else if(key=='r')error=root74(v->number,&result);
    else if(key=='i')error=ratio74(SCALE74,SCALE74,v->number,&result);
    else if(key=='%')result=v->number/100;
    else if(key=='s')error=sine74(v->number,&result);
    else if(key=='o'){int64_t degree=v->number%(360*SCALE74);error=sine74(degree+90*SCALE74,&result);}
    else if(key=='l')error=logarithm74(v->number,&result);
    else if(key=='k')error=ratio74(v->number,1000000,1609344,&result);
    else if(key=='m')error=ratio74(v->number,1609344,1000000,&result);
    else if(key=='g')error=ratio74(v->number,100000000,45359237,&result);
    else if(key=='b')error=ratio74(v->number,45359237,100000000,&result);
    else if(key=='f'){error=ratio74(v->number,9,5,&result);if(!error)error=__builtin_add_overflow(result,32*SCALE74,&result);}
    else if(key=='t'){int64_t offset;error=__builtin_sub_overflow(v->number,32*SCALE74,&offset);if(!error)error=ratio74(offset,5,9,&result);}
    else return;
    v->calc_error=(unsigned)error;if(!error){v->number=result;format74(v);}v->entered=0;
}
