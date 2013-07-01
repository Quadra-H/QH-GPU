SUBDIRS = qhgpu test_module scripts

all: $(SUBDIRS)


.PHONY: $(SUBDIRS)

$(SUBDIRS): mkbuilddir
	$(MAKE) -C $@ $(TARGET) kv=$(kv) BUILD_DIR=`pwd`/build

mkbuilddir:
	mkdir -p build

services: qhgpu

distclean:
	$(MAKE) all kv=$(kv) TARGET=clean

clean: distclean
	rm -rf build/*
