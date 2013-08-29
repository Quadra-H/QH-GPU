
all:


#g++ -L/opt/AMDAPP/lib/x86_64/ -I/opt/AMDAPP/include -c cl_radix_sort.c -lOpenCL
	gcc -L/opt/AMDAPP/lib/x86_64/ -I/opt/AMDAPP/include -std=c99 -c -g -fPIC cl_radix_sort.c libsrv_radix.c -lOpenCL
	gcc -shared -fPIC -std=c99 -o libsrv_radix2.so libsrv_radix.o cl_radix_sort.o	 
	$(if $(BUILD_DIR), cp libsrv_radix2.so $(BUILD_DIR)/ )

clean:
	rm -f *.o *.so
