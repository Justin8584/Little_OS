// include/serial.h
#ifndef INCLUDE_SERIAL_H
#define INCLUDE_SERIAL_H

#include "common.h" // Include common types like uint32_t, uint8_t

// Define the base I/O port for the first serial port (COM1)
#define SERIAL_COM1_BASE 0x3F8

/** serial_configure:
 *  Configures the serial port COM1 with default settings (baud rate, 8N1).
 */
void serial_configure();

/** serial_write:
 *  Writes the contents of the buffer buf of length len to serial port COM1.
 *  Waits for the port to be ready before sending each byte.
 *
 *  @param buf Buffer containing characters to write
 *  @param len Length of the buffer
 *  @return Number of bytes written (currently returns len)
 */
int serial_write(const char *buf, uint32_t len);

/** serial_printf:
 *  A very basic printf implementation for the serial port.
 *  Supports %s (string), %c (char), %d (decimal int), %x (hexadecimal int), %%.
 */
void serial_printf(const char *format, ...);

#endif // INCLUDE_SERIAL_H