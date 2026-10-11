#include "Function_Init.H"
#include "uart.h"

/* ------------------------------------------------------------------
 * SC95F7617B UART0 transmit driver (polling).
 *   Baud rate generator : Timer1 (TMCON.1 T1FD = 1 -> T1 clock = Fsys)
 *   Baud rate           : Fsys / (TH1:TL1)
 *   Reference           : c/Uart_Init.c of the vendor demo code
 * Timer0 is owned by the RTX51 Tiny kernel, Timer1 is only used here.
 * ------------------------------------------------------------------ */
#define UART0_FREQ_MHZ   32UL
#define UART0_BAUD       115200UL
#define UART0_DIVISOR    (UART0_FREQ_MHZ * 1000000UL / UART0_BAUD)

void uart0_init(void)
{
    P2CON &= 0xFC;                              /* P2.0 (TX) / P2.1 (RX) */
    P2PH  |= 0x03;
    SCON  |= 0x50;                              /* mode 1, 8-bit UART, REN = 1 */
    TXINX  = 0x02;                              /* TXCON/... point to Timer2 group */
    TXCON |= 0x00;                              /* UART0 baud clock source = Timer1 */
    TMCON |= 0x02;                              /* T1 clock source = Fsys */
    TH1 = (unsigned char)(UART0_DIVISOR >> 8);
    TL1 = (unsigned char)(UART0_DIVISOR & 0xFF);
    TR1 = 0;                                    /* Timer1 serves UART0, not TR1-run */
    ET1 = 0;                                    /* no Timer1 interrupt */
}

void uart0_putc(char c)
{
    SBUF = c;
    while (!TI);
    TI = 0;
}

void uart0_puts(char *s)
{
    while (*s != '\0') {
        uart0_putc(*s++);
    }
}

void uart0_put_uint(unsigned int v)
{
    char buf[6];
    char i = 0;

    if (v == 0) {
        uart0_putc('0');
        return;
    }
    while ((v != 0) && (i < 5)) {
        buf[i++] = (char)('0' + (char)(v % 10));
        v /= 10;
    }
    while (i > 0) {
        uart0_putc(buf[--i]);
    }
}
