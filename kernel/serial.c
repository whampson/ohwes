#include <errno.h>
#include <ohwes/serial.h>
#include <ohwes/io.h>
#include <ohwes/irq.h>

#define COM1            0x3F8

#define PORT_DATA       0
#define PORT_DIVISOR_LO 0
#define PORT_DIVISOR_HI 1

#define PORT_LINECTL    3
#define PORT_MODEMCTL   4
#define PORT_LINESTAT   5
#define PORT_MODEMSTAT  6
#define PORT_SCRATCH    7

static bool rx_ready(void);
static bool tx_ready(void);

void serial_init(void)
{
    // TODO: clean this up

    outb(COM1 + 1, 0x00);    // Disable all interrupts
    outb(COM1 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(COM1 + 0, 12);     // Set divisor to 12 (lo byte) 9600 baud
    outb(COM1 + 1, 0x00);    //                  (hi byte)
    outb(COM1 + 3, 0x03);    // DLAB off, 8 bits, no parity, one stop bit
    outb(COM1 + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold ????
    outb(COM1 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    // outb(COM1 + 4, 0x1E);    // Set in loopback mode, test the serial chip
    // outb(COM1 + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)

    // Check if serial is faulty (i.e: not same byte as sent)
    // if(inb(COM1 + 0) != 0xAE) {
    //     panic("COM1 is fucked up!");
    // }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    outb(COM1 + 4, 0x0F);

    irq_unmask(IRQ_COM1);
}

int serial_read(char *c)
{
    if (!rx_ready()) {
        return -EAGAIN;
    }
    return *c = inb(COM1);
}

int serial_write(char c)
{
    if (!tx_ready()) {
        return -EAGAIN;
    }
    outb(COM1, c);
    return 0;
}

static bool rx_ready(void)
{
    return inb(COM1 + 5) & 0x01;
}

static bool tx_ready(void)
{
    return inb(COM1 + 5) & 0x20;
}
