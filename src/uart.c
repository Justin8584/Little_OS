// src/uart.c
#include "uart.h"
#include "string.h"

// QEMU UART address for Raspberry Pi model
#define UART0_BASE 0x3F201000

// Register offsets
#define UART0_DR (UART0_BASE + 0x00)
#define UART0_FR (UART0_BASE + 0x18)
#define UART0_IBRD (UART0_BASE + 0x24)
#define UART0_FBRD (UART0_BASE + 0x28)
#define UART0_LCRH (UART0_BASE + 0x2C)
#define UART0_CR (UART0_BASE + 0x30)
#define UART0_IMSC (UART0_BASE + 0x38)
#define UART0_ICR (UART0_BASE + 0x44)

// Helper macros for memory-mapped IO
#define mmio_write(addr, val) (*(volatile unsigned int *)(addr) = (val))
#define mmio_read(addr) (*(volatile unsigned int *)(addr))

// Initialize UART for console I/O
void uart_init(void)
{
    // Disable UART0
    mmio_write(UART0_CR, 0x00000000);

    // Clear pending interrupts
    mmio_write(UART0_ICR, 0x7FF);

    // Set baud rate - 115200 at 48MHz
    mmio_write(UART0_IBRD, 26);
    mmio_write(UART0_FBRD, 3);

    // 8 bits, no parity, one stop bit, FIFO enabled
    mmio_write(UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6));

    // Enable UART0, rx and tx
    mmio_write(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));
}

// Send a character
void uart_putc(unsigned char c)
{
    // Wait for UART to become ready
    while (mmio_read(UART0_FR) & (1 << 5))
        ;
    mmio_write(UART0_DR, c);
}

// Receive a character
unsigned char uart_getc(void)
{
    // Wait for UART to have data
    while (mmio_read(UART0_FR) & (1 << 4))
        ;
    return mmio_read(UART0_DR);
}

// Output a string
void uart_puts(const char *str)
{
    for (size_t i = 0; str[i] != '\0'; i++)
        uart_putc((unsigned char)str[i]);
}