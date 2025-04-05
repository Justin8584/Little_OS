; io.s - Implementation of inb and outb

global outb         ; Make outb visible to the linker
global inb         ; Make inb visible to the linker

section .text

; outb - send a byte to an I/O port
; Expects:
;   [esp + 8]: data byte (unsigned char)
;   [esp + 4]: port number (unsigned short)
;   [esp    ]: return address
outb:
    mov al, [esp + 8]   ; Get the data byte from the stack (argument 2)
    mov dx, [esp + 4]   ; Get the port number from the stack (argument 1)
    out dx, al          ; Execute the out instruction: out port (dx), data (al)
    ret                 ; Return to the caller

; inb - read a byte from an I/O port
; Expects:
;   [esp + 4]: port number (unsigned short)
;   [esp    ]: return address
; Returns:
;   The byte read in the AL register (which is the low byte of EAX)
inb:
    mov dx, [esp + 4]   ; Get the port number from the stack (argument 1)
    in al, dx           ; Execute the in instruction: in data (al), port (dx)
    ret                 ; Return to the caller (value is in AL/EAX)