doppler: doppler.cpp usbtg.cpp usbtg.h
	g++ doppler.cpp usbtg.cpp -o doppler -Wl,-rpath,/usr/local/lib -lbb_api -lftdi
	
run: doppler
	./doppler