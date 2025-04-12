/* link.ld - Linker script for Little OS */
ENTRY(loader) /* entry point */

SECTIONS {
    /* Start placing code/data at 1MB physical address */

    . = 0x00100000;

    /* Text section (code) */
    .text ALIGN (0x1000) : {
        *(.text)           
    }

    /* Read-only data section */
    .rodata ALIGN (0x1000) : {
        *(.rodata*)        
    }

    /* Initialized data section */
    .data ALIGN (0x1000) : {
        *(.data)            
    }

    /* Uninitialized data section (stack, etc.) */
    .bss ALIGN (0x1000) : {
        *(COMMON)          
        *(.bss)            
    }

    kernel_end = .;
}