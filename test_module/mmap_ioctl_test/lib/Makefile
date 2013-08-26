
all:

	gcc -L/opt/AMDAPP/lib/x86_64/ -I/opt/AMDAPP/include -std=c99 -c -g -fPIC libsrv_mmap_ioctl.c -lOpenCL
	gcc -shared -fPIC -std=c99 -o libsrv_mmap_ioctl.so libsrv_mmap_ioctl.o	 
	$(if $(BUILD_DIR), cp libsrv_mmap_ioctl.so $(BUILD_DIR)/ )

clean:
	rm -f *.o *.so
