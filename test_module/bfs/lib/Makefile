
all:


#g++ -L/opt/AMDAPP/lib/x86_64/ -I/opt/AMDAPP/include -c cl_radix_sort.c -lOpenCL
	gcc -L/opt/AMDAPP/lib/x86_64/ -I/opt/AMDAPP/include -std=c99 -c -g -fPIC cl_bfs.c libsrv_bfs.c -lOpenCL
	gcc -shared -fPIC -std=c99 -o libsrv_bfs.so libsrv_bfs.o cl_bfs.o	 
	$(if $(BUILD_DIR), cp libsrv_bfs.so $(BUILD_DIR)/ )

clean:
	rm -f *.o *.so
