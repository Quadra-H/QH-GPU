/*
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the GPL-COPYING file in the top-level directory.
 *
 * Copyright (c) 2010-2011 University of Utah and the Flux Group.
 * All rights reserved.
 *
 * Log functions used by both kernel and user space.
 */
#include "qhgpu.h"
#include "qhgpu_log.h"

#ifdef QH_KERNEL //kernel level
#include <linux/kernel.h>
#include <linux/module.h>
#else //user level
#include <stdio.h>
#include <stdarg.h>
#define printk printf
#define vprintk vprintf
#endif //end QH_KERNEL

void qhgpu_log(int level, const char* module, const char *func, int line_num, const char *fmt, ...) {
	va_list args;

	#ifdef QH_LOG_LEVEL
	if(level < QH_LOG_LEVEL)
		return;
	#endif

	switch(level) {
	case QH_LOG_ERROR:
		printk("[%s] %s() %d ERROR: ", module, func, line_num);
		break;
	case QH_LOG_ALERT:
		printk("[%s] %s() %d ALERT: ", module, func, line_num);
		break;
	case QH_LOG_DEBUG:
		printk("[%s] %s() %d DEBUG: ", module, func, line_num);
		break;
	case QH_LOG_INFO:
		printk("[%s] %s() %d INFO: ", module, func, line_num);
		break;
	case QH_LOG_PRINT:
	default: //default PRINT
		printk("[%s] %s() %d: ", module, func, line_num);
		break;
	}

	//todo : log file save
#ifdef QH_LOG_PATH

#endif

	va_start(args, fmt);
	vprintk(fmt, args);
	va_end(args);
}

#ifdef QH_KERNEL //kernel level
MODULE_LICENSE(“GPL”);
#endif //end QH_KERNEL
