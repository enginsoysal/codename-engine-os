#include <engine/desktop/interaction11.h>
unsigned interaction11_checks(void){
 i11_state s={0};i11_target a={17,1000,I11_ITEM},b={17,1001,I11_ITEM},button={17,1,I11_BUTTON};unsigned n=0;
#define CHECK(x) do{++n;if(!(x))return n;}while(0)
 CHECK(!i11_pointer(&s,a,10,10,1,0));CHECK(i11_pointer(&s,a,10,10,0,10)==I11_SELECT);
 CHECK(!i11_pointer(&s,a,10,10,1,100));CHECK(i11_pointer(&s,a,10,10,0,150)==(I11_SELECT|I11_ACTIVATE));
 CHECK(!i11_pointer(&s,a,10,10,1,160));CHECK(i11_pointer(&s,a,10,10,0,170)==I11_SELECT);
 CHECK(!i11_pointer(&s,b,10,10,1,200));CHECK(i11_pointer(&s,b,10,10,0,210)==I11_SELECT);
 CHECK(!i11_pointer(&s,b,10,10,1,800));CHECK(i11_pointer(&s,b,10,10,0,810)==I11_SELECT);
 CHECK(!i11_pointer(&s,b,10,10,2,900));CHECK(i11_pointer(&s,b,10,10,0,910)==I11_CONTEXT);
 CHECK(!i11_pointer(&s,a,10,10,1,1000));CHECK(i11_pointer(&s,a,15,10,1,1010)==I11_DRAG);CHECK(i11_pointer(&s,a,10,10,0,1020)==I11_CANCEL);
 CHECK(!i11_pointer(&s,button,10,10,1,1100));CHECK(i11_pointer(&s,b,10,10,0,1110)==I11_CANCEL);
 CHECK(!i11_pointer(&s,button,10,10,1,1200));CHECK(i11_pointer(&s,button,10,10,0,1210)==I11_ACTIVATE);
 CHECK(!i11_pointer(&s,a,10,10,1,1300));i11_cancel(&s);CHECK(!i11_pointer(&s,a,10,10,0,1310));
 s.double_ms=200;CHECK(!i11_pointer(&s,a,10,10,1,1400));CHECK(i11_pointer(&s,a,10,10,0,1410)==I11_SELECT);
 CHECK(!i11_pointer(&s,a,10,10,1,1700));CHECK(i11_pointer(&s,a,10,10,0,1710)==I11_SELECT);
 CHECK(!i11_far(10,10,14,14));CHECK(i11_far(10,10,15,10));
 i11_cancel(&s);s.buttons=0;s.target_identity=41;
 CHECK(!i11_pointer(&s,a,10,10,1,1800));s.target_identity=42;
 CHECK(i11_pointer(&s,a,10,10,0,1810)==I11_CANCEL);
 CHECK(!i11_pointer(&s,a,10,10,1,1900));CHECK(i11_pointer(&s,a,10,10,0,1910)==I11_SELECT);
 s.target_identity=43;CHECK(!i11_pointer(&s,a,10,10,1,1950));CHECK(i11_pointer(&s,a,10,10,0,1960)==I11_SELECT);
 i11_cancel(&s);s.buttons=0;
 CHECK(!i11_pointer(&s,a,10,10,1,2000));CHECK(i11_pointer(&s,a,10,10,3,2010)==I11_CANCEL);
 CHECK(!i11_pointer(&s,a,10,10,2,2020));CHECK(!i11_pointer(&s,a,10,10,0,2030));
 CHECK(!i11_pointer(&s,a,10,10,2,2100));CHECK(i11_pointer(&s,a,10,10,3,2110)==I11_CANCEL);CHECK(!i11_pointer(&s,a,10,10,0,2120));
 return 0;
}
