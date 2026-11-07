/* Reuse the protected M7.1 service buffers; never borrow the active V6 volume. */
unsigned disk74_select(unsigned disk){
    u71_disk=disk;u71_notice=0;u71_refresh();return storage_get(disk)?0:V6_NOTFOUND;
}
const d71_disk *disk74_state(void){return &u71_state;}
const d71_plan *disk74_plan(void){return &u71_plan;}
unsigned disk74_prepare(unsigned op,unsigned index,uint64_t bytes,const char *name){
    if(u71_disk==system_disk74())return D71_PROTECTED;
    uint8_t nonce[16];uint64_t tick=m3_clock_ticks();memcpy(nonce,&tick,8);memcpy(nonce+8,&u71_state.fingerprint,4);memcpy(nonce+12,&op,4);
    return d71_prepare(&u71_device,&u71_state,op,index,bytes,name,nonce,&u71_plan);
}
unsigned disk74_apply(void){
    if(u71_disk==system_disk74())return D71_PROTECTED;
    unsigned status=d71_apply(&u71_device,&u71_state,&u71_plan,m71_workspace());u71_refresh();return status;
}
