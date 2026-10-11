#ifndef _UART_H_
#define _UART_H_

/* Simple polling UART0 TX driver for SC95F7617B (115200 bps @ Fsys = 32 MHz). */
void uart0_init(void);
void uart0_putc(char c);
void uart0_puts(char *s);
void uart0_put_uint(unsigned int v);

#endif
