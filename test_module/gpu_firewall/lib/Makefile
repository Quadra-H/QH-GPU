all:
	gcc -L/opt/AMDAPP/lib/x86_64/ -I/opt/AMDAPP/include -std=c99 -c -g -fPIC cl_gpu_firewall.c libsrv_gpu_firewall.c -lOpenCL -lnfnetlink -lnetfilter_queue
	gcc -shared -fPIC -std=c99 -o libsrv_gpu_firewall.so libsrv_gpu_firewall.o cl_gpu_firewall.o
	gcc -o packet_poll gpu_firewall_packet_poll.c -lnfnetlink -lnetfilter_queue	 
	$(if $(BUILD_DIR), cp libsrv_gpu_firewall.so $(BUILD_DIR)/ )
	$(if $(BUILD_DIR), cp packet_poll $(BUILD_DIR)/ )
	
clean:
	rm -f *.o *.so packet_poll
