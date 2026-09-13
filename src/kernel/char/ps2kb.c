/* =============================================================================
 * Copyright (C) 2020-2025 Wes Hampson. All Rights Reserved.
 *
 * This file is part of the OH-WES Operating System.
 * OH-WES is free software; you may redistribute it and/or modify it under the
 * terms of the GNU GPLv2. See the LICENSE file in the root of this repository.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * -----------------------------------------------------------------------------
 *         File: kernel/char/ps2kbd.c
 *      Created: February 25, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

// http://www-ug.eecg.toronto.edu/msl/nios_devices/datasheets/PS2%20Keyboard%20Protocol.htm
// http://www-ug.eecg.utoronto.ca/desl/manuals/ps2.pdf
// https://wiki.osdev.org/PS/2_Keyboard
// https://www.tayloredge.com/reference/Interface/atkeyboard.pdf
// https://stanislavs.org/helppc/8042.html
// http://www.quadibloc.com/comp/scan.htm
// https://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html

#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <i386/bitops.h>
#include <i386/interrupt.h>
#include <i386/io.h>
#include <i386/ps2.h>
#include <i386/x86.h>
#include <kernel/dpc.h>
#include <kernel/kernel.h>
#include <kernel/input.h>
#include <kernel/irq.h>
#include <kernel/terminal.h>

#define pr_fmt(fmt) "ps2-kb: " fmt
#include <kernel/kprint.h>

#define SCANCODE_SET        1       // DO NOT CHANGE, only set 1 supported for now :)

#define PRINT_EVENTS        0       // print key events
#define SELFTEST            1       // perform keyboard self-test on connect
#define NUMLOCK_ON          1       // NumLock on by default
#define TYPEMATIC_BYTE      0x22    // repeat rate = 24cps, delay = 500ms
#define WARN_INTERVAL       10      // warn every N times a stray packet shows up
#define MAX_PROBES          3       // max times to probe for device before giving up
#define MAX_RESENDS         3       // max command resends (0xFE) before giving up
#define MAX_STRAYS          6       // max stray chars to accept during command sequence

static inline void atomic_inc(uint32_t volatile *value)
{
    __asm__ volatile (
        "lock incl %0"
        : "+m"(*value)
        :
        : "memory"
    );
}

static inline uint32_t atomic_cmpxchg(uint32_t volatile *value, uint32_t xchg, uint32_t comp)
{
    uint32_t old_value;
    __asm__ volatile (
        "lock cmpxchg %2, %0"   // if *value === cmp, *value = xchg
        : "+m"(*value), "=a"(old_value)
        : "r"(xchg), "a"(comp)
        : "cc", "memory"
    );

    return old_value;
}

// keyboard configuration
struct ps2kb
{
    // keyboard hardware state
    uint16_t ident;             // hardware identifier word
    uint8_t leds;               // shadow of last LED state written to keyboard

    // DPC state
    struct dpc leds_dpc;        // LED update
    struct dpc hotplug_dpc;     // hotplug detection
    struct dpc termsw_dpc;      // terminal switch
    struct dpc sysrq_dpc;       // SysRq handler

    volatile bool pending_leds      : 1; // an LED update is armed
    volatile bool pending_hotplug   : 1; // a hotplug flush is armed
    volatile bool pending_termsw    : 1; // a terminal switch is armed
    volatile bool pending_sysrq     : 1; // a SysRq action is armed

    // atomic software state
    uint32_t atm_ps2ctl_init;   // (bool) PS/2 controller initialized
    uint32_t atm_ih_active;     // (bool) keyboard interrupt handler active
    uint32_t atm_hw_connected;  // (bool) keyboard hardware is connected
    uint32_t atm_init_count;    // (int) number of times keyboard initialized

    // keyboard scancode state
    bool e0     : 1;        // 0xE0 modifier received
    bool e1     : 1;        // 0xE1 modifier received
    uint8_t lock_edge;      // physical down-state of LOCK keys

    // counts of spurious scancodes seen by interrupt handler
    uint64_t stray_aa;      // selftest pass
    uint64_t stray_ee;      // echo reply
    uint64_t stray_fc;      // selftest fail
    uint64_t stray_fd;      // selftest fail
    uint64_t stray_ack;     // 0xFA
    uint64_t stray_resend;  // 0xFE
    uint64_t error_count;   // 0xFF or 0x0

    // other counts
    uint32_t parity_errors; // controller parity errors reported
    uint32_t timeout_errors;// controller timing errors reported
    uint64_t init_errors;   // keyboard failed to initialize
    uint64_t stray_int;     // "phantom" interrupts (no scancode to read)

    // // key event buffer
    // struct ring eventq;            // TODO: make queue w/ generic type
    // struct key_event ebuf[KB_BUFFER_SIZE];
};

static struct ps2kb _kb = { };
struct ps2kb *g_kb = &_kb;

#ifdef DEBUG
extern int g_test_crashkey;  // crash.c
#endif

static const uint8_t scanmap_set1[128];
static const uint8_t scanmap_set1_e0[128];
static const char keymap[256];
static const char keymap_shift[128];

#if PRINT_EVENTS
static const char * g_keynames[122];
#endif

// global exports

bool kb_avail(void)
{
    return atomic_cmpxchg(&g_kb->atm_init_count, 0, 0) > 0 &&
           test_bit(&g_kb->atm_hw_connected, 0);
}

int kb_getc(void);

// local functions

#define __isr   // TODO: do something with this...

static void __isr kb_interrupt(int irq, struct iregs *regs);
static void __isr kb_hotplug_detect(int irq, struct iregs *regs);

static void tty_putq(struct tty *tty, char c);

static void ps2kb_queue_led_update(uint8_t leds);

static void __dpc __ps2kb_leds_dpc(void *arg);
static void __dpc __ps2kb_hotplug_detect_dpc(void *arg);
static void __dpc __ps2kb_terminal_switch_dpc(void *arg);
static void __dpc __ps2kb_sysrq_dpc(void *arg);

static bool ps2kb_selftest(void);
static uint16_t ps2kb_identify(void);
static bool ps2kb_is_connected(void);

static bool ps2kb_set_scanmode(uint8_t sc_set);
static bool ps2kb_set_leds(uint8_t leds);
static bool ps2kb_set_typematic(uint8_t typ);

static bool ps2kb_send_cmd(uint8_t cmd, uint8_t *data);


// local imports

extern __init void init_ps2(void);

#define RIF(x)  if (!(x)) { return; }
#define RIF_FALSE(x)  if (!(x)) { return false; }

// ----------------------------------------------------------------------------

int kb_getc(void)
{
    struct tty *tty;
    int c;

    if (!test_bit(&g_kb->atm_hw_connected, 0)) {
        return -EAGAIN;
    }

    tty = get_terminal(0)->tty;

    while (true) {
        if (!test_bit(&g_kb->atm_hw_connected, 0)) {
            return -EAGAIN;
        }

        c = n_tty_getc(tty);
        if (c != -EAGAIN) {
            return c;
        }

        // TODO: scheduler yield instead of busy loop
    }
}

static bool ps2kb_set_scanmode(uint8_t set) // TODO: ioctl for this
{
    return ps2kb_send_cmd(PS2KB_CMD_SCANCODE, &set);
}

static bool ps2kb_set_typematic(uint8_t typ)    // TODO: ioctl for this
{
    typ &= 0x7F;    // bit[0] must be 0
    return ps2kb_send_cmd(PS2KB_CMD_TYPEMATIC, &typ);
}

static bool ps2kb_set_leds(uint8_t leds)    // TODO: ioctl for this
{
    bool success = ps2kb_send_cmd(PS2KB_CMD_SETLED, &leds);
    if (success) {
        g_kb->leds = leds;
    }

    return success;
}

static void __dpc __ps2kb_leds_dpc(void *arg)
{
    uint32_t flags;
    uint8_t leds = (uint8_t)(intptr_t) arg;

    cli_save(flags);
    g_kb->pending_leds = false;
    ps2kb_set_leds(leds);   // do not touch hardware with interrupts on
    restore_flags(flags);
}

static void ps2kb_queue_led_update(uint8_t leds)
{
    uint32_t flags;
    cli_save(flags);

    if (g_kb->leds != leds && !g_kb->pending_leds) {
        g_kb->pending_leds = true;
        schedule_dpc(&g_kb->leds_dpc, __ps2kb_leds_dpc, (void*)(intptr_t) leds);
    }

    restore_flags(flags);
}

void ps2kb_apply_state(const struct ps2kb_state *state)
{
    if (state) {
        ps2kb_queue_led_update(state->_leds);
    }
}

static bool ps2kb_init(void)
{
    struct ps2kb_state *kb_state;
    uint8_t ps2cfg;

    // self-test
#if SELFTEST
    if (!ps2kb_selftest()) {
        pr_error("self-test failed\n");
        g_kb->init_errors++;
        return false;
    }
#endif

    ps2_flush();

    // ensure keyboard interrupts and translation are off
    ps2cfg = ps2_read_config();
    ps2cfg &= ~(PS2_CFG_P1INTON|PS2_CFG_TRANSLATE);
    ps2_write_config(ps2cfg);

    // identify
    g_kb->ident = ps2kb_identify();
    switch (g_kb->ident) {
        case 0xAB83:
        case 0xABC1:
            pr_info("MF2");
            break;
        default:
            pr_info("unknown");
            break;
    }
    pr_cont(" detected (ident: %04Xh)\n", g_kb->ident);


    // select scancode set
    if (!ps2kb_set_scanmode(SCANCODE_SET)) {
        pr_error("failed to select scancode set %d\n", SCANCODE_SET);
        g_kb->init_errors++;
        return false;
    }
    pr_info("switched to %s mode\n",
        (SCANCODE_SET == 1) ? "XT" :
        (SCANCODE_SET == 2) ? "AT" :
        (SCANCODE_SET == 3) ? "MF2" : "???");

    // set typematic properties
    if (!ps2kb_set_typematic(TYPEMATIC_BYTE)) {
        pr_warn("unable to set typematic byte %02Xh\n", TYPEMATIC_BYTE);
    }

    // set LED state
    kb_state = &(get_terminal(0)->kb_state).hw_state;
    kb_state->numlk = NUMLOCK_ON;
    ps2kb_set_leds(kb_state->_leds);

    // enable scanning
    if (!ps2kb_send_cmd(PS2KB_CMD_SCANON, NULL)) {
        pr_warn("failed to re-enable scanning\n");
    }

    // reset counters
    g_kb->stray_aa = 0;
    g_kb->stray_ee = 0;
    g_kb->stray_fc = 0;
    g_kb->stray_fd = 0;
    g_kb->stray_ack = 0;
    g_kb->stray_resend = 0;
    g_kb->stray_int = 0;
    g_kb->error_count = 0;
    g_kb->parity_errors = 0;
    g_kb->timeout_errors = 0;

    // enable port and port interrupts
    ps2cfg |= PS2_CFG_P1INTON;
    ps2_write_config(ps2cfg);
    ps2_write_cmd(PS2_CMD_P1ON);

    atomic_inc(&g_kb->atm_init_count);
    return true;
}

// ----------------------------------------------------------------------------


static void ps2kb_on_connect(void)
{
    uint32_t flags;

    pr_debug("keyboard connected\n");
    atomic_cmpxchg(&g_kb->atm_hw_connected, 1, 0);

    const uint64_t c_settle_time_ms = 2;

    cli_save(flags);
    ps2_flush();
    restore_flags(flags);

    uint64_t settle = get_uptime() + (c_settle_time_ms * 1000000);
    while (get_uptime() < settle) {
        __pause();  // 2ms spin-wait
    }

    cli_save(flags);
    ps2_flush();
    ps2kb_init();
    restore_flags(flags);
}

static void ps2kb_on_disconnect(void)
{
    uint32_t flags;
    cli_save(flags);

    pr_debug("keyboard disconnected\n");
    atomic_cmpxchg(&g_kb->atm_hw_connected, 0, 1);

    ps2_flush();
    ps2_write_config(ps2_read_config() & ~PS2_CFG_P1INTON); // disable interrupts

    restore_flags(flags);
}

static void __dpc __ps2kb_hotplug_detect_dpc(void *arg)
{
    bool was, now;
    uint32_t flags;
    (void) arg;

    cli_save(flags);
    g_kb->pending_hotplug = false;

    was = test_bit(&g_kb->atm_hw_connected, 0);
    now = ps2kb_is_connected();

    if (was && !now) {
        ps2kb_on_disconnect();
    }
    if (!was && now) {
        ps2kb_on_connect();
    }

    restore_flags(flags);
}

static void __isr kb_hotplug_detect(int irq, struct iregs *regs)
{
    // PS/2 keyboard hotplug support

    assert(irq == IRQ_TIMER);
    (void) regs;

    const uint64_t c_heartbeat_ms = 500;

    static uint64_t s_last_tick = 0;
    uint64_t tick = get_uptime();

    if ((tick - s_last_tick) / 1000000 >= c_heartbeat_ms) {
        s_last_tick = tick;

        if (!g_kb->pending_hotplug) {
            g_kb->pending_hotplug = true;
            schedule_dpc(&g_kb->hotplug_dpc, __ps2kb_hotplug_detect_dpc, NULL);
        }
    }
}

__init void init_kb(void)
{
    uint32_t flags;

    if (atomic_cmpxchg(&g_kb->atm_init_count, 0, 0) > 0) {
        return;
    }

    cli_save(flags);

    // initialize PS/2 controller
    if (atomic_cmpxchg(&g_kb->atm_ps2ctl_init, 1, 0) == 0) {
        init_ps2();
    }

    // port 1 clock should be enabled if init passed...
    if (ps2_read_config() & PS2_CFG_P1CLKOFF) {
        pr_error("no keyboard port detected on controller\n");
        goto init_kb_done;
    }

    // check if keyboard connected...
    if (ps2kb_is_connected()) {
        ps2kb_on_connect();
    }
    else {
        ps2kb_on_disconnect();
    }

init_kb_done:
    // enable hotplug detection
    irq_register(IRQ_TIMER, kb_hotplug_detect);

    // register ISR and unmask IRQ1 on the PIC
    irq_register(IRQ_KEYBOARD, kb_interrupt);
    irq_unmask(IRQ_KEYBOARD);

    restore_flags(flags);
}

static void tty_putq(struct tty *tty, char c)
{
    if (tty && tty->ldisc.recv) {
        tty->ldisc.recv(tty, &c, 1);
    }
}

static void __dpc __ps2kb_terminal_switch_dpc(void *arg)
{
    uint32_t flags;
    int term = (int)(intptr_t) arg;

    cli_save(flags);
    g_kb->pending_termsw = false;
    restore_flags(flags);

    if (switch_terminal(term) != 0) {
        pr_error("failed to switch to tty%d!\n", term);
    }
}

static void __dpc __ps2kb_sysrq_dpc(void *arg)
{
    uint32_t flags;
    char c = (char)(intptr_t) arg;

    cli_save(flags);
    g_kb->pending_sysrq = false;
    restore_flags(flags);

    switch (c) {
        case 'c':   // c - crash
            __sti(); load_ss(0x00); for (;;);
            // TODO: trigger crash_key_irq poison selection prompt
        case 'g':   // g - debug break
            __debug_break();
            break;
        case 'r':   // r - hard reset
            __hard_reset();
            break;
        default:
            if (c != '\0') {
                pr_info("sysrq: HELP: crash(c) debug-break(g) reboot(r)\a\n");
            }
            break;
    }
}

static void __isr kb_interrupt(int irq, struct iregs *regs)
{
    uint32_t flags;
    uint16_t sc;
    uint16_t key;
    bool release;
    unsigned char c;
    char *s;
    struct key_event evt;

    assert(irq == IRQ_KEYBOARD);
    (void) regs;

    if (test_and_set_bit(&g_kb->atm_ih_active, 0) == 1) {
        pr_alert("keyboard interrupt recursion!!\n");
        return;
    }

    struct terminal *term = get_terminal(0);
    struct tty *tty = term->tty;
    struct keyboard_state *input_state = &term->kb_state;
    struct ps2kb_state *kb_state = &input_state->hw_state;

    //
    // Scan Code to Key Code Mapping
    // ----------------------------------------------------------------

    // prevent keyboard from sending more interrupts
    cli_save(flags);

    // check keyboard status
    uint8_t status = ps2_read_status();
    if (status & PS2_STATUS_TIMEOUT) {
        pr_debug("interrupt: timeout error\n");
        g_kb->timeout_errors++;
    }
    if (status & PS2_STATUS_PARITY) {
        pr_debug("interrupt: parity error\n");
        g_kb->parity_errors++;
    }
    if (!(status & PS2_STATUS_OPF)) {
        g_kb->stray_int++;
        goto done;  // nothing to read
    }

    // grab the scancode
    sc = ps2_read();

    // ignore it if the hardware hasn't been reconnected yet
    if (test_bit(&g_kb->atm_hw_connected, 0) == 0) {
        goto done;
    }

    // check for some unexpected scancodes
    switch (sc) {
        case 0xAA:
            if (!input_state->shift && !g_kb->e0) { // 0xAA is also shift break code...
                g_kb->stray_aa++;
                pr_debug("interrupt: got stray self-test pass %02Xh\n", sc);
                goto done;
            }
            break;
        case 0xEE:
            g_kb->stray_ee++;
            pr_debug("interrupt: got stray echo reply %02Xh\n", sc);
            goto done;

        case 0xFA:
            g_kb->stray_ack++;
            if ((g_kb->stray_ack % WARN_INTERVAL) == 0) {
                pr_warn("interrupt: seen %llu stray acks\n", g_kb->stray_ack);
            }
            goto done;

        case 0xFC:
            g_kb->stray_fc++;
            pr_debug("interrupt: got stray self-test failure %02Xh\n", sc);
            goto done;
        case 0xFD:
            g_kb->stray_fd++;
            pr_debug("interrupt: got stray self-test failure %02Xh\n", sc);
            goto done;

        case 0xFE:
            g_kb->stray_resend++;
            if ((g_kb->stray_resend % WARN_INTERVAL) == 0) {
                pr_warn("interrupt: seen %llu stray resend requests\n", g_kb->stray_resend);
            }
            goto done;

        case 0xFF: __fallthrough;   // error
        case 0x00:                  // error
            g_kb->error_count++;
            if (g_kb->error_count == 1 || (g_kb->error_count % WARN_INTERVAL) == 0) {
                pr_warn("interrupt: got error code %02Xh\n", sc);
            }
            if ((g_kb->error_count % WARN_INTERVAL) == 0) {
                pr_warn("interrupt: seen %llu keyboard errors\n", g_kb->error_count);
            }
            goto done;
    }

    // did we get an escape code?
    if (sc == 0xE0) {
        g_kb->e0 = true;
        goto done;
    }
    if (sc == 0xE1) {
        g_kb->e1 = true;
        goto done;
    }

    // determine if it's a break code
    release = (sc & 0x80);
    if (release) {
        sc &= ~0x80;
    }

    // translate the scancode to a virtual key
    key = (g_kb->e0) ? scanmap_set1_e0[sc] : scanmap_set1[sc];

    // end E0 escape sequence (should only be one byte)
    if (g_kb->e0) {
        assert(!g_kb->e1);
        sc |= 0xE000;
        g_kb->e0 = false;
    }

    // special handling for the PAUSE key (E1 1D 45 / E1 9D C5), the sequence
    // includes a fake CTRL (1D / 9D) for legacy reasons that we should ignore,
    // as well as an overloaded scancode (45) which is shared with NUMLK
    if (g_kb->e1) {
        assert(!g_kb->e0);
        if ((sc & 0x7F) == 0x1D) {
            goto done;  // fake CTRL, ignore
        }

        g_kb->e1 = false;
        if (key != KEY_NUMLK) {
            goto done;  // invalid sequence
        }
        key = KEY_PAUSE;
        sc |= 0xE100;
    }

    //
    // Key Code Handling
    // ----------------------------------------------------------------

    // numlock handling
    if (!kb_state->numlk) {
        switch (key) {
            case KEY_KP0: key = KEY_INSERT; break;
            case KEY_KP1: key = KEY_END; break;
            case KEY_KP2: key = KEY_DOWN; break;
            case KEY_KP3: key = KEY_PGDOWN; break;
            case KEY_KP4: key = KEY_LEFT; break;
            case KEY_KP6: key = KEY_RIGHT; break;
            case KEY_KP7: key = KEY_HOME; break;
            case KEY_KP8: key = KEY_UP; break;
            case KEY_KP9: key = KEY_PGUP; break;
            case KEY_KPDOT: key = KEY_DELETE; break;
        }
    }

    uint8_t lock_bit = 0;
    switch (key) {
        case KEY_CAPSLK: lock_bit = (1 << 0); break;
        case KEY_NUMLK:  lock_bit = (1 << 0); break;
        case KEY_SCRLK:  lock_bit = (1 << 0); break;
        default: break;
    }

    if (lock_bit) {
        if (release) {
            g_kb->lock_edge &= ~lock_bit;
        }
        else if (!(g_kb->lock_edge & lock_bit)) {
            switch (key) {
                case KEY_CAPSLK: kb_state->capslk ^= true; break;
                case KEY_NUMLK:  kb_state->numlk  ^= true; break;
                case KEY_SCRLK:  kb_state->scrlk  ^= true; break;
            }
            g_kb->lock_edge |= lock_bit;
        }
    }

    if (g_kb->leds != kb_state->_leds) {
        ps2kb_queue_led_update(kb_state->_leds);
    }

    // update modifier key state
    #define HANDLE_MODKEY(key,mod,l,r) \
    ({ \
        int _mask = 0; \
        if ((key) == (r)) { _mask |= _DNMASK_LEFT; } \
        if ((key) == (l)) { _mask |= _DNMASK_RIGHT; } \
        if (_mask && !release) { \
            input_state->mod |= _mask; \
        } \
        else if (_mask && release) { \
            input_state->mod &= ~_mask; \
        } \
    })
    HANDLE_MODKEY(key, ctrl, KEY_LCTRL, KEY_RCTRL);
    HANDLE_MODKEY(key, shift, KEY_LSHIFT, KEY_RSHIFT);
    HANDLE_MODKEY(key, alt, KEY_LALT, KEY_RALT);
    HANDLE_MODKEY(key, meta, KEY_LWIN, KEY_RWIN);
    HANDLE_MODKEY(key, sysrq, KEY_SYSRQ, 0);
    #undef HANDLE_MODKEY

    // submit alt code upon release of ALT key
    if (is_alt(key) && release && input_state->altchar) {
        tty_putq(tty, (unsigned char) input_state->altchar);
        input_state->altchar = 0;
        goto record_key_event;
    }

    // we don't care about key break events after this point
    if (release) {
        goto record_key_event;
    }

    //
    // Handle Special Keystrokes
    // ----------------------------------------------------------------

#ifdef DEBUG
    // Ctrl+Alt+<FN>: crash function select
    if (input_state->ctrl && input_state->alt && is_fnkey(key)) {
        if (!g_test_crashkey) {
            g_test_crashkey = fnkey_index(key);
        }
    }
#endif

    // Ctrl+Alt+Del: system reboot
    if (input_state->ctrl && input_state->alt &&
        (key == KEY_DELETE || key == KEY_KPDOT)) {
        __hard_reset(); // TODO: soft reset
    }

    // SCRLK: toggle terminal pause; uses flow control (CTRL+S/Q)
    if (key == KEY_SCRLK) {
        if (kb_state->scrlk)
            tty_putq(tty, ASCII_DC3); // CTRL+S XOFF
        else
            tty_putq(tty, ASCII_DC1); // CTRL+Q XON
    }

    // TODO: CTRL+SCRLK = dump/flush kernel log

    // ALT+<FN>: switch terminal
    if (!input_state->ctrl && input_state->alt && is_fnkey(key)) {
        int term = fnkey_index(key);
        if (term >= 1 && term <= NR_TERMINAL && !g_kb->pending_termsw) {
            g_kb->pending_termsw = true;
            schedule_dpc(&g_kb->termsw_dpc, __ps2kb_terminal_switch_dpc, (void*)(intptr_t) term);
        }
        goto done;
    }

    // ALT+<NUMPAD>: handle character code entry (if NumLk on)
    if (input_state->alt && is_numpad(key)) {
        input_state->altchar *= 10;
        input_state->altchar += numpad_index(key);
        goto record_key_event;
    }

    //
    // Keystroke to Character Sequence Mapping
    // ----------------------------------------------------------------

    // map key to character
    c = (input_state->shift && key >= 0x20 && key <= 0x60)
        ? keymap_shift[key & 0x7F]
        : keymap[key & 0xFF];

    // handle SyRrq press
    if (input_state->sysrq) {
        if (!g_kb->pending_sysrq) {
            g_kb->pending_sysrq = true;
            schedule_dpc(&g_kb->sysrq_dpc, __ps2kb_sysrq_dpc, (void *)(intptr_t) c);
        }
        goto done;
    }

    // no character to process, record keydown
    if (c == '\0') {
        goto record_key_event;
    }

    // handle non-character keys
    if (c == 0xE0 || (key == KEY_KP5 && !kb_state->numlk)) {
        c = '\0';
        switch (key) {
            // xterm sequences
            case KEY_UP:    s = "\e[A"; break;
            case KEY_DOWN:  s = "\e[B"; break;
            case KEY_RIGHT: s = "\e[C"; break;
            case KEY_LEFT:  s = "\e[D"; break;
            case KEY_KP5:   s = "\e[G"; break;  // maybe, conflicts with terminal (move cursor to column n)
            case KEY_PRTSC: s = "\e[P"; break;  // maybe
            // VT sequences
            case KEY_HOME:  s = "\e[1~"; break;
            case KEY_INSERT:s = "\e[2~"; break;
            case KEY_DELETE:s = "\e[3~"; break;
            case KEY_END:   s = "\e[4~"; break;
            case KEY_PGUP:  s = "\e[5~"; break;
            case KEY_PGDOWN:s = "\e[6~"; break;
            case KEY_F1:    s = "\e[11~"; break;
            case KEY_F2:    s = "\e[12~"; break;
            case KEY_F3:    s = "\e[13~"; break;
            case KEY_F4:    s = "\e[14~"; break;
            case KEY_F5:    s = "\e[15~"; break;
            case KEY_F6:    s = "\e[17~"; break;
            case KEY_F7:    s = "\e[18~"; break;
            case KEY_F8:    s = "\e[19~"; break;
            case KEY_F9:    s = "\e[20~"; break;
            case KEY_F10:   s = "\e[21~"; break;
            case KEY_F11:   s = "\e[23~"; break;
            case KEY_F12:   s = "\e[24~"; break;
            default:        s = "\0"; break;
        }
        while (*s != '\0') {
            tty_putq(tty, *s++);
        }
        goto record_key_event;
    }

    // handle control characters
    if (input_state->ctrl) {
        switch (key) {
            case KEY_2: c = '@'; break;
            case KEY_6: c = '^'; break;
            case KEY_LEFTBRACKET: c = '['; break;
            case KEY_BACKSLASH: c = '\\'; break;
            case KEY_RIGHTBRACKET: c = ']'; break;
            case KEY_MINUS: c = '_'; break;
            case KEY_SLASH: c = '?'; break;
            case KEY_BACKSPACE: c = '\b'; break;
        }
        if (key >= KEY_A && key <= KEY_Z) {
            c = toupper(c);
        }
        if ((c >= '@' && c <= '_') || c == '?') {
            c ^= 0x40;
        }
    }

    // handle caps lock
    if (kb_state->capslk && !input_state->alt) {
        if (isupper(c)) {
            c = tolower(c);
        }
        else if (islower(c)) {
            c = toupper(c);
        }
    }

    // put the character in the queue
    if (input_state->alt) {
        tty_putq(tty, '\e');
    }
    tty_putq(tty, c);

record_key_event:
    zeromem(&evt, sizeof(struct key_event));
    evt.key.keycode = key;
    evt.key.scancode = sc;
    evt.key.release = release;
    evt.c = c;
    // TODO: add to event queue

#if PRINT_EVENTS
    pr_debug("%-8s  ", (release) ? "release" : "press");
    pr_cont("%c  ", isprint(c) ? c : ' ');
    pr_cont("% 4.2x ", key);
    pr_cont("% 4.2x ", sc);
    pr_cont("  %s\n", g_keynames[key]);
#endif

done:
    clear_bit(&g_kb->atm_ih_active, 0);
    restore_flags(flags);
}

static bool ps2kb_selftest(void)
{
    uint8_t recv;

    if (!ps2kb_send_cmd(PS2KB_CMD_SELFTEST, NULL)) {
        return false;
    }

    for (int i = 0; i < MAX_STRAYS; i++) {
        recv = ps2_read();
        if (recv == 0xAA) {
            return true;        // BAT pass
        }
        if (recv == 0xFC || recv == 0xFD) {
            return false;       // BAT fail
        }
    }

    pr_debug("self-test timed out\n");
    return false;
}

static uint16_t ps2kb_identify(void)
{
    uint8_t hi, lo;

    if (!ps2kb_send_cmd(PS2KB_CMD_IDENT, NULL)) {
        return 0;
    }

    hi = ps2_read();
    if (hi == 0) {
        return 0;
    }

    lo = ps2_read();
    if (lo == 0) {
        return hi;
    }

    return (hi << 8) | lo;
}

static bool ps2kb_is_connected(void)
{
    bool connected = false;

    ps2_flush();
    for (int i = 0; i < MAX_PROBES; i++) {
        ps2_write_fast(PS2KB_CMD_ECHO);
        uint8_t resp = ps2_read_fast();
        if (resp == 0xEE) {
            connected = true;
            break;
        }
    }
    ps2_flush();

    return connected;
}

static bool ps2kb_send_cmd(uint8_t cmd, uint8_t *data)
{
    enum { P_CMD, P_DATA } phase = P_CMD;
    uint8_t byte = cmd, resp;
    int resends = 0, strays = 0;

    while (true) {
        // write command or data, depending on phase
        if (!ps2_write_fast(byte)) {
            goto sync_fail;     // device unresponsive
        }

        // read response
        resp = ps2_read_fast(); // device ACK, or 0 on timeout
        if (resp == 0) {
            goto sync_fail;
        }

        switch (resp) {
            case 0xFA:      // ACK
                resends = 0;
                if (data != NULL && phase == P_CMD) {
                    phase = P_DATA;
                    byte = *data;
                }
                else {
                    return true;
                }
                break;

            case 0xFE:      // 'resend last command'
                if (++resends > MAX_RESENDS) {
                    goto sync_fail;
                }
                break;

            case 0xAA:      // BAT in progress (0xFF/reset/selftest); ignore
                break;

            default:        // could be queued scancode; ignore for a while
                if (++strays > MAX_STRAYS) {
                    goto sync_fail;
                }
                break;

        }
    }

sync_fail:
    if (data != NULL && phase == P_CMD) {
        ps2_write_fast(*data);  // make sure keyboard doesn't get out of whack
    }
    ps2_flush();                // drain leftover so IRQ doesn't get confused
    return false;
}

static const uint8_t scanmap_set1[128] =
{
/*00-07*/  0,KEY_ESCAPE,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,
/*08-0F*/  KEY_7,KEY_8,KEY_9,KEY_0,KEY_MINUS,KEY_EQUAL,KEY_BACKSPACE,KEY_TAB,
/*10-17*/  KEY_Q,KEY_W,KEY_E,KEY_R,KEY_T,KEY_Y,KEY_U,KEY_I,
/*18-1F*/  KEY_O,KEY_P,KEY_LEFTBRACKET,KEY_RIGHTBRACKET,KEY_ENTER,KEY_LCTRL,KEY_A,KEY_S,
/*20-27*/  KEY_D,KEY_F,KEY_G,KEY_H,KEY_J,KEY_K,KEY_L,KEY_SEMICOLON,
/*28-2F*/  KEY_APOSTROPHE,KEY_GRAVE,KEY_LSHIFT,KEY_BACKSLASH,KEY_Z,KEY_X,KEY_C,KEY_V,
/*30-37*/  KEY_B,KEY_N,KEY_M,KEY_COMMA,KEY_DOT,KEY_SLASH,KEY_RSHIFT,KEY_KPASTERISK,
/*38-3F*/  KEY_LALT,KEY_SPACE,KEY_CAPSLK,KEY_F1,KEY_F2,KEY_F3,KEY_F4,KEY_F5,
/*40-47*/  KEY_F6,KEY_F7,KEY_F8,KEY_F9,KEY_F10,KEY_NUMLK,KEY_SCRLK,KEY_KP7,
/*48-4F*/  KEY_KP8,KEY_KP9,KEY_KPMINUS,KEY_KP4,KEY_KP5,KEY_KP6,KEY_KPPLUS,KEY_KP1,
/*50-57*/  KEY_KP2,KEY_KP3,KEY_KP0,KEY_KPDOT,KEY_SYSRQ,0,0,KEY_F11,
/*58-5F*/  KEY_F12,0,0,0,0,0,0,0,
/*60-67*/  0,0,0,0,0,0,0,0,
/*68-6F*/  0,0,0,0,0,0,0,0,
/*70-77*/  0,0,0,0,0,0,0,0,
/*78-7F*/  0,0,0,0,0,0,0,0,
};

static const uint8_t scanmap_set1_e0[128] =
{
/*00-07*/  0,0,0,0,0,0,0,0,
/*08-0F*/  0,0,0,0,0,0,0,0,
/*10-17*/  0,0,0,0,0,0,0,0,
/*18-1F*/  0,0,0,0,KEY_KPENTER,KEY_RCTRL,0,0,
/*20-27*/  0,0,0,0,0,0,0,0,
/*28-2F*/  0,0,KEY_LSHIFT,0,0,0,0,0,    // fake shift
/*30-37*/  0,0,0,0,0,KEY_KPSLASH,KEY_RSHIFT,KEY_PRTSC,  // fake shift
/*38-3F*/  KEY_RALT,0,0,0,0,0,0,0,
/*40-47*/  0,0,0,0,0,0,KEY_BREAK,KEY_HOME,
/*48-4F*/  KEY_UP,KEY_PGUP,0,KEY_LEFT,0,KEY_RIGHT,0,KEY_END,
/*50-57*/  KEY_DOWN,KEY_PGDOWN,KEY_INSERT,KEY_DELETE,0,0,0,0,
/*58-5F*/  0,0,0,KEY_LWIN,KEY_RWIN,KEY_MENU,0,0,
/*60-67*/  0,0,0,0,0,0,0,0,
/*68-6F*/  0,0,0,0,0,0,0,0,
/*70-77*/  0,0,0,0,0,0,0,0,
/*78-7F*/  0,0,0,0,0,0,0,0,
};

static const char keymap[256] =
{          // 0xE0 is for multi-byte key codes, mapping is deferred
/*00-0F*/  0,0,0,0,0,0,0,0,0x7F,'\t','\r',0xE0,0xE0,0xE0,0xE0,0xE0,
/*10-1F*/  0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0,0,0,0,'\e',0,0,0,0,
/*20-2F*/  ' ',0,0,0,0,0,0,'\'',0,0,'*','+',',','-','.','/',
/*30-3F*/  '0','1','2','3','4','5','6','7','8','9',0,';',0,'=',0,0,
/*40-4F*/  0,'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',
/*50-5F*/  'p','q','r','s','t','u','v','w','x','y','z','[','\\',']',0,0,
/*60-6F*/  '`','-','.','/','0','1','2','3','4','5','6','7','8','9','\r',0xE0,
/*70-7F*/  0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0,0,0,0,0,
/*80-8F*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*90-9F*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*A0-AF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*B0-BF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*C0-CF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*D0-DF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*E0-EF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*F0-FF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

static const char keymap_shift[128] =
{
/*00-0F*/  0,0,0,0,0,0,0,0,0x7F,'\t','\r',0,0,0,0,0,
/*10-1F*/  0,0,0,0,0,0,0,0,0,0,0,'\e',0,0,0,0,
/*20-2F*/  ' ',0,0,0,0,0,0,'"',0,0,'*','+','<','_','>','?',
/*30-3F*/  ')','!','@','#','$','%','^','&','*','(',0,':',0,'+',0,0,
/*40-4F*/  0,'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O',
/*50-5F*/  'P','Q','R','S','T','U','V','W','X','Y','Z','{','|','}',0,0,
/*60-6F*/  '~','-','.','/','0','1','2','3','4','5','6','7','8','9','\r',0,
/*70-7F*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

// static const char* keymap_e0[256] =
// {

// };

#if PRINT_EVENTS
static const char * g_keynames[122] =
{
    "",
    "KEY_LCTRL",
    "KEY_RCTRL",
    "KEY_LSHIFT",
    "KEY_RSHIFT",
    "KEY_LALT",
    "KEY_RALT",
    "KEY_BREAK",
    "KEY_BACKSPACE",
    "KEY_TAB",
    "KEY_ENTER",
    "KEY_F1",
    "KEY_F2",
    "KEY_F3",
    "KEY_F4",
    "KEY_F5",
    "KEY_F6",
    "KEY_F7",
    "KEY_F8",
    "KEY_F9",
    "KEY_F10",
    "KEY_F11",
    "KEY_F12",
    "KEY_LWIN",
    "KEY_RWIN",
    "KEY_MENU",
    "KEY_PAUSE",
    "KEY_ESCAPE",
    "KEY_SYSRQ",
    "KEY_CAPSLK",
    "KEY_NUMLK",
    "KEY_SCRLK",
    "KEY_SPACE",
    "KEY_RESERVED_33",
    "KEY_RESERVED_34",
    "KEY_RESERVED_35",
    "KEY_RESERVED_36",
    "KEY_RESERVED_37",
    "KEY_RESERVED_38",
    "KEY_APOSTROPHE",
    "KEY_RESERVED_40",
    "KEY_RESERVED_41",
    "KEY_KPASTERISK",
    "KEY_KPPLUS",
    "KEY_COMMA",
    "KEY_MINUS",
    "KEY_DOT",
    "KEY_SLASH",
    "KEY_0",
    "KEY_1",
    "KEY_2",
    "KEY_3",
    "KEY_4",
    "KEY_5",
    "KEY_6",
    "KEY_7",
    "KEY_8",
    "KEY_9",
    "KEY_RESERVED_58",
    "KEY_SEMICOLON",
    "KEY_RESERVED_60",
    "KEY_EQUAL",
    "KEY_RESERVED_62",
    "KEY_RESERVED_63",
    "KEY_RESERVED_64",
    "KEY_A",
    "KEY_B",
    "KEY_C",
    "KEY_D",
    "KEY_E",
    "KEY_F",
    "KEY_G",
    "KEY_H",
    "KEY_I",
    "KEY_J",
    "KEY_K",
    "KEY_L",
    "KEY_M",
    "KEY_N",
    "KEY_O",
    "KEY_P",
    "KEY_Q",
    "KEY_R",
    "KEY_S",
    "KEY_T",
    "KEY_U",
    "KEY_V",
    "KEY_W",
    "KEY_X",
    "KEY_Y",
    "KEY_Z",
    "KEY_LEFTBRACKET",
    "KEY_BACKSLASH",
    "KEY_RIGHTBRACKET",
    "KEY_GRAVE",
    "KEY_KPMINUS",
    "KEY_KPDOT",
    "KEY_KPSLASH",
    "KEY_KP0",
    "KEY_KP1",
    "KEY_KP2",
    "KEY_KP3",
    "KEY_KP4",
    "KEY_KP5",
    "KEY_KP6",
    "KEY_KP7",
    "KEY_KP8",
    "KEY_KP9",
    "KEY_KPENTER",
    "KEY_PRTSC",
    "KEY_INSERT",
    "KEY_DELETE",
    "KEY_HOME",
    "KEY_END",
    "KEY_PGUP",
    "KEY_PGDOWN",
    "KEY_LEFT",
    "KEY_DOWN",
    "KEY_RIGHT",
    "KEY_UP",
};
#endif
