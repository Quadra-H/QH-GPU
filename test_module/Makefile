obj-m += test_module.o

SUBDIRS = floyd_warshall radix_sort cpu_firewall gpu_firewall

all:  $(SUBDIRS)
.PHONY: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(TARGET) kv=$(kv) BUILD_DIR=$(BUILD_DIR)	
		
	cp ../qhgpu/Module.symvers ./
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
	$(if $(BUILD_DIR), cp test_module.ko $(BUILD_DIR)/ )

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
