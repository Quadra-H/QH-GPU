obj-m += radix_sort_module.o

SUBDIRS = lib

all: $(SUBDIRS)

.PHONY: $(SUBDIRS)

$(SUBDIRS): 
	$(MAKE) -C $@ $(TARGET) kv=$(kv) BUILD_DIR=$(BUILD_DIR)
	
	
	cp ../../qhgpu/Module.symvers ./
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
	cp radix_sort_module.ko ../../build/	

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean