// src/serial.c
#include "serial.h"
#include "io.h"     // For inb, outb functions
#include "string.h" // For strlen (needed by serial_printf -> serial_write)
#include <stdarg.h> // For va_list, va_start, va_arg, va_end (for serial_printf)

// I/O ports, relative to the base port for a specific COM port
#define SERIAL_DATA_PORT(base) (base)
#define SERIAL_FIFO_COMMAND_PORT(base) (base + 2)
#define SERIAL_LINE_COMMAND_PORT(base) (base + 3)
#define SERIAL_MODEM_COMMAND_PORT(base) (base + 4)
#define SERIAL_LINE_STATUS_PORT(base) (base + 5)

// Line Status Register Flags
#define SERIAL_LINE_STATUS_DR 0x01   // Data Ready
#define SERIAL_LINE_STATUS_THRE 0x20 // Transmitter Holding Register Empty

// Line Command Register Flags
#define SERIAL_LINE_ENABLE_DLAB 0x80 // Enable Divisor Latch Access Bit

// Internal function to configure baud rate
static void serial_configure_baud_rate(unsigned short com, unsigned short divisor)
{
    outb(SERIAL_LINE_COMMAND_PORT(com), SERIAL_LINE_ENABLE_DLAB);
    outb(SERIAL_DATA_PORT(com), (divisor >> 8) & 0x00FF); // Send high byte
    outb(SERIAL_DATA_PORT(com), divisor & 0x00FF);        // Send low byte
}

// Internal function to configure line parameters (8 bits, no parity, 1 stop bit - 8N1)
static void serial_configure_line(unsigned short com)
{
    /* Line control register layout:
     * Bit 0, 1: Data length (11 = 8 bits)
     * Bit 2: Stop bits (0 = 1 stop bit)
     * Bit 3, 4, 5: Parity (000 = No parity)
     * Bit 6: Break control (0 = disabled)
     * Bit 7: DLAB (0 = disabled after setting baud rate)
     * Value: 00000011 = 0x03 for 8N1
     */
    outb(SERIAL_LINE_COMMAND_PORT(com), 0x03);
}

// Internal function to configure FIFO buffer (Enable, clear, 14-byte threshold)
static void serial_configure_fifo(unsigned short com)
{
    /* FIFO control register layout:
     * Bit 0: Enable FIFO (1 = enabled)
     * Bit 1: Clear receive FIFO (1 = clear)
     * Bit 2: Clear transmit FIFO (1 = clear)
     * Bit 3: DMA mode select (0 = mode 0)
     * Bit 6, 7: Trigger level (11 = 14 bytes)
     * Value: 11000111 = 0xC7
     */
    outb(SERIAL_FIFO_COMMAND_PORT(com), 0xC7);
}

// Internal function to configure modem control (Set DTR and RTS)
static void serial_configure_modem(unsigned short com)
{
    /* Modem control register layout:
     * Bit 0: Data Terminal Ready (DTR) (1 = set)
     * Bit 1: Request To Send (RTS) (1 = set)
     * Bit 3: Auxiliary Output 1 (OUT1) (0 = disabled)
     * Bit 4: Loopback mode (0 = disabled)
     * Value: 00000011 = 0x03 (Set DTR and RTS, disable loopback and OUT1)
     */
    outb(SERIAL_MODEM_COMMAND_PORT(com), 0x03);
}

// Check if the transmit FIFO is empty
static int serial_is_transmit_fifo_empty(unsigned int com)
{
    // Check bit 5 (Transmitter holding register empty) of the line status register
    return inb(SERIAL_LINE_STATUS_PORT(com)) & SERIAL_LINE_STATUS_THRE;
}

// Write a single character to the serial port
static void serial_write_char(char c)
{
    // Wait until the transmit FIFO is empty before sending
    while (serial_is_transmit_fifo_empty(SERIAL_COM1_BASE) == 0)
        ;
    // Send the character
    outb(SERIAL_DATA_PORT(SERIAL_COM1_BASE), c);
}

// Public function to configure COM1
void serial_configure()
{
    serial_configure_baud_rate(SERIAL_COM1_BASE, 3); // Baud rate: 115200 / 3 = 38400 bps
    serial_configure_line(SERIAL_COM1_BASE);
    serial_configure_fifo(SERIAL_COM1_BASE);
    serial_configure_modem(SERIAL_COM1_BASE);
    // Maybe add a small test write here if desired
    // serial_write("Serial COM1 Configured.\r\n", 25);
}

// Public function to write a buffer to COM1
int serial_write(const char *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; ++i)
    {
        serial_write_char(buf[i]);
        // If we want to automatically convert LF to CRLF for serial terminals:
        // if (buf[i] == '\n') {
        //     serial_write_char('\r');
        // }
    }
    return len;
}

// --- Basic printf implementation for serial ---

// Helper to print numbers (unsigned decimal) to serial
static void serial_print_udec(uint32_t num)
{
    char buffer[12]; // Max 10 digits for uint32 + null
    int i = 10;
    buffer[11] = '\0';

    if (num == 0)
    {
        serial_write_char('0');
        return;
    }

    while (num > 0)
    {
        buffer[i--] = (num % 10) + '0';
        num /= 10;
    }
    serial_write(&buffer[i + 1], 10 - i); // Write the number part
}

// Helper to print numbers (signed decimal) to serial
static void serial_print_sdec(int32_t num)
{
    if (num < 0)
    {
        serial_write_char('-');
        serial_print_udec((uint32_t)(-num)); // Print absolute value as unsigned
    }
    else
    {
        serial_print_udec((uint32_t)num);
    }
}

// Helper to print numbers (hexadecimal) to serial
static void serial_print_hex(uint32_t num)
{
    char buffer[11]; // "0x" + 8 hex digits + null
    int i = 9;
    buffer[10] = '\0';

    if (num == 0)
    {
        serial_write("0x0", 3);
        return;
    }

    while (num > 0)
    {
        uint8_t rem = num % 16;
        buffer[i--] = (rem < 10) ? (rem + '0') : (rem - 10 + 'A'); // Use uppercase A-F
        num /= 16;
    }
    buffer[i--] = 'x';
    buffer[i] = '0';

    serial_write(&buffer[i], 10 - i); // Write the constructed hex string
}

// Simplified printf for serial
void serial_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    while (*format != '\0')
    {
        if (*format == '%')
        {
            format++; // Move past '%'
            if (*format == 's')
            {
                const char *str = va_arg(args, const char *);
                if (str == NULL)
                {
                    serial_write("(null)", 6);
                }
                else
                {
                    serial_write(str, strlen(str)); // Uses strlen from string.c
                }
            }
            else if (*format == 'c')
            {
                char c = (char)va_arg(args, int); // Chars are promoted to int in va_arg
                serial_write_char(c);
            }
            else if (*format == 'd' || *format == 'i')
            { // Signed decimal integer
                int d = va_arg(args, int);
                serial_print_sdec(d);
            }
            else if (*format == 'u')
            { // Unsigned decimal integer
                uint32_t u = va_arg(args, uint32_t);
                serial_print_udec(u);
            }
            else if (*format == 'x' || *format == 'X')
            { // Hexadecimal integer
                uint32_t x = va_arg(args, uint32_t);
                serial_print_hex(x);
            }
            else if (*format == '%')
            { // Literal '%'
                serial_write_char('%');
            }
            else
            { // Unknown format specifier - print literally
                serial_write_char('%');
                serial_write_char(*format);
            }
        }
        else
        {
            serial_write_char(*format);
            // Convert LF to CRLF if needed
            // if (*format == '\n') {
            //    serial_write_char('\r');
            // }
        }
        format++;
    }

    va_end(args);
}