#include <engine/network/clock.h>
unsigned clock7_calendar(unsigned year,unsigned month,unsigned day,unsigned hour,unsigned minute,unsigned second,clock7_time *out){
    static const unsigned lengths[]={31,28,31,30,31,30,31,31,30,31,30,31};
    if(!out||year<2020||year>2099||month<1||month>12||!day||hour>23||minute>59||second>59)return 1;
    unsigned leap=!(year%4)&&(year%100||!(year%400));
    if(day>lengths[month-1]+(month==2&&leap))return 1;
    /* Year zero is a leap year; match BearSSL's proleptic Gregorian epoch. */
    unsigned days=year*365+(year+3)/4-(year+99)/100+(year+399)/400;
    for(unsigned m=1;m<month;++m)days+=lengths[m-1]+(m==2&&leap);
    *out=(clock7_time){year,month,day,hour,minute,second,days+day-1,hour*3600+minute*60+second};return 0;
}
