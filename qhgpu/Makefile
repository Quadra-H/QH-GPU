LIBS := -lOpenCL

obj-m += qhgpu.o

all:	qhgpu connector

qhgpu-objs := main.o

qhgpu:
	make -C /lib/modules/$(shell uname -r)/build M=`pwd` modules
	$(if $(BUILD_DIR), cp qhgpu.ko $(BUILD_DIR)/ )

qhgpu_log:
	gcc -c qhgpu_log.c -o qhgpu_log_user.o
	ar -rcs qhgpu_log.a qhgpu_log_user.o
	$(if $(BUILD_DIR), cp qhgpu_log.a $(BUILD_DIR)/ )


connector: qhgpu_log
	g++ -c connector.c
	gcc -c qhgpu_daemon.c
	g++ -L/usr/local/cuda/lib64 -o "qhgpu" connector.o qhgpu_daemon.o $(LIBS)
	cp ../kernel/* $(BUILD_DIR)
	$(if $(BUILD_DIR), cp qhgpu $(BUILD_DIR)/ )

clean:
	make -C /lib/modules/$(shell uname -r)/build M=`pwd` clean
	rm -f qhgpu
	rm -f qhgpu_log.a
	rm -f *.o
