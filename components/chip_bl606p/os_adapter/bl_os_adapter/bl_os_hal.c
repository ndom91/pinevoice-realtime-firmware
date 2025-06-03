/****************************************************************************
 * components/platform/soc/bl602/bl602_os_adapter/bl602_os_adapter/bl602_os_hal.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <bl_os_hal.h>
#include <bl_os_adapter/bl_os_adapter.h>
#include <bl_os_adapter/bl_os_log.h>

#define OS_USING_ALIOS  1

#ifdef OS_USING_NUTTX
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <debug.h>
#include <sys/time.h>
#include <timer/timer.h>
#include <clock/clock.h>
#include <syslog.h>

#include <nuttx/config.h>
#include <nuttx/irq.h>
#include <nuttx/mqueue.h>
#include <nuttx/kmalloc.h>
#include <nuttx/pthread.h>
#include <nuttx/wqueue.h>
#include <nuttx/signal.h>
#include <nuttx/semaphore.h>

#include <bl602_netdev.h>
#endif

#ifdef OS_USING_FREERTOS
#include <stdio.h>
#include <stdarg.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <queue.h>
#include <event_groups.h>
#include <message_buffer.h>
#include <timers.h>
#include <blog.h>
#include <aos/yloop.h>

#include <bl_irq.h>
#endif

#ifdef OS_USING_ALIOS
#include <stdio.h>
#include <stdarg.h>

#include <aos/kernel.h>
#include <k_api.h>

#include <aos/yloop.h>

#include <bl_irq.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct mq_adpt
{
};

struct irq_adpt
{
    void (*func)(void *arg); /* Interrupt callback function */
    void *arg;               /* Interrupt private data */
};

enum bl_os_timer_mode
{
    BL_OS_TIEMR_ONCE = 0,
    BL_OS_TIEMR_CYCLE
};

typedef enum bl_os_timer_mode bl_os_timer_mode_t;

struct timer_adpt
{
    aos_timer_t timer;
    bl_os_timer_mode_t mode;
    void (*func)(void *arg);
    void *arg;
};

extern void vprint(const char *fmt, va_list argp);
volatile uint8_t sys_log_all_enable;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: bl_os_assert_func
 *
 * Description:
 *   Validation program developer's expected result
 *
 * Input Parameters:
 *   file  - configASSERT file
 *   line  - configASSERT line
 *   func  - configASSERT function
 *   expr  - configASSERT condition
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void bl_os_assert_func(const char *file, int line, const char *func, const char *expr)
{
    printf("Assert failed in %s, %s:%d (%s)", func, file, line, expr);
    while(1);
}

/****************************************************************************
 * Name: bl_os_event_create
 *
 * Description:
 *   Create event group
 *
 * Input Parameters:
 *
 * Returned Value:
 *   Event group data pointer
 *
 ****************************************************************************/

BL_EventGroup_t bl_os_event_create(void)
{
    BL_EventGroup_t event = aos_malloc(sizeof(aos_event_t));
    if (event == NULL) {
        return NULL;
    }

    aos_event_new(event, 0);

    return event;
}

/****************************************************************************
 * Name: bl_os_event_delete
 *
 * Description:
 *   Delete event and free resource
 *
 * Input Parameters:
 *   event  - event data point
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void bl_os_event_delete(BL_EventGroup_t event)
{
	aos_event_free(event);
	aos_free(event);
}

/****************************************************************************
 * Name: bl_os_event_send
 *
 * Description:
 *   Set event bits
 *
 * Input Parameters:
 *
 * Returned Value:
 *   Event value after setting
 *
 ****************************************************************************/

uint32_t bl_os_event_send(BL_EventGroup_t event, uint32_t bits)
{
    uint32_t xResult;
    aos_event_set(event, bits, RHINO_OR);
    xResult = bits;
    return xResult;
}

/****************************************************************************
 * Name: bl_os_event_wait
 *
 * Description:
 *   Delete timer and free resource
 *
 * Input Parameters:
 *   event
 *   bits_to_wait_for
 *   clear_on_exit
 *   wait_for_all_bits
 *   block_time_tick
 *
 * Returned Value:
 *   Current event value
 *
 ****************************************************************************/
uint32_t bl_os_event_wait(BL_EventGroup_t event, uint32_t bits_to_wait_for, int clear_on_exit,
                          int wait_for_all_bits, uint32_t block_time_tick)
{
    uint32_t xResult;

    aos_event_get(event, bits_to_wait_for, RHINO_OR_CLEAR, (unsigned int *)&xResult, block_time_tick);
    return xResult;
}

/****************************************************************************
 * Name: bl_os_event_register
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

int bl_os_event_register(int type, void *cb, void *arg)
{
    return 0;
}

/****************************************************************************
 * Name: bl_os_event_notify
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

int bl_os_event_notify(int evt, int val)
{
    return aos_post_event(EV_WIFI, evt, val);
}

/****************************************************************************
 * Name: bl_os_task_create
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

int bl_os_task_create(const char *name,
                      void *entry,
                      uint32_t stack_depth,
                      void *param,
                      uint32_t prio,
                      BL_TaskHandle_t task_handle)
{
    return (int)krhino_task_dyn_create((ktask_t **)&task_handle, name, param, prio, 0,
                                      stack_depth / sizeof(cpu_stack_t), entry, 1u);
}

/****************************************************************************
 * Name: bl_os_task_delete
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

void bl_os_task_delete(BL_TaskHandle_t task_handle)
{
    krhino_task_del(task_handle);
}

/****************************************************************************
 * Name: bl_os_task_get_current_task
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

BL_TaskHandle_t bl_os_task_get_current_task(void)
{
    return krhino_cur_task_get();
}

/****************************************************************************
 * Name: bl_os_task_notify_create
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

BL_TaskHandle_t bl_os_task_notify_create(void)
{
    ktask_t *task = krhino_cur_task_get();
    ksem_t *p_sem = aos_malloc(sizeof(ksem_t));
    if (p_sem == NULL) {
        return NULL;
    }
    krhino_task_sem_create(task, p_sem, "notify", 0);
    return (BL_TaskHandle_t)task;
}

/****************************************************************************
 * Name: bl_os_task_notify
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

void bl_os_task_notify(BL_TaskHandle_t task_handle)
{
    krhino_task_sem_give(task_handle);
}

/****************************************************************************
 * Name: bl_os_task_wait
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

void bl_os_task_wait(BL_TaskHandle_t task_handle, uint32_t tick)
{
    krhino_task_sem_take(tick);
}

/****************************************************************************
 * Name: bl_os_api_init
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

int bl_os_api_init(void)
{
    return 0;
}

/****************************************************************************
 * Name: bl_os_lock_gaint
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

void bl_os_lock_gaint(void)
{
}

/****************************************************************************
 * Name: bl_os_unlock_gaint
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

void bl_os_unlock_gaint(void)
{
}

/****************************************************************************
 * Name: bl_os_enter_critical
 *
 * Description:
 *   Enter critical state
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   CPU PS value
 *
 ****************************************************************************/

cpu_cpsr_t cpu_intrpt_save(void);
void   cpu_intrpt_restore(cpu_cpsr_t cpsr);

static uint32_t _os_enter_critical(void)
{
    return cpu_intrpt_save();
}

/****************************************************************************
 * Name: bl_os_exit_critical
 *
 * Description:
 *   Exit from critical state
 *
 * Input Parameters:
 *   level - CPU PS value
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void _os_exit_critical(uint32_t level)
{
    cpu_intrpt_restore(level);
}

/****************************************************************************
 * Name: bl_os_msleep
 *
 * Description:
 *   Sleep in milliseconds
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

int bl_os_msleep(long msec)
{
    aos_msleep(msec);

    return 0;
}

/****************************************************************************
 * Name: bl_os_sleep
 *
 * Description:
 *   Sleep in seconds
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

int bl_os_sleep(unsigned int seconds)
{
    aos_msleep(seconds * 1000);

    return 0;
}

/****************************************************************************
 * Name: bl_os_printf
 *
 * Description:
 *   Output format string and its arguments
 *
 * Input Parameters:
 *   format - format string
 *
 * Returned Value:
 *   0
 *
 ****************************************************************************/
#if CONFIG_BLOS_LOG_ENABLE
static char string[512];
static void _os_printf(const char *__fmt, ...)
{
    va_list args;

    if (sys_log_all_enable) {
        /* args point to the first variable parameter */
        va_start(args, __fmt);

        /* You can add your code under here. */
        vsprintf(string, __fmt, args);

        va_end(args);
    }
}
#endif

/****************************************************************************
 * Name: bl_os_puts
 *
 * Description:
 *   Output format string
 *
 * Input Parameters:
 *   s - string
 *
 * Returned Value:
 *   0
 *
 ****************************************************************************/

void bl_os_puts(const char *s)
{
    if (s != NULL) {
        puts(s);
    }
}

/****************************************************************************
 * Name: bl_os_malloc
 *
 * Description:
 *   Allocate a block of memory
 *
 * Input Parameters:
 *   size - memory size
 *
 * Returned Value:
 *   Memory pointer
 *
 ****************************************************************************/

void *bl_os_malloc(unsigned int size)
{
    return aos_malloc(size);
}

/****************************************************************************
 * Name: bl_os_free
 *
 * Description:
 *   Free a block of memory
 *
 * Input Parameters:
 *   ptr - memory block
 *
 * Returned Value:
 *   No
 *
 ****************************************************************************/

void bl_os_free(void *ptr)
{
    aos_free(ptr);
}

/****************************************************************************
 * Name: bl_os_zalloc
 *
 * Description:
 *   Allocate a block of memory
 *
 * Input Parameters:
 *   size - memory size
 *
 * Returned Value:
 *   Memory pointer
 *
 ****************************************************************************/

void *bl_os_zalloc(unsigned int size)
{
    return aos_calloc(1, size);
}

/****************************************************************************
 * Name: bl_os_update_time
 *
 * Description:
 *   Transform ticks to time and add this time to timespec value
 *
 * Input Parameters:
 *   timespec - Input timespec data pointer
 *   ticks    - System ticks
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

//static void bl_os_update_time(uint32_t *ms, uint32_t ticks)
//{
//
//}

/****************************************************************************
 * Name: bl_os_errno_trans
 *
 * Description:
 *   Transform from nuttx Os error code to Wi-Fi adapter error code
 *
 * Input Parameters:
 *   ret - NuttX error code
 *
 * Returned Value:
 *   Wi-Fi adapter error code
 *
 ****************************************************************************/

//static inline int32_t bl_os_errno_trans(int ret)
//{
//    if (ret == 0) {
//        return 0;
//    } else {
//        return 1;
//    }
//}

/****************************************************************************
 * Name: bl_os_mq_creat
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

BL_MessageQueue_t bl_os_mq_creat(uint32_t queue_len, uint32_t item_size)
{
    BL_MessageQueue_t q = aos_malloc(sizeof(aos_queue_t));
    if (q == NULL) {
    	return NULL;
    }
    aos_queue_create(q, item_size * queue_len, item_size, 0);
    return q;
}

/****************************************************************************
 * Name: bl_os_mq_delete
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

void bl_os_mq_delete(BL_MessageQueue_t mq)
{
    aos_queue_free((aos_queue_t *)mq);
    aos_free(mq);
}

/****************************************************************************
 * Name: bl_os_mq_send_wait
 *
 * Description:
 *   Generic send message to queue within a certain period of time
 *
 * Input Parameters:
 *   queue - Message queue data pointer
 *   item  - Message data pointer
 *   ticks - Wait ticks
 *   prio  - Message priority
 *
 * Returned Value:
 *   True if success or false if fail
 *
 ****************************************************************************/

int bl_os_mq_send_wait(BL_MessageQueue_t queue, void *item, uint32_t len, uint32_t ticks, int prio)
{
    int ms, ret = -1;

    ms = krhino_ticks_to_ms(ticks);

    ret = aos_queue_send(queue, item, len);
    while ((ms--) && (ret != 0)) {
        ret = aos_queue_send(queue, item, len);
        aos_msleep(1);
    }
    return ret; 
}

/****************************************************************************
 * Name: bl_os_mq_send
 *
 * Description:
 *   Send message of low priority to queue within a certain period of time
 *
 * Input Parameters:
 *   queue - Message queue data pointer
 *   item  - Message data pointer
 *   ticks - Wait ticks
 *
 * Returned Value:
 *   True if success or false if fail
 *
 ****************************************************************************/

int bl_os_mq_send(BL_MessageQueue_t queue, void *item, uint32_t len)
{
    return aos_queue_send(queue, item, len);
}

/****************************************************************************
 * Name: bl_os_mq_recv
 *
 * Description:
 *   Receive message from queue within a certain period of time
 *
 * Input Parameters:
 *   queue - Message queue data pointer
 *   item  - Message data pointer
 *   ticks - Wait ticks
 *
 * Returned Value:
 *   True if success or false if fail
 *
 ****************************************************************************/

int bl_os_mq_recv(BL_MessageQueue_t queue, void *item, uint32_t len, uint32_t tick)
{
    int ret;
    size_t length;
    ret = aos_queue_recv(queue, tick, item, (size_t *)&length);
    return ret;
}

/****************************************************************************
 * Name: bl_os_timer_callback
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

void _os_timer_callback(void *timer, void *arg)
{
    struct timer_adpt *t = (struct timer_adpt *)arg;

    if (t != NULL && t->func) {
        t->func(t->arg);
    }
}

/****************************************************************************
 * Name: bl_os_timer_create
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

BL_Timer_t bl_os_timer_create(void *func, void *argv)
{
    struct timer_adpt *timer;

    timer = (struct timer_adpt *)aos_calloc(1, sizeof(struct timer_adpt));

    if (!timer) {
        return NULL;
    }
    timer->func = func;
    timer->arg  = argv;
    aos_timer_new_ext(&timer->timer, _os_timer_callback, timer, (int)-1, 1, 0);

    return (BL_Timer_t)&timer->timer;
}

/****************************************************************************
 * Name: bl_os_timer_delete
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

int bl_os_timer_delete(BL_Timer_t timerid, uint32_t tick)
{
    struct timer_adpt *timer;

    timer = (struct timer_adpt *)timerid;

    if (!timer) {
        return -1;
    }

    aos_timer_stop(&timer->timer);

    aos_timer_free(&timer->timer);

    aos_free(timer);

    return 0;
}

/****************************************************************************
 * Name: bl_os_timer_start_once
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

int bl_os_timer_start_once(BL_Timer_t timerid, long t_sec, long t_nsec)
{
    struct timer_adpt *timer;

    timer = (struct timer_adpt *)timerid;

    tick_t tick = MS2TICK(t_sec*1000) + MS2TICK(t_nsec/1000000);
    krhino_timer_change(timer->timer, tick, 0);
    krhino_timer_start(timer->timer);
    return 0;
}

/****************************************************************************
 * Name: bl_os_timer_start_periodic
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

int bl_os_timer_start_periodic(BL_Timer_t timerid, long t_sec, long t_nsec)
{
    struct timer_adpt *timer;

    timer = (struct timer_adpt *)timerid;

    tick_t tick = MS2TICK(t_sec*1000) + MS2TICK(t_nsec/1000000);
    krhino_timer_change(timer->timer, tick, tick);
    krhino_timer_start(timer->timer);
    return 0;
}

/****************************************************************************
 * Name: bl_os_workqueue_create
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

void *bl_os_workqueue_create(void)
{
	printf("%s error\r\n", __func__);
    return NULL;
}

/****************************************************************************
 * Name: bl_os_workqueue_submit_hpwork
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

int bl_os_workqueue_submit_hpwork(void *work, void *worker, void *argv, long tick)
{
	printf("%s error\r\n", __func__);
    return 0;
}

/****************************************************************************
 * Name: bl_os_workqueue_submit_lpwork
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

int bl_os_workqueue_submit_lpwork(void *work, void *worker, void *argv, long tick)
{
	printf("%s error\r\n", __func__);
    return 0;
}

/****************************************************************************
 * Name: bl_os_clock_gettime_ms
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

uint64_t bl_os_clock_gettime_ms(void)
{
    return aos_now_ms();
}

/****************************************************************************
 * Name: bl_os_get_tick
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

uint32_t bl_os_get_tick()
{
    return krhino_sys_tick_get();
}

/****************************************************************************
 * Name: bl_os_isr_adpt_cb
 *
 * Description:
 *   Wi-Fi interrupt adapter callback function
 *
 * Input Parameters:
 *   arg - interrupt adapter private data
 *
 * Returned Value:
 *   0 on success
 *
 ****************************************************************************/

//static int bl_os_isr_adpt_cb(int irq, void *context, void *arg)
//{
//    struct irq_adpt *adapter = (struct irq_adpt *)arg;
//
//    adapter->func(adapter->arg);
//
//    return 0;
//}

/****************************************************************************
 * Name: bl_os_irq_attach
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

void bl_os_irq_attach(int32_t n, void *f, void *arg)
{
	printf("%s error\r\n", __func__);
}

/****************************************************************************
 * Name: bl_os_irq_enable
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

void bl_os_irq_enable(int32_t n)
{
	printf("%s error\r\n", __func__);
}

/****************************************************************************
 * Name: bl_os_irq_disable
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

void bl_os_irq_disable(int32_t n)
{
	printf("%s error\r\n", __func__);
}

/****************************************************************************
 * Name: bl_os_mutex_create
 *
 * Description:
 *   Create mutex
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   Mutex data pointer
 *
 ****************************************************************************/

BL_Mutex_t bl_os_mutex_create(void)
{
    BL_Mutex_t mutex = aos_malloc(sizeof(aos_mutex_t));
    if (mutex == NULL) {
        return NULL;
    }
    aos_mutex_new(mutex);
    return mutex;
}

/****************************************************************************
 * Name: bl_os_mutex_delete
 *
 * Description:
 *   Delete mutex
 *
 * Input Parameters:
 *   mutex_data - mutex data pointer
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void bl_os_mutex_delete(BL_Mutex_t mutex_data)
{
    aos_mutex_free(mutex_data);
    aos_free(mutex_data);
}

/****************************************************************************
 * Name: bl_os_mutex_lock
 *
 * Description:
 *   Lock mutex
 *
 * Input Parameters:
 *   mutex_data - mutex data pointer
 *
 * Returned Value:
 *   True if success or false if fail
 *
 ****************************************************************************/

int32_t bl_os_mutex_lock(BL_Mutex_t mutex_data)
{
	aos_mutex_lock(mutex_data, (uint32_t)-1);

    return 0;
}

/****************************************************************************
 * Name: bl_os_mutex_unlock
 *
 * Description:
 *   Lock mutex
 *
 * Input Parameters:
 *   mutex_data - mutex data pointer
 *
 * Returned Value:
 *   True if success or false if fail
 *
 ****************************************************************************/

int32_t bl_os_mutex_unlock(BL_Mutex_t mutex_data)
{
	aos_mutex_unlock(mutex_data);
	return 0;
}

/****************************************************************************
 * Name: bl_os_sem_create
 *
 * Description:
 *   Create and initialize semaphore
 *
 * Input Parameters:
 *   max  - No mean
 *   init - semaphore initialization value
 *
 * Returned Value:
 *   Semaphore data pointer
 *
 ****************************************************************************/

BL_Sem_t bl_os_sem_create(uint32_t init)
{
    BL_Sem_t sem = aos_malloc(sizeof(aos_sem_t));
    if (sem == NULL) {
        return NULL;
    }
    aos_sem_new(sem, init);
    return sem;
}

/****************************************************************************
 * Name: bl_os_sem_delete
 *
 * Description:
 *   Delete semaphore
 *
 * Input Parameters:
 *   semphr - Semaphore data pointer
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void bl_os_sem_delete(BL_Sem_t semphr)
{
    aos_sem_free(semphr);
    aos_free(semphr);
}

/****************************************************************************
 * Name: bl_os_sem_take
 *
 * Description:
 *   Wait semaphore within a certain period of time
 *
 * Input Parameters:
 *   semphr - Semaphore data pointer
 *   ticks  - Wait system ticks
 *
 * Returned Value:
 *   True if success or false if fail
 *
 ****************************************************************************/

int32_t bl_os_sem_take(BL_Sem_t semphr, uint32_t ticks)
{
	return aos_sem_wait(semphr, ticks);
}

/****************************************************************************
 * Name: bl_os_sem_give
 *
 * Description:
 *   Post semaphore
 *
 * Input Parameters:
 *   semphr - Semaphore data pointer
 *
 * Returned Value:
 *   True if success or false if fail
 *
 ****************************************************************************/

int32_t bl_os_sem_give(BL_Sem_t semphr)
{
	aos_sem_signal(semphr);
	return 0;
}

/****************************************************************************
 * Name: bl_os_log_writev
 *
 * Description:
 *   Output log with by format string and its arguments
 *
 * Input Parameters:
 *   level  - log level, no mean here
 *   tag    - log TAG, no mean here
 *   file   - file name
 *   line   - assert line
 *   format - format string
 *   args   - arguments list
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

typedef enum _blog_leve {
     BLOG_LEVEL_ALL = 0,
     BLOG_LEVEL_DEBUG,
     BLOG_LEVEL_INFO,
     BLOG_LEVEL_WARN,
     BLOG_LEVEL_ERROR,
     BLOG_LEVEL_ASSERT,
     BLOG_LEVEL_NEVER,
} blog_level_t;

void bl_os_log_writev(uint32_t level,
                             const char *tag,
                             const char *file,
                             int line,
                             const char *format,
                             va_list args)
{
#if (CFG_COMPONENT_BLOG_ENABLE == 1)
    if ((level >= REFC_LEVEL(__COMPONENT_NAME_DEQUOTED__)) &&
        (level >= REFF_LEVEL(__COMPONENT_FILE_NAME_DEQUOTED__)))
    {
        bl_os_printf("[%10u][%s: %s:%4d] ",
                     bl_os_get_tick(),
                     tag,
                     file,
                     line);

        if (sys_log_all_enable) {
            vprint(format, args);
        }
    }
#endif
}

/****************************************************************************
 * Name: bl_os_log_write
 *
 * Description:
 *   Output log with by format string and its arguments
 *
 * Input Parameters:
 *   level  - log level, no mean here
 *   file   - file name
 *   line   - assert line
 *   tag    - log TAG, no mean here
 *   format - format string
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void bl_os_log_write(uint32_t level, const char *tag, const char *file, int line, const char *format, ...)
{
    va_list list;
    va_start(list, format);

    switch (level) {
        case LOG_LEVEL_ERROR:
        {
            bl_os_log_writev(BLOG_LEVEL_ERROR, "\x1b[31mERROR \x1b[0m", file, line, format, list);
            break;
        }
        case LOG_LEVEL_WARN:
        {
            bl_os_log_writev(BLOG_LEVEL_WARN, "\x1b[33mWARN  \x1b[0m", file, line, format, list);
            break;
        }
        case LOG_LEVEL_INFO:
        {
            bl_os_log_writev(BLOG_LEVEL_INFO, "\x1b[32mINFO  \x1b[0m", file, line, format, list);
            break;
        }
        case LOG_LEVEL_DEBUG:
        {
            bl_os_log_writev(BLOG_LEVEL_DEBUG, "DEBUG ", file, line, format, list);
            break;
        }
    }

    va_end(list);
}

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern void *__attribute__((weak)) _wifi_log_flag;

bl_ops_funcs_t g_bl_ops_funcs =
{
    ._version = BL_OS_ADAPTER_VERSION,
#if CONFIG_BLOS_LOG_ENABLE
    ._printf = _os_printf,
#endif
    ._puts = bl_os_puts,
    ._assert = bl_os_assert_func,
    ._init = bl_os_api_init,
    ._enter_critical = _os_enter_critical,
    ._exit_critical = _os_exit_critical,
    ._msleep = bl_os_msleep,
    ._sleep = bl_os_sleep,
    ._event_group_create = bl_os_event_create,
    ._event_group_delete = bl_os_event_delete,
    ._event_group_send = bl_os_event_send,
    ._event_group_wait = bl_os_event_wait,
    ._event_register = bl_os_event_register,
    ._event_notify = bl_os_event_notify,
    ._task_create = bl_os_task_create,
    ._task_delete = bl_os_task_delete,
    ._task_get_current_task = bl_os_task_get_current_task,
    ._task_notify_create = bl_os_task_notify_create,
    ._task_notify = bl_os_task_notify,
    ._task_wait = bl_os_task_wait,
    ._irq_attach = bl_os_irq_attach,
    ._irq_enable = bl_os_irq_enable,
    ._irq_disable = bl_os_irq_disable,
    ._workqueue_create = bl_os_workqueue_create,
    ._workqueue_submit_hp = bl_os_workqueue_submit_hpwork,
    ._workqueue_submit_lp = bl_os_workqueue_submit_lpwork,
    ._timer_create = bl_os_timer_create,
    ._timer_delete = bl_os_timer_delete,
    ._timer_start_once = bl_os_timer_start_once,
    ._timer_start_periodic = bl_os_timer_start_periodic,
    ._sem_create = bl_os_sem_create,
    ._sem_delete = bl_os_sem_delete,
    ._sem_take = bl_os_sem_take,
    ._sem_give = bl_os_sem_give,
    ._mutex_create = bl_os_mutex_create,
    ._mutex_delete = bl_os_mutex_delete,
    ._mutex_lock = bl_os_mutex_lock,
    ._mutex_unlock = bl_os_mutex_unlock,
    ._queue_create = bl_os_mq_creat,
    ._queue_delete = bl_os_mq_delete,
    ._queue_send_wait = bl_os_mq_send_wait,
    ._queue_send = bl_os_mq_send,
    ._queue_recv = bl_os_mq_recv,
    ._malloc = bl_os_malloc,
    ._free = bl_os_free,
    ._zalloc = bl_os_zalloc,
    ._get_time_ms = bl_os_clock_gettime_ms,
    ._get_tick = bl_os_get_tick,
    ._log_write = bl_os_log_write
};

