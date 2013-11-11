all:
	#gcc -L/opt/AMDAPP/lib/x86_64/ -I/opt/AMDAPP/include -std=c99 -c -g -fPIC cl_floyd_warshall.c libsrv_floyd_warshall.c -lOpenCL
	#gcc -shared -fPIC -std=c99 -o libsrv_floyd_warshall.so libsrv_floyd_warshall.o cl_floyd_warshall.o	 
	#$(if $(BUILD_DIR), cp libsrv_floyd_warshall.so $(BUILD_DIR)/ )
	gcc libsrv_netfilter_firewall.c -lnfnetlink -lnetfilter_queue
	
clean:
	rm -f *.o *.so
