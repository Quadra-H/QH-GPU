obj-m += test_module.o

all: 
	cp ../qhgpu/Module.symvers ./
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
	$(if $(BUILD_DIR), cp test_module.ko $(BUILD_DIR)/ )

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
