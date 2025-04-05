/* link.ld - Linker script for Simple OS */
ENTRY(loader) /* Define the entry point symbol */

SECTIONS {
    /* Start placing code/data at 1MB physical address */
    /* This is where GRUB loads the kernel */
    . = 0x00100000;

    /* Text section (code) */
    .text ALIGN (0x1000) : { /* Align section start to 4KB page boundary */
        *(.text)            /* Include all .text sections from input object files */
    }

    /* Read-only data section */
    .rodata ALIGN (0x1000) : {
        *(.rodata*)         /* Include all .rodata and .rodata.* sections */
    }

    /* Initialized data section */
    .data ALIGN (0x1000) : {
        *(.data)            /* Include all .data sections */
    }

    /* Uninitialized data section (stack, etc.) */
    .bss ALIGN (0x1000) : {
        *(COMMON)           /* Include common symbols (uninitialized globals) */
        *(.bss)             /* Include all .bss sections */
    }

    /* Optional: Add symbol to mark the end of the kernel image */
    /* kernel_end = .; */
}