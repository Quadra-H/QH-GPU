/*
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the GPL-COPYING file in the top-level directory.
 *
 * Copyright (c) 2010-2011 University of Utah and the Flux Group.
 * All rights reserved.
 *
 * Common header for userspace helper, kernel mode KGPU and KGPU clients
 *
 */

#ifndef __QHGPU_H__
#define __QHGPU_H__

#define TO_UL(v) ((unsigned long)(v))

#define ADDR_WITHIN(pointer, base, size)		\
    (TO_UL(pointer) >= TO_UL(base) &&			\
     (TO_UL(pointer) < TO_UL(base)+TO_UL(size)))

#define ADDR_REBASE(dst_base, src_base, pointer)			\
    (TO_UL(dst_base) + (						\
	TO_UL(pointer)-TO_UL(src_base)))


/*
 * Only for kernel code or helper
 */
#if defined __KERNEL__ || defined __QHGPU__

#endif /* __KERNEL__ || __QHGPU__  */

/*
 * For helper and service providers
 */
#ifndef __KERNEL__

#include "qhgpu_daemon.h"

#endif /* no __KERNEL__ */


/*
 * For kernel code only
 */
#ifdef __KERNEL__


#endif /* __KERNEL__ */

#endif
