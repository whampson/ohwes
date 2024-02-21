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
 * Intel 8042 PS/2 Controller and PS/2 Keyboard driver.                       *
 *============================================================================*/

#ifndef __PS2_H
#define __PS2_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* I/O Ports */
#define PS2_PORT_DATA           0x60    /* Data Register, Read/Write */
#define PS2_PORT_CMD            0x64    /* Command Register, Write-Only */
#define PS2_PORT_STATUS            0x64    /* Status Register, Read-Only */

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

/* Controller Response Values */
#define PS2_RESP_PASS            0x55    /* Controller Self-Test Pass */
#define PS2_RESP_FAIL            0xFC    /* Controller Self-Test Fail */
#define PS2_RESP_P1PASS          0x00    /* Port 1 Self-Test Pass */
#define PS2_RESP_P2PASS          0x00    /* Port 2 Self-Test Pass */

/* Controller Status Register Fields */
#define PS2_STATUS_OUTPUT          (1<<0)  /* Output Buffer Status (1 = full) */
#define PS2_STATUS_INPUT           (1<<1)  /* Input Buffer Status (1 = full) */
#define PS2_STATUS_POST            (1<<2)  /* System Passed POST */
#define PS2_STATUS_TIMEOUT         (1<<6)  /* Timeout Error */
#define PS2_STATUS_PARITY          (1<<7)  /* Parity Error */

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
#define PS2_OUT_P2INT           (1<<5)  /* Second Device Port Interrupt (IRQ12) */
#define PS2_OUT_P1CLK           (1<<6)  /* First Device Port Clock (output) */
#define PS2_OUT_P1DAT           (1<<7)  /* First Device Port Data (output) */

/* Keyboard Commands */
#define PS2KBD_CMD_SETLED          0xED    /* Set ScrLk, CapsLk, and NumLk LEDs */
#define PS2KBD_CMD_SCANCODE        0xF0    /* Set Scancode Mapping */
#define PS2KBD_CMD_SCANON          0xF4    /* Enable scanning */
#define PS2KBD_CMD_SCANOFF         0xF5    /* Disable scanning */
#define PS2KBD_CMD_DEFAULTS        0xF6    /* Set keyboard defaults */
#define PS2KBD_CMD_ALL_TR          0xF7    /* Set all keys to typematic/autorepeat only (scancode 3) */
#define PS2KBD_CMD_ALL_MB          0xF8    /* Set all keys to make/break only (scancode 3) */
#define PS2KBD_CMD_ALL_M           0xF9    /* Set all keys to make only (scancode 3) */
#define PS2KBD_CMD_ALL_MBTR        0xFA    /* Set all keys to make/break/typematic/autorepeat (scancode 3) */
#define PS2KBD_CMD_KEY_TR          0xFB    /* Set specific key to typematic/autorepeat only (scancode 3) */
#define PS2KBD_CMD_KEY_MB          0xFC    /* Set specific key to make/break only (scancode 3) */
#define PS2KBD_CMD_KEY_M           0xFD    /* Set specific key to make only (scancode 3) */
#define PS2KBD_CMD_SELFTEST        0xFF    /* Run self-test */

/* Keyboard LED masks */
#define PS2KBD_LED_SCRLK           (1<<0)  /* Scroll Lock Light */
#define PS2KBD_LED_NUMLK           (1<<1)  /* Num Lock Light */
#define PS2KBD_LED_CAPLK           (1<<2)  /* Caps Lock Light */

/* Keyboard Command Responses */
#define KBD_RESP_PASS            0xAA    /* Self-Test Passed */
#define KBD_RESP_ACK             0xFA    /* Data Received */
#define KBD_RESP_RESEND          0xFE    /* Data Not Received, Resend */


/**
 * Flushes the PS/2 Controller's output buffer.
 * Any bytes read-in from the buffer will be discarded.
 */
void ps2_flush(void);

/**
 * Reads the PS/2 Controller's Status Register.
 *
 * @return status register contents (use PS2_STATUS_* to check fields)
 */
uint8_t ps2_status(void);

/**
 * Tests PS/2 Controller.
 *
 * @return true if the test passed, false if the test failed
 */
bool ps2_test(void);

/**
 * Tests Port 1 of the PS/2 Controller.
 *
 * @return true if the test passed, false if the test failed
 */
bool ps2_testp1(void);

/**
 * Tests Port 2 of the PS/2 Controller.
 *
 * @return true if the test passed, false if the test failed
 */
bool ps2_testp2(void);

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
 * Use ps2_canread() to check whether the controller is ready for reading.
 *
 * @return data from the controller's output buffer
 */
uint8_t ps2_read(void);

/**
 * Checks whether the PS/2 controller has byte available to read.
 *
 * @return true if the PS/2 controller's output buffer is full
 */
bool ps2_canread(void);

/**
 * Writes a byte to the PS/2 Controller's Data Register.
 * WARNING: this function will block until the controller is ready to accept
 * another byte. Use ps2_canwrite() to check whether the controller is ready for
 * writing.
 *
 * @param data the data to write to the controller's input buffer
 */
void ps2_write(uint8_t data);

/**
 * Checks whether the PS/2 controller is ready to accept another byte.
 *
 * @return true if the PS/2 controller's input buffer is empty
 */
bool ps2_canwrite(void);

/**
 * Allows the controller to receive interrupts from the PS/2 keyboard device.
 */
void ps2kbd_on(void);

/**
 * Tests the keyboard device.
 *
 * @return true if the test passed, false if the test failed
 */
bool ps2kbd_test(void);

/**
 * Issues a command to the keyboard device.
 *
 * @param cmd the command to issue (one of PS2KBD_CMD_*)
 * @param data the command data, if required
 * @param n the number of bytes in 'data' to transmit
 * @return 0 on success, nonzero value if the command didn't complete as
 *         expected (keyboard response byte), -1 if the command timed out
 */
int ps2kbd_cmd(uint8_t cmd, uint8_t *data, size_t n);

#endif /* __PS2_H */
