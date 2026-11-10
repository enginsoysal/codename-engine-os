#include <engine/hid10.h>
#include <engine/hardware10.h>
static unsigned events10;
void input_feed(uint8_t value){if(value==138)++events10;}
__declspec(dllexport) unsigned wheel_events10(void){return events10;}
__declspec(dllexport) unsigned descriptor10(const uint8_t*p,unsigned n){return hid10_boot_wheel(p,n);}
