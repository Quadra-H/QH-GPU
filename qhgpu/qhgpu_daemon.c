/*
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the GPL-COPYING file in the top-level directory.
 *
 * Copyright (c) 2013 Quadra-H
 * All rights reserved.
 *
 * GPU service management.
 *
 * link with -ldl
 */
#include <string.h>
#include <dlfcn.h>
#include <stdio.h>
#include <glob.h>
#include <stdlib.h>
#include "list.h"
#include "connector.h"
#include "qhgpu_daemon.h"

LIST_HEAD(services);



cl_context context;             // OpenCL context
cl_device_id* devices;


static struct _qhgpu_sitem *lookup_qhgpu_sitem(const char *name)
{
	struct _qhgpu_sitem *i;
	struct list_head *e;

	if (!name)
		return NULL;

	list_for_each(e, &services) {
		i = list_entry(e, struct _qhgpu_sitem, list);
		if (!strncmp(name, i->s->name, QHGPU_SERVICE_NAME_SIZE)){
			printf("lookup_qhgpu_sitem: %s \n",i->s->name);
			return i;
		}
	}
	return NULL;
}

struct _qhgpu_sitem *qc_lookup_service(const char *name)
{
	struct _qhgpu_sitem *item = lookup_qhgpu_sitem(name);
	if (!item){
		return NULL;
	}
	printf("qc_lookup_service: %s \n",item->s->name);
	return item;
}

int qc_register_service(struct qhgpu_service *s, void *libhandle)
{
	struct _qhgpu_sitem *i;

	if (!s)
		return 1;
	i = (struct _qhgpu_sitem *)malloc(sizeof(struct _qhgpu_sitem));
	if (!i)
		return 1;

	i->s = s;
	i->libhandle = libhandle;
	INIT_LIST_HEAD(&i->list);

	list_add_tail(&i->list, &services);

	return 0;
}

static int __unregister_service(struct _qhgpu_sitem *i)
{
	if (!i)
		return 1;

	list_del(&i->list);
	free(i);

	return 0;
}

int qc_unregister_service(const char *name)
{
	return __unregister_service(lookup_qhgpu_sitem(name));
}

int qc_load_service(const char *libpath)
{
	void *lh;
	fn_init_service init;
	char *err;
	int r=1;

	lh = dlopen(libpath, RTLD_LAZY);
	if (!lh)
	{
		fprintf(stderr,
				"Warning: open %s error, %s\n",
				libpath, dlerror());
	} else {
		init = (fn_init_service)dlsym(lh, SERVICE_INIT);
		if (!init)
		{
			printf("Warning: %s has no service %s\n",
					libpath, ((err=dlerror()) == NULL?"": err));
			fprintf(stderr,
					"Warning: %s has no service %s\n",
					libpath, ((err=dlerror()) == NULL?"": err));
			dlclose(lh);
		} else {
			if (init(lh, qc_register_service,context,devices))
			{
				printf("Warning: %s failed to register service\n",
						libpath);
				fprintf(stderr,
						"Warning: %s failed to register service\n",
						libpath);
				dlclose(lh);
			} else{
				printf("qc_load_service success !!\n");
				r = 0;
			}

		}
	}
	return r;
}

int qc_load_all_services(const char *dir)
{
	char path[256];
	int i;
	char *libpath;
	int e=0;

	glob_t glb = {0,NULL,0};

	snprintf(path, 256, "%s/%s*", dir, SERVICE_LIB_PREFIX);
	glob(path, 0, NULL, &glb);

	for (i=0; i<glb.gl_pathc; i++)
	{
		libpath = glb.gl_pathv[i];
		e += qc_load_service(libpath);
	}
	globfree(&glb);
	return e;
}

static int __unload_service(struct _qhgpu_sitem* i)
{
	void *lh;
	fn_finit_service finit;
	int r=1;
	if (!i)
		return 1;

	lh = i->libhandle;

	if (lh) {
		finit = (fn_finit_service)dlsym(lh, SERVICE_FINIT);
		if (finit)
		{
			if (finit(lh, qc_unregister_service))
			{
				fprintf(stderr,
						"Warning: failed to unregister service %s\n",
						i->s->name);
			} else
				r = 0;
		} else {
			__unregister_service(i);
			r = 0;
		}
		dlclose(lh);
	} else {
		__unregister_service(i);
		r=0;
	}
	return r;
}

int qc_unload_service(const char *name)
{
    return __unload_service(lookup_qhgpu_sitem(name));
}

int qc_unload_all_services()
{
	struct list_head *p, *n;
	int e=0;
	list_for_each_safe(p, n, &services) {
		e += __unload_service(list_entry(p, struct _qhgpu_sitem, list));
	}
	return e;
}



