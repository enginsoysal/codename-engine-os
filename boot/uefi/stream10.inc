/* Read validated ELF segments directly into their reserved physical pages.
 * This unsigned M10 path avoids a second contiguous whole-kernel allocation. */
#ifdef ENGINE_SECURE_PAYLOAD
#error M10 streaming needs a streaming payload verifier before owner-signed qualification
#endif
static void kernel_stream10(efi_file*root,elf_plan*plan){
    efi_file*file=0;
    if(EFI_ERROR(root->open(root,&file,(uint16_t*)u"\\KERNEL.ELF",1,0))||!file)fail("open kernel file");
    _Alignas(8) uint8_t info[1024];uint64_t info_size=sizeof(info);
    if(EFI_ERROR(file->get_info(file,&info_guid,&info_size,info))||info_size<80)fail("kernel file info");
    uint64_t size=os_read64(info+8);if(size<64||size>12*1024*1024)fail("kernel file size");
    _Alignas(8) static uint8_t prefix[4096];uint64_t count=size<sizeof(prefix)?size:sizeof(prefix),read=count;
    if(EFI_ERROR(file->read(file,&read,prefix))||read!=count)fail("kernel header read");
    elf_header header;memcpy(&header,prefix,sizeof(header));
    /* elf_validate reads only ELF/program headers, never payload bytes. Bound
     * those accesses to the actual prefix before supplying the full file size. */
    if(header.phoff>count||(uint64_t)header.phnum*56>count-header.phoff||elf_validate(prefix,size,plan))fail("ELF validation");
    uint64_t address=plan->low,pages=(plan->high-plan->low)/4096;
    if(EFI_ERROR(bs->allocate_pages(2,1,pages,&address)))fail("kernel fixed allocation");
    reserve(address,pages*4096);memset((void*)(uintptr_t)address,0,(size_t)(pages*4096));
    typedef efi_status(*seek10)(efi_file*,uint64_t);
    if(!file->set_position)fail("kernel file seek unavailable");
    for(unsigned i=0;i<header.phnum;++i){elf_segment segment;memcpy(&segment,prefix+header.phoff+(uint64_t)i*56,sizeof(segment));
        if(segment.type!=1)continue;
        if(EFI_ERROR(((seek10)file->set_position)(file,segment.offset)))fail("kernel segment seek");
        for(uint64_t at=0;at<segment.filesz;){uint64_t bytes=segment.filesz-at;if(bytes>4096)bytes=4096;uint64_t actual=bytes;
            if(EFI_ERROR(file->read(file,&actual,(void*)(uintptr_t)(segment.paddr+at)))||actual!=bytes)fail("kernel segment read");at+=bytes;
        }
    }
    if(EFI_ERROR(file->close(file)))fail("close kernel file");
    serial_text("M10_KERNEL_STREAM_LOADED whole_file_copy_bytes=0\n");
}
