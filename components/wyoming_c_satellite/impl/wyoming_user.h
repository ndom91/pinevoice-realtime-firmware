#ifndef __WYOMING_USER_H_
#define __WYOMING_USER_H_

#include <ulog/ulog.h>

#include <aos/kernel.h>

// Include required libraries
#include <cJSON.h>

// Logging implementation
#include <sys/types.h>
#include <lwip/sockets.h>
#include <unistd.h> // For Sockets

#undef LOGE
#undef LOGD
#undef LOGI
#ifdef CONFIG_DEBUG
#define LOGD(...) ulog(LOG_DEBUG, "wsat", ULOG_TAG, __VA_ARGS__)
#else
#define LOGD(...)  
#endif
#define LOGE(...) ulog(LOG_ERR, "wsat", ULOG_TAG, __VA_ARGS__)
#define LOGI(...) ulog(LOG_INFO, "wsat", ULOG_TAG, __VA_ARGS__)

// Platform related macros

#define PLAT_THREAD_TYPE void*
#define PLAT_THREAD_CREATE(thread, start_routine, name, stack_size, priority) 1
#define PLAT_THREAD_JOIN(thread) 1

#define PLAT_MUTEX_TYPE aos_mutex_t
#define PLAT_MUTEX_CREATE(mutex) aos_mutex_new(mutex)
#define PLAT_MUTEX_DESTROY(mutex) aos_mutex_free(mutex)
#define PLAT_MUTEX_LOCK(mutex) aos_mutex_lock(mutex, AOS_WAIT_FOREVER)
#define PLAT_MUTEX_UNLOCK(mutex) aos_mutex_unlock(mutex)

#define EVENT_DECODER_BUFFER_SIZE (4096)

#endif