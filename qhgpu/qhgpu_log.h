/*
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the GPL-COPYING file in the top-level directory.
 *
 * Copyright (c) 2010-2011 University of Utah and the Flux Group.
 * All rights reserved.
 *
 * Log functions
 *
 */
#ifndef __QHGPU_LOG_H__
#define __QHGPU_LOG_H__

#define QH_LOG_PRINT 4
#define QH_LOG_INFO  3
#define QH_LOG_DEBUG 2
#define QH_LOG_ALERT 1
#define QH_LOG_ERROR 0

void qhgpu_log(int level, const char* module, const char *func, int line_num, const char *fmt, ...);

#ifdef QH_LOG_PATH

#endif //end QH_LOG_PATH

#endif //end __QHGPU_LOG_H__
