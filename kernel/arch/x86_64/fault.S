.text
.globl kernel_fault_probe
.type kernel_fault_probe,@function
kernel_fault_probe:
    ud2
    ret
.size kernel_fault_probe,.-kernel_fault_probe
.section .note.GNU-stack,"",@progbits
