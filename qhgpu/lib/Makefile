
all:
	g++ -L/opt/AMDAPP/lib/x86_64/ -I/opt/AMDAPP/include -c -fPIC libsrv_default.cpp -lOpenCL
	g++ -shared -fPIC -o libsrv_default.so libsrv_default.o	
	$(if $(BUILD_DIR), cp libsrv_default.so $(BUILD_DIR)/ )

clean:
	rm -f *.o *.so
