obj-m += async_benchmark_module.o

SUBDIRS = lib

all: 	
	cp ../../qhgpu/Module.symvers ./
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
	cp async_benchmark_module.ko ../../build/	

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean