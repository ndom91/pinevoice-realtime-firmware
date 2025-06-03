#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <reent.h>
#include <errno.h>
#include <stdio.h>
#include <sys/unistd.h>

bool g_sys_log_disable = 0;

void kernel_log_enable(bool enable)
{
	g_sys_log_disable = !enable;
}

int printk(const char *fmt, ...)
{
    va_list argp;
    int ret = 0;

    if (g_sys_log_disable) {
    	return ret;
    }
    va_start(argp, fmt);
    ret = vprintf(fmt, argp);
    va_end(argp);

    return ret;
}

int putsk(const char *str)
{
	int i;

    if (g_sys_log_disable) {
    	return 0;
    }
    for (i = 0; i < strlen(str); i++) {
    	extern int uart_putc(int ch);
    	uart_putc(str[i]);
    }
    if (str[i-1] != '\r' && str[i-1] != '\n') {
    	uart_putc('\n');
    }

    return i;
}
