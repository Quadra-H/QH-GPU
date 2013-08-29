#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include "connector.h"
//#include "global.h"

#include "qhgpu.h"
#include "qhgpu_log.h"
#include "cl_operator.h"

#define ssc(...) _safe_syscall(__VA_ARGS__, __FILE__, __LINE__)


cl_context context;             // OpenCL context
cl_device_id* devices;   // tableau des devices
cl_uint nb_platforms;
cl_uint nb_devices;   // number of devices of the gpu
cl_uint numdev;
cl_device_type device_type;   // type de device pour le calcul (cpu ou gpu)
cl_command_queue command_que;     // OpenCL command queue

char* mmap_address;

int _safe_syscall(int r, const char *file, int line) {
	if (r < 0) {
		fprintf(stderr, "Error in %s:%d, ", file, line);
		perror("");
		abort();
	}
	return r;
}




struct _qhgpu_sritem {
	struct qhgpu_service_request sr;
	struct list_head glist;
	struct list_head list;
};

static int devfd;

static struct qhgpu_ku_response resp_temp;

struct qhgpu_gpu_mem_info hostbuf;
struct qhgpu_gpu_mem_info hostvma;

volatile int qc_loop_continue = 1;

static char *service_lib_dir;
static char *qhgpu_dev_path;

/* lists of requests of different states */
LIST_HEAD(all_reqs);
LIST_HEAD(init_reqs);
LIST_HEAD(memdone_reqs);
LIST_HEAD(prepared_reqs);
LIST_HEAD(running_reqs);
LIST_HEAD(post_exec_reqs);
LIST_HEAD(done_reqs);

static char** mmap_addr_arr;

static int init_mmap(int cnt) {
	//b
	/*mmap_address = mmap(NULL, 0x400000, PROT_READ | PROT_WRITE, MAP_SHARED, devfd, 0);
	if (mmap_address == MAP_FAILED) {
		printf("address map failed.\n");
		perror("mmap");
		return -1;
	}*/

	mmap_addr_arr = (char**)malloc(sizeof(char*)*cnt);
	int i=0;
	for(i=0;i<cnt;i++){

		mmap_addr_arr[i] =  mmap(NULL, 0x400000, PROT_READ | PROT_WRITE, MAP_SHARED, devfd, 0);
		if( mmap_addr_arr[i] == NULL ) {
			printf("qhgpu_mmap alloc free pages error %d\n",i);
			return -1;
		}
		printf("[qhpgpu connector]mmap_addr_arr alloc. [%p]\n", mmap_addr_arr[i]);
	}

	//

	/*mmap_addr_arr[1] = mmap(NULL, 0x400000, PROT_READ | PROT_WRITE, MAP_SHARED, devfd, 0);
	if (mmap_addr_arr[1] == MAP_FAILED) {
		printf("address map failed1.\n");
		perror("mmap");
		return -1;
	}
	mmap_addr_arr[2] = mmap(NULL, 0x400000, PROT_READ | PROT_WRITE, MAP_SHARED, devfd, 0);
	if (mmap_addr_arr[2] == MAP_FAILED) {
		printf("address map failed2.\n");
		perror("mmap");
		return -1;
	}
	mmap_addr_arr[3] = mmap(NULL, 0x400000, PROT_READ | PROT_WRITE, MAP_SHARED, devfd, 0);
	if (mmap_addr_arr[3] == MAP_FAILED) {
		printf("address map failed3.\n");
		perror("mmap");
		return -1;
	}*/
	//

	//printf("[qhgpu connector]init mmap done. [%p][%p][%p][%p]\n", mmap_addr_arr[0], mmap_addr_arr[1], mmap_addr_arr[2], mmap_addr_arr[3]);

	return 0;
}


int qc_init(void) {
	int i, len, r;
	void *p = NULL;


	printf("alloc GPU Pinned memory buffers : start \n");
	//dbg("alloc GPU Pinned memory buffers : start \n");
	//// device drive open
	//devfd = ssc(open(qhgpu_dev_path, O_RDWR));
	devfd = open(qhgpu_dev_path, O_RDWR);
	printf("open dev [%s], ret fd = %d\n", qhgpu_dev_path, devfd);

	// init memory map


	gpu_init();
	// alloc GPU Pinned memory buffers
	p = (void*) gpu_alloc_pinned_mem(QHGPU_BUF_SIZE + PAGE_SIZE);
	printf("alloc GPU Pinned memory buffers : %p \n", p);
	hostbuf.uva = p;
	hostbuf.size = QHGPU_BUF_SIZE;

	//dbg("alloc GPU Pinned memory buffers : %p \n", hostbuf.uva);
	memset(hostbuf.uva, 0, QHGPU_BUF_SIZE);
	printf("alloc GPU Pinned memory buffers : memset Done! \n");
	//ssc( mlock(hostbuf.uva, QHGPU_BUF_SIZE));
	ssc(mlock(hostbuf.uva, QHGPU_BUF_SIZE));

	printf("alloc GPU Pinned memory buffers : mlock Done! \n");

	len = sizeof(struct qhgpu_gpu_mem_info);
	printf("qhgpu_gpu_mem_info len  : %d",len);
	/**/
	/* tell kernel the buffers */
	r = ioctl(devfd, QHGPU_IOC_SET_GPU_BUFS, (unsigned long)&hostbuf);
	if (r < 0) {
		printf("device write fail!");
		perror("device write fail!");
		abort();
	}/**/







	/////////////////////////////////////////////////////////////////////
	//////init OpenCL
	/////////////////////////////////////////////////////////////////////
	cl_int status;
	status = clGetPlatformIDs(0, NULL, &nb_platforms);
	assert (status == CL_SUCCESS);
	assert(nb_platforms > 0);
	//cout << "Found "<<nb_platforms<<" OpenCL platform"<<endl;

	//cl_platform_id* platforms = new cl_platform_id[nb_platforms];
	cl_platform_id* platforms =(cl_platform_id *) malloc ( nb_platforms * sizeof(cl_platform_id) );

	status = clGetPlatformIDs(nb_platforms, platforms, NULL);
	assert (status == CL_SUCCESS);

	// affichage
	char pbuf[1000];
	//int i=0;
	for ( i = 0; i < (int)nb_platforms; ++i) {
		status = clGetPlatformInfo(platforms[0],
				CL_PLATFORM_VENDOR,
				sizeof(pbuf),
				pbuf,
				NULL);
		assert (status == CL_SUCCESS);
	}


	for (i = 0; i < (int)nb_platforms; ++i) {
		status = clGetPlatformInfo(platforms[0],
				CL_PLATFORM_VERSION,
				sizeof(pbuf),
				pbuf,
				NULL);
		assert (status == CL_SUCCESS);

		//cout << pbuf <<endl;
	}
	// affichages divers
	for ( i = 0; i < (int)nb_platforms; ++i) {
		status = clGetPlatformInfo(platforms[0],
				CL_PLATFORM_NAME,
				sizeof(pbuf),
				pbuf,
				NULL);
		assert (status == CL_SUCCESS);

		//cout << pbuf <<endl;
	}

	// comptage du nombre de devices
	status = clGetDeviceIDs(platforms[0],
			CL_DEVICE_TYPE_ALL,
			0,
			NULL,
			&nb_devices);
	assert (status == CL_SUCCESS);
	assert(nb_devices > 0);

	//cout <<"Found "<<nb_devices<< " OpenCL device"<<endl;


	devices =(cl_device_id *) malloc ( nb_devices * sizeof(cl_device_id) );


	numdev=0;
	assert(numdev < nb_devices);

	// remplissage du tableau des devices
	status = clGetDeviceIDs(platforms[0],
			CL_DEVICE_TYPE_ALL,
			nb_devices,
			devices,
			NULL);
	assert (status == CL_SUCCESS);


	// informations diverses

	// type du device
	status = clGetDeviceInfo(
			devices[numdev],
			CL_DEVICE_TYPE,
			sizeof(cl_device_type),
			(void*)&device_type,
			NULL);
	assert (status == CL_SUCCESS);


	/*status = clGetDeviceInfo(
			devices[numdev],
			CL_DEVICE_EXTENSIONS,
			sizeof(exten),
			exten,
			NULL);
	assert (status == CL_SUCCESS);*/

	//cout<<"OpenCL extensions for this device:"<<endl;
	//cout << exten<<endl<<endl;

	// type du device
	status = clGetDeviceInfo(
			devices[numdev],
			CL_DEVICE_TYPE,
			sizeof(cl_device_type),
			(void*)&device_type,
			NULL);
	assert (status == CL_SUCCESS);

	if (device_type == CL_DEVICE_TYPE_CPU){
		//cout << "Calcul sur CPU"<<endl;
	}
	else{
		//cout << "Calcul sur Carte Graphique"<<endl;
	}

	// mémoire cache du  device
	cl_ulong memcache;
	status = clGetDeviceInfo(
			devices[numdev],
			CL_DEVICE_LOCAL_MEM_SIZE,
			sizeof(cl_ulong),
			(void*)&memcache,
			NULL);
	assert (status == CL_SUCCESS);

	//cout << "GPU cache="<<memcache<<endl;
	//cout << "Needed cache="<< _ITEMS*_RADIX*sizeof(int)<<endl;

	// nombre de CL_DEVICE_MAX_COMPUTE_UNITS
	cl_int cores;
	status = clGetDeviceInfo(
			devices[numdev],
			CL_DEVICE_MAX_COMPUTE_UNITS,
			sizeof(cl_int),
			(void*)&cores,
			NULL);
	assert (status == CL_SUCCESS);

	//cout << "Compute units="<<cores<<endl;
	// création d'un contexte opencl
	//cout <<"Create the context"<<endl;
	context = clCreateContext(0,
			1,
			&devices[numdev],
			NULL,
			NULL,
			&status);

	assert (status == CL_SUCCESS);
	////////////////////////////////////////////////////////////////////////
	/// eof create context
	////////////////////////////////////////////////////////////////////////



	return 0;
}

static int qc_finit(void) {
	//unmap mmap
	if( munmap(mmap_address, 0x400000) == -1 )
		printf("[qhgpu connector] error : unmap fail.\n");

	ioctl(devfd, QHGPU_IOC_SET_STOP);
	close(devfd);
	//gpu_finit();

    //gpu_free_pinned_mem(hostbuf.uva);

    return 0;
}



static struct _qhgpu_sritem *qc_alloc_service_request()
{
	struct _qhgpu_sritem *s = (struct _qhgpu_sritem *)malloc(sizeof(struct _qhgpu_sritem));
	if (s) {
		memset(s, 0, sizeof(struct _qhgpu_sritem));
		INIT_LIST_HEAD(&s->list);
		INIT_LIST_HEAD(&s->glist);
	}
	return s;
}

static void qc_free_service_request(struct _qhgpu_sritem *s)
{
	free(s);
}



static void qc_fail_request(struct _qhgpu_sritem *sreq, int serr)
{
	sreq->sr.state = QHGPU_REQ_DONE;
	sreq->sr.errcode = serr;
	list_del(&sreq->list);
	list_add_tail(&sreq->list, &done_reqs);
}


static void qc_init_service_request(struct _qhgpu_sritem *item,
			       struct qhgpu_ku_request *kureq)
{


	printf("qc_init_service_request !!! %p \n",kureq->mmap_addr);


	//// add to all_reqs for request empty check
	list_add_tail(&item->glist, &all_reqs);


	memset(&item->sr, 0, sizeof(struct qhgpu_service_request));
	item->sr.id = kureq->id;
	item->sr.hin = kureq->in;
	item->sr.hout = kureq->out;
	item->sr.hdata = kureq->data;
	item->sr.insize = kureq->insize;
	item->sr.outsize = kureq->outsize;
	item->sr.datasize = kureq->datasize;
	item->sr.stream_id = -1;
	item->sr.mmap_addr = kureq->mmap_addr;
	item->sr.kmmap_addr = kureq->kmmap_addr;
	item->sr.mmap_size = kureq->mmap_size;


	printf("qc_init_service_request lookup service start !!! %s \n",kureq->service_name);

	struct _qhgpu_sitem *temp = qc_lookup_service(kureq->service_name);
	//printf("qc_init_service_request lookup service end !!! %s \n",temp->s->name);

	//item->sr.s= qc_lookup_service(kureq->service_name);

	//printf("qc_init_service_request lookup service end !!! %s \n",item->sr.s->name);
	if (!temp) {
		printf("can't find service\n");
		qc_fail_request(item, QHGPU_NO_SERVICE);
	} else {

		item->sr.s = temp->s;//qc_lookup_service(kureq->service_name);
		printf("qc_init_service_request compute_size !!! \n");
		//item->sr.s->compute_size(&item->sr);				//// [?]
		item->sr.state = QHGPU_REQ_INIT;
		item->sr.errcode = 0;

		printf("qc_init_service_request add_list start !!! \n");
		//// add to init_reqs
		list_add_tail(&item->list, &init_reqs);
		printf("qc_init_service_request add_list END !!! \n");
	}
}



//static int req_call_count = 0;

static int qc_get_next_service_request(void)
{
	int err;
	struct pollfd pfd;

	struct _qhgpu_sritem *sreq;
	struct qhgpu_ku_request kureq;

	pfd.fd = devfd;
	pfd.events = POLLIN;
	pfd.revents = 0;


	//printf("qc_get_next_service_request start!!! \n");

	err = poll(&pfd, 1, list_empty(&all_reqs)? -1:0);	/// if list is empty polling (timeout -1)
																/// qhgpudev.reqq poll_wait interrupted by call

	//printf("qc_get_next_service_request err: %d \n",err);

	if (err == 0 || (err && !(pfd.revents & POLLIN)) ) {
		//printf("-1-1-1 111\n");
		return -1;
	}
	else if (err == 1 && pfd.revents & POLLIN)
	{
		//	_qhgpu_sritem allocation
		sreq = qc_alloc_service_request();
		if (!sreq) {
			//printf("-1-1-1 222\n");
			return -1;
		}

		//// get request --> kureq
		err = read(devfd, (char*)(&kureq), sizeof(struct qhgpu_ku_request));

		if (err <= 0) {
			if (errno == EAGAIN || err == 0) {
				//printf("-1-1-1 222\n");

				return -1;
			} else {
				//printf("abort 111\n");
				perror("Read request.");
				abort();
			}
		} else {
			if( kureq.service_name[0] == '0' && kureq.service_name[1] == '0' && kureq.service_name[2] == '0' )
				return -123;

			char *num = (char*)kureq.data;
			printf("kureq.data: %s \n",num);
			kureq.mmap_addr = mmap_addr_arr[num[0]-'0'];
			//req_call_count++;
			qc_init_service_request(sreq, &kureq);
			return 0;
		}
	} else {
		if (err < 0) {
			//printf("abort 222\n");
			perror("Poll request");
			abort();
		} else {
			//printf("abort 333\n");
			fprintf(stderr, "Poll returns multiple fd's results\n");
			abort();
		}
    }

	//finsh request incomming
	//if(1) {
	//	return 1;
	//}

	//printf("000 111\n");
	return 0;
}








static int qc_launch_exec(struct _qhgpu_sritem *sreq)
{

	printf("qc_launch_exec !!!\n");


	cl_int status;
	command_que= clCreateCommandQueue(
			context,
			devices[numdev],
			CL_QUEUE_PROFILING_ENABLE,
			&status);
	assert (status == CL_SUCCESS);


	//// GPU computation
	int r = sreq->sr.s->launch(&sreq->sr);
	if (r) {
		printf("%d fails launch\n", sreq->sr.id);
		qc_fail_request(sreq, r);
	} else {
		sreq->sr.state = QHGPU_REQ_RUNNING;
		list_del(&sreq->list);
		list_add_tail(&sreq->list, &running_reqs);
	}
	return 0;
}

static int qc_post_exec(struct _qhgpu_sritem *sreq)
{
	int r = 1;

	printf("qc_post_exec !!!\n");

	if (!(r = sreq->sr.s->post(&sreq->sr))) {
		sreq->sr.state = QHGPU_REQ_POST_EXEC;
		list_del(&sreq->list);
		list_add_tail(&sreq->list, &post_exec_reqs);
	}


	return r;
}




////////////////////////////////////////////////////////////////////////////////////
// response
///////////////////////////////////////////////////PAGE_SIZE/////////////////////////////////
static int qc_send_response(struct qhgpu_ku_response *resp)
{
	printf("qc_send_response: %d \n",resp->id);

	/// write result to device driver
	ssc(write(devfd, resp, sizeof(struct qhgpu_ku_response)));
	return 0;
}


static int qc_service_done(struct _qhgpu_sritem *sreq)
{
    struct qhgpu_ku_response resp;


    printf("qc_service_done !!!\n");

    resp.id = sreq->sr.id;
    resp.errcode = sreq->sr.errcode;

    qc_send_response(&resp);

    list_del(&sreq->list);
    list_del(&sreq->glist);

    qc_free_service_request(sreq);
    printf("qc_service_done return !!!\n");
    return 0;
}
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////




static int __qc_process_request(int (*op)(struct _qhgpu_sritem *),
			      struct list_head *lst, int once)
{
    struct list_head *pos, *n;
    int r = 0;

    list_for_each_safe(pos, n, lst) {
    	r = op(list_entry(pos, struct _qhgpu_sritem, list));
    	if (!r && once)
    		break;
    }
    return r;
}


static int qc_main_loop()
{
	while (qc_loop_continue)
	{
		__qc_process_request(qc_service_done, &post_exec_reqs, 0);
		__qc_process_request(qc_post_exec, &running_reqs, 1);
		__qc_process_request(qc_launch_exec, &init_reqs, 0);

		//qc_get_next_service_request();
		if( qc_get_next_service_request() == -123 )
			break;
    }
	return 0;
}

int main(int argc, char *argv[])
{
	int c;
	qhgpu_dev_path = "/dev/qhgpu";
	service_lib_dir = "./";
	qc_init();


	int service_cnt = qc_load_all_services(service_lib_dir,context,devices);

	printf("service_cnt: %d \n",service_cnt);
	if(service_cnt == 0) service_cnt = 1;


	int r = ioctl(devfd, QHGPU_IOC_SET_MMAP,(unsigned long)service_cnt);
	if (r < 0) {
		printf("device write fail!");
		perror("device write fail!");
		abort();
	}
	init_mmap(service_cnt);


	qc_main_loop();
	qc_finit();


	return 0;
}







