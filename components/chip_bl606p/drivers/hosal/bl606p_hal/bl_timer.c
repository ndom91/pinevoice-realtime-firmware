#include "bl_timer.h"

#include <FreeRTOS.h>
#include <task.h>

uint32_t bl_timer_now_us(void)
{
    return xTaskGetTickCount() * 1000000 / configTICK_RATE_HZ;
}

