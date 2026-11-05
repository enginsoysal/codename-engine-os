/* M0 environment fixture only. Does not load a kernel or install an OS. */
typedef __UINTPTR_TYPE__ status;
static void out8(unsigned short port, unsigned char value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}
static unsigned char in8(unsigned short port) {
    unsigned char value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}
status efi_main(void *image, void *table) {
    (void)image; (void)table;
    out8(0x3f9, 0); out8(0x3fb, 0x80); out8(0x3f8, 1);
    out8(0x3f9, 0); out8(0x3fb, 3); out8(0x3fa, 0xc7); out8(0x3fc, 0x0b);
    const char *text = "ENGINE_OS_M0_ENVIRONMENT_OK\r\nNOT_A_KERNEL_OR_INSTALLER\r\n";
    for (; *text; ++text) {
        unsigned int retry = 100000;
        while (!(in8(0x3fd) & 0x20) && --retry) {}
        out8(0x3f8, (unsigned char)*text);
    }
    __asm__ volatile ("outl %0, %1" : : "a"(0x10u), "Nd"((unsigned short)0xf4));
    return 0;
}
