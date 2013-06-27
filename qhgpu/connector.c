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

static int devfd;
static char *qhgpudev;

#define ssc(...) _safe_syscall(__VA_ARGS__, __FILE__, __LINE__)

int _safe_syscall(int r, const char *file, int line) {
	if (r < 0) {
		fprintf(stderr, "Error in %s:%d, ", file, line);
		perror("");
		abort();
	}
	return r;
}

static int kh_init(void) {
	int i, len, r;
	void *p;

	devfd = ssc(open(qhgpudev, O_RDWR));

	printf("open dev [%s], ret fd = %d\n", qhgpudev, devfd);

	return 0;
}

static int kh_finit(void) {
	//ioctl(devfd, KGPU_IOC_SET_STOP);
	close(devfd);

	return 0;
}

static int kh_main_loop() {
	int flag;
	int bflag;
	bflag = flag = 0;

	while (1) {
		read(devfd, (char*) (&flag), sizeof(int));
		if (bflag != flag)
			printf("flag = %d\n", flag);
		bflag = flag;
	}

	return 0;
}

int main(void) {
	int c;
	qhgpudev = "/dev/qhgpu";

	kh_init();
	kh_main_loop();
	kh_finit();
	return 0;

	return 0;
}
