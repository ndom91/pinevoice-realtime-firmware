#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <reent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>

bool g_sys_log_disable = 0;

extern int usb_console_raw_write(const void *buf, size_t len) __attribute__((weak));

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
    extern int uart_putc(int ch);
    size_t len;
    size_t i;

    if (g_sys_log_disable || str == NULL) {
    	return 0;
    }

    len = strlen(str);
    for (i = 0; i < len; i++) {
    	uart_putc(str[i]);
    }

    if (usb_console_raw_write != NULL) {
        usb_console_raw_write(str, len);
    }

    if (len == 0 || (str[len - 1] != '\r' && str[len - 1] != '\n')) {
    	uart_putc('\n');
        if (usb_console_raw_write != NULL) {
            usb_console_raw_write("\n", 1);
        }
    }

    return (int)len;
}
