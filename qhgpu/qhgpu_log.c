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

#ifndef __KERNEL__

#include <stdio.h>
#include <stdarg.h>

#define printk printf
#define vprintk vprintf

#else

#include <linux/kernel.h>
#include <linux/module.h>

#endif /* __KERNEL__ */




#ifdef __KERNEL__


#endif /* __KERNEL__ */
