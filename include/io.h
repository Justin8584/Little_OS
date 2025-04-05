// io.h - Basic I/O functions declarations
#ifndef IO_H
#define IO_H

// Output a byte (8 bits) to an I/O port
void outb(unsigned short port, unsigned char data);

// Input a byte (8 bits) from an I/O port
unsigned char inb(unsigned short port);

#endif // IO_H