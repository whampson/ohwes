/*============================================================================*
 * Copyright (C) 2020-2021 Wes Hampson. All Rights Reserved.                  *
 *                                                                            *
 * This file is part of the OHWES Operating System.                           *
 * OHWES is free software; you may redistribute it and/or modify it under the *
 * terms of the license agreement provided with this software.                *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
 *============================================================================*
 *    File: include/drivers/ps2.h                                             *
 * Created: December 24, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *                                                                            *
 * Intel 8042 PS/2 Controller driver.                                         *
 *============================================================================*/

#ifndef __PS2_H
#define __PS2_H

#include <stdint.h>

/* I/O Ports */
#define PS2_PORT_DATA           0x60    /* Data Register, Read/Write */
#define PS2_PORT_CMD            0x64    /* Command Register, Write-Only */
#define PS2_PORT_STS            0x64    /* Status Register, Read-Only */

/* PS/2 Controller Commands */
#define PS2_CMD_RDCFG           0x20    /* Read Controller Configuration Register */
#define PS2_CMD_WRCFG           0x60    /* Write Controller Configuration Regiter */
#define PS2_CMD_RDOUT           0xD0    /* Read Controller Output Register */
#define PS2_CMD_WROUT           0xD1    /* Write Controller Output Register */
#define PS2_CMD_TEST            0xAA    /* Test PS/2 Controller */
#define PS2_CMD_P1OFF           0xAD    /* Disable First Device Port */
#define PS2_CMD_P1ON            0xAE    /* Enable First Device Port */
#define PS2_CMD_P1TEST          0xAB    /* Test First Device Port */
#define PS2_CMD_P2OFF           0xA7    /* Disable Second Device Port */
#define PS2_CMD_P2ON            0xA8    /* Enable Second Device Port */
#define PS2_CMD_P2TEST          0xA9    /* Test Second Device Port */
#define PS2_CMD_SYSRESET        0xFE    /* Reset the System */

/* Test Result Values */
#define PS2_TEST_PASS           0x55    /* Controller Test Pass */
#define PS2_TEST_FAIL           0xFC    /* Controller Test Fail */
#define PS2_P1TEST_PASS         0x00    /* Port 1 Test Pass */
#define PS2_P2TEST_PASS         0x00    /* Port 2 Test Pass */

/* Status Register Fields */
#define PS2_STS_OUTPUT          (1<<0)  /* Output Buffer Status (1 = full) */
#define PS2_STS_INPUT           (1<<1)  /* Input Buffer Status (1 = full) */
#define PS2_STS_POST            (1<<2)  /* System Passed POST */
#define PS2_STS_TIMEOUT         (1<<6)  /* Timeout Error */
#define PS2_STS_PARITY          (1<<7)  /* Parity Error */

/* Controller Configuration Register Fields */
#define PS2_CFG_P1INTON         (1<<0)  /* Interrupt on First Device Port */
#define PS2_CFG_P2INTON         (1<<1)  /* Interrupt on Second Device Port */
#define PS2_CFG_POST            (1<<2)  /* System Passed POST */
#define PS2_CFG_P1CLKOFF        (1<<4)  /* First Device Port Clock Off */
#define PS2_CFG_P2CLKOFF        (1<<5)  /* Second Device Port Clock Off */
#define PS2_CFG_XLATON          (1<<6)  /* Keyboard Scan Code Translation */

/* Controller Output Register Fields */
#define PS2_OUT_SYSON           (1<<0)  /* System Reset Flag, must be set */
#define PS2_OUT_A20             (1<<1)  /* Address Line 20, must be set */
#define PS2_OUT_P2CLK           (1<<2)  /* Second Device Port Clock (output) */
#define PS2_OUT_P2DAT           (1<<3)  /* Second Device Port Data (output) */
#define PS2_OUT_P1INT           (1<<4)  /* First Device Port Interrupt (IRQ1) */
#define PS2_OUT_P2INT           (1<<5)  /* Second Debice Port Interrupt (IRQ12) */
#define PS2_OUT_P1CLK           (1<<6)  /* First Device Port Clock (output) */
#define PS2_OUT_P1DAT           (1<<7)  /* First Device Port Data (output) */

/**
 * Initializes the PS/2 Controller.
 */
void ps2_init(void);

/**
 * Flushes the PS/2 Controller's output buffer.
 * Any bytes read-in from the buffer will be discarded.
 */
void ps2_flush(void);

/**
 * Reads the PS/2 Controller's Status Register.
 *
 * @return status register contents (use PS2_STS_* to check fields)
 */
uint8_t ps2_status(void);

/**
 * Issues a command to the PS/2 Controller.
 * WARNING: this function will block until the controller is ready to accept
 * another byte.
 *
 * @param cmd the command to send (one of PS2_CMD_*)
 */
void ps2_cmd(uint8_t cmd);

/**
 * Reads a byte from the PS/2 Controller's Data Register.
 * WARNING: this function will block until there is a byte available to read.
 *
 * @return data from the controller's output buffer
 */
uint8_t ps2_inb(void);

/**
 * Writes a byte to the PS/2 Controller's Data Register.
 * WARNING: this function will block until the controller is ready to accept
 * another byte.
 *
 * @param data the data to write to the controller's input buffer
 */
void ps2_outb(uint8_t data);


#endif /* __PS2_H */
