all:
	gcc -L/opt/AMDAPP/lib/x86_64/ -I/opt/AMDAPP/include -c -g -fPIC cl_gpu_firewall.c libsrv_gpu_firewall.c -lOpenCL -lnfnetlink -lnetfilter_queue
	gcc -shared -fPIC -std=c99 -o libsrv_gpu_firewall.so libsrv_gpu_firewall.o cl_gpu_firewall.o -lnfnetlink -lnetfilter_queue 
	$(if $(BUILD_DIR), cp libsrv_gpu_firewall.so $(BUILD_DIR)/ )
	
clean:
	rm -f *.o *.so
