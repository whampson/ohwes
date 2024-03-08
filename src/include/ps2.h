/*============================================================================*
 * Copyright (C) 2020-2024 Wes Hampson. All Rights Reserved.                  *
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
 *    File: include/ps2.h                                                     *
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

// PS/2 Controller I/O Ports
#define PS2_PORT_DATA           0x60    // (r/w) Data Register
#define PS2_PORT_CMD            0x64    // (w) Command Register
#define PS2_PORT_STATUS         0x64    // (r) Status Register

// PS/2 Controller Commands
#define PS2_CMD_RDCFG           0x20    // Read Controller Configuration Register
#define PS2_CMD_WRCFG           0x60    // Write Controller Configuration Register
#define PS2_CMD_RDOUT           0xD0    // Read Controller Output Register
#define PS2_CMD_WROUT           0xD1    // Write Controller Output Register
#define PS2_CMD_TEST            0xAA    // Test PS/2 Controller
#define PS2_CMD_P1OFF           0xAD    // Disable Port 1
#define PS2_CMD_P1ON            0xAE    // Enable Port 1
#define PS2_CMD_P1TEST          0xAB    // Test Port 1
#define PS2_CMD_P2OFF           0xA7    // Disable Port 2
#define PS2_CMD_P2ON            0xA8    // Enable Port 2
#define PS2_CMD_P2TEST          0xA9    // Test Port 2
#define PS2_CMD_SYSRESET        0xFE    // Reset the System

// Controller Response Values
#define PS2_RESP_PASS           0x55    // Controller Self-Test Pass
#define PS2_RESP_FAIL           0xFC    // Controller Self-Test Fail
#define PS2_RESP_P1PASS         0x00    // Port 1 Self-Test Pass
#define PS2_RESP_P2PASS         0x00    // Port 2 Self-Test Pass

// Controller Status Register Fields
#define PS2_STATUS_OPF          (1<<0)  // Output Full (1 = controller output contains data for CPU to read)
#define PS2_STATUS_IPF          (1<<1)  // Input Full (0 = controller input is empty and available for CPU to write)
#define PS2_STATUS_SYS          (1<<2)  // System Passed POST
#define PS2_STATUS_A2           (1<<3)  // Port Last Written To (0 = 0x60, 1 = 0x64)*/
#define PS2_STATUS_INH          (1<<4)  // Inhibit Keyboard
#define PS2_STATUS_MOBF         (1<<5)  // Mouse Output Buffer Full
#define PS2_STATUS_TIMEOUT      (1<<6)  // Timeout Error
#define PS2_STATUS_PARITY       (1<<7)  // Parity Error

// Controller Configuration Register Fields
#define PS2_CFG_P1INTON         (1<<0)  // Enable Port 1 Interrupt
#define PS2_CFG_P2INTON         (1<<1)  // Enable Port 2 Interrupt
#define PS2_CFG_POST            (1<<2)  // System Passed POST
#define PS2_CFG_P1CLKOFF        (1<<4)  // Disable Port 1 Clock
#define PS2_CFG_P2CLKOFF        (1<<5)  // Disable Port 2 Clock
#define PS2_CFG_TRANSLATE       (1<<6)  // Enable Scan Code Translation

// Controller Output Register Fields
#define PS2_OUT_SYSON           (1<<0)  // System Reset Flag, must be set
#define PS2_OUT_A20             (1<<1)  // Address Line 20, must be set
#define PS2_OUT_P2CLK           (1<<2)  // Second Device Port Clock (output)
#define PS2_OUT_P2DAT           (1<<3)  // Second Device Port Data (output)
#define PS2_OUT_P1INT           (1<<4)  // First Device Port Interrupt (IRQ1)
#define PS2_OUT_P2INT           (1<<5)  // Second Device Port Interrupt (IRQ12)
#define PS2_OUT_P1CLK           (1<<6)  // First Device Port Clock (output)
#define PS2_OUT_P1DAT           (1<<7)  // First Device Port Data (output)

bool ps2_canread(void);
bool ps2_canwrite(void);
uint8_t ps2_read(void);
void ps2_write(uint8_t data);
void ps2_flush(void);
uint8_t ps2_status(void);
void ps2_cmd(uint8_t cmd);


// Keyboard Commands
#define PS2KB_CMD_SETLED        0xED    // Set Caps Lock, Num Lock, and Scroll Lock LEDs
#define PS2KB_CMD_SCANCODE      0xF0    // Set Scan Code Mapping (1, 2, or 3)
#define PS2KB_CMD_IDENT         0xF2    // Identify Keyboard
#define PS2KB_CMD_TYPEMATIC     0xF3    // Set Typematic Rate
#define PS2KB_CMD_SCANON        0xF4    // Enable Scanning
#define PS2KB_CMD_SCANOFF       0xF5    // Disable Scanning
#define PS2KB_CMD_SELFTEST      0xFF    // Run self-test

// // Keyboard Command Responses
// #define KBD_RESP_PASS            0xAA    // Self-Test Passed
// #define KBD_RESP_ACK             0xFA    // Data Received
// #define KBD_RESP_RESEND          0xFE    // Data Not Received, Resend

// Keyboard LED Flags
#define PS2KB_LED_SCRLK         (1<<0)  // Scroll Lock Light
#define PS2KB_LED_NUMLK         (1<<1)  // Num Lock Light
#define PS2KB_LED_CAPLK         (1<<2)  // Caps Lock Light

// Typematic Byte
//  [0:4] repeat rate           (00000 = 30Hz, 11111 = 2Hz)
//  [6:5] delay                 (00 = 250ms, 11 = 1000ms)
//    [7] must be zero

char kb_read(void);

#endif // __PS2_H
