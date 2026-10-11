#include "Function_Init.H"
#include <rtx51tny.h>
#include "uart.h"

/* ------------------------------------------------------------------
 * RTX51 Tiny demo for SC95F7617B @ Fsys = 32 MHz
 *   system tick : 10 ms   (INT_CLOCK = 26667, Timer0 = Fsys/12)
 *
 *   task 0 : initialise UART, create task 1/2, then delete itself
 *   task 1 : producer, every 100 ticks (~1 s) update a counter and
 *            signal task 2
 *   task 2 : printer, wait for the signal of task 1 and print the value
 *
 * The UART transmit routines are called from task 2 only. This avoids
 * C51/linker overlay warnings (L15) that appear when a non-reentrant
 * function is shared between different tasks.
 * ------------------------------------------------------------------ */

#define TASK_PRODUCER   1
#define TASK_PRINTER    2

#define ONE_SECOND      100         /* 100 ticks * 10 ms = 1 s */

unsigned char g_seconds = 0;        /* shared counter, written by task 1 */

void task_init (void) _task_ 0
{
    uart0_init();

    os_create_task(TASK_PRODUCER);
    os_create_task(TASK_PRINTER);
    os_delete_task(0);
}

void task_producer (void) _task_ TASK_PRODUCER
{
    while (1) {
        g_seconds++;
        os_send_signal(TASK_PRINTER);       /* wake up the printer task */
        os_wait(K_IVL, ONE_SECOND, 0);       /* periodic: every 100 ticks */
    }
}

void task_printer (void) _task_ TASK_PRINTER
{
    uart0_puts("\r\nRTX51 Tiny on SC95F7617B (32MHz, 10ms tick)\r\n");

    while (1) {
        os_wait(K_SIG, 0, 0);               /* wait for task 1 signal */
        uart0_puts("task2: got signal, t = ");
        uart0_put_uint(g_seconds);
        uart0_puts(" s\r\n");
    }
}
