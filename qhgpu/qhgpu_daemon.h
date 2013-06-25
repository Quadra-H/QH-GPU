/*
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the GPL-COPYING file in the top-level directory.
 *
 * Copyright (c) 2010-2011 University of Utah and the Flux Group.
 * All rights reserved.
 *
 */

#ifndef __QHGPU_DAEMON_H__
#define __QHGPU_DAEMON_H__

#include "qhgpu.h"


#define __round_mask(x, y) ((__typeof__(x))((y)-1))
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
#define round_down(x, y) ((x) & ~__round_mask(x, y))

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
} helper
#endif

#endif
